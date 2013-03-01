#ifndef _NETDEVICE_H
#define _NETDEVICE_H

#include <net/if.h>
#include <linux/types.h>
#include <linux/if_ether.h>

/* Since this is not a real net device in kernel, so some attribute about
 * hardware and many complex features are omitted for now. And some operations
 * will be added later */

struct net_device
{
	char name[IFNAMSIZ];
	struct net_device *next;
	
	/* config attributes, read from files or be configured with DHCP
	 * protocol later */

	__be32 ip;
	__be32 netmask;
	__be32 gateway;
	unsigned char dev_addr[ETH_ALEN]; /* mac for ethernet */
	unsigned char broadcast[ETH_ALEN];

	unsigned int mtu;
	unsigned short type;	/* ethernet assumed here */
	unsigned short hard_header_len;
	unsigned char addr_len;	/* hardware address length */
};

extern struct net_device nic;

#endif
