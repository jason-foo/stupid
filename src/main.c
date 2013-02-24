#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

/* #include <sys/socket.h> */
#include <netinet/if_ether.h>
#include <net/if.h>

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

void net_device_config_manually(struct net_device *nic)
{
	strncpy(nic->name, "eth250", IFNAMSIZ - 1);
	nic->next = NULL;

	/* nic->ip = htonl(0xc0a80265);	/\* 192.168.2.101 *\/ */
	nic->ip = htonl(0x0a521953); /* 10.82.25.83 */
	nic->netmask = htonl(0xffffff00);
	nic->gateway = htonl(0x0a521901);

	unsigned char eth0_mac[7] = {0xc8, 0x60, 0x00, 0x0c, 0x1b, 0x39, 0};
	/* unsigned char wlan0_mac[7] = {0x78, 0x92, 0x9c, 0x82, 0x06, 0x68, 0}; */
	int i;
	for (i = 0; i < 7; i++)
		nic->dev_addr[i] = eth0_mac[i];

	nic->mtu = ETH_FRAME_LEN;
	nic->hard_header_len = ETH_HLEN;
	nic->addr_len = ETH_ALEN;
}

void net_device_init()
{
	net_device_config_manually(&nic);
}

void sock_init()
{
	/* sock_demo.sk_receive_queue.next = NULL; */
	/* sock_demo.sk_receive_queue.prev = NULL; */
	/* sock_demo.sk_receive_queue.len = 0; */
	/* sock_demo.dest.ip = htonl(0xc0a80201); /\* 192.168.2.1 *\/ */
	sock_demo.dest.ip = htonl(0x0a521962);
	sock_demo.dest.port = htons(7);
	sock_demo.nic = &nic;
	skb_queue_head_init(&sk_buff_list);
}

void write_pid_file()
{
	/* char *pid = "/run/milk.pid"; */
	/* int fd; */
	/* pid = open(pid, O_CREAT | O_WRONLY, 0644); */
	/* if (pid < 0) */
	/* 	error_msg_and_die("pid file"); */
	
}

static void *do_recv_thread(void *arg);
static void *do_protocol(void *arg);

void receive_thread_init()
{
	/* printf("trace: in function %s\n", __FUNCTION__); */
	pthread_create(&recv_thread, NULL, do_recv_thread, NULL);

	/* void *thread_res; */
	/* pthread_join(recv_thread, &thread_res); */
}

void protocol_stack_thread_init()
{
	pthread_create(&protocol_stack_thread, NULL, do_protocol, NULL);
}

static void sig_int(int sig)
{
	/* is this signaled many times when using thread? */
	fprintf(stdout, "\n--- statistics ---\n");
	fflush(stdout);
	printf("%d %d %d\n", cnt_recv, cnt_protocol, cnt_processed);
	exit(EXIT_SUCCESS);
}
	
void signal_init()
{
	signal(SIGINT, sig_int);
}

static struct sk_buff *get_available_sk_buff()
{
	/* no free list desiged here, so every time we allocate one */
	return alloc_skb(&nic);
}

static void *do_recv_thread(void *arg)
{
	int fd;
	/* if ( (fd = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ALL))) < 0) */
	if ( (fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("error create listening datalink socket, are you root?");
		exit(EXIT_FAILURE);
	}
	
	for (;;)
	{
		/* in fact these function should be invoked from the
		   corresponding functions specified in the sock structure */
		struct sk_buff *skb = get_available_sk_buff();
		if (skb == NULL)
		{
			perror("unknown error");
			exit(EXIT_FAILURE);
		}
		int n;
		if ( (n = recvfrom(fd, skb->data, nic.mtu, 0, NULL, NULL)) < 0)
		/* { */
		/* 	perror("receiving packets"); */
		/* 	exit(EXIT_FAILURE); */
		/* } */
			error_msg_and_die("receiving packets");
		skb->len = n;
		skb->tail = skb->end = skb->head + skb->len;
		
//		printf("in recv thread: received data: %d bytes, cnt_recv is: %d\n",  n, cnt_recv);
		/* pthread_kill(protocol_stack_thread, SIGCONT); */
		pthread_mutex_lock(&qlock);
		cnt_recv++;
		skb_queue_head(&sk_buff_list, skb);
		
		pthread_mutex_unlock(&qlock);
		pthread_cond_signal(&qready);
	}
}

/* static void __do_protocol(struct sk_buff *skb) */
/* { */
/* 	cnt_processed++; */
/* 	printf("processing skb: length is: %d\n", skb->len); */
/* } */

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
			/* __do_protocol(skb); */
			cnt_processed++;
			printf("\n-- %d --\n", cnt_processed);
			net_rx_action(skb);
		}
		/* printf("trace: in function %s, cnt_protocol is: %d\n", */
		/*        __FUNCTION__, cnt_protocol); */
		

	}
	return 0;
}

void app_demo()
{
	printf("nm! not done yet...\n");
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
