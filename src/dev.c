#include <stdio.h>
#include <memory.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <linux/ip.h>

#include "dev.h"
#include "ip.h"
#include "skbuff.h"
#include "arp.h"
#include "utils.h"

static int dst_local(struct sk_buff *skb)
{
	return memcmp(skb->mac.ethernet->h_dest, skb->nic->dev_addr, ETH_ALEN)
		== 0 || memcmp(skb->mac.ethernet->h_dest, skb->nic->broadcast,
			       ETH_ALEN) == 0;
}

void net_rx_action(struct sk_buff *skb)
{
	skb->mac.ethernet = (struct ethhdr *)skb->data;
	skb->data += sizeof(struct ethhdr);

	if (!dst_local(skb))
		goto drop;

	switch (ntohs(skb->mac.ethernet->h_proto))
	{
	case ETH_P_IP:
		ip_rcv(skb);
		break;
	case ETH_P_ARP:
		arp_rcv(skb);
		break;
	default:
		goto drop;
	}
	return;

drop:
	/* printf("raw packet: dropped\n"); */
	return;
}

static int dev_xmit(struct sk_buff *skb)
{
	int fd;
	struct sockaddr_ll sl;
	memset(&sl, 0, sizeof(sl));
	sl.sll_family = AF_PACKET;
	sl.sll_ifindex = IFF_BROADCAST;

	if ( (fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
		error_msg_and_die("sendfd");

	int k, len;
	len = skb->len;
	if (len < 60)
		len = 60;

	if ( (k = sendto(fd, skb->data, len, 0, (struct sockaddr *)
			 &sl, sizeof(sl))) == -1)
		error_msg_and_die("send");

	data_dump("raw: sent", skb->data, k);
	skb_free(skb);

	return 0;
}

void dev_send(struct sk_buff *skb)
{
	struct ethhdr *eh;
	unsigned char mac[ETH_ALEN];
	int hl;

	struct arptab *arpentry;

	hl = sizeof(struct ethhdr);
	skb->data -= hl;
	skb->len += hl;
	eh = (struct ethhdr *) skb->data;

	if (skb->head > skb->data)
	{
		printf("data multiplexing error in dev_xmit\n");
		return;
	}

	switch (skb->protocol)
	{
	case ETHERTYPE_IP:
		arpentry = arp_lookup(skb, skb->sock->dest.nexthop, mac);
		if (arpentry == NULL)
		{
			printf("put to wait list\n");
			return;
		}
		memcpy(eh->h_dest, mac, ETH_ALEN);
		memcpy(eh->h_source, skb->nic->dev_addr, ETH_ALEN);
		eh->h_proto = htons(ETH_P_IP);
		break;
	case ETHERTYPE_ARP:
		/* ethernet header has been handled */
		break;
	default:
		goto bad;
	}

	dev_xmit(skb);
	return;

bad:
	printf("error internet protocol in dev_xmit\n");
}
