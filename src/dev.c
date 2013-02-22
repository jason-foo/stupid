#include <stdio.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include "dev.h"
#include "ip.h"

#define skb_ethernet_offset() 14
#define dst_local(skb, dev) 1

__be16 eth_type_trans(struct sk_buff *skb)
{
	struct ethhdr *eth;
	
	eth = (struct ethhdr *)skb->data;
	skb->data += skb_ethernet_offset();

	return eth->h_proto;
}

void net_rx_action(struct sk_buff *skb)
{
	__be16 proto;

	proto = eth_type_trans(skb);

	if (!dst_local(skb, dev))
		goto drop;

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

drop:
	printf("received a packet\n");

	return;
}
