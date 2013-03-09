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
extern struct sk_buff_head dev_backlog;

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
	skb_queue_head_init(&dev_backlog);
}

void net_device_init()
{
	net_device_config(&nic);
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
	pthread_create(&recv_thread, NULL, do_dev_receive_thread, NULL);
}

void protocol_stack_thread_init()
{
	pthread_create(&protocol_stack_thread, NULL, do_protocol_receive_thread, NULL);
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
	printf("bye\n");
	exit(EXIT_SUCCESS);
}
	
void signal_init()
{
	signal(SIGINT, sig_int);
}

int main()
{
	write_pid_file();
	signal_init();

	net_device_init();
	route_table_init();
	arp_init();
	dev_xmit_init();

	protocol_stack_thread_init();
	arp_queue_thread_init();
	sock_thread_init();
	receive_thread_init();	/* init this at last is better */

	/* daemon(1, 1); */

	pause();

	return 0;
}
