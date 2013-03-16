#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/ip.h>

#include "utils.h"
#include "../include/common.h"
#include "sock.h"
#include "udp.h"
#include "skbuff.h"

struct sock sock_table[SOCK_TABLE_SIZE];
pthread_spinlock_t sock_lock;

struct sockaddr_un servaddr;
int server_fd;
char stupid_server_path[] = "/tmp/stupid_server";

int client_fd;

int lock_fd;

void _sock_init()
{
	int i;
	int addr_len;

	pthread_spin_init(&sock_lock, PTHREAD_PROCESS_SHARED);

	for (i = 0; i < SOCK_TABLE_SIZE; i++)
		sock_table[i].nic = &nic;

	unlink(stupid_server_path);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strncpy(servaddr.sun_path, stupid_server_path, sizeof(servaddr.sun_path));

	if ( (server_fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		error_msg_and_die("UNIX domain socket error");

	addr_len = offsetof(struct sockaddr_un, sun_path) +
		strlen(servaddr.sun_path);
	if (bind(server_fd, (struct sockaddr *)&servaddr, addr_len) < 0)
		error_msg_and_die("UNIX domain socket bind error");
	if (chmod(stupid_server_path, 0666) < 0)
		error_msg_and_die("error set stupid_server's permission");

	if ( (client_fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		error_msg_and_die("UNIX domain socket error");
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

void sock_active_update(struct sock *sock)
{
	sock->time_active = get_second();
}

void __sock_free(struct sock *sock)
{
	skb_queue_free(&sock->sk_receive_queue);
	unlink(sock->sa_path);
	if (sock->protocol == IPPROTO_UDP)
	{
		if (sock->port != 0)
		{
			udp_ephemeral_port_free(sock->port);
			sock->port = 0;
		}
	}
}

void sock_free(struct sock *sock)
{
	pthread_spin_lock(&sock_lock);
	__sock_free(sock);
	pthread_spin_unlock(&sock_lock);
}

int sock_alloc(struct socket_type *st)
{
	struct sock *sock;
	int time = get_second();
	int i;
	int app_running;

	pthread_spin_lock(&sock_lock);
	for (i = 0; i < SOCK_TABLE_SIZE; i++)
	{
		sock = &sock_table[i];

		if (sock->inuse == 1)
		{
			if (sock->time_active + SOCK_CHECK_TIME < time)
			{
				app_running = is_sockid_locked(lock_fd, i);
				if (!app_running)
				{
					__sock_free(sock);
					sock->inuse = 0;
				}
				else
					sock->time_active = time;
			}
		}

		if (sock->inuse == 0)
		{
			sock->inuse = 1;
			pthread_spin_unlock(&sock_lock);
			skb_queue_head_init(&sock->sk_receive_queue);
			if (st->type == SOCK_DGRAM)
				sock->protocol = IPPROTO_UDP;
			return i;
		}
	}
	pthread_spin_unlock(&sock_lock);
	return -1;
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

	printf("sock_try_insert: found corresponding sock, inserting\n");
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

int write_to_app(struct sock *sock, char *data, int len)
{
	int tot;

	tot = sendto(client_fd, data, len, 0, (struct sockaddr *) &sock->sa,
		     sock->sa_len);
	if (tot <= 0)
		perror("write_to_app error");
	else
		printf("write_to_app: wrote %d bytes\n", tot);
	return tot;
}

int read_from_app(char *data, int len)
{
	int tot;

	tot = recv(server_fd, data, len, 0);
	return tot;
}

void *do_sock(void *arg)
{
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
		/* blocking reading */
		read_from_app(recvbuff, sizeof(recvbuff));

		switch (recvbuff[0]) /* the type */
		{
		case API_SOCKET_REQUEST:
			req_socket = (struct request_socket *) recvbuff;
			st = (struct socket_type *) &req_socket->socket;
			if (st->domain != AF_INET || st->type != SOCK_DGRAM ||
			    st->protocol != 0)
			{
				/* in fact we should send back error message */
				printf("unsupported socket type\n");
				continue;
			}
			sock_id = sock_alloc(st);
			if (sock_id == -1)
				error_msg_and_die("wow, sock_table too small?");

			sock = &sock_table[sock_id];
			sock_active_update(sock);
			sock->domain = req_socket->socket.domain;
			sock->type = req_socket->socket.type;
			sock->protocol = req_socket->socket.protocol;
			check_protocol_default(sock);

			/* initialize the domain socket */
			bzero(&sock->sa, sizeof(sock->sa));
			sock->sa.sun_family = AF_LOCAL;
			sprintf(sock->sa_path, "/tmp/stupid.%d", req_socket->pid);
			strncpy(sock->sa.sun_path, sock->sa_path,
				sizeof(sock->sa.sun_path));
			sock->sa_len = offsetof(struct sockaddr_un, sun_path) + strlen(sock->sa.sun_path);

			/* send response back to the client */
			resp_socket.type = API_SOCKET_RESPONSE;
			resp_socket.sock_id = sock_id;
			tot_len = sizeof(struct response_socket);
			write_to_app(sock, (char *)&resp_socket, tot_len);
			break;
		case API_SEND:
			packet = (struct send_packet *) recvbuff;
			sock_id = packet->sock_id;
			sock_id_check(sock_id);

			sock = &sock_table[sock_id];
			sock_active_update(sock);
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
			sock_active_update(sock);
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
			sock = &sock_table[sock_id];
			sock_active_update(sock);
			destaddr = req_packet->destaddr;

			resp_packet.type = API_RECEIVE_RESPONSE;

			/* in fact this should be a notification chain for
			   blocking reading. A timer is also necessary since
			   user may want to read with timeout option set */
			/* for now we assume the network traffic is not heave
			   and packets sending to our machine will be received
			   in a specific period of time */
			int times = 20;
			while (times--)
			{
				printf("do_sock: received %d packets\n",
				       skb_queue_len(&sock->sk_receive_queue));
				skb = skb_match(&sock->sk_receive_queue, sock->ip,
						sock->port);
				if (skb)
					break;
				usleep(100);
			}

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
			write_to_app(sock, (char *) &resp_packet, tot_len);
			break;
		default:
			printf("unsupported communication type: %d\n", recvbuff[0]);
		}
	}
}
