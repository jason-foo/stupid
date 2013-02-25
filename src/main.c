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

#include "skbuff.h"
#include "netdevice.h"
#include "sock.h"
#include "skbuff.h"
#include "dev.h"
#include "udp.h"

pthread_t recv_thread;
pthread_t protocol_stack_thread;
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

struct net_device nic;	/* it should be a list of virtual devices */
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
		skb_queue_head(&sk_buff_list, skb);
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

void app_demo()
{
	printf("nm! not done yet...\n");
}

void net_device_config_manually(struct net_device *nic)
{
	strncpy(nic->name, "eth250", IFNAMSIZ - 1);
	nic->next = NULL;

	nic->ip = inet_addr("10.82.25.83");
	nic->netmask = inet_addr("255.255.255.0");
	nic->gateway = inet_addr("10.82.25.1");

	unsigned char eth0_mac[7] = {0xc8, 0x60, 0x00, 0x0c, 0x1b, 0x39, 0};
	/* unsigned char wlan0_mac[7] = {0x78, 0x92, 0x9c, 0x82, 0x06, 0x68, 0}; */
	memcpy(nic->dev_addr, eth0_mac, 6);

	nic->mtu = ETH_FRAME_LEN;
	nic->hard_header_len = ETH_HLEN;
	nic->addr_len = ETH_ALEN;
	memset(nic->broadcast, 0xff, ETH_ALEN);
}

void net_device_init()
{
	net_device_config_manually(&nic);
}

void sock_init()
{
	sock_demo.dest.ip = inet_addr("10.82.25.98");
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

int main()
{
	net_device_init();
	sock_init();

	protocol_stack_thread_init();

	signal_init();
	receive_thread_init();

	write_pid_file();
	/* daemon(1, 1); */

	udp_send((unsigned char *)"hello kitty", 12, &sock_demo);
	/* debug */
	usleep(100000);
	/* system("../test/www/download.sh"); */
	usleep(2000);

	/* raise(SIGINT); */
	pause();

	/* the application should be independently executed
	   and it communicates with this daemon server with 
	   IPCs */
	app_demo();

	return 0;
}
