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

/* #define BANDWIDTH_DEBUG 1 */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match bandwidth used, designed for use with Gargoyle web interface (www.gargoyle-router.com)");

extern struct timezone sys_tz; /* ouch */


static spinlock_t bandwidth_lock = SPIN_LOCK_UNLOCKED;
static struct semaphore userspace_lock;

static string_map* id_map = NULL;

typedef struct info_and_maps_struct
{
	struct ipt_bandwidth_info* info;
	long_map* ip_map;
	long_map* ip_history_map;
}info_and_maps;

typedef struct history_struct
{
	time_t first_start;
	time_t first_end;
	time_t last_end; /* also beginning of current time frame */
	uint32_t max_nodes;
	uint32_t num_nodes;
	uint32_t non_zero_nodes;
	uint32_t current_index;
	uint64_t* history_data;
} bw_history;



static unsigned char set_in_progress = 0;
static char set_id[BANDWIDTH_MAX_ID_LENGTH] = "";

/* 
 * function prototypes
 *
 * (prototypes only provided for 
 * functions not part of iptables API)
 *
*/


static void adjust_ip_for_backwards_time_shift(unsigned long key, void* value);
static void adjust_id_for_backwards_time_shift(char* key, void* value);
static void check_for_backwards_time_shift(time_t now);


static void shift_timezone_of_ip(unsigned long key, void* value);
static void shift_timezone_of_id(char* key, void* value);
static void check_for_timezone_shift(time_t now);



static bw_history* initialize_history(uint32_t max_nodes);
static unsigned char update_history(bw_history* history, time_t interval_start, time_t interval_end, struct ipt_bandwidth_info* info);



static void do_reset(unsigned long key, void* value);
static void set_bandwidth_to_zero(unsigned long key, void* value);
static void handle_interval_reset(info_and_maps* iam, time_t now);

static uint64_t pow64(uint64_t base, uint64_t pow);
static uint64_t get_bw_record_max(void); /* called by init to set global variable */
static uint64_t add_up_to_max(uint64_t original, uint64_t add, unsigned char is_check);

static inline int is_leap(unsigned int y);
static time_t get_next_reset_time(struct ipt_bandwidth_info *info, time_t now, time_t previous_reset);

static uint64_t* initialize_map_entries_for_ip(info_and_maps* iam, unsigned long ip, uint64_t initial_bandwidth);




static time_t backwards_check = 0;
static time_t backwards_adjust_current_time = 0;
static info_and_maps* backwards_adjust_iam = NULL;
static void adjust_ip_for_backwards_time_shift(unsigned long key, void* value)
{

	bw_history* history = (bw_history*)value;


	time_t next_start = history->first_start == 0 ? backwards_adjust_iam->info->previous_reset : history->first_start;
	if(next_start > backwards_adjust_current_time)
	{
		next_start = backwards_adjust_current_time;
	}
	time_t next_end = get_next_reset_time(backwards_adjust_iam->info, next_start, next_start);


	uint32_t new_num_nodes = 1; /* there's always at least one, since even when no updates have been performed we have current node */
	uint32_t node_start_index = history->num_nodes == history->max_nodes ? (history->current_index+1) % history->max_nodes : 0;
	
	history->current_index = node_start_index;
	if(history->first_start < backwards_adjust_current_time)
	{
		(history->history_data)[history->current_index] = 0;
	}
	history->first_start = 0;
	history->first_end = 0;
	history->last_end = 0;
	while(next_end < backwards_adjust_current_time && new_num_nodes < history->num_nodes)
	{		
		if(history->first_start == 0 && history->first_end == 0)
		{
			history->first_start = next_start;
			history->first_end = next_end;
		}
		history->last_end = next_end;
		backwards_adjust_iam->info->previous_reset = next_start;
		
		
		history->current_index = (history->current_index+1) % history->max_nodes;
		new_num_nodes++;

		next_start = next_end;
		next_end = get_next_reset_time(backwards_adjust_iam->info, next_start, next_start);
	}

	history->num_nodes = new_num_nodes;
	while(next_end < backwards_adjust_current_time)
	{
		update_history(history, next_start, next_end, backwards_adjust_iam->info);
		backwards_adjust_iam->info->previous_reset = next_start;
		next_start = next_end;
		next_end = get_next_reset_time(backwards_adjust_iam->info, next_start, next_start);
	}
	backwards_adjust_iam->info->previous_reset = next_start;
	backwards_adjust_iam->info->next_reset = next_end;

	/* zero positions that don't contain data */
	if(history->num_nodes < history->max_nodes)
	{
		uint32_t zero_index = (history->current_index + 1) % history->max_nodes;
		while(zero_index != node_start_index)
		{
			(history->history_data)[zero_index] = 0;
			zero_index = (zero_index + 1) % history->max_nodes;
		}
	}

	/* set value in ip_map to current index in history */
	set_long_map_element(backwards_adjust_iam->ip_map, key, (void*)(history->history_data + history->current_index) );
}
static void adjust_id_for_backwards_time_shift(char* key, void* value)
{
	info_and_maps* iam = (info_and_maps*)value;
	if(iam == NULL)
	{
		return;
	}
	if(iam->info == NULL)
	{
		return;
	}

	backwards_adjust_iam = iam;
	if( (iam->info->reset_is_constant_interval == 0 && iam->info->reset_interval == BANDWIDTH_NEVER) || iam->info->cmp == BANDWIDTH_CHECK )
	{
		return;
	}
	if(iam->ip_history_map != NULL)
	{
		apply_to_every_long_map_value(iam->ip_history_map, adjust_ip_for_backwards_time_shift);
	}
	else
	{
		time_t next_reset_after_adjustment = get_next_reset_time(iam->info, backwards_adjust_current_time, backwards_adjust_current_time);
		if(next_reset_after_adjustment < iam->info->next_reset)
		{
			iam->info->previous_reset = backwards_adjust_current_time;
			iam->info->next_reset = next_reset_after_adjustment;
		}
	}
	backwards_adjust_iam = NULL;
}
static void check_for_backwards_time_shift(time_t now)
{
	if(now < backwards_check && backwards_check != 0)
	{
		/* adjust */
		down(&userspace_lock);
		spin_lock_bh(&bandwidth_lock);


		backwards_adjust_current_time = now;
		apply_to_every_string_map_value(id_map, adjust_id_for_backwards_time_shift);


		spin_unlock_bh(&bandwidth_lock);
		up(&userspace_lock);

	}
	backwards_check = now;
}



