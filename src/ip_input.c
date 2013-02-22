#include "ip.h"

#include <stdio.h>
#include <linux/ip.h>
#include <netinet/in.h>

void ip_rcv(struct sk_buff *skb)
{
	struct iphdr *iph;

	iph = (struct iphdr *) skb->data;
	skb->data += ntohs(iph->tot_len);
	
	printf("received ip packet, length: %d bytes\n", ntohs(iph->tot_len));
}
