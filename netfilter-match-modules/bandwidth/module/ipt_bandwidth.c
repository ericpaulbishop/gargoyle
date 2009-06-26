/*  bandwidth --	An iptables extension for bandwidth monitoring/control
 *  			Can be used to efficiently monitor bandwidth and/or implement bandwidth quotas
 *  			Can be queried using the iptbwctl userspace library
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26) 
#include <linux/semaphore.h> 
#else 
#include <asm/semaphore.h> 
#endif 


#include "bandwidth_deps/tree_map.h"
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

/*
#define BANDWIDTH_DEBUG 1
*/


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match bandwidth used, designed for use with Gargoyle web interface (www.gargoyle-router.com)");

static spinlock_t bandwidth_lock = SPIN_LOCK_UNLOCKED;
static struct semaphore userspace_lock;

static string_map* id_map = NULL;

typedef struct info_map_pair_struct
{
	struct ipt_bandwidth_info* info;
	long_map* ip_map;
}info_map_pair;

static unsigned char* output_buffer = NULL;
static unsigned long output_buffer_index = 0;
static unsigned long output_buffer_length = 0;

static unsigned char* input_buffer = NULL;
static unsigned long input_buffer_index = 0;
static unsigned long input_buffer_length = 0;
static char input_id[BANDWIDTH_MAX_ID_LENGTH] = "";
static time_t input_backup_time = 0;



/*
 * Shamelessly yoinked from xt_time.c
 * "That is so amazingly amazing, I think I'd like to steal it." 
 *      -- Zaphod Beeblebrox
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
	//first calculate when next reset would be if reset_time is 0 (which it may be)
	time_t next_reset = 0;
	if(info->reset_interval == BANDWIDTH_MINUTE)
	{
		next_reset = ( (long)(now/60) + 1)*60;
		if(info->reset_time > 0)
		{
			time_t alt_reset = next_reset + info->reset_time - 60;
			next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
		}
	}
	else if(info->reset_interval == BANDWIDTH_HOUR)
	{
		next_reset = ( (long)(now/(60*60)) + 1)*60*60;
		if(info->reset_time > 0)
		{
			time_t alt_reset = next_reset + info->reset_time - (60*60);
			next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
		}
	}
	else if(info->reset_interval == BANDWIDTH_DAY)
	{
		next_reset = ( (long)(now/(60*60*24)) + 1)*60*60*24;
		if(info->reset_time > 0)
		{
			time_t alt_reset = next_reset + info->reset_time - (60*60*24);
			next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
		}
	}	
	else if(info->reset_interval == BANDWIDTH_WEEK)
	{
		long days_since_epoch = now/(60*60*24);
		long current_weekday = (4 + days_since_epoch ) % 7 ;
		next_reset = (days_since_epoch + (7-current_weekday) )*(60*60*24);
		if(info->reset_time > 0)
		{
			time_t alt_reset = next_reset + info->reset_time - (60*60*24*7);
			next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
		}
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
		
		/* end majority of yoinkage */
		
		time_t alt_reset = (days_since_epoch_for_each_year_start[year_index] + month_start_days[month])*(60*60*24) + info->reset_time;
		if(alt_reset > now)
		{
			next_reset = alt_reset;
		}
		else if(month == 11)
		{
			next_reset = days_since_epoch_for_each_year_start[year_index-1]*(60*60*24) + info->reset_time;
		}
		else
		{
			next_reset = (days_since_epoch_for_each_year_start[year_index] + month_start_days[month+1])*(60*60*24) + info->reset_time;
		}
	}


	return next_reset;
}


static void set_bandwidth_to_zero(unsigned long key, void*value)
{
	*((uint64_t*)value) = 0;
}