static int old_minutes_west;
static time_t shift_timezone_current_time;
static info_and_maps* shift_timezone_iam = NULL;
static void shift_timezone_of_id(char* key, void* value);
static void shift_timezone_of_ip(unsigned long key, void* value);
static void shift_timezone_of_ip(unsigned long key, void* value)
{
	#ifdef BANDWIDTH_DEBUG
		unsigned long* ip = &key;
		printk("shifting ip = %d.%d.%d.%d\n", *((char*)ip), *(((char*)ip)+1), *(((char*)ip)+2), *(((char*)ip)+3) );
	#endif


	bw_history* history = (bw_history*)value;
	uint32_t timezone_adj = (old_minutes_west-sys_tz.tz_minuteswest)*60;
	time_t next_start = history->first_start == 0 ? shift_timezone_iam->info->previous_reset + timezone_adj : history->first_start + timezone_adj;
	time_t next_end = get_next_reset_time(shift_timezone_iam->info, next_start, next_start);
	#ifdef BANDWIDTH_DEBUG
		printk("  before jump:\n");
		printk("    current time = %ld\n",  shift_timezone_current_time);
		printk("    first_start  = %ld\n", history->first_start);
		printk("    first_end    = %ld\n", history->first_end);
		printk("    last_end     = %ld\n", history->last_end);
		printk("    next_start   = %ld\n", next_start);
		printk("    next_end     = %ld\n", next_end);
		printk("\n");
	#endif

	
	

	uint32_t new_num_nodes = 1;  /* always have at least current node, even when no updates have been performed */
	uint32_t node_start_index = history->num_nodes == history->max_nodes ? (history->current_index+1) % history->max_nodes : 0;
	
	history->first_start = 0;
	history->first_end = 0;
	history->last_end = 0;
	history->current_index = node_start_index;
	while(next_end < shift_timezone_current_time && new_num_nodes < history->num_nodes)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("    next_start    = %ld\n", next_start);
		#endif
		
		if(history->first_start == 0 && history->first_end == 0)
		{
			history->first_start = next_start;
			history->first_end = next_end;
		}
		history->last_end = next_end;
		shift_timezone_iam->info->previous_reset = next_start;
		
		
		history->current_index = (history->current_index+1) % history->max_nodes;
		new_num_nodes++;

		next_start = next_end;
		next_end = get_next_reset_time(shift_timezone_iam->info, next_start, next_start);
	}

	/* if we want deletion of nodes to show up at beginning of history (vs right before shift) we need to shift all bws */
	int node_shift = history->num_nodes - new_num_nodes;
	if(node_shift > 0)
	{
		int adj_index = node_start_index;
		uint64_t tmp_bw = (history->history_data)[ (adj_index+node_shift)% history->max_nodes ];
		int node_num;
		for(node_num=0; node_num < history->num_nodes; node_num++)
		{
			(history->history_data)[adj_index] = tmp_bw;
			tmp_bw = (history->history_data)[ (adj_index+node_shift)% history->max_nodes ];
			adj_index = (adj_index+1) % history->max_nodes;
		}
	}


	history->num_nodes = new_num_nodes;
	while(next_end < shift_timezone_current_time)
	{
		/* 
		 * if we do update_history here, we insert history nodes right before timezone shift,
		 * if we increment first_start, we insert history nodes at beginning of history 
		 */
		//update_history(history, next_start, next_end, shift_timezone_iam->info);
		history->first_start = history->first_end;
		history->first_end = get_next_reset_time(shift_timezone_iam->info, history->first_start, history->first_start);


		shift_timezone_iam->info->previous_reset = next_start;
		next_start = next_end;
		next_end = get_next_reset_time(shift_timezone_iam->info, next_start, next_start);
	}
	shift_timezone_iam->info->previous_reset = next_start;
	shift_timezone_iam->info->next_reset = next_end;

	/* zero positions that don't contain data */
	if(history->num_nodes < history->max_nodes)
	{
		uint32_t zero_index = (history->current_index + 1) % history->max_nodes;
		while(zero_index != node_start_index)
		{
			(history->history_data)[zero_index] = 0;
			zero_index = (zero_index + 1) % history->max_nodes;
		}
	}

	/* set value in ip_map to current index in history */
	set_long_map_element(shift_timezone_iam->ip_map, key, (void*)(history->history_data + history->current_index) );

	#ifdef BANDWIDTH_DEBUG
		printk("\n");
		printk("  after jump:\n");
		printk("    first_start = %ld\n", history->first_start);
		printk("    first_end   = %ld\n", history->first_end);
		printk("    last_end    = %ld\n", history->last_end);
		printk("\n\n");
	#endif

}
static void shift_timezone_of_id(char* key, void* value)
{
	info_and_maps* iam = (info_and_maps*)value;
	if(iam == NULL)
	{
		return;
	}
	if(iam->info == NULL)
	{
		return;
	}
	
	#ifdef BANDWIDTH_DEBUG
		printk("shifting id %s\n", key);
	#endif	

	shift_timezone_iam = iam;
	if( (iam->info->reset_is_constant_interval == 0 && iam->info->reset_interval == BANDWIDTH_NEVER) || iam->info->cmp == BANDWIDTH_CHECK )
	{
		return;
	}

	if(iam->ip_history_map != NULL)
	{
		apply_to_every_long_map_value(iam->ip_history_map, shift_timezone_of_ip);
	}
	else
	{
		iam->info->previous_reset = iam->info->previous_reset + ((old_minutes_west - sys_tz.tz_minuteswest )*60);
		if(iam->info->previous_reset > shift_timezone_current_time)
		{
			iam->info->next_reset = get_next_reset_time(iam->info, shift_timezone_current_time, shift_timezone_current_time);
			iam->info->previous_reset = shift_timezone_current_time;
		}
		else
		{
			iam->info->next_reset = get_next_reset_time(iam->info, shift_timezone_current_time, iam->info->previous_reset);
			while (iam->info->next_reset < shift_timezone_current_time)
			{
				iam->info->previous_reset = iam->info->next_reset;
				iam->info->next_reset = get_next_reset_time(iam->info, iam->info->previous_reset, iam->info->previous_reset);
			}
		}

	}
	shift_timezone_iam = NULL;
	
	
}
static void check_for_timezone_shift(time_t now)
{

	if(sys_tz.tz_minuteswest != old_minutes_west)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("timezone shift detected, shifting...\n");
		#endif	


		down(&userspace_lock);
		spin_lock_bh(&bandwidth_lock);


		shift_timezone_current_time = now;
		apply_to_every_string_map_value(id_map, shift_timezone_of_id);
		old_minutes_west = sys_tz.tz_minuteswest;
	

		/*
		 * make sure timezone shift doesn't inadvertantly 
		 * trigger backwards shift since
		 * we've already dealt with the problem 
		 */
		backwards_check = now; 



		spin_unlock_bh(&bandwidth_lock);
		up(&userspace_lock);
	}
	
}



static bw_history* initialize_history(uint32_t max_nodes)
{
	bw_history* new_history = (bw_history*)kmalloc(sizeof(bw_history), GFP_ATOMIC);
	if(new_history != NULL)
	{
		new_history->history_data = (uint64_t*)kmalloc((1+max_nodes)*sizeof(uint64_t), GFP_ATOMIC); /*number to save +1 for current */
		if(new_history->history_data == NULL) /* deal with malloc failure */
		{
			kfree(new_history);
			new_history = NULL;
		}
		else
		{
			new_history->first_start = 0;
			new_history->first_end = 0;
			new_history->last_end = 0;
			new_history->max_nodes = max_nodes+1; /*number to save +1 for current */
			new_history->num_nodes = 1;
			new_history->non_zero_nodes = 0; /* counts non_zero nodes other than current, so initialize to 0 */
			new_history->current_index = 0;
			memset(new_history->history_data, 0, max_nodes*sizeof(uint64_t));
		}
	}
	return new_history; /* in case of malloc failure new_history will be NULL, this should be safe */
}

/* returns 1 if there are non-zero nodes in history, 0 if history is empty (all zero) */
static unsigned char update_history(bw_history* history, time_t interval_start, time_t interval_end, struct ipt_bandwidth_info* info)
{
	unsigned char history_is_nonzero = 0;
	if(history != NULL) /* should never be null, but let's be sure */
	{

		/* adjust number of non-zero nodes */
		if(history->num_nodes == history->max_nodes)
		{
			uint32_t first_index =  (history->current_index+1) % history->max_nodes; 
			if( (history->history_data)[first_index] > 0)
			{
				history->non_zero_nodes = history->non_zero_nodes -1;
			}
		}
		if( (history->history_data)[history->current_index] > 0 ) 
		{
			history->non_zero_nodes = history->non_zero_nodes + 1;
		}
		history_is_nonzero = history->non_zero_nodes > 0 ? 1 : 0;


		/* update interval start/end */
		if(history->first_start == 0)
		{
			history->first_start = interval_start;
			history->first_end = interval_end;
		}
		if(history->num_nodes >= history->max_nodes)
		{
			history->first_start = history->first_end;
			history->first_end = get_next_reset_time(info, history->first_start, history->first_start);
		}
		history->last_end = interval_end;


		history->num_nodes = history->num_nodes < history->max_nodes ? history->num_nodes+1 : history->max_nodes;
		history->current_index = (history->current_index+1) % history->max_nodes;
		(history->history_data)[history->current_index] = 0;
		
		#ifdef BANDWIDTH_DEBUG
			printk("after update history->num_nodes = %d\n", history->num_nodes);
			printk("after update history->current_index = %d\n", history->current_index);
		#endif	
	}
	return history_is_nonzero;
}


