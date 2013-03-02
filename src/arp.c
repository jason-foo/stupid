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
#include <time.h>

#include "arp.h"
#include "skbuff.h"
#include "dev.h"
#include "utils.h"

pthread_spinlock_t arp_lock;
pthread_mutex_t arp_mutex_lock;
pthread_cond_t arp_queue_check = PTHREAD_COND_INITIALIZER;
struct sk_buff_head arp_queue;
struct arptab arp_table[ARP_TABLE_SIZE];

void arp_init()
{
	pthread_spin_init(&arp_lock, PTHREAD_PROCESS_SHARED);
	skb_queue_head_init(&arp_queue);
}

static int arp_hash(__u32 x)
{
	return (x >> 24) & (ARP_TABLE_SIZE - 1);
}

static void arp_request(__be32 daddr)
{
	struct sk_buff *skb;
	struct ethhdr *eh;
	struct arppkt *ah;

	skb = alloc_skb(&nic);
	skb->protocol = ETHERTYPE_ARP;
	skb->len = sizeof(struct arppkt);
	eh = (struct ethhdr *) (skb->data);
	ah = (struct arppkt *) (skb->data + ETH_HLEN);
	skb->data += ETH_HLEN;

	ah->ar_hrd = htons(ARPHRD_ETHER);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ETH_ALEN;
	ah->ar_pln = 4;
	ah->ar_op = htons(ARPOP_REQUEST);

	memcpy(ah->__ar_sha, nic.dev_addr, ETH_ALEN);
	*(__be32 *)ah->__ar_sip = nic.ip;
	memset(ah->__ar_tha, 0, ETH_ALEN);
	*(__be32 *)ah->__ar_tip = daddr;

	eh->h_proto = htons(skb->protocol);
	memcpy(eh->h_source, nic.dev_addr, ETH_ALEN);
	memcpy(eh->h_dest, nic.broadcast, ETH_ALEN);

	dev_send(skb);
}

struct arptab *arp_lookup(struct sk_buff *skb, __be32 daddr, unsigned char *mac)
{
	struct arptab *entry;
	struct timeval tv;

	pthread_spin_lock(&arp_lock);
	entry = &arp_table[arp_hash(daddr)];
	if (entry->status == ARP_STATUS_EMPTY)
	{
		skb_queue_head(&arp_queue, skb);
		entry->status = ARP_STATUS_REQUEST;
		pthread_spin_unlock(&arp_lock);
		arp_request(daddr);
		goto wait;
	}
	else if (entry->status == ARP_STATUS_REQUEST)
	{
		skb_queue_head(&arp_queue, skb);
		pthread_spin_unlock(&arp_lock);
		arp_request(daddr);
		goto wait;
	}
	else
	{
		gettimeofday(&tv, NULL);
		if (tv.tv_sec > entry->time + ARP_MAX_LIFE)
		{
			skb_queue_head(&arp_queue, skb);
			entry->status = ARP_STATUS_REQUEST;
			pthread_spin_unlock(&arp_lock);
			arp_request(daddr);
			goto wait;
		}
		else
		{
			memcpy(mac, entry->mac, ETH_ALEN);
			pthread_spin_unlock(&arp_lock);
			return entry;
		}
	}
wait:
	return NULL;
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
		goto reused;
		break;
	case ARPOP_REPLY:
		sip = *(unsigned int *)ap->__ar_sip;
		tip = *(unsigned int *)ap->__ar_tip;

		pthread_spin_lock(&arp_lock);
		h = &arp_table[arp_hash(sip)];

		if (/* h->ip == sip && */ nic->ip == tip)
		{
			if (ap->__ar_sha[0] & 1) /* why? */
				goto unlock_bad;
			memcpy(h->mac, ap->__ar_sha, ETH_ALEN);
			gettimeofday(&tv, NULL);
			/* if (h->hold && (tv.tv_sec - h->time > ARP_MAX_HOLD)) */
			/* { */
			/* 	skb_free(h->hold); */
			/* 	h->hold = NULL; */
			/* } */
			h->time = tv.tv_sec;
			if (h->status == ARP_STATUS_REQUEST)
			{
				h->status = ARP_STATUS_OK;
				pthread_spin_unlock(&arp_lock);
				pthread_cond_signal(&arp_queue_check);
			}
			else
			{
				/* received broadcast reply who was not requested */
				h->status = ARP_STATUS_OK;
				pthread_spin_unlock(&arp_lock);
			}
			/* if ((skb = h->hold)) */
			/* { */
			/* 	h->hold = NULL; */
			/* 	memcpy(eh->h_dest, h->mac, ETH_ALEN); */
			/* 	pthread_spin_unlock(&arp_lock); */
			/* 	/\* fragmentation? *\/ */
			/* 	dev_send(skb); */
			/* } */
			/* else */
			/* 	pthread_spin_unlock(&arp_lock); */
		} else {
			goto unlock_bad;
		}
		break;
	default:
		goto bad;
	}

reused:
	return;
drop:
	skb_free(skb);
	return;
unlock_bad:
	pthread_spin_unlock(&arp_lock);
bad:
	printf("arp: packet invalid\n");
	skb_free(skb);
}

void *do_arp_queue(void *arg)
{
	struct sk_buff *skb;

	for (;;)
	{
		pthread_mutex_lock(&arp_mutex_lock);
		/* while empty? */
		/* and change to timewait? */
		pthread_cond_wait(&arp_queue_check, &arp_mutex_lock);
		pthread_mutex_unlock(&arp_mutex_lock);

		while ((skb = skb_dequeue(&arp_queue)) != NULL)
		{
			/* should I only wake up which with arp request */
			skb->data += ETH_HLEN;
			skb->len += ETH_HLEN;
			dev_send(skb);
		}
	}
	return 0;
}
