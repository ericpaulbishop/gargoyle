/*
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_bandwidth.h>


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
MODULE_DESCRIPTION("Match bandwidth used, designed for use with Gargoyle web interface (www.gargoyle-router.com)");

static spinlock_t bandwidth_lock = SPIN_LOCK_UNLOCKED;

/*
 * Shamelessly yoinked from xt_time.c
 * "That is so amazingly amazing, I think I'd like to steal it." -- Zaphod Beeblebrox
 */

extern struct timezone sys_tz; /* ouch */

static const u_int16_t days_since_year[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
};

static const u_int16_t days_since_leapyear[] = {
	0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335,
};

/*
 * Since time progresses forward, it is best to organize this array in reverse,
 * to minimize lookup time.  These are days since epoch since start of each year,
 * going back to 1970
 */
#define DSE_FIRST 2039
static const u_int16_t days_since_epoch_for_each_year_start[] = {
	/* 2039 - 2030 */
	25202, 24837, 24472, 24106, 23741, 23376, 23011, 22645, 22280, 21915,
	/* 2029 - 2020 */
	21550, 21184, 20819, 20454, 20089, 19723, 19358, 18993, 18628, 18262,
	/* 2019 - 2010 */
	17897, 17532, 17167, 16801, 16436, 16071, 15706, 15340, 14975, 14610,
	/* 2009 - 2000 */
	14245, 13879, 13514, 13149, 12784, 12418, 12053, 11688, 11323, 10957,
	/* 1999 - 1990 */
	10592, 10227, 9862, 9496, 9131, 8766, 8401, 8035, 7670, 7305,
	/* 1989 - 1980 */
	6940, 6574, 6209, 5844, 5479, 5113, 4748, 4383, 4018, 3652,
	/* 1979 - 1970 */
	3287, 2922, 2557, 2191, 1826, 1461, 1096, 730, 365, 0,
};

static inline int is_leap(unsigned int y)
{
	return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

/* end of code  yoinked from xt_time */



static time_t get_next_reset_time(struct ipt_bandwidth_info *info, time_t now)
{
	time_t next_reset = 0;
	if(info->reset_interval == BANDWIDTH_MINUTE)
	{
		next_reset = ( (long)(now/60) + 1)*60;
	}
	else if(info->reset_interval == BANDWIDTH_HOUR)
	{
		next_reset = ( (long)(now/(60*60)) + 1)*60*60;
	}
	else if(info->reset_interval == BANDWIDTH_DAY)
	{
		next_reset = ( (long)(now/(60*60*24)) + 1)*60*60*24;
	}	
	else if(info->reset_interval == BANDWIDTH_WEEK)
	{
		long days_since_epoch = now/(60*60*24);
		long current_weekday = (4 + days_since_epoch ) % 7 ;
		next_reset = (days_since_epoch + (7-current_weekday) )*(60*60*24);

	}
	else if(info->reset_interval == BANDWIDTH_MONTH)
	{
		/* yeah, most of this is yoinked from xt_time too */
		int year;
		int year_index;
		int year_day;
		int month;
		long days_since_epoch = now/(60*60*24);
		u_int16_t* month_start_days;	

		for (year_index = 0, year = DSE_FIRST; days_since_epoch_for_each_year_start[year_index] > days_since_epoch; year_index++)
		{
			year--;
		}
		year_day = days_since_epoch - days_since_epoch_for_each_year_start[year_index];
		if (is_leap(year)) 
		{
			month_start_days = (u_int16_t*)days_since_leapyear;
		}
		else
		{
			month_start_days = (u_int16_t*)days_since_year;
		}
		for (month = 11 ; month > 0 && month_start_days[month] > year_day; month--){}

		if(month == 11)
		{
			next_reset = days_since_epoch_for_each_year_start[year_index-1]*(60*60*24);
		}
		else
		{
			next_reset = (days_since_epoch_for_each_year_start[year_index] + month_start_days[month+1])*(60*60*24);
		}
	}
	return next_reset;
}

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
	struct ipt_bandwidth_info *info = ((const struct ipt_bandwidth_info*)matchinfo)->non_const_self;

	struct timeval test_time;
	time_t now;
	int match_found = 0;

	if(info->reset_interval != BANDWIDTH_NEVER)
	{
		do_gettimeofday(&test_time);
		now = test_time.tv_sec;
		now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */

		if(info->next_reset == 0)
		{
			spin_lock_bh(&bandwidth_lock);
			info->next_reset = get_next_reset_time(info, now);
			spin_unlock_bh(&bandwidth_lock);
		}
		else if(info->next_reset < now)
		{
			spin_lock_bh(&bandwidth_lock);
			info->current_bandwidth = 0;
			info->next_reset = get_next_reset_time(info, now);
			spin_unlock_bh(&bandwidth_lock);
		}
	}

	spin_lock_bh(&bandwidth_lock);
	info->current_bandwidth = info->current_bandwidth + skb->len;
	if(info->gt_lt == BANDWIDTH_GT)
	{
		match_found = info->current_bandwidth > info->bandwidth_cutoff ? 1 : 0;
	}
	else if(info->gt_lt == BANDWIDTH_LT)
	{
		match_found = info->current_bandwidth < info->bandwidth_cutoff ? 1 : 0;
	}
	spin_unlock_bh(&bandwidth_lock);


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
	struct ipt_bandwidth_info *b = (struct ipt_bandwidth_info*)matchinfo;
	b->non_const_self = b;
	return 1;
}


static struct ipt_match bandwidth_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"bandwidth",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.name		= "bandwidth",
	.match		= &match,
	.family		= AF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.matchsize	= sizeof(struct ipt_bandwidth_info),
#endif
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	return ipt_register_match(&bandwidth_match);

}

static void __exit fini(void)
{
	ipt_unregister_match(&bandwidth_match);
}

module_init(init);
module_exit(fini);