static void add_to_output_buffer(unsigned long key, void*value)
{
	*( (uint32_t*)(output_buffer + output_buffer_index) ) = (uint32_t)key;
	*( (uint64_t*)(output_buffer + 4 + output_buffer_index) ) = *( (uint64_t*)value );
	output_buffer_index = output_buffer_index + BANDWIDTH_ENTRY_LENGTH;
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
	int match_found;

	/* want to get this within lock, intialize to NULL */
	long_map* ip_map = NULL;

	do_gettimeofday(&test_time);
	now = test_time.tv_sec;
	now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */

	if(info->reset_interval != BANDWIDTH_NEVER)
	{
		if(info->next_reset < now)
		{
			spin_lock_bh(&bandwidth_lock);
			if(info->next_reset < now) /* check again after waiting for lock */
			{
				//do reset
				info_map_pair* imp = NULL;
				info->current_bandwidth = 0;
				info->next_reset = get_next_reset_time(info, now);
				
				imp = (info_map_pair*)get_string_map_element(id_map, info->id);
				if(imp != NULL)
				{
					ip_map = imp->ip_map;
					apply_to_every_long_map_value(ip_map, set_bandwidth_to_zero);
				}
			}
			spin_unlock_bh(&bandwidth_lock);
		}
	}

	uint64_t* bws[2] = {NULL, NULL};
	if(info->type == BANDWIDTH_COMBINED)
	{
		spin_lock_bh(&bandwidth_lock);
		if(ip_map == NULL)
		{
			info_map_pair* imp = (info_map_pair*)get_string_map_element(id_map, info->id);
			if(imp != NULL)
			{
				ip_map = imp->ip_map;
			}	
		}
		if(ip_map != NULL)
		{
			bws[0] = (uint64_t*)get_long_map_element(ip_map, 0);
			if(bws[0] == NULL)
			{
				bws[0] = (uint64_t*)kmalloc(sizeof(uint64_t), GFP_ATOMIC);
				*(bws[0]) = skb->len;
				set_long_map_element(ip_map, (unsigned long)(*(bws[0])), (void*)(bws[0]) );
			}
			else
			{
				*(bws[0]) = *(bws[0]) + skb->len;
			}
		}
		info->current_bandwidth = info->current_bandwidth + skb->len;
	}
	else
	{
		int bw_ip_index;
		uint32_t bw_ips[2] = {0, 0};
		struct iphdr* iph = (struct iphdr*)(skb_network_header(skb));
		if(info->type == BANDWIDTH_INDIVIDUAL_SRC)
		{
			//src ip
			bw_ips[0] = iph->saddr;
		}
		else if (info->type == BANDWIDTH_INDIVIDUAL_DST)
		{
			//dst ip
			bw_ips[0] = iph->daddr;
		}
		else if(info->type ==  BANDWIDTH_INDIVIDUAL_LOCAL ||  info->type == BANDWIDTH_INDIVIDUAL_REMOTE)
		{
			//remote or local ip -- need to test both src && dst
			uint32_t src_ip = iph->saddr;
			uint32_t dst_ip = iph->daddr;
			if(info->type == BANDWIDTH_INDIVIDUAL_LOCAL)
			{
				bw_ips[0] = ((info->local_subnet_mask & src_ip) == info->local_subnet) ? src_ip : 0;
				bw_ips[1] = ((info->local_subnet_mask & dst_ip) == info->local_subnet) ? dst_ip : 0;
			}
			else if(info->type == BANDWIDTH_INDIVIDUAL_REMOTE)
			{
				bw_ips[0] = ((info->local_subnet_mask & src_ip) != info->local_subnet ) ? src_ip : 0;
				bw_ips[1] = ((info->local_subnet_mask & dst_ip) != info->local_subnet ) ? dst_ip : 0;
			}
		}
		
		spin_lock_bh(&bandwidth_lock);
		if(ip_map == NULL)
		{
			info_map_pair* imp = (info_map_pair*)get_string_map_element(id_map, info->id);
			if(imp != NULL)
			{
				ip_map = imp->ip_map;
			}	
		}
		for(bw_ip_index=0; bw_ip_index < 2 && ip_map != NULL; bw_ip_index++)
		{
			uint32_t bw_ip = bw_ips[bw_ip_index];
			if(bw_ip != 0)
			{
				uint64_t* oldval = get_long_map_element(ip_map, (unsigned long)bw_ip);
				if(oldval == NULL)
				{
					oldval  = (uint64_t*)kmalloc(sizeof(uint64_t), GFP_ATOMIC);
					*oldval = skb->len;
					set_long_map_element(ip_map, (unsigned long)bw_ip, (void*)oldval);
				}
				else
				{
					*oldval = *oldval + skb->len;
				}
				bws[bw_ip_index] = oldval;
			}
		}
	}

	match_found = 0;
	if(info->cmp == BANDWIDTH_GT)
	{
		match_found = bws[0] != NULL ? ( *(bws[0]) > info->bandwidth_cutoff ? 1 : match_found ) : match_found;
		match_found = bws[1] != NULL ? ( *(bws[1]) > info->bandwidth_cutoff ? 1 : match_found ) : match_found;
		match_found = info->current_bandwidth > info->bandwidth_cutoff ? 1 : match_found;
	}
	else if(info->cmp == BANDWIDTH_LT)
	{
		match_found = bws[0] != NULL ? ( *(bws[0]) < info->bandwidth_cutoff ? 1 : match_found ) : match_found;
		match_found = bws[1] != NULL ? ( *(bws[1]) < info->bandwidth_cutoff ? 1 : match_found ) : match_found;
		match_found = info->current_bandwidth < info->bandwidth_cutoff ? 1 : match_found;
	}
	spin_unlock_bh(&bandwidth_lock);

	return match_found;
}


