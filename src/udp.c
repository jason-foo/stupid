#include <stdio.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdlib.h>
#include <memory.h>

#include "sock.h"
#include "skbuff.h"
#include "udp.h"
#include "utils.h"
#include "ip.h"

#define  port_range_lower_bound 32768
#define  port_range_up_bound 61000
#define  port_range_size (port_range_up_bound - port_range_lower_bound + 1)

char port_table[port_range_size];
int port_occupied;

/* really small chance with race condition here, so lock will be added later */

__be16 udp_ephemeral_port_alloc()
{
	int ret;
	int try = 200;

	while (try--)
	{
		ret = rand() / (double)RAND_MAX * port_range_size;
		if (port_table[ret] == 0)
		{
			port_table[ret] = 1;
			port_occupied++;
			return htons(port_range_lower_bound + ret);
		}
	}
	printf("didn't get available ephemeral port\n");
	return ret;
}

void udp_ephemeral_port_free(__be16 port)
{
	int hport = ntohs(port);
	if (port_table[hport] == 0)
	{
		printf("error release udp ephemeral port\n");
		return;
	}
	port_table[hport] = 0;
	port_occupied--;
}

void udp_rcv(struct sk_buff *skb)
{
	struct udphdr *uh;

	uh = skb->h.uh = (struct udphdr *) skb->data;
	skb->data += 8;
	skb->len -= 8;

	/* checksum is related to a pseudo L3 header, not checked for now */

	printf("    UDP: sport %d, dport %d, length %d\n",
	       ntohs(uh->source), ntohs(uh->dest), skb->len);

	data_dump("    udp data", skb->data, skb->len);

	sock_try_insert(skb);
}

/* this length should be determined by IP options */
int udp_all_header_len()
{
	/* we assume no IP options here */
	return 42;
}

void udp_send(unsigned char *data, unsigned int len, struct sock *sock)
{
	struct udphdr *uh;
	struct sk_buff *skb;
	struct pseudo_header *ph;

	skb = alloc_skb(sock->nic);
	skb->protocol = IPPROTO_UDP;
	skb->len = len + 8;
	skb->sock = sock;

	/* the space between head and data should be enough for L2, L3 and L4
	   headers */
	/* and we assume that fragmentation is unnecessary */

	/* udp payload */
	skb->data = skb->head + udp_all_header_len();
	memcpy(skb->data, data, len);
	/* udp header */
	skb->data -= 8;
	skb->h.uh = uh = (struct udphdr *) skb->data;

	if (skb->sock->port == 0)
		skb->sock->port = htons(udp_ephemeral_port_alloc());
	uh->source = skb->sock->port;
	uh->dest = sock->dest.port;
	uh->len = htons(skb->len);
	uh->check = 0;

	ph = (struct pseudo_header *) (skb->data - 12);
	ph->saddr = sock->nic->ip;
	ph->daddr = sock->dest.ip;
	ph->resv = 0;
	ph->protocol = skb->protocol;
	ph->len = uh->len;
	uh->check = in_checksum((__u16 *)ph, 12 + skb->len);

	ip_send(skb);
}
