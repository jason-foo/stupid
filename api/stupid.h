#ifndef _SOCKET_H
#define _SOCKET_H

#include <sys/types.h>

struct sfd {
	int inuse;
	key_t key;
	int sock_id;
};

#define MAX_SFD 256

extern int stupid_socket(int domain, int type, int protocol);
extern void stupid_bind(int sfd, struct sockaddr_in *servaddr, int addr_len);
extern void stupid_sendto(int sfd, char *buf, int len, struct sockaddr_in
			  *servaddr, int addr_len);
extern int stupid_recvfrom(int sfd, char *buf, int len, struct sockaddr_in
			   *servaddr, int addr_lne);

extern void sfd_release(int sfd);

#endif