static int ipt_bandwidth_set_ctl(struct sock *sk, int cmd, void *user, u_int32_t len)
{
	down(&userspace_lock);

	char query[BANDWIDTH_QUERY_LENGTH];
	copy_from_user(query, user, BANDWIDTH_QUERY_LENGTH);
	

	/* 
	 * buffer starts with length of total entries to expect (-1 for first entry which should be last_backup_time) 
	 * Unlike when we're doing a get, buffer length is valid only with first query, otherwise it is zero.
	 * In the case it is non-zero (initial query) it is followed by the id that is being querried.
	 */
	uint32_t *buffer_length = (uint32_t*)query;
	int query_index = 4; /* offset after buffer_length */
	if(*buffer_length != 0)
	{
		if(input_buffer != NULL)
		{
			kfree(input_buffer);
		}
		input_buffer_index = 0;
		input_buffer_length = *buffer_length;
		input_buffer = (char*)kmalloc(input_buffer_length, GFP_ATOMIC);
		
		/* read id, and offset query index */
		memcpy(input_id, query+4, BANDWIDTH_MAX_ID_LENGTH);
		query_index = query_index + BANDWIDTH_MAX_ID_LENGTH;

		/* read backup_time */
		input_backup_time = *( (time_t*)(query+query_index) );
		query_index = query_index + sizeof(time_t);

	}

	int clear_input_buffer = 1;
	long_map* ip_map = NULL;
	struct ipt_bandwidth_info* info = NULL;
	if(input_buffer_index < input_buffer_length && strlen(input_id) > 0)
	{
		info_map_pair* imp;
		spin_lock_bh(&bandwidth_lock);
		
		imp = get_string_map_element(id_map, input_id);
		if(imp != NULL)
		{
			ip_map = imp->ip_map;
			info = imp->info;
		}
		clear_input_buffer = ip_map == NULL ? 1 : 0;
		spin_unlock_bh(&bandwidth_lock);

		/* note: even though we release spinlock we retain ip_map variable for later. 
		 * There's no reason this should cause a problem -- we re-activate lock before
		 * we query/change anything.  We just retain the pointer, which shouldn't change
		 */
	}
	if(clear_input_buffer == 0)
	{
		int query_start_index = query_index;
		int input_buffer_start_index = input_buffer_index;
		while(query_index+BANDWIDTH_ENTRY_LENGTH < BANDWIDTH_QUERY_LENGTH-1 && input_buffer_index < input_buffer_length)
		{
			query_index=query_index+BANDWIDTH_ENTRY_LENGTH;
			input_buffer_index=input_buffer_index+BANDWIDTH_ENTRY_LENGTH;
		}
		memcpy(input_buffer+input_buffer_start_index, query+query_start_index, input_buffer_index-input_buffer_start_index );

		/* done reading input, set it */
		if(input_buffer_index >= input_buffer_length)
		{

			int backup_time_is_consistent = 1;
			spin_lock_bh(&bandwidth_lock);
			
			/* 
			 * if we specify last backup time, check that next reset is consistent, 
			 * otherwise reset current_bandwidth to 0 
			 * 
			 * only applies to combined type -- otherwise we need to handle setting bandwidth
			 * through userspace library
			 */
			if(input_backup_time != 0 && info->reset_interval != BANDWIDTH_NEVER)
			{
				struct timeval test_time;
				time_t now;
				time_t next_reset;
				
				do_gettimeofday(&test_time);
				now = test_time.tv_sec;
				now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
				next_reset = get_next_reset_time(info, now);

				time_t next_reset_of_last_backup;
				time_t adjusted_last_backup_time = info->last_backup_time - (60 * sys_tz.tz_minuteswest); 
				next_reset_of_last_backup = get_next_reset_time(info, adjusted_last_backup_time);
				if(next_reset_of_last_backup != info->next_reset)
				{
					backup_time_is_consistent = 0;
				}
			}
			if(backup_time_is_consistent)
			{
				/* reset existing bandwidth values to 0 */
				info->current_bandwidth = 0;
				apply_to_every_long_map_value(ip_map, set_bandwidth_to_zero);


				/* set values from input buffer to ip_map */
				uint32_t out_index;
				for(out_index=0; out_index < input_buffer_length; out_index=out_index+BANDWIDTH_ENTRY_LENGTH)
				{
					uint32_t *ip = (uint32_t*)(input_buffer + out_index);
					uint64_t *bw = (uint64_t*)(input_buffer + out_index + 4);
					uint64_t *map_bw = (uint64_t*)get_long_map_element(ip_map, (unsigned long)(*ip) );
					if(map_bw == NULL)
					{
						map_bw = (uint64_t*)kmalloc(sizeof(uint64_t), GFP_ATOMIC);
						*map_bw = *bw;
						set_long_map_element(ip_map, (unsigned long)(*ip), map_bw);
					}
					else
					{
						*map_bw = *bw;
					}
					if(*ip == 0)
					{
						info->current_bandwidth = *bw;
					}
				}
			}


			spin_unlock_bh(&bandwidth_lock);
			clear_input_buffer = 1;
		}
	}
	if(clear_input_buffer) /* either we had an invalid id, or set operation is complete */
	{
		if(input_buffer != NULL)
		{
			kfree(input_buffer);
		}
		input_buffer = NULL;
		input_buffer_index = 0;
		input_buffer_length = 0;
		input_backup_time = 0;
		input_id[0] = '\0';
	}
	up(&userspace_lock);

	return 0;
}


