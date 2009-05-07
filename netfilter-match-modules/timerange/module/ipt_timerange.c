/*  timerange --	A netfilter module to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009 by Eric Bishop <eric@gargoyle-router.com>
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
#include <linux/time.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_timerange.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	#include <linux/ktime.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	#define ipt_register_match      xt_register_match
	#define ipt_unregister_match    xt_unregister_match
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	#include <linux/ip.h>
#else
	#define skb_network_header(skb) (skb)->nh.raw 
#endif


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match time ranges, designed for use with Gargoyle web interface (www.gargoyle-router.com)");


extern struct timezone sys_tz;

static int match(	const struct sk_buff *skb,
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
			int *hotdrop)
{
	const struct ipt_timerange_info *info = matchinfo;
	time_t stamp_time;
	int weekday;
	int seconds_since_midnight;
	int test_index;
	int match_found;

	struct timeval test_time;
	
	do_gettimeofday(&test_time);
	stamp_time = test_time.tv_sec;
	stamp_time = stamp_time -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
	seconds_since_midnight = stamp_time % 86400; /* 86400 seconds per day */
	weekday = (4 + (stamp_time/86400)) % 7;      /* 1970-01-01 (time=0) was a Thursday (4). */

	/*
	printk("time=%d, since midnight = %d, day=%d, minuteswest=%d\n", stamp_time, seconds_since_midnight, weekday, sys_tz.tz_minuteswest);
	*/

	match_found = 0;
	if(info->type == HOURS)
	{
		for(test_index=0; info->ranges[test_index] != -1 && match_found == 0 && seconds_since_midnight >= info->ranges[test_index]; test_index=test_index+2)
		{
			match_found = seconds_since_midnight >= info->ranges[test_index] && seconds_since_midnight <= info->ranges[test_index+1] ? 1 : match_found;
		}
	}
	else if(info->type == WEEKDAYS)
	{
		match_found = info->days[weekday];
	}
	else if(info->type == DAYS_HOURS)
	{
		match_found = info->days[weekday];
		if(match_found == 1)
		{
			match_found = 0;
			for(test_index=0; info->ranges[test_index] != -1 && match_found == 0 && seconds_since_midnight >= info->ranges[test_index]; test_index=test_index+2)
			{
				match_found = seconds_since_midnight >= info->ranges[test_index] && seconds_since_midnight <= info->ranges[test_index+1] ? 1 : match_found;
			}
		}
	}
	else if(info->type == WEEKLY_RANGE)
	{
		time_t seconds_since_sunday_midnight = seconds_since_midnight + (weekday*86400);
		for(test_index=0; info->ranges[test_index] != -1 && match_found == 0 && seconds_since_sunday_midnight >= info->ranges[test_index]; test_index=test_index+2)
		{
			match_found = seconds_since_sunday_midnight >= info->ranges[test_index] && seconds_since_sunday_midnight <= info->ranges[test_index+1] ? 1 : match_found;
		}
		
	}
	

	return match_found;
}



static int checkentry(	const char *tablename,
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
{
	return 1;
}


static struct ipt_match timerange_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"timerange",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.name		= "timerange",
	.match		= &match,
	.family		= AF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.matchsize	= sizeof(struct ipt_timerange_info),
#endif
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	return ipt_register_match(&timerange_match);

}

static void __exit fini(void)
{
	ipt_unregister_match(&timerange_match);
}

module_init(init);
module_exit(fini);

