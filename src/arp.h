#ifndef _ARP_H
#define _APR_H

#include <linux/if_ether.h>
#include <linux/types.h>

#include "skbuff.h"

#define ARP_TABLE_SIZE 256

#define	ARP_INCOMP	0
#define	ARP_COMP	1
#define	ARP_MAX_LIFE	(3*60)
#define	ARP_MAX_HOLD	5

struct arppkt
{
	unsigned short ar_hrd;
	unsigned short ar_pro;
	unsigned char ar_hln;
	unsigned char ar_pln;
	unsigned short ar_op;

	unsigned char __ar_sha[ETH_ALEN];
	unsigned char __ar_sip[4];
	unsigned char __ar_tha[ETH_ALEN];
	unsigned char __ar_tip[4];
};

struct arptab {
	unsigned ip;
	unsigned time;
	unsigned rtime;
	struct sk_buff *hold;
	unsigned short status;
	unsigned char mac[ETH_ALEN];
};

void arp_rcv(struct sk_buff *skb);
void arp_send(struct sk_buff *skb);
extern pthread_spinlock_t arp_lock;

#endif
