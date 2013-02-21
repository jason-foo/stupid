#ifndef _NETDEVICE_H
#define _NETDEVICE_H

#include <net/if.h>

/* Since this is not a real net device in kernel, so some attribute about
 * hardware and many complex features are omitted for now. And some operations
 * will be added later */

struct net_device
{
	char name[IFNAMSIZ];
	struct net_device *next;
	
	/* config attributes, read from files or be configured with DHCP
	 * protocol later */

	unsigned int ip;
	unsigned int netmask;
	unsigned int gateway;
	unsigned char mac[7];
};

#endif
