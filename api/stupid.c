#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "../src/utils.h"
#include "common.h"
#include "stupid.h"

int init_done;
struct sfd sfd_table[MAX_SFD];
pthread_spinlock_t sfd_lock;
pid_t pid;

struct sockaddr_un server;
int server_fd;
char stupid_server_path[] = "/tmp/stupid_server";

struct sockaddr_un client;
int client_fd;
char stupid_client_path[128];

int lock_fd;

void __error_sys(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

/* the real initialization process will be done only once */
void socket_init()
{
	int addr_len;
	char *pidfile = "/run/stupid.pid";

	if (init_done)
		return;

	if ( (lock_fd = open(pidfile, O_RDWR | O_CREAT, LOCKMODE)) < 0)
		__error_sys("error opening lock_file");
	if (!is_stupid_started(lock_fd))
		__error_sys("stupid server not started");

	pthread_spin_init(&sfd_lock, PTHREAD_PROCESS_SHARED);
	pid = getpid();

	if ( (server_fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		__error_sys("UNIX domain socket: server\n");
	if ( (client_fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		__error_sys("UNIX domain socket: client\n");

	bzero(&server, sizeof(server));
	server.sun_family = AF_LOCAL;
	strncpy(server.sun_path, stupid_server_path, sizeof(server.sun_path));
	addr_len = offsetof(struct sockaddr_un, sun_path) +
		strlen(server.sun_path);

	bzero(&client, sizeof(client));
	client.sun_family = AF_LOCAL;
	sprintf(stupid_client_path, "/tmp/stupid.%d", pid);
	strncpy(client.sun_path, stupid_client_path, sizeof(client.sun_path));
	addr_len = offsetof(struct sockaddr_un, sun_path) +
		strlen(client.sun_path);
	if (bind(client_fd, (struct sockaddr *) &client, addr_len) < 0)
		__error_sys("bind to client");

	init_done = 1;
}

static int sfd_alloc()
{
	int i;

	pthread_spin_lock(&sfd_lock);
	for (i = 0; i < MAX_SFD; i++)
		if (!sfd_table[i].inuse)
			break;
	if (i != MAX_SFD)
		sfd_table[i].inuse = 1;
	pthread_spin_unlock(&sfd_lock);
	return (i == MAX_SFD) ? -1 : i;
}

void sfd_release(int index)
{
	pthread_spin_lock(&sfd_lock);
	sfd_table[index].inuse = 0;
	pthread_spin_unlock(&sfd_lock);
}

int read_from_server(char *buf, int len)
{
	int tot = 0;

	tot = recv(client_fd, buf, len, 0);
	/* printf("read_from_server: received %d bytes\n", tot); */

	return tot;
}

int write_to_server(char *buf, int len)
{
	int tot;

	tot = sendto(server_fd, buf, len, 0, (struct sockaddr *) &server,
		     sizeof(server));
	if (tot < 0)
		__error_sys("sendto server error\n");
	return tot;
}

int stupid_socket(int domain, int type, int protocol)
{
	struct request_socket req;
	struct response_socket *resp;
	int sfd;
	int sock_id;
	int res, len;
	char readbuf[1600];

	/* the init process will only perform only once, and we should check the
	   init_done value before using the socket */
	socket_init();

	if ((sfd = sfd_alloc()) == -1)
		return -1;

	req.type = API_SOCKET_REQUEST;
	req.pid = pid;
	req.socket.domain = domain;
	req.socket.type = type;
	req.socket.protocol = protocol;
	len = sizeof(req);

	res = write_to_server((char *) &req, len);
	if (len != res)
		__error_sys("stupid_socket: write to server fifo error");

	read_from_server(readbuf, sizeof(readbuf));

	resp = (struct response_socket *) readbuf;
	if (resp->type == API_SOCKET_RESPONSE)
	{
		/* spin lock unnecessary here */
		sock_id = resp->sock_id;
		printf("got sock id: %d\n", sock_id);
		sockid_lock(lock_fd, sock_id);
		sfd_table[sfd].sock_id = sock_id;
	} else {
		printf("unknown type id %d in response\n", resp->type);
	}

	return sfd;
}

void stupid_bind(int sfd, struct sockaddr_in *servaddr, int addr_len)
{
	struct request_bind req_bind;
	int tot_len;

	req_bind.type = API_BIND;
	req_bind.sock_id = sfd_table[sfd].sock_id;
	req_bind.sockaddr = *servaddr;
	req_bind.len = addr_len;
	tot_len = sizeof(struct request_bind);
	write_to_server((char *)&req_bind, tot_len);
}

void stupid_sendto(int sfd, char *sendbuff, int len, struct sockaddr_in *servaddr, int addr_len)
{
	struct send_packet packet;
	int tot_len;

	packet.type = API_SEND;
	packet.sock_id = sfd_table[sfd].sock_id;
	packet.len = len;
	packet.destaddr = *servaddr;
	memcpy(packet.data, sendbuff, len);
	tot_len = offsetof(struct send_packet, data) + len;
	write_to_server((char *) &packet, tot_len);
}

/* addr_len is not a value-result variable here */
int stupid_recvfrom(int sfd, char *recvbuff, int len, struct sockaddr_in *servaddr, int addr_len)
{
	struct request_packet req_packet;
	struct response_packet *resp_packet;
	int tot_len, res;
	char readbuf[1600];

	/* send the request */
	req_packet.type = API_RECEIVE_REQUEST;
	req_packet.sock_id = sfd_table[sfd].sock_id;
	req_packet.destaddr = *servaddr;
	req_packet.len = len;

	tot_len = sizeof(struct request_packet);
	res = write_to_server((char *) &req_packet, tot_len);
	if (tot_len != res)
		__error_sys("write to server fifo");

	read_from_server(readbuf, sizeof(readbuf));

	resp_packet = (struct response_packet *) readbuf;
	if (resp_packet->type != API_RECEIVE_RESPONSE)
		__error_sys("recvfrom: wrong response type\n");
	else {
		memcpy(recvbuff, (char *)resp_packet->data, resp_packet->len);
		return resp_packet->len;
	}
	return 0;
}