static struct ipt_bandwidth_info* do_reset_info = NULL;
static long_map* do_reset_ip_map = NULL;
static long_map* do_reset_delete_ips = NULL;
static time_t do_reset_interval_start = 0;
static time_t do_reset_interval_end = 0;
static void do_reset(unsigned long key, void* value)
{
	bw_history* history = (bw_history*)value;
	if(history != NULL && do_reset_info != NULL) /* should never be null.. but let's be sure */
	{
		unsigned char history_contains_data = update_history(history, do_reset_interval_start, do_reset_interval_end, do_reset_info);
		if(history_contains_data == 0 || do_reset_ip_map == NULL)
		{
			//schedule data for ip to be deleted (can't delete history while we're traversing history tree data structure!)
			if(do_reset_delete_ips != NULL) /* should never be null.. but let's be sure */
			{
				set_long_map_element(do_reset_delete_ips, key, (void*)(history->history_data + history->current_index));
			}
		}
		else
		{
			set_long_map_element(do_reset_ip_map, key, (void*)(history->history_data + history->current_index) );
		}
	}
}

long_map* clear_ip_map = NULL;
long_map* clear_ip_history_map = NULL;
static void clear_ips(unsigned long key, void* value)
{
	if(clear_ip_history_map != NULL && clear_ip_map != NULL)
	{
		#ifdef BANDWIDTH_DEBUG
			unsigned long* ip = &key;
			printk("clearing ip = %d.%d.%d.%d\n", *((char*)ip), *(((char*)ip)+1), *(((char*)ip)+2), *(((char*)ip)+3) );
		#endif

		remove_long_map_element(clear_ip_map, key);
		bw_history* history = (bw_history*)remove_long_map_element(clear_ip_history_map, key);
		if(history != NULL)
		{
			kfree(history->history_data);
			kfree(history);
		}
	}
}

static void set_bandwidth_to_zero(unsigned long key, void* value)
{
	*((uint64_t*)value) = 0;
}


long_map* reset_histories_ip_map = NULL;
static void reset_histories(unsigned long key, void* value)
{
	bw_history* bh = (bw_history*)value;
	bh->first_start = 0;
	bh->first_end = 0;
	bh->last_end = 0; 
	bh->num_nodes = 1;
	bh->non_zero_nodes = 1;
	bh->current_index = 0;
	(bh->history_data)[0] = 0;
	if(reset_histories_ip_map != NULL)
	{
		set_long_map_element(reset_histories_ip_map, key, bh->history_data);
	}
}


static void handle_interval_reset(info_and_maps* iam, time_t now)
{
	#ifdef BANDWIDTH_DEBUG
		printk("now, handling interval reset\n");
	#endif
	if(iam == NULL)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("error: doing reset, iam is null \n");
		#endif
		return;
	}
	if(iam->ip_map == NULL)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("error: doing reset, ip_map is null\n");
		#endif
		return;
	}
	if(iam->info == NULL)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("error: doing reset, info is null\n");
		#endif

		return;
	}

	struct ipt_bandwidth_info* info = iam->info;
	if(info->num_intervals_to_save == 0)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("doing reset for case where no intervals are saved\n");
		#endif

		if(info->next_reset <= now)
		{
			info->next_reset = get_next_reset_time(info, info->previous_reset, info->previous_reset);
			if(info->next_reset <= now)
			{
				info->next_reset = get_next_reset_time(info, now, info->previous_reset);
			}
		}
		apply_to_every_long_map_value(iam->ip_map, set_bandwidth_to_zero);
	}
	else
	{
		#ifdef BANDWIDTH_DEBUG
			printk("doing reset for case where at least one interval is saved\n");
		#endif


		if(iam->ip_history_map == NULL)
		{
			#ifdef BANDWIDTH_DEBUG
				printk("error: doing reset, history_map is null when num_intervals_to_save > 0\n");
			#endif
			return;
		}
		
		do_reset_info = info;
		do_reset_ip_map = iam->ip_map;
		clear_ip_map = iam->ip_map;
		clear_ip_history_map = iam->ip_history_map;
		

		/* 
		 * at most update as many times as we have intervals to save -- prevents
		 * rediculously long loop if interval length is 2 seconds and time was 
		 * reset to 5 years in the future
		 */
		unsigned long num_updates = 0;
		while(info->next_reset <= now && num_updates < info->num_intervals_to_save)
		{
			do_reset_delete_ips = initialize_long_map();
			/* 
			 * don't check for malloc failure here -- we 
			 * include tests for whether do_reset_delete_ips 
			 * is null below (reset should still be able to procede)
			 */

			do_reset_interval_start = info->previous_reset;
			do_reset_interval_end = info->next_reset;
			
			apply_to_every_long_map_value(iam->ip_history_map, do_reset);
			

			info->previous_reset = info->next_reset;
			info->next_reset = get_next_reset_time(info, info->previous_reset, info->previous_reset);

			/* free all data for ips whose entire histories contain only zeros to conserve space */
			if(do_reset_delete_ips != NULL)
			{
				unsigned long num_destroyed;

				/* only clear ips if this is the last iteration of this update */
				if(info->next_reset >= now)
				{
					apply_to_every_long_map_value(do_reset_delete_ips, clear_ips);
				}

				/* but clear do_reset_delete_ips no matter what, values are just pointers to history data so we can ignore them */
				destroy_long_map(do_reset_delete_ips, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				do_reset_delete_ips = NULL;
			}
			num_updates++;
		}
		do_reset_info = NULL;
		do_reset_ip_map = NULL;
		clear_ip_map = NULL;
		clear_ip_history_map = NULL;

		do_reset_interval_start = 0;
		do_reset_interval_end = 0;

		/* 
		 * test if we've cycled past all existing data -- if so wipe all existing histories
		 * and set previous reset time to now, and compute next reset time from
		 * current time
		 */
		if(info->next_reset <= now)
		{
			reset_histories_ip_map = iam->ip_map;
			apply_to_every_long_map_value(iam->ip_history_map, reset_histories);
			reset_histories_ip_map = NULL;

			info->previous_reset = now;
			info->next_reset = get_next_reset_time(info, now, info->previous_reset);
		}
	}
	info->current_bandwidth = 0;
}

/* 
 * set max bandwidth to be max possible using 63 of the
 * 64 bits in our record.  In some systems uint64_t is treated
 * like signed, so to prevent errors, use only 63 bits
 */
static uint64_t pow64(uint64_t base, uint64_t pow)
{
	uint64_t val = 1;
	if(pow > 0)
	{
		val = base*pow64(base, pow-1);
	}
	return val;
}
static uint64_t get_bw_record_max(void) /* called by init to set global variable */
{
	return  (pow64(2,62)) + (pow64(2,62)-1);
}
static uint64_t bandwidth_record_max;

static uint64_t add_up_to_max(uint64_t original, uint64_t add, unsigned char is_check)
{
	if(is_check)
	{
		return original;
	}
	else
	{
		return bandwidth_record_max-original < add ? bandwidth_record_max : original+add ;
	}
}

/*
 * Shamelessly yoinked from xt_time.c
 * "That is so amazingly amazing, I think I'd like to steal it." 
 *      -- Zaphod Beeblebrox
 */

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



