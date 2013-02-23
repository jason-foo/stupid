#include <stdio.h>
#include <memory.h>

#include <linux/types.h>
#include <netinet/in.h>
#include <linux/ip.h>

#include "skbuff.h"
#include "dev.h"
#include "utils.h"

void ip_send(struct sk_buff *skb)
{
	struct iphdr *iph;
	int hl;

	hl = sizeof(struct iphdr);
	/* assumed that no ip options here */
	skb->data -= hl;
	skb->len += hl;

	iph = (struct iphdr *) skb->data;
	memset(iph, 0, hl);

	iph->version = 4;
	iph->ihl = 5;
	iph->tot_len = htons(skb->len);
	iph->ttl = 32;
	iph->protocol = skb->protocol;
	iph->saddr = skb->nic->ip;
	iph->daddr = skb->sock->dest.ip;
	iph->check = in_checksum((__u16 *)skb->data, skb->len);

	dev_send(skb);
}
