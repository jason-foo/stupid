#include <stdio.h>
#include <pthread.h>
#include <memory.h>
#include <stdlib.h>

#include "ip_route.h"
#include "utils.h"
#include "netdevice.h"

extern struct net_device nic;
struct route_table_base route_table_base;

void route_table_init()
{
	/* for now the table is static, so all entries are initialized here */
	struct route_table *r1, *r2;

	r1 = (struct route_table *)malloc(sizeof(struct route_table));
	r2 = (struct route_table *)malloc(sizeof(struct route_table));

	if (!r1 || !r2)
		error_msg_and_die("alloc route_table entry");
	route_table_base.table = r1;
	r1->next = r2;
	r2->next = NULL;

	/* route for lan */
	r1->dst = nic.ip & nic.netmask;
	r1->gateway = 0;
	r1->mask = 24;
	r1->metric = 0;
	r1->interface = nic.ip;

	r2->dst = 0;
	r2->gateway = nic.gateway;
	r2->mask = 0;
	r2->metric = 10;
	r2->interface = nic.ip;
}

/* we are simulating only one network adapter now, so we just return the ip
 * address to be wrapped in ip header, but no device info */
__be32 route_table_lookup(__be32 dst)
{
	struct route_table *current;
	__be32 mask;

	current = route_table_base.table;
	while (current)
	{
		mask = 0xffffffffll >> (32 - current->mask);
		if ( (dst & mask) == (current->dst & mask) )
		{
			if (current->interface == nic.ip)
			{
				if (current->gateway)
					return current->gateway;
				else
					return dst;
			}
		}
		current = current->next;
	}
	return -1;
}
