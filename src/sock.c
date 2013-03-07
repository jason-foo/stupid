#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/ip.h>

#include "../include/common.h"
#include "sock.h"
#include "utils.h"
#include "udp.h"
#include "skbuff.h"

struct sock sock_table[SOCK_TABLE_SIZE];
pthread_spinlock_t sock_lock;

char server_fifo[FIFO_MAXLEN] = "/tmp/fifo.stupid";
char client_fifo[FIFO_MAXLEN];	/* write data to application */
int server_fd, client_fd;

int d_sock_id;

void _sock_init()
{
	int i;

	pthread_spin_init(&sock_lock, PTHREAD_PROCESS_SHARED);

	for (i = 0; i < SOCK_TABLE_SIZE; i++)
	{
		skb_queue_head_init(&sock_table[i].sk_receive_queue);
		sock_table[i].nic = &nic;
	}

	if (access(server_fifo, F_OK) == 0)
		unlink(server_fifo);

	umask(0);
	if (mkfifo(server_fifo, 0777))
		error_msg_and_die("mkfifo: /tmp/fifo.stupid\n");
}

/* since allocating sock_id is our work, we should be
   terminated if sock id is wrong */
void __sock_id_check(int sock_id)
{
	if (sock_id >= SOCK_TABLE_SIZE)
		error_msg_and_die("spicified wrong sock id\n");

	if (sock_table[sock_id].inuse == 0)
		error_msg_and_die("spicified wrong sock id\n");
};		

void sock_id_check(int sock_id)
{
	pthread_spin_lock(&sock_lock);
	__sock_id_check(sock_id);
	pthread_spin_unlock(&sock_lock);
}

int sock_alloc(struct socket_type *st)
{
	int i;

	pthread_spin_lock(&sock_lock);
	for (i = 0; i < SOCK_TABLE_SIZE; i++)
		if (sock_table[i].inuse == 0)
		{
			sock_table[i].inuse = 1;
			pthread_spin_unlock(&sock_lock);
			if (st->type == SOCK_DGRAM)
				sock_table[i].protocol = IPPROTO_UDP;
			return i;
		}
	pthread_spin_unlock(&sock_lock);
	return -1;
}

void sock_free(struct sock *sock)
{
	pthread_spin_lock(&sock_lock);
	/* sk_receive_queue free */
	if (sock->port != 0)
	{
		udp_ephemeral_port_free(sock->port);
		sock->port = 0;
	}
	pthread_spin_unlock(&sock_lock);
}

struct sock *sock_match(struct sk_buff *skb)
{
	int i;

	/* hash table is necessary */
	pthread_spin_lock(&sock_lock);
	switch (skb->protocol)
	{
	case IPPROTO_UDP:
		for (i = 0; i < SOCK_TABLE_SIZE; i++)
		{
			if (sock_table[i].inuse == 0)
				continue;
			if (skb->h.uh->dest == sock_table[i].port &&
			    (skb->nh.iph->daddr == sock_table[i].ip ||
			     sock_table[i].ip == INADDR_ANY) )
				break;
		}
		break;
	case IPPROTO_TCP:
		for (i = 0; i < SOCK_TABLE_SIZE; i++)
		{
			if (sock_table[i].inuse == 0)
				continue;
			if (skb->h.th->dest == sock_table[i].port &&
			    (skb->nh.iph->daddr == sock_table[i].ip ||
			     sock_table[i].ip == INADDR_ANY) )
				break;
		}
		break;
	default:
		i = SOCK_TABLE_SIZE;
	}
	pthread_spin_unlock(&sock_lock);

	if (i != SOCK_TABLE_SIZE)
		return &sock_table[i];
	return NULL;
}

void sock_try_insert(struct sk_buff *skb)
{
	struct sock *sock;

	sock = sock_match(skb);
	if (sock == NULL)
	{
		printf("sock_try_insert: dropping unwanted udp packet\n");
		goto drop;
	}
	if (skb_queue_len(&sock->sk_receive_queue) > MAX_SKB_QUEUE_SIZE)
	{
		printf("sock queue is full\n");
		goto drop;
	}

	printf("sock_try_insert: inserting into queue\n");
	skb_queue_tail(&sock->sk_receive_queue, skb);
	return;
drop:
	skb_free(skb);
}

void check_protocol_default(struct sock *sock)
{
	/* we can check a table here */
	if (sock->domain == AF_INET && sock->type == SOCK_DGRAM &&
	    sock->protocol == 0)
		sock->protocol = IPPROTO_UDP;
	if (sock->domain == AF_INET && sock->type == SOCK_STREAM &&
	    sock->protocol == 0)
		sock->protocol = IPPROTO_TCP;
}

