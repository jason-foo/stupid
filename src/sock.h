#ifndef _SOCK_H
#define _SOCK_H

#include "skbuff.h"

struct sk_buff_head;
struct sk_buff;

struct sock {
	struct sk_buff_head *sk_receive_queue;
	struct {
		/* atomic_t  */
		int len;
		struct sk_buff *head;
	} sk_backlog;
	int mtu;
};
		
#endif
