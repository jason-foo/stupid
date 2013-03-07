#ifndef _SOCK_H
#define _SOCK_H

#include <linux/types.h>

#include "skbuff.h"
#include "netdevice.h"

struct sk_buff_head;
//struct sk_buff;

struct sock {
	int inuse;

	struct sk_buff_head sk_receive_queue;

	struct net_device *nic;

	int domain;
	int type;
	int protocol;
	__be32 ip;
	__be16 port;
	struct {
		/* atomic_t  */
		int len;
		struct sk_buff *head;
	} sk_backlog;

	struct
	{
		__u32 ip;
		__u16 port;
		__u32 nexthop;
	} dest;       
};

#define SOCK_TABLE_SIZE 1024
extern void *do_sock(void *arg);
extern void sock_try_insert(struct sk_buff *skb);
		
#endif
