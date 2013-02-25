#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

#include "arp.h"
#include "skbuff.h"
#include "dev.h"
#include "utils.h"

void arp_rcv(struct sk_buff *skb)
{
	struct arppkt *ap;
	struct ethhdr *eh;
	unsigned int tip, sip;
	int hl;

	hl = sizeof(struct ethhdr);
	skb->len -= hl;
	ap = (struct arppkt *) skb->data;
	skb->nh.arph = (struct arphdr *) skb->data;
	eh = (struct ethhdr *) (skb->data - hl);
	/* no higher layer there */
	/* skb->data += sizeof(struct arphdr); */

	printf("arp\n");

	if (ap->ar_hrd != htons(ARPHRD_ETHER) || ap->ar_pro != htons(ETHERTYPE_IP)
	    || ap->ar_hln != ETH_ALEN || ap->ar_pln != 4)
		goto bad;

	switch (ntohs(ap->ar_op)) {
	case ARPOP_REQUEST:
		tip = *(unsigned int *)ap->__ar_tip;
		sip = *(unsigned int *)ap->__ar_sip;
		if (tip != skb->nic->ip)
			goto drop;
		ap->ar_op = htons(ARPOP_REPLY);
		*(unsigned int *)ap->__ar_sip = skb->nic->ip;
		*(unsigned int *)ap->__ar_tip = sip;
		memcpy(ap->__ar_sha, skb->nic->dev_addr, ETH_ALEN);
		memcpy(ap->__ar_tha, eh->h_source, ETH_ALEN);
		memcpy(eh->h_dest, eh->h_source, ETH_ALEN);
		memcpy(eh->h_source, skb->nic->dev_addr, ETH_ALEN);

		skb->ip_summed = 0;
		skb->protocol = ETHERTYPE_ARP;
		dev_send(skb);
		break;
	case ARPOP_REPLY:
		break;
	default:
		goto bad;
	}
drop:
	return;
bad:
	printf("arp: packet invalid\n");
}
