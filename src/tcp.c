#include <stdio.h>
#include <netinet/in.h>
#include <linux/tcp.h>

#include "tcp.h"
#include "skbuff.h"

void tcp_rcv(struct sk_buff *skb)
{
	struct tcphdr *th;
	th = skb->h.th = (struct tcphdr *) skb->data;
	skb->data += th->doff * 4;

	printf("tcp: sport %5d, dport %5d, hlen %5d\n",
	       ntohs(th->source), ntohs(th->dest), th->doff * 4);
}
