/* heavily borrowed from uld */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

#include "arp.h"
#include "skbuff.h"
#include "dev.h"
#include "utils.h"

pthread_spinlock_t arp_lock;
struct arptab arp_table[ARP_TABLE_SIZE];

static int arp_hash(__u32 x)
{
	return (x >> 24) & (ARP_TABLE_SIZE - 1);
}

void arp_rcv(struct sk_buff *skb)
{
	struct arppkt *ap;
	struct ethhdr *eh;
	unsigned int tip, sip;
	struct arptab *h;
	int hl;
	struct net_device *nic;
	struct timeval tv;

	hl = sizeof(struct ethhdr);
	skb->len -= hl;
	ap = (struct arppkt *) skb->data;
	skb->nh.arph = (struct arphdr *) skb->data;
	eh = (struct ethhdr *) (skb->data - hl);
	nic = skb->nic;

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
		data_dump("arp deadlock?", skb->data, skb->len);
		sip = *(unsigned int *)ap->__ar_tip;
		tip = *(unsigned int *)ap->__ar_sip;
		pthread_spin_lock(&arp_lock);
		h = &arp_table[arp_hash(sip)];
		printf("hash for sip: %x is %d\n", sip, arp_hash(sip));
		skb_free(skb);

		if (h->ip == sip && nic->ip == tip)
		{
			if (ap->__ar_sha[0] & 1)
				goto unlock_bad;
			memcpy(h->mac, ap->__ar_sha, ETH_ALEN);
			gettimeofday(&tv, NULL);
			if (h->hold && (tv.tv_sec - h->time > ARP_MAX_HOLD))
			{
				skb_free(h->hold);
				h->hold = NULL;
			}
			h->time = tv.tv_sec;
			printf("%d\n", h->time);
			h->status = ARP_COMP;
			if ((skb = h->hold))
			{
				h->hold = NULL;
				memcpy(eh->h_dest, h->mac, ETH_ALEN);
				pthread_spin_unlock(&arp_lock);
				/* fragmentation? */
				dev_send(skb);
			}
			else
				pthread_spin_unlock(&arp_lock);
		} else
			goto unlock_bad;
		break;
	default:
		goto bad;
	}
drop:
	return;
unlock_bad:
	pthread_spin_unlock(&arp_lock);
bad:
	printf("arp: packet invalid\n");
}
