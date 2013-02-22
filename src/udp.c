#include <stdio.h>
#include <netinet/in.h>
#include <linux/udp.h>

#include "skbuff.h"
#include "udp.h"

void udp_rcv(struct sk_buff *skb)
{
	struct udphdr *uh;

	uh = skb->h.uh = (struct udphdr *) skb->data;
	skb->data += 8;

	/* checksum is related to a pseudo L3 header, not handled for now */

	printf("udp packet sport: %5d, dport %5d, length %5d\n",
	       ntohs(uh->source), ntohs(uh->dest), ntohs(uh->len));

	/* add to sock's received queue */
	/* or we can run a test application program here */

}

void udp_send(unsigned char *data, unsigned int len)
{
	struct udphdr *uh;
	struct sk_buff *skb;
	int i;

	skb = alloc_skb();
	/* the space between head and data should be enough for L2, L3 and L4
	   headers */
	/* and we assume that fragmentation is unnecessary for now*/
	if (len > 100)
	{
		printf("sorry, the data you want to transmit is too long\n");
		goto out;
	}

	/* udp payload */
	skb->data = skb->head + 42;
	for (i = 0; i < len; i++)
		skb->data[i] = data[i];
	/* udp header */
	skb->data -= 8;
	uh = (struct udphdr *) skb->data;
	uh->source = htons(48250);
	uh->dest = htons(7);	/* send the datagram to echo reply server */
	uh->len = htons(8 + len);
	/* checksum with pseudo header, so we need a way to find the ip address */
	/* so sock and arp table must be in concern here? */
	/* I'll do it later */

out:
	return;
}
