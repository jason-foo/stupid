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

struct sk_buff *alloc_skb(struct sock *sock)
{
	struct sk_buff *skb = (struct sk_buff *)malloc(sizeof(struct sk_buff));
	if (skb == NULL)
		error_msg_and_die("allocating sk_buff");

	/* should mtu be defined in net_device structure? */
	skb->sock = sock;
	skb->data = (unsigned char *)malloc(skb->sock->mtu * sizeof(unsigned char));
	if (skb->data == NULL)
		error_msg_and_die("allocating sk_buff->data");
	return skb;
}

void free_skb(struct sk_buff *skb)
{
	free(skb->data);
	free(skb);
}
