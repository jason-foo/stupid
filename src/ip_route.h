#ifndef _IP_ROUTE_H
#define _IP_ROUTE_H

#include <linux/types.h>
#include <pthread.h>

struct route_table {
	struct route_table *next;
	__be32 dst;
	__be32 gateway;
	int mask;
	unsigned int metric;
	__be32 interface;
};

struct route_table_base {
	struct route_table *table;
	pthread_mutex_t lock;
};

extern __be32 route_table_lookup(__be32 dst);
extern void route_table_init();

#endif
