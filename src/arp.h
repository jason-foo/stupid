#ifndef _ARP_H
#define _APR_H

#include <linux/if_ehter.h>

#include "skbuff.h"

#define ARP_TABLE_SIZE 64

struct arptab {
	unsigned ip;
	unsigned time;
	unsigned rtime;
	struct sk_buff *hold;
	unsigned short status;
	unsigned char mac[ETH_ALEN];
} arp_table[ARP_TABLE_SIZE];

pthread_mutex_t arp_lock;

