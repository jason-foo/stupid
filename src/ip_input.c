#include <stdio.h>
#include <linux/ip.h>
#include <netinet/in.h>

#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"

void ip_rcv(struct sk_buff *skb)
{
	struct iphdr *iph;

	iph = skb->nh.iph = (struct iphdr *) skb->data;
	skb->data += iph->ihl * 4;

	if (!skb->ip_summed)
		skb->csum = in_checksum((__u16 *) iph, iph->ihl * 4);
	if (skb->csum != 0)
		goto drop;
	if (ntohl(iph->daddr) != skb->nic->ip)
		goto drop;
	
	printf("ip packet, %5d bytes  dest: %x  src: %x  ttl:%d\n",
	       ntohs(iph->tot_len), ntohl(iph->daddr), ntohl(iph->saddr), iph->ttl);

	switch (iph->protocol)
	{
	case IPPROTO_UDP:
		udp_rcv(skb);
		break;
	case IPPROTO_TCP:
		tcp_rcv(skb);
		break;
	case IPPROTO_ICMP:
		break;
	case IPPROTO_IGMP:
		break;
	default:
		printf("Transport layer protocol %d not supported\n", iph->protocol);
	}
		
	goto out;

drop:
	printf("ip packet dropped with csum %x, header length: %d\n", skb->csum,
	       iph->ihl * 4);
out:
	return;
}
