#include <stdio.h>
#include <memory.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <net/if.h>

#include "dev.h"
#include "ip.h"
#include "skbuff.h"

/* #define skb_ethernet_offset() 14 */
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
	/* skb->data += skb_ethernet_offset(); */
	skb->data += sizeof(struct ethhdr);

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

static int dev_xmit(struct sk_buff *skb)
{
	int fd;
	struct sockaddr_ll sl;
	memset(&sl, 0, sizeof(sl));
	sl.sll_family = AF_PACKET;
	sl.sll_ifindex = IFF_BROADCAST;

	if ( (fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("sendfd");
		return 1;
	}

	int k, len;
	len = skb->len;
	if (len < 60)
		len = 60;
//	skb->data[43] = 22;
	if ( (k = sendto(fd, skb->data, len, 0, (struct sockaddr *)
			 &sl, sizeof(sl))) == -1)
	{
		perror("send");
		return 1;
	}
	printf("raw packet: %d bytes sent\n", k);
	/* debug */
	int i;
	for (i = 0; i < skb->len; i++)
		printf("%x ", skb->data[i]);
	printf("\n");

	return 0;
}

void dev_send(struct sk_buff *skb)
{
	struct ethhdr *eh;
	int hl;
	int i;

	hl = sizeof(struct ethhdr);
	skb->data -= hl;
	skb->len += hl;

	if (skb->head != skb->data)
	{
		printf("data demultiplexing error until dev_xmit\n");
		return;
	}

	eh = (struct ethhdr *) skb->data;
	/* in fact we should lookup the arp table */
	unsigned char eth0_dst_mac[7] = {0x08, 0x00, 0x27, 0xbc, 0x30, 0x39};
	for (i = 0; i < 6; i++)
		eh->h_dest[i] = eth0_dst_mac[i];
	for (i = 0; i < 6; i++)
		eh->h_source[i] = skb->nic->dev_addr[i];
	eh->h_proto = htons(ETH_P_IP);

	dev_xmit(skb);
}
