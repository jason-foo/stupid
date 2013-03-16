#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/netfilter_arp.h>

unsigned int drop_hook(unsigned int hooknum,
		       struct sk_buff *skb,
		       const struct net_device *in,
		       const struct net_device *out,
		       int (*okfn)(struct sk_buff*))
{
	return NF_DROP;
}

static struct nf_hook_ops netfilter_ops_in = {
	.hook = drop_hook,
	.pf = PF_INET,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops netfilter_ops_out = {
	.hook = drop_hook,
	.pf = PF_INET,
	.hooknum = NF_INET_POST_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops arp_filter_in = {
	.hook = drop_hook,
	.pf = NFPROTO_ARP,
	.hooknum = NF_ARP_IN,
	.priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops arp_filter_out = {
	.hook = drop_hook,
	.pf = NFPROTO_ARP,
	.hooknum = NF_ARP_OUT,
	.priority = NF_IP_PRI_FIRST,
};

static int __init init(void)
{
	nf_register_hook(&netfilter_ops_in);
	nf_register_hook(&netfilter_ops_out);
	nf_register_hook(&arp_filter_in);
	nf_register_hook(&arp_filter_out);

	return 0;
}

static void __exit exit(void)
{
	nf_unregister_hook(&netfilter_ops_in);
	nf_unregister_hook(&netfilter_ops_out);
	nf_unregister_hook(&arp_filter_in);
	nf_unregister_hook(&arp_filter_out);
}

module_init(init);
module_exit(exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jason-foo");
MODULE_DESCRIPTION("block all ip and arp packets");
