/*  netset
 *
 *
 *  Copyright © 2012 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <net/sock.h>
#include <net/ip.h>
#include <net/tcp.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_netset.h>



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	#define ipt_register_match      xt_register_match
	#define ipt_unregister_match    xt_unregister_match
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	#include <linux/ip.h>
#else
	#define skb_network_header(skb) (skb)->nh.raw 
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	#include <linux/netfilter/x_tables.h>
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match sets of network addresses and ports, designed for use with Gargoyle web interface (www.gargoyle-router.com)");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
		static bool 
	#else
		static int
	#endif
	match(		const struct sk_buff *skb,
			const struct net_device *in,
			const struct net_device *out,
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
				const struct xt_match *match,
			#endif
			const void *matchinfo,
			int offset,
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
				unsigned int protoff,
			#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
				const void *hdr,
				u_int16_t datalen,
			#endif
			#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
				bool *hotdrop
			#else
				int *hotdrop
			#endif	
			)
#else
	static bool match(const struct sk_buff *skb, const struct xt_match_param *par)
#endif
{
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
		const struct ipt_netset_info *info =	(const struct ipt_netset_info*)matchinfo;
	#else
		const struct ipt_netset_info *info = (const struct ipt_netset_info*)(par->matchinfo);
	#endif
	
	return info == NULL ? 1 : 0;
}



#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
	static bool
	#else
	static int
	#endif
	checkentry(	const char *tablename,
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
				const void *ip,
				const struct xt_match *match,
	#else
				const struct ipt_ip *ip,
	#endif
				void *matchinfo,
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
		    		unsigned int matchsize,
	#endif
				unsigned int hook_mask
				)
#else
	static bool checkentry(const struct xt_mtchk_param *par)
#endif
{
	return 1;
}


static struct ipt_match netset_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"netset",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.name		= "netset",
	.match		= &match,
	.family		= AF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.matchsize	= sizeof(struct ipt_netset_info),
#endif
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	return ipt_register_match(&netset_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&netset_match);
}

module_init(init);
module_exit(fini);

