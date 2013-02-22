#ifndef _DEV_H
#define _DEV_H

#include <linux/types.h>

#include "skbuff.h"

extern void net_rx_action(struct sk_buff *skb);
extern __be16 eth_type_trans(struct sk_buff *skb);

#endif
