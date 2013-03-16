#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "sock.h"
#include "skbuff.h"
#include "utils.h"
#include "arp.h"

void skb_queue_head(struct sk_buff_head *list,
		    struct sk_buff *newsk)
{
	pthread_spin_lock(&list->lock);
	__skb_queue_head(list, newsk);
	pthread_spin_unlock(&list->lock);
}

void skb_queue_tail(struct sk_buff_head *list,
		    struct sk_buff *newsk)
{
	pthread_spin_lock(&list->lock);
	__skb_queue_tail(list, newsk);
	pthread_spin_unlock(&list->lock);
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *ret;

	pthread_spin_lock(&list->lock);
	ret = __skb_dequeue(list);
	pthread_spin_unlock(&list->lock);
	return ret;
}

struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list)
{
	struct sk_buff *ret;

	pthread_spin_lock(&list->lock);
	ret = __skb_dequeue_tail(list);
	pthread_spin_unlock(&list->lock);
	return ret;
}

struct sk_buff *skb_match(struct sk_buff_head *list, __be32 ip, __be16 port)
{
	struct sk_buff *skb;

	if (skb_queue_len(list) == 0)
		return NULL;
	/* we assume all queued skb matches temporarily */
	skb = skb_dequeue(list);
	return skb;
}

/* no slab management or shared_info or DMA here, so shabby */
struct sk_buff *alloc_skb(struct net_device *nic)
{
	struct sk_buff *skb = (struct sk_buff *)malloc(sizeof(struct sk_buff));
	if (skb == NULL)
		error_msg_and_die("allocating sk_buff");

	skb->nic = nic;

	skb->data = (unsigned char *)malloc(skb->nic->mtu * sizeof(unsigned char));
	if (skb->data == NULL)
		error_msg_and_die("allocating sk_buff->data");
	skb->head = skb->tail = skb->data;

	skb->ip_summed = 0;
	skb->arp_try_times = ARP_MAX_TRY_TIMES;
	return skb;
}

void skb_free(struct sk_buff *skb)
{
	if (skb->head == NULL)
		error_msg_and_die("the skb->data has been freed\n");
	free(skb->head);
	skb->head = NULL;

	if (skb == NULL)
		error_msg_and_die("the skb has been freed\n");
	free(skb);
	skb = NULL;
}

void skb_queue_free(struct sk_buff_head *list)
{
	struct sk_buff *skb;

	pthread_spin_lock(&list->lock);
	while ( (skb = __skb_dequeue(list)) != NULL)
		skb_free(skb);
	pthread_spin_unlock(&list->lock);
}