static time_t get_next_reset_time(struct ipt_bandwidth_info *info, time_t now, time_t previous_reset)
{
	//first calculate when next reset would be if reset_time is 0 (which it may be)
	time_t next_reset = 0;
	if(info->reset_is_constant_interval == 0)
	{
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
	}
	else
	{
		if(info->reset_time > 0 && previous_reset > 0 && previous_reset <= now)
		{
			if(info->reset_time > now)
			{
				unsigned long whole_intervals = ((info->reset_time - now)/info->reset_interval) + 1; /* add one to make sure integer gets rounded UP (since we're subtracting) */
				next_reset = info->reset_time - (whole_intervals*info->reset_interval);
				while(next_reset <= now)
				{
					next_reset = next_reset + info->reset_interval;
				}
				
			}
			else /* info->reset_time <= now */
			{
				unsigned long whole_intervals = (now-info->reset_time)/info->reset_interval; /* integer gets rounded down */
				next_reset = info->reset_time + (whole_intervals*info->reset_interval);
				while(next_reset <= now)
				{
					next_reset = next_reset + info->reset_interval;
				}
			}
		}
		else if(previous_reset > 0)
		{
			next_reset = previous_reset;
			if(next_reset <= now) /* check just to be sure, if this is not true VERY BAD THINGS will happen */
			{
				unsigned long  whole_intervals = (now-next_reset)/info->reset_interval; /* integer gets rounded down */
				next_reset = next_reset + (whole_intervals*info->reset_interval);
				while(next_reset <= now)
				{
					next_reset = next_reset + info->reset_interval;
				}
			}
		}
		else
		{
			next_reset = now + info->reset_interval;
		}
	}
	
	return next_reset;
}



static uint64_t* initialize_map_entries_for_ip(info_and_maps* iam, unsigned long ip, uint64_t initial_bandwidth)
{
	#ifdef BANDWIDTH_DEBUG
		printk("initializing entry for ip, bw=%lld\n", initial_bandwidth);
	#endif
	
	#ifdef BANDWIDTH_DEBUG
		if(iam == NULL){ printk("error in initialization: iam is null!\n"); }
	#endif


	uint64_t* new_bw = NULL;
	if(iam != NULL) /* should never happen, but let's be certain */
	{
		struct ipt_bandwidth_info *info = iam->info;
		long_map* ip_map = iam->ip_map;
		long_map* ip_history_map = iam->ip_history_map;

		#ifdef BANDWIDTH_DEBUG
			if(info == NULL){ printk("error in initialization: info is null!\n"); }
			if(ip_map == NULL){ printk("error in initialization: ip_map is null!\n"); }
		#endif


		if(info != NULL && ip_map != NULL) /* again... should never happen but let's be sure */
		{
			if(info->num_intervals_to_save == 0 || ip_history_map == NULL)
			{
				#ifdef BANDWIDTH_DEBUG
					printk("  initializing entry for ip without history\n");
				#endif
				new_bw = (uint64_t*)kmalloc(sizeof(uint64_t), GFP_ATOMIC);
			}
			else
			{
				#ifdef BANDWIDTH_DEBUG
					printk("  initializing entry for ip with history\n");
				#endif

				bw_history *new_history = initialize_history(info->num_intervals_to_save);
				if(new_history != NULL) /* check for kmalloc failure */
				{
					#ifdef BANDWIDTH_DEBUG
						printk("  malloc succeeded, new history is non-null\n");
					#endif

					new_bw = (uint64_t*)(new_history->history_data + new_history->current_index);
					bw_history* old_history = set_long_map_element(ip_history_map, ip, (void*)new_history);
					if(old_history != NULL)
					{
						#ifdef BANDWIDTH_DEBUG
							printk("  after initialization old_history not null!  (something is FUBAR)\n");
						#endif
						kfree(old_history->history_data);
						kfree(old_history);
					}

					#ifdef BANDWIDTH_DEBUG
						
					#endif
				}
			}
			if(new_bw != NULL) /* check for kmalloc failure */
			{
				*new_bw = initial_bandwidth;
				uint64_t* old_bw = set_long_map_element(ip_map, ip, (void*)new_bw );
				if(old_bw != NULL)
				{
					free(old_bw);
				}

				#ifdef BANDWIDTH_DEBUG
					uint64_t *test = (uint64_t*)get_long_map_element(ip_map, ip);
					if(test == NULL)
					{
						printk("  after initialization bw is null!\n");
					}
					else
					{
						printk("  after initialization bw is %lld\n", *new_bw);
						printk("  after initialization test is %lld\n", *test);
					}
				#endif
			}
		}
	}

	return new_bw;
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


	unsigned char is_check = info->cmp == BANDWIDTH_CHECK ? 1 : 0;
	unsigned char do_src_dst_swap = 0;
	info_and_maps* iam = NULL;
	long_map* ip_map = NULL;

	/* if we're currently setting this id, ignore new data until set is complete */
	if(set_in_progress == 1)
	{
		if(strcmp(info->id, set_id) == 0)
		{
			return 0;
		}
	}
	
	


	

	/* 
	 * BEFORE we lock, check for timezone shift 
	 * this will almost always be be very,very quick,
	 * but in the event there IS a shift this
	 * function will lock both kernel update spinlock 
	 * and userspace i/o semaphore,  and do a lot of 
	 * number crunching so we shouldn't 
	 * already be locked.
	 */
	do_gettimeofday(&test_time);
	now = test_time.tv_sec;
	now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
	check_for_timezone_shift(now);
	check_for_backwards_time_shift(now);



	spin_lock_bh(&bandwidth_lock);
	
	if(is_check)
	{
		do_src_dst_swap = info->check_type == BANDWIDTH_CHECK_SWAP ? 1 : 0;
		info_and_maps* check_iam = (info_and_maps*)get_string_map_element(id_map, info->id);
		if(check_iam == NULL)
		{
			spin_unlock_bh(&bandwidth_lock);
			return 0;
		}
		info = check_iam->info;
	}



	if(info->reset_interval != BANDWIDTH_NEVER)
	{
		if(info->next_reset < now)
		{
			//do reset
			iam = (info_and_maps*)get_string_map_element(id_map, info->id);
			if(iam != NULL) /* should never be null, but let's be sure */
			{
				handle_interval_reset(iam, now);
				ip_map = iam->ip_map;
			}
			else
			{
				/* even in case of malloc failure or weird error we can update these params */
				info->current_bandwidth = 0;
				info->next_reset = get_next_reset_time(info, now, info->previous_reset);
			}
		}
	}

	uint64_t* bws[2] = {NULL, NULL};
	if(info->type == BANDWIDTH_COMBINED)
	{
		if(iam == NULL)
		{
			iam = (info_and_maps*)get_string_map_element(id_map, info->id);
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
			}
		}
		if(ip_map != NULL) /* if this ip_map != NULL iam can never be NULL, so we don't need to check this */
		{
			bws[0] = (uint64_t*)get_long_map_element(ip_map, 0); /* if this is null and remains so due to kmalloc failure, that's ok (won't cause crash) */
			if(bws[0] == NULL)
			{
				uint64_t* new_bw = initialize_map_entries_for_ip(iam, 0, skb->len);
				if(new_bw != NULL)
				{
					bws[0] =  new_bw;
				}
			}
			else
			{
				*(bws[0]) = add_up_to_max(*(bws[0]), (uint64_t)skb->len, is_check);
			}
		}
		else
		{
			#ifdef BANDWIDTH_DEBUG
				printk("error: ip_map is null in match!\n");
			#endif
		}
		info->current_bandwidth = add_up_to_max(info->current_bandwidth, (uint64_t)skb->len, is_check);
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
			if(do_src_dst_swap)
			{
				bw_ips[0] = iph->daddr;
			}
		}
		else if (info->type == BANDWIDTH_INDIVIDUAL_DST)
		{
			//dst ip
			bw_ips[0] = iph->daddr;
			if(do_src_dst_swap)
			{
				bw_ips[0] = iph->saddr;
			}
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
		
		if(ip_map == NULL)
		{
			iam = (info_and_maps*)get_string_map_element(id_map, info->id);
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
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
					oldval = initialize_map_entries_for_ip(iam, (unsigned long)bw_ip, (uint64_t)skb->len); /* may return NULL on malloc failure but that's ok */
				}
				else
				{
					*oldval = add_up_to_max(*oldval, (uint64_t)skb->len, is_check);
				}
				bws[bw_ip_index] = oldval; //this is fine, setting bws[bw_ip_index] to NULL on kmalloc failure won't crash anything
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










/**********************
 * Get functions
 *********************/

#define MAX_IP_STR_LENGTH 16

#define ERROR_NONE 0
#define ERROR_NO_ID 1
#define ERROR_BUFFER_TOO_SHORT 2
#define ERROR_NO_HISTORY 3
#define ERROR_UNKNOWN 4
typedef struct get_req_struct 
{
	uint32_t ip;
	uint32_t next_ip_index;
	unsigned char return_history;
	char id[BANDWIDTH_MAX_ID_LENGTH];
} get_request;

static unsigned long* output_ip_list = NULL;
static unsigned long output_ip_list_length = 0;

static char add_ip_block(	uint32_t ip, 
			unsigned char full_history_requested,
			info_and_maps* iam,
			unsigned char* output_buffer, 
			uint32_t* current_output_index, 
			uint32_t buffer_length 
			);
static void parse_get_request(unsigned char* request_buffer, get_request* parsed_request);
static int handle_get_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, int error_code, unsigned char* out_buffer, unsigned char* free_buffer );


