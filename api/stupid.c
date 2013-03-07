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

#include "common.h"
#include "stupid.h"

struct sfd sfd_table[MAX_SFD];
pthread_spinlock_t sfd_lock;
pid_t pid;
char server_fifo[FIFO_MAXLEN] = "/tmp/fifo.stupid";
char client_fifo[FIFO_MAXLEN];

int server_fd, client_fd;

void __error_sys(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void socket_init()
{
	static int done = 0;

	if (done)
		return;

	pthread_spin_init(&sfd_lock, PTHREAD_PROCESS_SHARED);
	pid = getpid();
	sprintf(client_fifo, "/tmp/fifo.%d", pid);
	if (mkfifo(client_fifo, 0777))
		__error_sys(client_fifo);
	if (access(server_fifo, F_OK) == -1)
		__error_sys("server not started\n");

	done = 1;
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

int stupid_socket(int domain, int type, int protocol)
{
	struct request_socket req;
	struct response_socket *resp;
	int sfd;
	int res, len;
	char readbuf[1600];

	socket_init();

	if ((sfd = sfd_alloc()) == -1)
		return -1;

	server_fd = open(server_fifo, O_WRONLY | O_NONBLOCK);
	if (server_fd == -1)
		__error_sys("server not open");

	req.type = API_SOCKET_REQUEST;
	req.pid = pid;
	req.content.socket.domain = domain;
	req.content.socket.type = type;
	req.content.socket.protocol = protocol;
	len = sizeof(req);

	res = write(server_fd, (char *)&req, len);
	if (len != res)
		__error_sys("stupid_socket: write to server fifo");

	client_fd = open(client_fifo, O_RDONLY);
	if (client_fd == -1)
		__error_sys("read from client fifo");

	res = read(client_fd, readbuf, sizeof(readbuf));
	close(client_fd);

	resp = (struct response_socket *) readbuf;
	if (resp->type == API_SOCKET_RESPONSE)
	{
		/* spin lock unnecessary here */
		/* and we should replace fifo with shared memory or UNIX domain
		   sockets */
		memcpy((char *)&sfd_table[sfd].key, &resp->key, sizeof(key_t));
		sfd_table[sfd].sock_id = resp->sock_id;
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
	write(server_fd, (char *)&req_bind, tot_len);
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
	write(server_fd, (char *)&packet, tot_len);
	usleep(10000);
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
	req_packet.pid = pid;
	req_packet.destaddr = *servaddr;
	req_packet.len = len;
	tot_len = sizeof(struct request_packet);
	res = write(server_fd, (char *)&req_packet, tot_len);
	if (tot_len != res)
		__error_sys("write to server fifo");

	/* time window here? */
	client_fd = open(client_fifo, O_RDONLY);
	if (client_fd == -1)
		__error_sys("read from client fifo");

	res = read(client_fd, readbuf, sizeof(readbuf));
	close(client_fd);

	resp_packet = (struct response_packet *) readbuf;
	if (resp_packet->type != API_RECEIVE_RESPONSE)
		__error_sys("recvfrom: wrong response type\n");
	else {
		memcpy(recvbuff, (char *)resp_packet->data, resp_packet->len);
		return resp_packet->len;
	}
	return 0;
}
