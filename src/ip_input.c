#include <stdio.h>
#include <linux/ip.h>
#include <netinet/in.h>
#include <string.h>

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
	if (iph->daddr != skb->nic->ip) /* broadcast? */
		goto drop;
	
	char s[20], d[20];
	strncpy(s, c_ntoa(iph->saddr), 20);
	strncpy(d, c_ntoa(iph->daddr), 20);
	printf("ip: %d bytes  dest: %s  src: %s  ttl:%d\n",
	       ntohs(iph->tot_len), d, s, iph->ttl);

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
		
	return;

drop:
	printf("ip: dropped, csum %x, hlen: %d, dest %s\n", skb->csum, iph->ihl *
	       4, c_ntoa(iph->daddr));
}