static int ipt_bandwidth_get_ctl(struct sock *sk, int cmd, void *user, int *len)
{
	down(&userspace_lock);

	char query[BANDWIDTH_QUERY_LENGTH];
	copy_from_user(query, user, BANDWIDTH_QUERY_LENGTH);
	
	char id[BANDWIDTH_MAX_ID_LENGTH] = "";
	char type[BANDWIDTH_MAX_ID_LENGTH] = "";
	sscanf(query, "%s %s", id, type);
	
	#ifdef BANDWIDTH_DEBUG
		printk("ipt_bandwidth: query id=\"%s\" type=\"%s\"\n", id, type); 
	#endif

	// reinitialize output buffer to necessary length dynamically, begin output
	// last byte of output will be 0 if all data is finished dumping, 1 if theres more
	// and client needs to query again with blank query to get rest
	memset( query, 0, BANDWIDTH_QUERY_LENGTH);
	if(strcmp(id, "") != 0) /* this is initial query, not follow-up requesting more data from a previous query */
	{
		spin_lock_bh(&bandwidth_lock);
		long_map* ip_map = NULL;
		struct ipt_bandwidth_info* info = NULL;
		if(strlen(id) > 0)
		{

			info_map_pair* imp = (info_map_pair*)get_string_map_element(id_map, id);
			if(imp != NULL)
			{
				ip_map = imp->ip_map;
				info = imp->info;
				
				#ifdef BANDWIDTH_DEBUG
					printk("ipt_bandwidth: ip_map found for id=\"%s\"\n", id);
				#endif
			}
			else
			{
				printk("ipt_bandwidth: ip_map NOT found for id=\"%s\"\n", id);
			}

			/* note: even though we release spinlock we retain ip_map variable for later. 
			 * There's no reason this should cause a problem -- we re-activate lock before
			 * we query/change anything.  We just retain the pointer, which shouldn't change.
			 */
		}
		spin_unlock_bh(&bandwidth_lock);

		if( ip_map != NULL && (strcmp(type, "ALL") == 0 || strcmp(type, "") == 0))
		{
			int query_index = 0;
			struct timeval test_time;
			time_t now;

			
			if(output_buffer != NULL)
			{
				kfree(output_buffer);
				output_buffer = NULL;
			}
			output_buffer_index = 0;
			output_buffer_length = 0;


			spin_lock_bh(&bandwidth_lock);

			/* values only reset when a packet hits a rule, so 
			 * reset may have expired without data being reset.
			 * So, test if we need to reset values to zero 
			 */
			do_gettimeofday(&test_time);
			now = test_time.tv_sec;
			now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
			if(info->reset_interval != BANDWIDTH_NEVER)
			{
				if(info->next_reset < now)
				{
					//do reset
					info->current_bandwidth = 0;
					info->next_reset = get_next_reset_time(info, now);
					apply_to_every_long_map_value(ip_map, set_bandwidth_to_zero);
				}
			}


			
			output_buffer_length = (BANDWIDTH_ENTRY_LENGTH*ip_map->num_elements);
			output_buffer = (unsigned char *)kmalloc(output_buffer_length, GFP_ATOMIC);
			output_buffer_index = 0;
			apply_to_every_long_map_value(ip_map, add_to_output_buffer);
			output_buffer_index = 0;

			spin_unlock_bh(&bandwidth_lock);
			

			
			*((uint32_t*)(query)) = output_buffer_length;
			for(	query_index=4; 
				output_buffer_index < output_buffer_length && 
				query_index + BANDWIDTH_ENTRY_LENGTH < BANDWIDTH_QUERY_LENGTH; 
				query_index=query_index+BANDWIDTH_ENTRY_LENGTH
				)
			{
				*((uint32_t*)(query + query_index)) = *((uint32_t*)(output_buffer + output_buffer_index));
				*((uint64_t*)(query + query_index + 4)) = *((uint64_t*)(output_buffer + output_buffer_index + 4));
				output_buffer_index=output_buffer_index+BANDWIDTH_ENTRY_LENGTH;
			}
			if(output_buffer_index < output_buffer_length)
			{
				query[BANDWIDTH_QUERY_LENGTH-1] = 1;
			}
			else
			{
				kfree(output_buffer);
				output_buffer = NULL;
				output_buffer_index = 0;
				output_buffer_length = 0;
			}
		}
		else if( ip_map != NULL )
		{
			uint32_t query_ip = 0;
			uint32_t A,B,C,D, num_read;
			unsigned char* ip_buf = (unsigned char*)(&query_ip);
			num_read = sscanf(type, "%u.%u.%u.%u", &A, &B, &C, &D);
			if(num_read == 4)
			{
	
				struct timeval test_time;
				time_t now;	
				uint64_t* bw = NULL;

				spin_lock_bh(&bandwidth_lock);
				/* values only reset when a packet hits a rule, so 
				 * reset may have expired without data being reset.
				 * So, test if we need to reset values to zero 
				 */
				do_gettimeofday(&test_time);
				now = test_time.tv_sec;
				now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
				if(info->reset_interval != BANDWIDTH_NEVER)
				{
					if(info->next_reset < now)
					{
						//do reset
						info->current_bandwidth = 0;
						info->next_reset = get_next_reset_time(info, now);
						apply_to_every_long_map_value(ip_map, set_bandwidth_to_zero);
					}
				}
	
				ip_buf[0] = (unsigned char)A;
				ip_buf[1] = (unsigned char)B;
				ip_buf[2] = (unsigned char)C;
				ip_buf[3] = (unsigned char)D;

				*( (uint32_t*)query ) = BANDWIDTH_ENTRY_LENGTH;
				*( (uint32_t*)(query+4) ) = query_ip;
				*( (uint64_t*)(query+8) ) =  0;
				bw = (uint64_t*)get_long_map_element(ip_map, (unsigned long)query_ip);
				if(bw != NULL)
				{
					*( (uint64_t*)(query+8) ) =  *bw;
				}
				spin_unlock_bh(&bandwidth_lock);
			}
		}
	}
	else
	{
		if(output_buffer != NULL )
		{
			if(output_buffer[output_buffer_index] != '\0')
			{
				int query_index;
				*((uint32_t*)(query)) = output_buffer_length;
				for(	query_index=4;
					output_buffer_index < output_buffer_length && 
					query_index + BANDWIDTH_ENTRY_LENGTH < BANDWIDTH_QUERY_LENGTH; 
					query_index=query_index+BANDWIDTH_ENTRY_LENGTH
					)
				{
					*((uint32_t*)(query + query_index)) = *((uint32_t*)(output_buffer + output_buffer_index));
					*((uint64_t*)(query + query_index + 4)) = *((uint64_t*)(output_buffer + output_buffer_index + 4));
					output_buffer_index=output_buffer_index+BANDWIDTH_ENTRY_LENGTH;
				}
				if(output_buffer_index < output_buffer_length)
				{
					query[BANDWIDTH_QUERY_LENGTH-1] = 1;
				}
				else
				{
					kfree(output_buffer);
					output_buffer = NULL;
					output_buffer_index = 0;
					output_buffer_length = 0;
				}
			}
		}
	}
	copy_to_user(user, query, BANDWIDTH_QUERY_LENGTH);

	up(&userspace_lock);

	return 0;
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
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info*)matchinfo;
	#ifdef BANDWIDTH_DEBUG
		printk("checkentry called\n");	
	#endif
	
	if(info->ref_count == NULL) /* first instance, we're inserting rule */
	{
		info->ref_count = (unsigned long*)kmalloc(sizeof(unsigned long), GFP_ATOMIC);
		*(info->ref_count) = 1;
		info->non_const_self = info;
		
		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(info->ref_count) );
		#endif

		down(&userspace_lock);
		spin_lock_bh(&bandwidth_lock);
	
	
		info_map_pair  *imp = (info_map_pair*)get_string_map_element(id_map, info->id);
		if(imp != NULL)
		{
			printk("ipt_bandwidth: error, \"%s\" is a duplicate id\n", info->id); 
			spin_unlock_bh(&bandwidth_lock);
			up(&userspace_lock);
			return 0;
		}

		if(info->reset_interval != BANDWIDTH_NEVER)
		{
			struct timeval test_time;
			time_t now;
			do_gettimeofday(&test_time);
			now = test_time.tv_sec;
			now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */

			if(info->next_reset == 0)
			{
				info->next_reset = get_next_reset_time(info, now);
				
				/* 
				 * if we specify last backup time, check that next reset is consistent, 
				 * otherwise reset current_bandwidth to 0 
				 * 
				 * only applies to combined type -- otherwise we need to handle setting bandwidth
				 * through userspace library
				 */
				if(info->last_backup_time != 0 && info->type == BANDWIDTH_COMBINED)
				{
					time_t next_reset_of_last_backup;
					time_t adjusted_last_backup_time = info->last_backup_time - (60 * sys_tz.tz_minuteswest); 
					next_reset_of_last_backup = get_next_reset_time(info, adjusted_last_backup_time);
					if(next_reset_of_last_backup != info->next_reset)
					{
						info->current_bandwidth = 0;
					}
					info->last_backup_time = 0;
				}
			}
		}
	
		imp = (info_map_pair*)kmalloc( sizeof(info_map_pair), GFP_ATOMIC);
		imp->ip_map = initialize_long_map();
		imp->info = info;
		set_string_map_element(id_map, info->id, imp);
		if(info->type == BANDWIDTH_COMBINED)
		{
			uint64_t *bw = (uint64_t*)kmalloc(sizeof(uint64_t), GFP_ATOMIC);
			*bw = info->current_bandwidth;
			set_long_map_element(imp->ip_map, 0, bw);
		}


		spin_unlock_bh(&bandwidth_lock);
		up(&userspace_lock);
	}
	else
	{
		info->non_const_self = info;
		*(info->ref_count) = *(info->ref_count) + 1;
		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(info->ref_count) );
		#endif
		spin_lock_bh(&bandwidth_lock);
		info_map_pair* imp = (info_map_pair*)get_string_map_element(id_map, info->id);
		if(imp != NULL)
		{
			imp->info = info;
		}
		spin_unlock_bh(&bandwidth_lock);

	}
	#ifdef BANDWIDTH_DEBUG
		printk("checkentry complete\n");
	#endif
	return 1;
}

