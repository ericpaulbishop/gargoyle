/*  timerange --	An xtables extension to match multiple timeranges within a week
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009-2010 by Eric Bishop <eric@gargoyle-router.com>
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

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_timerange.h>
#include <linux/math64.h>
#include <linux/ktime.h>


#include <linux/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match time ranges, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS("ipt_timerange");
MODULE_ALIAS("ip6t_timerange");
MODULE_ALIAS("ebt_timerange");
MODULE_ALIAS("arpt_timerange");


extern struct timezone sys_tz;


static bool timerange_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_timerange_info *info = (const struct xt_timerange_info*)(par->matchinfo);

	
	ktime_t stamp_time;
	int weekday;
	int seconds_since_midnight;
	s64 days_since_epoch;
	s64 weeks_since_epoch;
	int test_index;
	int match_found;

	struct timespec64 test_time;
	
	ktime_get_real_ts64(&test_time);
	stamp_time = test_time.tv_sec;
	stamp_time = stamp_time -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
	days_since_epoch = div_s64_rem(stamp_time,86400,&seconds_since_midnight); /* 86400 seconds per day */
	weeks_since_epoch = div_s64_rem(4 + days_since_epoch,7,&weekday);      /* 1970-01-01 (time=0) was a Thursday (4). */

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
		ktime_t seconds_since_sunday_midnight = seconds_since_midnight + (weekday*86400);
		for(test_index=0; info->ranges[test_index] != -1 && match_found == 0 && seconds_since_sunday_midnight >= info->ranges[test_index]; test_index=test_index+2)
		{
			match_found = seconds_since_sunday_midnight >= info->ranges[test_index] && seconds_since_sunday_midnight <= info->ranges[test_index+1] ? 1 : match_found;
		}
		
	}
	
	match_found = info->invert == 0 ? match_found : !match_found;
	return match_found;
}

static int checkentry(const struct xt_mtchk_param *par)
{
	return 0;
}

static struct xt_match timerange_mt_reg[]  __read_mostly = 
{
	{	
		.name		= "timerange",
		.family		= NFPROTO_UNSPEC,
		.match		= timerange_mt,
		.matchsize	= sizeof(struct xt_timerange_info),
		.checkentry	= checkentry,
		.me		= THIS_MODULE,
	},
};

static int __init init(void)
{
	return xt_register_matches(timerange_mt_reg, ARRAY_SIZE(timerange_mt_reg));
}

static void __exit fini(void)
{
	xt_unregister_matches(timerange_mt_reg, ARRAY_SIZE(timerange_mt_reg));
}

module_init(init);
module_exit(fini);