/* 
 * returns whether we succeeded in adding ip block, 0= success, 
 * otherwise error code of problem that we found
 */
static char add_ip_block(	uint32_t ip, 
				unsigned char full_history_requested,
				info_and_maps* iam,
				unsigned char* output_buffer, 
				uint32_t* current_output_index, 
				uint32_t output_buffer_length 
				)
{
	#ifdef BANDWIDTH_DEBUG
		uint32_t *ipp = &ip;
		printk("doing output for ip = %d.%d.%d.%d\n", *((unsigned char*)ipp), *(((unsigned char*)ipp)+1), *(((unsigned char*)ipp)+2), *(((unsigned char*)ipp)+3) );
	#endif

	if(full_history_requested)
	{
		bw_history* history = NULL;
		if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map != NULL)
		{
			history = (bw_history*)get_long_map_element(iam->ip_history_map, ip);
		}
		if(history == NULL)
		{
			#ifdef BANDWIDTH_DEBUG
				printk("  no history map for ip, dumping latest value in history format\n" );
			#endif


			uint32_t block_length = (2*4) + (3*8);
			if(*current_output_index + block_length > output_buffer_length)
			{
				return ERROR_BUFFER_TOO_SHORT;
			}
			*( (uint32_t*)(output_buffer + *current_output_index) ) = ip;
			*current_output_index = *current_output_index + 4;
	
			*( (uint32_t*)(output_buffer + *current_output_index) ) = 1;
			*current_output_index = *current_output_index + 4;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * sys_tz.tz_minuteswest);
			*current_output_index = *current_output_index + 8;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * sys_tz.tz_minuteswest);
			*current_output_index = *current_output_index + 8;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * sys_tz.tz_minuteswest);
			*current_output_index = *current_output_index + 8;

			uint64_t *bw = (uint64_t*)get_long_map_element(iam->ip_map, ip);
			if(bw == NULL)
			{
				*( (uint64_t*)(output_buffer + *current_output_index) ) = 0;
			}
			else
			{
				*( (uint64_t*)(output_buffer + *current_output_index) ) = *bw;
			}
			*current_output_index = *current_output_index + 8;

		}
		else
		{
			uint32_t block_length = (2*4) + (3*8) + (8*history->num_nodes);
			if(*current_output_index + block_length > output_buffer_length)
			{
				return ERROR_BUFFER_TOO_SHORT;
			}
		
			*( (uint32_t*)(output_buffer + *current_output_index) ) = ip;
			*current_output_index = *current_output_index + 4;
	
			*( (uint32_t*)(output_buffer + *current_output_index) )= history->num_nodes;
			*current_output_index = *current_output_index + 4;

			
			
			/* need to return times in regular UTC not the UTC - minutes west, which is useful for processing */
			uint64_t last_reset = (uint64_t)iam->info->previous_reset + (60 * sys_tz.tz_minuteswest);
			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->first_start > 0 ? (uint64_t)history->first_start + (60 * sys_tz.tz_minuteswest) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping first start = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->first_end > 0 ?   (uint64_t)history->first_end + (60 * sys_tz.tz_minuteswest) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping first end   = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->last_end > 0 ?    (uint64_t)history->last_end + (60 * sys_tz.tz_minuteswest) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping last end    = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			uint32_t node_num = 0;
			uint32_t next_index = history->num_nodes == history->max_nodes ? history->current_index+1 : 0;
			next_index = next_index >= history->max_nodes ? 0 : next_index;
			for(node_num=0; node_num < history->num_nodes; node_num++)
			{
				*( (uint64_t*)(output_buffer + *current_output_index) ) = (history->history_data)[ next_index ];
				*current_output_index = *current_output_index + 8;
				next_index = (next_index + 1) % history->max_nodes;
			}
		}
	}
	else
	{
		if(*current_output_index + 8 > output_buffer_length)
		{
			return ERROR_BUFFER_TOO_SHORT;
		}

		*( (uint32_t*)(output_buffer + *current_output_index) ) = ip;
		*current_output_index = *current_output_index + 4;


		uint64_t *bw = (uint64_t*)get_long_map_element(iam->ip_map, ip);
		if(bw == NULL)
		{
			*( (uint64_t*)(output_buffer + *current_output_index) ) = 0;
		}
		else
		{
			*( (uint64_t*)(output_buffer + *current_output_index) ) = *bw;
		}
		*current_output_index = *current_output_index + 8; 
	}
	return ERROR_NONE;
}



/* 
 * convenience method for cleaning crap up after failed malloc or other 
 * error that we can't recover  from in get function
 */
static int handle_get_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, int error_code, unsigned char* out_buffer, unsigned char* free_buffer )
{
	copy_to_user(out_buffer, &error_code, 1);
	if( free_buffer != NULL ) { kfree(free_buffer); }
	if(unlock_bandwidth_spin) { spin_unlock_bh(&bandwidth_lock); }
	if(unlock_user_sem) { up(&userspace_lock); }
	return ret_value;
}

/* 
 * request structure: 
 * bytes 1:4 is ip (uint32_t)
 * bytes 4:8 is the next ip index (uint32_t)
 * byte  9   is whether to return full history or just current usage (unsigned char)
 * bytes 10:10+MAX_ID_LENGTH are the id (a string)
 */
static void parse_get_request(unsigned char* request_buffer, get_request* parsed_request)
{
	uint32_t* ip = (uint32_t*)(request_buffer+0);
	uint32_t* next_ip_index = (uint32_t*)(request_buffer+4);
	unsigned char* return_history = (unsigned char*)(request_buffer+8);

	

	parsed_request->ip = *ip;
	parsed_request->next_ip_index = *next_ip_index;
	parsed_request->return_history = *return_history;
	memcpy(parsed_request->id, request_buffer+9, BANDWIDTH_MAX_ID_LENGTH);
	(parsed_request->id)[BANDWIDTH_MAX_ID_LENGTH-1] = '\0'; /* make sure id is null terminated no matter what */
	
	#ifdef BANDWIDTH_DEBUG
		printk("ip = %d.%d.%d.%d\n", *((char*)ip), *(((char*)ip)+1), *(((char*)ip)+2), *(((char*)ip)+3) );
		printk("next ip index = %d\n", *next_ip_index);
		printk("return_history = %d\n", *return_history);
	#endif
}


