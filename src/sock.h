#ifndef _SOCK_H
#define _SOCK_H

#include <linux/types.h>
#include <sys/un.h>

#include "skbuff.h"
#include "netdevice.h"

struct sock {
	int inuse;
	int time_active;

	struct sk_buff_head sk_receive_queue;

	struct net_device *nic;

	struct sockaddr_un sa;
	int sa_len;
	char sa_path[15];

	int domain;
	int type;
	int protocol;
	__be32 ip;
	__be16 port;

	struct
	{
		__u32 ip;
		__u16 port;
		__u32 nexthop;
	} dest;       
};

extern int lock_fd;

#define SOCK_TABLE_SIZE 1024
#define SOCK_CHECK_TIME 60
extern void *do_sock(void *arg);
extern void sock_try_insert(struct sk_buff *skb);
extern void lock_init();
		
#endif
