#ifndef _UDP_H
#define _UDP_H

#include <linux/types.h>
#include "skbuff.h"
#include "sock.h"

extern void udp_rcv(struct sk_buff *skb);
extern void udp_send(unsigned char *data, unsigned int len, struct sock *sock);
extern void udp_ephemeral_port_free(__be16 port);

#endif