static int ipt_bandwidth_get_ctl(struct sock *sk, int cmd, void *user, int *len)
{
	/* check for timezone shift & adjust if necessary */
	time_t now;
	struct timeval test_time;
	do_gettimeofday(&test_time);
	now = test_time.tv_sec;
	now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
	check_for_timezone_shift(now);
	check_for_backwards_time_shift(now);
	

	down(&userspace_lock);
	
	
	/* first check that query buffer is big enough to hold the info needed to parse the query */
	if(*len < BANDWIDTH_MAX_ID_LENGTH + 9)
	{

		return handle_get_failure(0, 1, 0, ERROR_BUFFER_TOO_SHORT, user, NULL);
	}
	
	

	/* copy the query from userspace to kernel space & parse */
	char* buffer = kmalloc(*len, GFP_ATOMIC);
	get_request query;
	if(buffer == NULL) /* check for malloc failure */
	{
		return handle_get_failure(0, 1, 0, ERROR_UNKNOWN, user, NULL);
	}
	copy_from_user(buffer, user, *len);
	parse_get_request(buffer, &query);
	


	
	
	
	/* 
	 * retrieve data for this id and verify all variables are properly defined, just to be sure
	 * this is a kernel module -- it pays to be paranoid! 
	 */
	spin_lock_bh(&bandwidth_lock);
	
	info_and_maps* iam = (info_and_maps*)get_string_map_element(id_map, query.id);
	
	if(iam == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	if(iam->info == NULL || iam->ip_map == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	
	/* allocate ip list if this is first query */
	if(query.next_ip_index == 0 && query.ip == 0)
	{
		if(output_ip_list != NULL)
		{
			kfree(output_ip_list);
		}
		if(iam->info->type == BANDWIDTH_COMBINED)
		{
			output_ip_list_length = 1;
			output_ip_list = (unsigned long*)kmalloc(sizeof(unsigned long), GFP_ATOMIC);
			if(output_ip_list != NULL) { *output_ip_list = 0; }
		}
		else
		{
			output_ip_list = get_sorted_long_map_keys(iam->ip_map, &output_ip_list_length);
		}
		
		if(output_ip_list == NULL)
		{
			return handle_get_failure(0, 1, 1, ERROR_UNKNOWN, user, buffer);
		}
	}

	/* if this is not first query do a sanity check -- make sure it's within bounds of allocated ip list */
	if(query.next_ip_index > 0 && (output_ip_list == NULL || query.next_ip_index > output_ip_list_length))
	{
		return handle_get_failure(0, 1, 1, ERROR_UNKNOWN, user, buffer);
	}




	/*
	// values only reset when a packet hits a rule, so 
	// reset may have expired without data being reset.
	// So, test if we need to reset values to zero 
	*/
	if(iam->info->reset_interval != BANDWIDTH_NEVER)
	{
		if(iam->info->next_reset < now)
		{
			//do reset
			handle_interval_reset(iam, now);
		}
	}



	/* compute response & store it in buffer
	 *
	 * format of response:
	 * byte 1 : error code (0 for ok)
	 * bytes 2-5 : total_num_ips found in query (further gets may be necessary to retrieve them)
	 * bytes 6-9 : start_index, index (in a list of total_num_ips) of first ip in response
	 * bytes 10-13 : num_ips_in_response, number of ips in this response
	 * bytes 14-21 : reset_interval (helps deal with DST shifts in userspace)
	 * bytes 22-29 : reset_time (helps deal with DST shifts in userspace)
	 * byte  30    : reset_is_constant_interval (helps deal with DST shifts in userspace)
	 * remaining bytes contain blocks of ip data
	 * format is dependent on whether history was queried
	 * 
	 * if history was NOT queried we have
	 * bytes 1-4 : ip
	 * bytes 5-12 : bandwidth
	 *
	 * if history WAS queried we have
	 *   (note we are using 64 bit integers for time here
	 *   even though time_t is 32 bits on most 32 bit systems
	 *   just to be on the safe side)
	 * bytes 1-4 : ip
	 * bytes 4-8 : history_length number of history values (including current)
	 * bytes 9-16 : first start
	 * bytes 17-24 : first end
	 * bytes 25-32 : recent end 
	 * 33 onward : list of 64 bit integers of length history_length
	 *
	 */
	unsigned char* error = buffer;
	uint32_t* total_ips = (uint32_t*)(buffer+1);
	uint32_t* start_index = (uint32_t*)(buffer+5);
	uint32_t* num_ips_in_response = (uint32_t*)(buffer+9);
	uint64_t* reset_interval = (uint64_t*)(buffer+13);
	uint64_t* reset_time = (uint64_t*)(buffer+21);
	unsigned char* reset_is_constant_interval = (char*)(buffer+29);

	*reset_interval = (uint64_t)iam->info->reset_interval;
	*reset_time = (uint64_t)iam->info->reset_time;
	*reset_is_constant_interval = iam->info->reset_is_constant_interval;

	uint32_t  current_output_index = 30;
	if(query.ip != 0)
	{
		*error = add_ip_block(	query.ip, 
					query.return_history,
					iam,
					buffer, 
					&current_output_index, 
					*len 
					);

		*total_ips = *error == 0;
		*start_index = 0;
		*num_ips_in_response = *error == 0 ? 1 : 0;
	}
	else
	{
		uint32_t next_index = query.next_ip_index;
		*error = ERROR_NONE;
		*total_ips = output_ip_list_length;
		*start_index = next_index;
		*num_ips_in_response = 0;
		while(*error == ERROR_NONE && next_index < output_ip_list_length)
		{
			uint32_t next_ip = output_ip_list[next_index];
			*error = add_ip_block(	next_ip, 
						query.return_history,
						iam,
						buffer, 
						&current_output_index, 
						*len
						);
			if(*error == ERROR_NONE)
			{
				*num_ips_in_response = *num_ips_in_response + 1;
				next_index++;
			}
		}
		if(*error == ERROR_BUFFER_TOO_SHORT && *num_ips_in_response > 0)
		{
			*error = ERROR_NONE;
		}
		if(next_index == output_ip_list_length)
		{
			kfree(output_ip_list);
			output_ip_list = NULL;
			output_ip_list_length = 0;
		}
	}

	spin_unlock_bh(&bandwidth_lock);
	
	copy_to_user(user, buffer, *len);
	kfree(buffer);



	up(&userspace_lock);


	return 0;
}





/********************
 * Set functions
 ********************/

typedef struct set_header_struct
{
	uint32_t total_ips;
	uint32_t next_ip_index;
	uint32_t num_ips_in_buffer;
	unsigned char history_included;
	unsigned char zero_unset_ips;
	time_t last_backup;
	char id[BANDWIDTH_MAX_ID_LENGTH];
} set_header;

static int handle_set_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, unsigned char* free_buffer );
static void parse_set_header(unsigned char* input_buffer, set_header* header);
static void set_single_ip_data(unsigned char history_included, info_and_maps* iam, unsigned char* buffer, uint32_t* buffer_index, time_t now);

static int handle_set_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, unsigned char* free_buffer )
{
	if( free_buffer != NULL ) { kfree(free_buffer); }
	set_in_progress = 0;
	if(unlock_bandwidth_spin) { spin_unlock_bh(&bandwidth_lock); }
	if(unlock_user_sem) { up(&userspace_lock); }
	return ret_value;
}

