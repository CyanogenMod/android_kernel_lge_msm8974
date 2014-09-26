/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack_l4proto.h>

static unsigned int nf_ct_generic_timeout __read_mostly;

static bool nf_generic_should_process(u8 proto)
{
	switch (proto) {
#ifdef CONFIG_NF_CT_PROTO_SCTP_MODULE
	case IPPROTO_SCTP:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_DCCP_MODULE
	case IPPROTO_DCCP:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_GRE_MODULE
	case IPPROTO_GRE:
		return false;
#endif
#ifdef CONFIG_NF_CT_PROTO_UDPLITE_MODULE
	case IPPROTO_UDPLITE:
		return false;
#endif
	default:
		return true;
	}
}

static bool generic_pkt_to_tuple(const struct sk_buff *skb,
				 unsigned int dataoff,
				 struct nf_conntrack_tuple *tuple)
{
	return false;
}

static bool generic_invert_tuple(struct nf_conntrack_tuple *tuple,
				 const struct nf_conntrack_tuple *orig)
{
	return false;
}

/* Print out the per-protocol part of the tuple. */
static int generic_print_tuple(struct seq_file *s,
			       const struct nf_conntrack_tuple *tuple)
{
	return 0;
}

static unsigned int *generic_get_timeouts(struct net *net)
{
	return &nf_ct_generic_timeout;
}

/* Returns verdict for packet, or -1 for invalid. */
static int generic_packet(struct nf_conn *ct,
			  const struct sk_buff *skb,
			  unsigned int dataoff,
			  enum ip_conntrack_info ctinfo,
			  u_int8_t pf,
			  unsigned int hooknum,
			  unsigned int *timeout)
{
	return NF_DROP;
}

/* Called when a new connection for this protocol found. */
static bool generic_new(struct nf_conn *ct, const struct sk_buff *skb,
			unsigned int dataoff, unsigned int *timeouts)
{
	return false
}

#ifdef CONFIG_SYSCTL
static struct ctl_table_header *generic_sysctl_header;
static struct ctl_table generic_sysctl_table[] = {
	{
		.procname	= "nf_conntrack_generic_timeout",
		.data		= &nf_ct_generic_timeout,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
static struct ctl_table generic_compat_sysctl_table[] = {
	{
		.procname	= "ip_conntrack_generic_timeout",
		.data		= &nf_ct_generic_timeout,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{ }
};
#endif /* CONFIG_NF_CONNTRACK_PROC_COMPAT */
#endif /* CONFIG_SYSCTL */

struct nf_conntrack_l4proto nf_conntrack_l4proto_generic __read_mostly =
{
	.l3proto		= PF_UNSPEC,
	.l4proto		= 255,
	.name			= "unknown",
	.pkt_to_tuple		= generic_pkt_to_tuple,
	.invert_tuple		= generic_invert_tuple,
	.print_tuple		= generic_print_tuple,
	.packet			= generic_packet,
	.get_timeouts		= generic_get_timeouts,
	.new			= generic_new,
#ifdef CONFIG_SYSCTL
	.ctl_table_header	= &generic_sysctl_header,
	.ctl_table		= generic_sysctl_table,
#ifdef CONFIG_NF_CONNTRACK_PROC_COMPAT
	.ctl_compat_table	= generic_compat_sysctl_table,
#endif
#endif
};