void write_to_user(int pid, char *data, int len)
{
}

void *do_sock(void *arg)
{
	int res;
	char recvbuff[1600];
	struct request_socket *req_socket;
	struct send_packet *packet;
	struct response_socket resp_socket;
	struct socket_type *st;
	struct sockaddr_in destaddr;
	struct sock *sock;
	struct request_bind *req_bind;
	struct request_packet *req_packet;
	struct response_packet resp_packet;
	struct sk_buff *skb;
	int sock_id;
	int tot_len;

	_sock_init();

	for (;;)
	{
		/* if we don't open and close the fifo every time, the fifo will
		   not be blocked after the first read, I don't know if select
		   works here */
		/* And the big problem is that linux has a limitation for
		   opening files, so if the clients send request for thousands
		   of times, the server_fifo can't be opened. Perhaps signal is
		   necessary here or some other IPC. does select() work here? */
		server_fd = open(server_fifo, O_RDONLY); 
		if (server_fd == -1)
			error_msg_and_die("error open server fifo");
		res = read(server_fd, recvbuff, sizeof(recvbuff));
		if (res == 0)
			continue;

		switch (recvbuff[0]) /* the type */
		{
		case API_SOCKET_REQUEST:
			req_socket = (struct request_socket *) recvbuff;
			st = (struct socket_type *) &req_socket->content.socket;
			if (st->domain != AF_INET ||st->type != SOCK_DGRAM ||
			    st->protocol != 0)
			{
				/* should send back error message */
				printf("unsupported socket type\n");
				continue;
			}
			d_sock_id = sock_id = sock_alloc(st);
			sock = &sock_table[sock_id];
			sock->domain = req_socket->content.socket.domain;
			sock->type = req_socket->content.socket.type;
			sock->protocol = req_socket->content.socket.protocol;
			check_protocol_default(sock);

			/* send response back to the client */
			resp_socket.type = API_SOCKET_RESPONSE;
			resp_socket.sock_id = sock_id;

			sprintf(client_fifo, "/tmp/fifo.%d", req_socket->pid);
			client_fd = open(client_fifo, O_WRONLY);
			if (client_fd == -1)
			{
				printf("request_socket: error open client's fifo\n");
				break;
			}
			tot_len = sizeof(struct response_socket);
			write(client_fd, (char *)&resp_socket, tot_len);
			close(client_fd);
			break;
		case API_SEND:
			packet = (struct send_packet *) recvbuff;
			sock_id = packet->sock_id;
			sock_id_check(sock_id);

			sock = &sock_table[sock_id];
			destaddr = packet->destaddr;
			sock->dest.ip = destaddr.sin_addr.s_addr;
			sock->dest.port = destaddr.sin_port;

			udp_send(packet->data, packet->len, sock);
			break;
		case API_BIND:
			req_bind = (struct request_bind *) recvbuff;
			sock_id = req_bind->sock_id;
			sock_id_check(sock_id);

			sock = &sock_table[sock_id];
			/* send error message back and break? */
			if (sock->domain != req_bind->sockaddr.sin_family)
				printf("bind: wrong protocol family\n");
			sock->ip = req_bind->sockaddr.sin_addr.s_addr;
			sock->port = req_bind->sockaddr.sin_port;
			break;
		case API_RECEIVE_REQUEST:
			req_packet = (struct request_packet *) recvbuff;
			sock_id = req_packet->sock_id;
			sock_id_check(sock_id);
			printf("do_sock: sock_id: %d\n", sock_id);
			sock = &sock_table[sock_id];
			destaddr = req_packet->destaddr;

			resp_packet.type = API_RECEIVE_RESPONSE;
			printf("do_sock: received %d packets\n",
			       skb_queue_len(&sock->sk_receive_queue));
			skb = skb_match(&sock->sk_receive_queue, sock->ip,
					sock->port);

			if (skb == NULL)
			{
				resp_packet.len = 0;
			} else {
				resp_packet.len = skb->len;
				memcpy(resp_packet.data, skb->data,
				       resp_packet.len);
				skb_free(skb);
			}
			tot_len = offsetof(struct response_packet, data) +
				resp_packet.len;

			sprintf(client_fifo, "/tmp/fifo.%d", req_packet->pid);
			client_fd = open(client_fifo, O_WRONLY);
			if (client_fd == -1)
			{
				printf("requet packet: error open client's fifo\n");
				break;
			}
			write(client_fd, (char *)&resp_packet, tot_len);
			close(client_fd);

			break;
		default:
			printf("unsupported communication type: %d\n", recvbuff[0]);
		}
		close(server_fd);
	}
}
