#ifndef _COMMON_H
#define _COMMON_H

#include <linux/types.h>
#include <pthread.h>

#define offsetof(st, m) ((size_t)(&((st *)0)->m))

struct socket_type {
	int domain;
	int type;
	int protocol;
};

struct request_socket {
	unsigned char type;
	pid_t pid;
	struct socket_type socket;
};

struct response_socket {
	unsigned char type;
	int sock_id;
};

struct request_bind {
	unsigned char type;
	int sock_id;
	struct sockaddr_in sockaddr;
	unsigned int len;
};

struct send_packet {
	unsigned char type;
	int sock_id;
	struct sockaddr_in destaddr;
	unsigned int len;
	unsigned char data[1600];
};

struct request_packet {
	unsigned char type;
	int sock_id;
	struct sockaddr_in destaddr;
	unsigned int len;	/* len is ommited for now */
};

struct response_packet {
	unsigned char type;
	int len;
	char data[1600];
};

#define API_SOCKET_REQUEST 1
#define API_SOCKET_RESPONSE 2
#define API_SEND 3
#define API_RECEIVE_REQUEST 4
#define API_RECEIVE_RESPONSE 5
#define API_BIND 6

#define REQUEST_MAX_REPEAT 5

#endif