static void destroy(	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			void* matchinfo,
			unsigned int matchinfosize
#else
			const struct xt_match *match,
			void* matchinfo
#endif
		)
{
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info*)matchinfo;
	
	#ifdef BANDWIDTH_DEBUG
		printk("destroy called\n");
	#endif
	
	*(info->ref_count) = *(info->ref_count) - 1;
	
	#ifdef BANDWIDTH_DEBUG
		printk("   after decrement refcount = %ld\n", *(info->ref_count));
	#endif
	
	if(*(info->ref_count) == 0)
	{
		down(&userspace_lock);
		spin_lock_bh(&bandwidth_lock);
		
		info_map_pair* imp = (info_map_pair*)remove_string_map_element(id_map, info->id);
		if(imp != NULL)
		{
			long_map* ip_map = imp->ip_map;
			unsigned long num_destroyed;
			destroy_long_map(ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			kfree(imp);
			/* info portion of imp gets taken care of automatically */
		}	
		kfree(info->ref_count);

		spin_unlock_bh(&bandwidth_lock);
		up(&userspace_lock);
	}
	
	#ifdef BANDWIDTH_DEBUG
		printk("destroy complete\n");
	#endif
}

static struct nf_sockopt_ops ipt_bandwidth_sockopts = 
{
	.pf = PF_INET,
	.set_optmin = BANDWIDTH_SET,
	.set_optmax = BANDWIDTH_SET+1,
	.set = ipt_bandwidth_set_ctl,
	.get_optmin = BANDWIDTH_GET,
	.get_optmax = BANDWIDTH_GET+1,
	.get = ipt_bandwidth_get_ctl
};



