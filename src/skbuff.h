#ifndef _SKBUFF_H
#define _SKBUFF_H

#include <linux/types.h>
#include <linux/if_ether.h>
#include <pthread.h>

#include "netdevice.h"
#include "sock.h"

struct sock;

struct sk_buff {
	struct sk_buff *next;
	struct sk_buff *prev;

	struct net_device *nic;
	struct sock *sock;
	int len;
	unsigned short csum;

	union
	{
		struct tcphdr *th;
		struct udphdr *uh;
		struct icmphdr *icmph;
		struct igmphdr *igmph;
		struct iphdr *ipiph;
		unsigned char *raw;
	} h;

	union
	{
		struct iphdr *iph;
		struct arphdr *arph;
		unsigned char *raw;
	} nh;

	union
	{
		struct ethhdr *ethernet;
		unsigned char *raw;
	} mac;

	unsigned char *head;
	unsigned char *data;
	unsigned char *tail;
	unsigned char *end;
};

struct sk_buff_head {
	struct sk_buff *next;
	struct sk_buff *prev;
	int qlen;

	pthread_mutex_t lock;
};

/* all following __xxxx() functions are non-atomic ones */

static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
	list->prev = list->next = (struct sk_buff *)list;
	list->qlen = 0;
}

static inline void skb_queue_head_init(struct sk_buff_head *list)
{
	pthread_mutex_init(&list->lock, NULL);
	__skb_queue_head_init(list);
}

static inline void __skb_insert(struct sk_buff *newsk, struct sk_buff *prev,
				struct  sk_buff *next, struct sk_buff_head *list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev = prev->next = newsk;
	list->qlen++;
}

static inline void __skb_queue_after(struct sk_buff_head *list,
				     struct sk_buff *prev,
				     struct sk_buff *newsk)
{
	__skb_insert(newsk, prev, prev->next, list);
}

extern void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk);static inline void __skb_queue_head(struct sk_buff_head *list,
				    struct sk_buff *newsk)
{
	__skb_queue_after(list, (struct sk_buff *)list, newsk);
}

static inline struct sk_buff *skb_peek(struct sk_buff_head *list)
{
	struct sk_buff *skb = list->next;
	if (skb == (struct sk_buff *)list)
		skb = NULL;
	return skb;
}

/* since __skb_unlink is called by __skb_dequeue, so atomic operating issues has
 * been concerned */
static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
	struct sk_buff *next, *prev;
	list->qlen--;
	next = skb->next;
	prev = skb->prev;
	skb->next = skb->prev = NULL;
	next->prev = prev;
	prev->next = next;
}

extern struct sk_buff *skb_dequeue(struct sk_buff_head *list);
static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *skb = skb_peek(list);
	if (skb)
		__skb_unlink(skb, list);
	return skb;
}

extern struct sk_buff *alloc_skb();
extern void free_skb();

#endif
