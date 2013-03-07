#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "utils.h"

#include "sock.h"
#include "skbuff.h"
#include "netdevice.h"
#include "dev.h"
#include "udp.h"
#include "arp.h"
#include "ip_route.h"

pthread_t recv_thread;
pthread_t protocol_stack_thread;
pthread_t arp_queue_thread;
pthread_t sock_thread;
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

extern struct net_device nic;
struct sock sock_demo;	/* many of these should be allocated dynamically,
			 * including sk_buff_list */

struct sk_buff_head sk_buff_list;

int cnt_recv, cnt_protocol, cnt_processed;

static struct sk_buff *get_available_sk_buff()
{
	/* no free list desiged here, so every time we allocate one */
	return alloc_skb(&nic);
}

static void *do_recv_thread(void *arg)
{
	int sockfd, n;

	if ( (sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
		error_msg_and_die("error create listening datalink socket, are you root?");
	
	for (;;)
	{
		/* in fact these function should be invoked from the
		   corresponding functions specified in the sock structure */
		struct sk_buff *skb = get_available_sk_buff();
		if (skb == NULL)
			error_msg_and_die("unknown error");

		if ( (n = recvfrom(sockfd, skb->data, nic.mtu, 0, NULL, NULL)) < 0)
			error_msg_and_die("receiving packets");

		skb->len = n;
		skb->tail = skb->end = skb->head + skb->len;
		
		pthread_mutex_lock(&qlock);
		cnt_recv++;
		skb_queue_tail(&sk_buff_list, skb);
		pthread_mutex_unlock(&qlock);
		pthread_cond_signal(&qready);
	}
	return 0;
}

static void *do_protocol(void *arg)
{
	struct sk_buff *skb;

	for (;;)
	{
		pthread_mutex_lock(&qlock);
		pthread_cond_wait(&qready, &qlock);
		/* I'm using too many locks here? */
		cnt_protocol++;
		pthread_mutex_unlock(&qlock);
		
		while ((skb = skb_dequeue(&sk_buff_list)) != NULL)
		{
			/* printf("--%d--: \n", ++cnt_processed); */
			net_rx_action(skb);
		}
	}
	return 0;
}

extern void *do_arp_queue(void *arg);

#define BUFSIZE 80

char *_trim(char *s, char *t)
{
	while (*s == ' ')
		s++;
	t--;
	while (*t == ' ' || *t == '\n' || *t == '\0')
		*t-- = '\0';
	return s;
}

void net_device_config(struct net_device *nic)
{
	FILE *fp;
	char buf[BUFSIZE];
	char *key, *value;
	char *d;
	unsigned char *mac;

	nic->next = NULL;
	mac = nic->dev_addr;

	fp = fopen("dev.conf", "r");
	if (fp == NULL)
		error_msg_and_die("Can not find dev.conf");

	while (fgets(buf, BUFSIZE, fp) != NULL)
	{
		d = strchr(buf, '=');
		if (d == NULL)
			continue; /* class info or invalid */
		*d = '\0';

		key = _trim(buf, d);
		d++;
		value = _trim(d, d + strlen(d));

		if (strcmp(key, "name") == 0)
			strncpy(nic->name, value, IFNAMSIZ - 1);
		else if (strcmp(key, "mac") == 0)
		{
			int t, i;
			for (i = 0; i < ETH_ALEN; i++)
			{
				sscanf(value + i * 3, "%x", &t);
				mac[i] = t;
			}

		}
		else if (strcmp(key, "ip") == 0)
			nic->ip = inet_addr(value);
		else if (strcmp(key, "netmask") == 0)
			nic->netmask = inet_addr(value);
		else if (strcmp(key, "gateway") == 0)
			nic->gateway = inet_addr(value);
		else
			error_msg_and_die("invalid config file");
	}

	nic->mtu = ETH_FRAME_LEN;
	nic->hard_header_len = ETH_HLEN;
	nic->addr_len = ETH_ALEN;
	memset(nic->broadcast, 0xff, ETH_ALEN);
}

void net_device_init()
{
	net_device_config(&nic);
}

void sock_init()
{
	sock_demo.dest.ip = inet_addr("10.82.58.191");
	sock_demo.dest.port = htons(7); /* echo server */
	sock_demo.nic = &nic;
	skb_queue_head_init(&sk_buff_list);
}

void write_pid_file()
{
	char *pidfile = "/run/stupid.pid";
	FILE *fp;

	unlink(pidfile);
	fp = fopen(pidfile, "w");
	if (fp == NULL)
		error_msg_and_die("pid file");
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
}

void receive_thread_init()
{
	pthread_create(&recv_thread, NULL, do_recv_thread, NULL);
}

void protocol_stack_thread_init()
{
	pthread_create(&protocol_stack_thread, NULL, do_protocol, NULL);
}

void arp_queue_thread_init()
{
	pthread_create(&arp_queue_thread, NULL, do_arp_queue, NULL);
}

void sock_thread_init()
{
	pthread_create(&sock_thread, NULL, do_sock, NULL);
}

static void sig_int(int sig)
{
	/* is this signaled many times when using thread? */
	fprintf(stdout, "\n--- statistics ---\n");
	printf("%d %d %d\n", cnt_recv, cnt_protocol, cnt_processed);
	exit(EXIT_SUCCESS);
}
	
void signal_init()
{
	signal(SIGINT, sig_int);
}

void app_demo()
{
	udp_send((unsigned char *)"hello kitty", 12, &sock_demo);
	sleep(2);
	udp_send((unsigned char *)"what the xx", 12, &sock_demo);
	usleep(100000);
}

int main()
{
	write_pid_file();

	net_device_init();
	sock_init();
	route_table_init();
	arp_init();

	signal_init();

	protocol_stack_thread_init();
	arp_queue_thread_init();
	sock_thread_init();
	receive_thread_init();	/* init this at last is better */

	/* daemon(1, 1); */

	/* app_demo(); */

	/* raise(SIGINT); */

	pause();

	return 0;
}
