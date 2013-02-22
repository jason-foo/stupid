#include <stdio.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include "dev.h"
#include "ip.h"

#define skb_ethernet_offset() 14
/* #define dst_local(skb, dev) 1 */

static int dst_local(struct sk_buff *skb)
{
	int i;
	int local = 1;
	int broadcast = 1;

	for (i = 0; i < skb->nic->addr_len; i++)
	{
		if (skb->mac.ethernet->h_dest[i] != skb->nic->dev_addr[i])
			local = 0;
		if (skb->mac.ethernet->h_dest[i] != 0xff)
			broadcast = 0;
	}
	return local || broadcast;
}

__be16 eth_type_trans(struct sk_buff *skb)
{
	return skb->mac.ethernet->h_proto;
}

void net_rx_action(struct sk_buff *skb)
{
	__be16 proto;

	skb->mac.ethernet = (struct ethhdr *)skb->data;
	skb->data += skb_ethernet_offset();

	if (!dst_local(skb))
		goto drop;

	proto = eth_type_trans(skb);

	switch (ntohs(proto))
	{
	case ETH_P_IP:
		ip_rcv(skb);
		break;
	case ETH_P_ARP:
		/* arp_rcv(skb); */
		break;
	default:
		goto drop;
	}

	goto out;

drop:
	printf("raw packet dropped\n");
out:
	return;
}