static void parse_set_header(unsigned char* input_buffer, set_header* header)
{
	/* 
	 * set header structure:
	 * bytes 1-4   :  total_ips being set in this and subsequent requests
	 * bytes 5-8   :  next_ip_index, first ip being set in this set command
	 * bytes 9-12  :  num_ips_in_buffer, the number of ips in this set request
	 * byte 13     :  history_included (whether history data is included, or just current data)
	 * byte 14     :  zero_unset_ips, whether to zero all ips not included in this and subsequent requests
	 * bytes 15-22 :  last_backup time (64 bit)
	 * bytes 23-23+BANDWIDTH_MAX_ID_LENGTH : id
	 * bytes 23+   :  ip data
	 */

	uint32_t* total_ips = (uint32_t*)(input_buffer+0);
	uint32_t* next_ip_index = (uint32_t*)(input_buffer+4);
	uint32_t* num_ips_in_buffer = (uint32_t*)(input_buffer+8);
	unsigned char* history_included = (unsigned char*)(input_buffer+12);
	unsigned char* zero_unset_ips = (unsigned char*)(input_buffer+13);
	uint64_t* last_backup = (uint64_t*)(input_buffer+14);


	header->total_ips = *total_ips;
	header->next_ip_index = *next_ip_index;
	header->num_ips_in_buffer = *num_ips_in_buffer;
	header->history_included = *history_included;
	header->zero_unset_ips = *zero_unset_ips;
	header->last_backup = (time_t)*last_backup;
	memcpy(header->id, input_buffer+22, BANDWIDTH_MAX_ID_LENGTH);
	(header->id)[BANDWIDTH_MAX_ID_LENGTH-1] = '\0'; /* make sure id is null terminated no matter what */

	#ifdef BANDWIDTH_DEBUG
		printk("parsed set header:\n");
		printk("  total_ips         = %d\n", header->total_ips);
		printk("  next_ip_index     = %d\n", header->next_ip_index);
		printk("  num_ips_in_buffer = %d\n", header->num_ips_in_buffer);
		printk("  zero_unset_ips    = %d\n", header->zero_unset_ips);
		printk("  last_backup       = %ld\n", header->last_backup);
		printk("  id                = %s\n", header->id);
	#endif
}
static void set_single_ip_data(unsigned char history_included, info_and_maps* iam, unsigned char* buffer, uint32_t* buffer_index, time_t now)
{
	/* 
	 * note that times stored within the module are adjusted so they are equal to seconds 
	 * since unix epoch that corrosponds to the UTC wall-clock time (timezone offset 0) 
	 * that is equal to the wall-clock time in the current time-zone.  Incoming values must 
	 * be adjusted similarly
	 */
	uint32_t ip = *( (uint32_t*)(buffer + *buffer_index) );
			
	#ifdef BANDWIDTH_DEBUG
		uint32_t* ipp = &ip;
		printk("doing set for ip = %d.%d.%d.%d\n", *((unsigned char*)ipp), *(((unsigned char*)ipp)+1), *(((unsigned char*)ipp)+2), *(((unsigned char*)ipp)+3) );
		printk("ip index = %d\n", *buffer_index);
	#endif

	if(history_included)
	{
		uint32_t num_history_nodes = *( (uint32_t*)(buffer + *buffer_index+4));
		if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map != NULL)
		{
			time_t first_start = (time_t) *( (uint64_t*)(buffer + *buffer_index+8));
			/* time_t first_end   = (time_t) *( (uint64_t*)(buffer + *buffer_index+16)); //not used */
			/* time_t last_end    = (time_t) *( (uint64_t*)(buffer + *buffer_index+24)); //not used */

			#ifdef BANDWIDTH_DEBUG
				printk("setting history with first start = %ld\n", first_start);
			#endif


			*buffer_index = *buffer_index + (2*4) + (3*8);
			
			/* adjust for timezone */
			time_t next_start = first_start - (60 * sys_tz.tz_minuteswest);
			time_t next_end = get_next_reset_time(iam->info, next_start, next_start);
			uint32_t node_index=0;
			bw_history* history = NULL;
			while(next_start < now)
			{
				uint64_t next_bw = 0;
				if(node_index < num_history_nodes)
				{
					next_bw = *( (uint64_t*)(buffer + *buffer_index));
					*buffer_index = *buffer_index + 8;
				}
				if(node_index == 0 || history == NULL)
				{
					initialize_map_entries_for_ip(iam, ip, next_bw);
					history = get_long_map_element(iam->ip_history_map, (unsigned long)ip);
				}
				else if(next_end < now) /* if this is most recent node, don't do update since last node is current bandwidth */ 
				{
					int is_nonzero = update_history(history, next_start, next_end, iam->info);
					(history->history_data)[ history->current_index ] = next_bw;
					if(is_nonzero)
					{
						next_start = next_end;
						next_end = get_next_reset_time(iam->info, next_start, next_start);
					}
					else
					{
						/* do history reset */
						history->first_start = 0;
						history->first_end = 0;
						history->last_end = 0; 
						history->num_nodes = 1;
						history->non_zero_nodes = 1;
						history->current_index = 0;
						(history->history_data)[0] = 0;
						
						next_start = now;
						next_end = get_next_reset_time(iam->info, now, next_start);
					}
				}
				else /* if this is most recent node, we still need to exit loop*/
				{
					break;
				}
				node_index++;
			}
			while(node_index < num_history_nodes)
			{
				*buffer_index = *buffer_index + 8;
				node_index++;
			}
			if(history != NULL)
			{
				set_long_map_element(iam->ip_map, ip, (history->history_data + history->current_index) );
				iam->info->previous_reset = next_start;
				iam->info->next_reset = next_end;
				if(ip == 0)
				{
					iam->info->current_bandwidth = (history->history_data)[history->current_index];
				}
			}
		}
		else
		{
			*buffer_index = *buffer_index + (2*4) + (3*8) + ((num_history_nodes-1)*8);
			uint64_t bw = *( (uint64_t*)(buffer + *buffer_index));
			initialize_map_entries_for_ip(iam, ip, bw); /* automatically frees existing values if they exist */
			*buffer_index = *buffer_index + 8;
			if(ip == 0)
			{
				iam->info->current_bandwidth = bw;
			}
		}

	}
	else
	{
		uint64_t bw = *( (uint64_t*)(buffer + *buffer_index+4) );
		#ifdef BANDWIDTH_DEBUG
			printk("  setting bw to %lld\n", bw );
		#endif

		
		initialize_map_entries_for_ip(iam, ip, bw); /* automatically frees existing values if they exist */
		*buffer_index = *buffer_index + 12;

		if(ip == 0)
		{
			iam->info->current_bandwidth = bw;
		}
	}
	

}