static struct ipt_match bandwidth_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"bandwidth",
	&match,
	&checkentry,
	&destroy,
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
	.destroy	= &destroy,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	/* Register setsockopt */
	if (nf_register_sockopt(&ipt_bandwidth_sockopts) < 0)
	{
		printk("ipt_bandwidth: Can't register sockopts. Aborting\n");
	}

	id_map = initialize_string_map(0);

	init_MUTEX(&userspace_lock); 

	return ipt_register_match(&bandwidth_match);
}

static void __exit fini(void)
{
	down(&userspace_lock);
	spin_lock_bh(&bandwidth_lock);
	if(id_map != NULL)
	{
		unsigned long num_returned;
		info_map_pair **imps = (info_map_pair**)destroy_string_map(id_map, DESTROY_MODE_RETURN_VALUES, &num_returned);
		int imp_index;
		for(imp_index=0; imp_index < num_returned; imp_index++)
		{
			info_map_pair* imp = imps[imp_index];
			long_map* ip_map = imp->ip_map;
			unsigned long num_destroyed;
			destroy_long_map(ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			kfree(imp);
			/* info portion of imp gets taken care of automatically */
		}
	}
	ipt_unregister_match(&bandwidth_match);
	spin_unlock_bh(&bandwidth_lock);
	up(&userspace_lock);

}

module_init(init);
module_exit(fini);

