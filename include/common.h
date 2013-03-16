#ifndef _COMMON_H
#define _COMMON_H

#include <linux/types.h>
#include <pthread.h>
#include "../src/sock.h"

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

/* we need (SOCK_TABLE_SIZE + 1) locks for every sock and the main server */
#define MAX_LOCK_BIT SOCK_TABLE_SIZE

#define write_lock(fd, offset, whence, len) \
	lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define un_lock(fd, offset, whence, len) \
	lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

#define is_write_lockable(fd, offset, whence, len) \
	(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

#define sockid_lock(fd, sock_id) \
	(write_lock((fd), (sock_id), SEEK_SET, 1))
#define sockid_unlock(fd, sock_id) \
	(un_lock((fd), (sock_id), SEEK_SET, 1))
#define is_stupid_started(fd) \
	(!is_write_lockable((fd), MAX_LOCK_BIT, SEEK_SET, 1))
#define is_sockid_locked(fd, sock_id) \
	(!is_write_lockable((fd), (sock_id), SEEK_SET, 1))

#endif
