#ifndef _IP_H
#define _IP_H

#include "skbuff.h"

extern void ip_rcv(struct sk_buff *skb);
extern void ip_send(struct sk_buff *skb);

#endif