static int ipt_bandwidth_set_ctl(struct sock *sk, int cmd, void *user, u_int32_t len)
{
	/* check for timezone shift & adjust if necessary */
	time_t now;
	struct timeval test_time;
	do_gettimeofday(&test_time);
	now = test_time.tv_sec;
	now = now -  (60 * sys_tz.tz_minuteswest);  /* Adjust for local timezone */
	check_for_timezone_shift(now);
	check_for_backwards_time_shift(now);


	/* just return right away if user buffer is too short to contain even the header */
	if(len < (3*4) + 2 + 8 + BANDWIDTH_MAX_ID_LENGTH)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("set error: buffer not large enough!\n");
		#endif
		return 0;
	}

	down(&userspace_lock);
	set_in_progress = 1;
	
	set_header header;
	char* buffer = kmalloc(len, GFP_ATOMIC);
	if(buffer == NULL) /* check for malloc failure */
	{
		return handle_set_failure(0, 1, 0, NULL);
	}
	copy_from_user(buffer, user, len);
	parse_set_header(buffer, &header);

	
	
	
	/* 
	 * retrieve data for this id and verify all variables are properly defined, just to be sure
	 * this is a kernel module -- it pays to be paranoid! 
	 */
	spin_lock_bh(&bandwidth_lock);
	

	info_and_maps* iam = (info_and_maps*)get_string_map_element(id_map, header.id);
	if(iam == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}
	if(iam->info == NULL || iam->ip_map == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}
	if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}

	//if zero_unset_ips == 1 && next_ip_index == 0
	//then clear data for all ips for this id
	if(header.zero_unset_ips && header.next_ip_index == 0)
	{
		//clear data
		if(iam->info->num_intervals_to_save > 0)
		{
			while(iam->ip_map->num_elements > 0)
			{
				unsigned long key;
				remove_smallest_long_map_element(iam->ip_map, &key);
				/* ignore return value -- it's actually malloced in history, not here */
			}
			while(iam->ip_history_map->num_elements > 0)
			{
				unsigned long key;
				bw_history* history = remove_smallest_long_map_element(iam->ip_history_map, &key);
				kfree(history->history_data);
				kfree(history);
			}
		}
		else
		{
			while(iam->ip_map->num_elements > 0)
			{
				unsigned long key;
				uint64_t *bw = remove_smallest_long_map_element(iam->ip_map, &key);
				kfree(bw);
			}
		}
	}

	/* 
	 * last_backup parameter is only relevant for case where we are not setting history
	 * and when we don't have a constant interval length or a specified reset_time (since in this case start time gets reset when rule is inserted and there is therefore no constant end)
	 * If num_intervals_to_save =0 and is_constant_interval=0, check it.  If it's nonzero (0=ignore) and invalid, return.
	 */
	if(header.last_backup > 0 && iam->info->num_intervals_to_save == 0 && (iam->info->reset_is_constant_interval == 0 || iam->info->reset_time != 0) )
	{
		time_t adjusted_last_backup_time = header.last_backup - (60 * sys_tz.tz_minuteswest); 
		time_t next_reset_of_last_backup = get_next_reset_time(iam->info, adjusted_last_backup_time, adjusted_last_backup_time);
		if(next_reset_of_last_backup != iam->info->next_reset)
		{
			return handle_set_failure(0, 1, 1, buffer);
		}
	}


	/*
	 * iterate over each ip block in buffer, 
	 * loading data into necessary kerenel-space data structures
	*/
	uint32_t buffer_index = (3*4) + 1 + 1 + 8 + BANDWIDTH_MAX_ID_LENGTH;
	uint32_t next_ip_index = header.next_ip_index;
	
	while(next_ip_index < header.num_ips_in_buffer)
	{
		set_single_ip_data(header.history_included, iam, buffer, &buffer_index, now);
		next_ip_index++;
	}

	if (next_ip_index == header.total_ips)
	{
		set_in_progress = 0;
	}

	kfree(buffer);
	spin_unlock_bh(&bandwidth_lock);
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
		if(info->ref_count == NULL) /* deal with kmalloc failure */
		{
			printk("ipt_bandwidth: kmalloc failure in checkentry!\n");
			return 0;
		}
		*(info->ref_count) = 1;
		info->non_const_self = info;
		
		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(info->ref_count) );
		#endif

		if(info->cmp != BANDWIDTH_CHECK)
		{
			down(&userspace_lock);
			spin_lock_bh(&bandwidth_lock);
		
	
			info_and_maps  *iam = (info_and_maps*)get_string_map_element(id_map, info->id);
			if(iam != NULL)
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
				info->previous_reset = now;
				if(info->next_reset == 0)
				{
					info->next_reset = get_next_reset_time(info, now, now);
					
					/* 
					 * if we specify last backup time, check that next reset is consistent, 
					 * otherwise reset current_bandwidth to 0 
					 * 
					 * only applies to combined type -- otherwise we need to handle setting bandwidth
					 * through userspace library
					 */
					if(info->last_backup_time != 0 && info->type == BANDWIDTH_COMBINED)
					{
						time_t adjusted_last_backup_time = info->last_backup_time - (60 * sys_tz.tz_minuteswest); 
						time_t next_reset_of_last_backup = get_next_reset_time(info, adjusted_last_backup_time, adjusted_last_backup_time);
						if(next_reset_of_last_backup != info->next_reset)
						{
							info->current_bandwidth = 0;
						}
						info->last_backup_time = 0;
					}
				}
			}
	
			iam = (info_and_maps*)kmalloc( sizeof(info_and_maps), GFP_ATOMIC);
			if(iam == NULL) /* handle kmalloc failure */
			{
				printk("ipt_bandwidth: kmalloc failure in checkentry!\n");
				spin_unlock_bh(&bandwidth_lock);
				up(&userspace_lock);
				return 0;
			}
			iam->ip_map = initialize_long_map();
			if(iam->ip_map == NULL) /* handle kmalloc failure */
			{
				printk("ipt_bandwidth: kmalloc failure in checkentry!\n");
				spin_unlock_bh(&bandwidth_lock);
				up(&userspace_lock);
				return 0;
			}
			iam->ip_history_map = NULL;
			if(info->num_intervals_to_save > 0)
			{
				iam->ip_history_map = initialize_long_map();
				if(iam->ip_history_map == NULL) /* handle kmalloc failure */
				{
					printk("ipt_bandwidth: kmalloc failure in checkentry!\n");
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return 0;
				}
			}


			iam->info = info;
			set_string_map_element(id_map, info->id, iam);


			spin_unlock_bh(&bandwidth_lock);
			up(&userspace_lock);
		}
	}
	else
	{
		info->non_const_self = info;
		*(info->ref_count) = *(info->ref_count) + 1;
		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(info->ref_count) );
		#endif
		
		if(info->cmp != BANDWIDTH_CHECK)
		{
			down(&userspace_lock);
			spin_lock_bh(&bandwidth_lock);
			info_and_maps* iam = (info_and_maps*)get_string_map_element(id_map, info->id);
			if(iam != NULL)
			{
				iam->info = info;
			}
			spin_unlock_bh(&bandwidth_lock);
			up(&userspace_lock);
		}
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
		
		info_and_maps* iam = (info_and_maps*)remove_string_map_element(id_map, info->id);
		if(iam != NULL)
		{
			unsigned long num_destroyed;
			if(iam->ip_map != NULL && iam->ip_history_map != NULL)
			{
				unsigned long history_index = 0;
				bw_history** histories_to_free;
				
				destroy_long_map(iam->ip_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				
				histories_to_free = (bw_history**)destroy_long_map(iam->ip_history_map, DESTROY_MODE_RETURN_VALUES, &num_destroyed);
				
				/* num_destroyed will be 0 if histories_to_free is null after malloc failure, so this is safe */
				for(history_index = 0; history_index < num_destroyed; history_index++) 
				{
					bw_history* h = histories_to_free[history_index];
					if(h != NULL)
					{
						kfree(h->history_data);
						kfree(h);
					}
				}
				
			}
			else if(iam->ip_map != NULL)
			{
				destroy_long_map(iam->ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			}
			kfree(iam);
			/* info portion of iam gets taken care of automatically */
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
	bandwidth_record_max = get_bw_record_max();
	old_minutes_west = sys_tz.tz_minuteswest;

	id_map = initialize_string_map(0);
	if(id_map == NULL) /* deal with kmalloc failure */
	{
		return -1;
	}
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
		info_and_maps **iams = (info_and_maps**)destroy_string_map(id_map, DESTROY_MODE_RETURN_VALUES, &num_returned);
		int iam_index;
		for(iam_index=0; iam_index < num_returned; iam_index++)
		{
			info_and_maps* iam = iams[iam_index];
			long_map* ip_map = iam->ip_map;
			unsigned long num_destroyed;
			destroy_long_map(ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			kfree(iam);
			/* info portion of iam gets taken care of automatically */
		}
	}
	ipt_unregister_match(&bandwidth_match);
	spin_unlock_bh(&bandwidth_lock);
	up(&userspace_lock);

}

module_init(init);
module_exit(fini);

