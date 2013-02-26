#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "skbuff.h"
#include "sock.h"
#include "utils.h"

void skb_queue_head(struct sk_buff_head *list,
				    struct sk_buff *newsk)
{
	pthread_mutex_lock(&list->lock);
	__skb_queue_head(list, newsk);
	pthread_mutex_unlock(&list->lock);
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *ret;

	pthread_mutex_lock(&list->lock);
	ret = __skb_dequeue(list);
	pthread_mutex_unlock(&list->lock);
	return ret;
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
	return skb;
}

void skb_free(struct sk_buff *skb)
{
	free(skb->head);
	/* free(skb); */
}
