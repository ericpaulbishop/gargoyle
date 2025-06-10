/*  bandwidth --	An netfilter module for bandwidth monitoring/control
 *  			Can be used to efficiently monitor bandwidth and/or implement bandwidth quotas
 *  			Can be queried using the nftbwctl userspace library
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009-2024 by Eric Bishop <eric@gargoyle-router.com>
 *  Rewritten for nftables by Michael Gray <support@lantisproject.com>
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
#include <net/ip.h>
#include <linux/inet.h>
#include <linux/math64.h>
#include <linux/time.h>

#include <linux/semaphore.h> 

#include "bandwidth_deps/tree_map.h"
#include <linux/netfilter/nft_bandwidth.h>

#include <linux/ip.h>
#include <linux/netfilter/nf_tables.h>
#include <net/netfilter/nf_tables.h>

/* #define BANDWIDTH_DEBUG 1 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Gray");
MODULE_DESCRIPTION("Match bandwidth used, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS_NFT_EXPR("bandwidth");

/* 
 * WARNING: accessing the sys_tz variable takes FOREVER, and kills performance 
 * keep a local variable that gets updated from the extern variable 
 */
extern struct timezone sys_tz; 
static int local_minutes_west;
static int local_seconds_west;
static ktime_t last_local_mw_update;


static spinlock_t bandwidth_lock = __SPIN_LOCK_UNLOCKED(bandwidth_lock);
DEFINE_SEMAPHORE(userspace_lock);

static string_map* id_map = NULL;

typedef struct info_and_maps_struct
{
	struct nft_bandwidth_info* info;
	string_map* ip_map;
	string_map* ip_history_map;
	string_map* ip_family_map;
	uint8_t info_family;
	uint8_t other_info_family;
	struct nft_bandwidth_info* other_info;

	unsigned long ref_count;
}info_and_maps;

typedef struct history_struct
{
	ktime_t first_start;
	ktime_t first_end;
	ktime_t last_end; /* also beginning of current time frame */
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


static void adjust_ip_for_backwards_time_shift(char* key, void* value);
static void adjust_id_for_backwards_time_shift(char* key, void* value);
static void check_for_backwards_time_shift(ktime_t now);


static void shift_timezone_of_ip(char* key, void* value);
static void shift_timezone_of_id(char* key, void* value);
static void check_for_timezone_shift(ktime_t now, int already_locked);



static bw_history* initialize_history(uint32_t max_nodes);
static unsigned char update_history(bw_history* history, ktime_t interval_start, ktime_t interval_end, struct nft_bandwidth_info* info);



static void do_reset(char* key, void* value);
static void set_bandwidth_to_zero(char* key, void* value);
static void handle_interval_reset(info_and_maps* iam, ktime_t now);

static uint64_t pow64(uint64_t base, uint64_t pow);
static uint64_t get_bw_record_max(void); /* called by init to set global variable */

static inline int is_leap(unsigned int y);
static ktime_t get_next_reset_time(struct nft_bandwidth_info *info, ktime_t now, ktime_t previous_reset);
static ktime_t get_nominal_previous_reset_time(struct nft_bandwidth_info *info, ktime_t current_next_reset);

static uint64_t* initialize_map_entries_for_ip(info_and_maps* iam, char* ip, uint64_t initial_bandwidth, uint32_t family);

int free_null_terminated_string_array(char** strs);

int free_null_terminated_string_array(char** strs)
{
	unsigned long str_index = 0;
	if(strs != NULL)
	{
		for(str_index=0; strs[str_index] != NULL; str_index++)
		{
			free(strs[str_index]);
		}
		free(strs);
	}
	return str_index;
}


static ktime_t backwards_check = 0;
static ktime_t backwards_adjust_current_time = 0;
static ktime_t backwards_adjust_info_previous_reset = 0;
static ktime_t backwards_adjust_ips_zeroed = 0;
static info_and_maps* backwards_adjust_iam = NULL;

/*
static char print_out_buf[25000];
static void print_to_buf(char* outdat);
static void reset_buf(void);
static void do_print_buf(void);

static void print_to_buf(char* outdat)
{
	int buf_len = strlen(print_out_buf);
	sprintf(print_out_buf+buf_len, "\t%s\n", outdat);
}
static void reset_buf(void)
{
	print_out_buf[0] = '\n';
	print_out_buf[1] = '\0';
}
static void do_print_buf(void)
{
	char* start = print_out_buf;
	char* next = strchr(start, '\n');
	while(next != NULL)
	{
		*next = '\0';
		printk("%s\n", start);
		start = next+1;
		next = strchr(start, '\n');
	}
	printk("%s\n", start);
	
	reset_buf();
}
*/

#define BANDWIDTH_SUBNET_STR_SIZE 128
static const struct nla_policy nft_bandwidth_policy[NFTA_BANDWIDTH_MAX + 1] = {
	[NFTA_BANDWIDTH_ID]			        = { .type = NLA_STRING, .len = BANDWIDTH_MAX_ID_LENGTH },
	[NFTA_BANDWIDTH_CMP]		        = { .type = NLA_U8 },
	[NFTA_BANDWIDTH_TYPE]		        = { .type = NLA_U8 },
	[NFTA_BANDWIDTH_CHECKTYPE]		    = { .type = NLA_U8 },
	[NFTA_BANDWIDTH_BWCUTOFF]           = { .type = NLA_U64 },
	[NFTA_BANDWIDTH_CURRENTBW]	        = { .type = NLA_U64 },
	[NFTA_BANDWIDTH_SUBNET]             = { .type = NLA_STRING, .len = BANDWIDTH_SUBNET_STR_SIZE },
	[NFTA_BANDWIDTH_SUBNET6]	        = { .type = NLA_STRING, .len = BANDWIDTH_SUBNET_STR_SIZE },
	[NFTA_BANDWIDTH_RSTINTVL]	        = { .type = NLA_U64 },
	[NFTA_BANDWIDTH_RSTINTVLCONST]	    = { .type = NLA_U8 },
	[NFTA_BANDWIDTH_RSTTIME]	        = { .type = NLA_U64 },
	[NFTA_BANDWIDTH_NUMINTVLSTOSAVE]    = { .type = NLA_U32 },
	[NFTA_BANDWIDTH_LASTBACKUPTIME]	    = { .type = NLA_U64 },
	[NFTA_BANDWIDTH_MINUTESWEST]	    = { .type = NLA_U32 },
};

static void adjust_ip_for_backwards_time_shift(char* key, void* value)
{
	bw_history* old_history = (bw_history*)value;
	
	if(old_history->num_nodes == 1)
	{
		if(backwards_adjust_info_previous_reset > backwards_adjust_current_time)
		{
			if(backwards_adjust_ips_zeroed == 0)
			{
				apply_to_every_string_map_value(backwards_adjust_iam->ip_map, set_bandwidth_to_zero);
				backwards_adjust_iam->info->next_reset = get_next_reset_time(backwards_adjust_iam->info, backwards_adjust_current_time, backwards_adjust_current_time);
				backwards_adjust_iam->info->previous_reset = backwards_adjust_current_time;
				backwards_adjust_iam->info->current_bandwidth = 0;
				backwards_adjust_ips_zeroed = 1;
			}
		}
		return;
	}
	else if(old_history->last_end < backwards_adjust_current_time)
	{
		return;
	}
	else
	{
		
		/* 
		 * reconstruct new history without newest nodes, to represent data as it was 
		 * last time the current time was set to the interval to which we just jumped back
		 */
		uint32_t next_old_index;
		ktime_t old_next_start =  old_history->first_start == 0 ? backwards_adjust_info_previous_reset : old_history->first_start; /* first time point in old history */
		bw_history* new_history = initialize_history(old_history->max_nodes);
		if(new_history == NULL)
		{
			printk("nft_bandwidth: warning, kmalloc failure!\n");
			return;
		}

		

		/* oldest index in old history -- we iterate forward through old history using this index */
		next_old_index = old_history->num_nodes == old_history->max_nodes ? (old_history->current_index+1) % old_history->max_nodes : 0;


		/* if first time point is after current time, just completely re-initialize history, otherwise set first time point to old first time point */
		(new_history->history_data)[ new_history->current_index ] = old_next_start < backwards_adjust_current_time ? (old_history->history_data)[next_old_index] : 0;
		backwards_adjust_iam->info->previous_reset                = old_next_start < backwards_adjust_current_time ? old_next_start : backwards_adjust_current_time;


		/* iterate through old history, rebuilding in new history*/
		while( old_next_start < backwards_adjust_current_time )
		{
			ktime_t old_next_end = get_next_reset_time(backwards_adjust_iam->info, old_next_start, old_next_start); /* 2nd param = last reset, 3rd param = current time */
			if(  old_next_end < backwards_adjust_current_time)
			{
				update_history(new_history, old_next_start, old_next_end, backwards_adjust_iam->info);
				next_old_index++;
				(new_history->history_data)[ new_history->current_index ] =  (old_history->history_data)[next_old_index];
			}
			backwards_adjust_iam->info->previous_reset = old_next_start; /*update previous_reset variable in bw_info as we iterate */
			old_next_start = old_next_end;
		}

		/* update next_reset variable from previous_reset variable which we've already set */
		backwards_adjust_iam->info->next_reset = get_next_reset_time(backwards_adjust_iam->info, backwards_adjust_iam->info->previous_reset, backwards_adjust_iam->info->previous_reset); 
		


		/* set old_history to be new_history */	
		kfree(old_history->history_data);
		old_history->history_data   = new_history->history_data;
		old_history->first_start    = new_history->first_start;
		old_history->first_end      = new_history->first_end;
		old_history->last_end       = new_history->last_end;
		old_history->num_nodes      = new_history->num_nodes;
		old_history->non_zero_nodes = new_history->non_zero_nodes;
		old_history->current_index  = new_history->current_index;
		set_string_map_element(backwards_adjust_iam->ip_map, key, (void*)(old_history->history_data + old_history->current_index) );
		if(strcmp(key,"0.0.0.0") == 0)
		{
			backwards_adjust_iam->info->combined_bw = (uint64_t*)(old_history->history_data + old_history->current_index);
			if(backwards_adjust_iam->other_info != NULL)
			{
				backwards_adjust_iam->other_info->combined_bw = backwards_adjust_iam->info->combined_bw;
			}
		}
		
		/* 
		 * free new history  (which was just temporary) 
		 * note that we don't need to free history_data from new_history
		 * we freed the history_data from old history, and set that to the history_data from new_history
		 * so, this cleanup has already been handled
		 */
		kfree(new_history);
		
	}
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
		backwards_adjust_info_previous_reset = iam->info->previous_reset;
		backwards_adjust_ips_zeroed = 0;
		apply_to_every_string_map_value(iam->ip_history_map, adjust_ip_for_backwards_time_shift);
	}
	else
	{
		ktime_t next_reset_after_adjustment = get_next_reset_time(iam->info, backwards_adjust_current_time, backwards_adjust_current_time);
		if(next_reset_after_adjustment < iam->info->next_reset)
		{
			iam->info->previous_reset = backwards_adjust_current_time;
			iam->info->next_reset = next_reset_after_adjustment;
		}
	}
	backwards_adjust_iam = NULL;
}
static void check_for_backwards_time_shift(ktime_t now)
{
	spin_lock_bh(&bandwidth_lock);
	if(now < backwards_check && backwards_check != 0)
	{
		printk("nft_bandwidth: backwards time shift detected, adjusting\n");

		/* adjust */
		down(&userspace_lock);

		/* This function is always called with absolute time, not time adjusted for timezone. Correct that before adjusting. */
		backwards_adjust_current_time = now - local_seconds_west;
		apply_to_every_string_map_value(id_map, adjust_id_for_backwards_time_shift);
		up(&userspace_lock);
	}
	backwards_check = now;
	spin_unlock_bh(&bandwidth_lock);
}



static int old_minutes_west;
static ktime_t shift_timezone_current_time;
static ktime_t shift_timezone_info_previous_reset;
static info_and_maps* shift_timezone_iam = NULL;
static void shift_timezone_of_ip(char* key, void* value)
{
	bw_history* history = (bw_history*)value;
	int32_t timezone_adj;
	ktime_t next_reset;
	ktime_t previous_reset;

	#ifdef BANDWIDTH_DEBUG
		printk("shifting ip = %s\n", key);
	#endif

	timezone_adj = (old_minutes_west-local_minutes_west)*60;
	#ifdef BANDWIDTH_DEBUG
		printk("  before jump:\n");
		printk("    current time = %lld\n",  ktime_to_ns(shift_timezone_current_time));
		printk("    first_start  = %lld\n", ktime_to_ns(history->first_start));
		printk("    first_end    = %lld\n", ktime_to_ns(history->first_end));
		printk("    last_end     = %lld\n", ktime_to_ns(history->last_end));
		printk("\n");
	#endif
	
	/* given time after shift, calculate next and previous reset times */
	next_reset = get_next_reset_time(shift_timezone_iam->info, shift_timezone_current_time, 0);
	previous_reset = get_nominal_previous_reset_time(shift_timezone_iam->info, next_reset);
	shift_timezone_iam->info->next_reset = next_reset;

	/*if we're resetting on a constant interval, we can just adjust -- no need to worry about relationship to constant boundaries, e.g. end of day */
	if(shift_timezone_iam->info->reset_is_constant_interval)
	{
		shift_timezone_iam->info->previous_reset = previous_reset;
		if(history->num_nodes > 1)
		{
			history->first_start = history->first_start + timezone_adj;
			history->first_end = history->first_end + timezone_adj;
			history->last_end = history->last_end + timezone_adj;
		}
	}
	else
	{
		/* next reset will be the newly computed next_reset. */
		int node_index=history->num_nodes - 1;
		if(node_index > 0)
		{
			/* based on new, shifted time, iterate back over all nodes in history */
			shift_timezone_iam->info->previous_reset = previous_reset ;
			history->last_end = previous_reset;

			while(node_index > 1)
			{
				previous_reset = get_nominal_previous_reset_time(shift_timezone_iam->info, previous_reset);
				node_index--;
			}
			history->first_end = previous_reset;
			
			previous_reset = get_nominal_previous_reset_time(shift_timezone_iam->info, previous_reset);
			history->first_start = previous_reset > history->first_start + timezone_adj ? previous_reset : history->first_start + timezone_adj;
		}
		else
		{
			/*
			 * history hasn't really been initialized -- there's only one, current time point.
			 * we only know what's in the current accumulator in info. Just adjust previous reset time and make sure it's valid 
			 */
			shift_timezone_iam->info->previous_reset = previous_reset > shift_timezone_info_previous_reset + timezone_adj ? previous_reset : shift_timezone_info_previous_reset + timezone_adj;
		}
	}

	#ifdef BANDWIDTH_DEBUG
		printk("\n");
		printk("  after jump:\n");
		printk("    first_start = %lld\n", ktime_to_ns(history->first_start));
		printk("    first_end   = %lld\n", ktime_to_ns(history->first_end));
		printk("    last_end    = %lld\n", ktime_to_ns(history->last_end));
		printk("\n\n");
	#endif

}
static void shift_timezone_of_id(char* key, void* value)
{
	info_and_maps* iam = (info_and_maps*)value;
	int history_found = 0;
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
		if(iam->ip_history_map->num_elements > 0)
		{
			history_found = 1;
			shift_timezone_info_previous_reset = iam->info->previous_reset;
			apply_to_every_string_map_value(iam->ip_history_map, shift_timezone_of_ip);
		}
	}
	if(history_found == 0)
	{
		iam->info->previous_reset = iam->info->previous_reset + ((old_minutes_west - local_minutes_west )*60);
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

static void check_for_timezone_shift(ktime_t now, int already_locked)
{
	
	if(already_locked == 0) { spin_lock_bh(&bandwidth_lock); }
	if(now != last_local_mw_update ) /* make sure nothing changed while waiting for lock */
	{
		local_minutes_west = sys_tz.tz_minuteswest;
		local_seconds_west = 60*local_minutes_west;
		last_local_mw_update = now;
		if(local_seconds_west > last_local_mw_update)
		{
			/* we can't let adjusted time be < 0 -- pretend timezone is still UTC */
			local_minutes_west = 0;
			local_seconds_west = 0;
		}

		if(local_minutes_west != old_minutes_west)
		{
			int adj_minutes = old_minutes_west-local_minutes_west;
			adj_minutes = adj_minutes < 0 ? adj_minutes*-1 : adj_minutes;	
			
			if(already_locked == 0) { down(&userspace_lock); }

			printk("nft_bandwidth: timezone shift of %d minutes detected, adjusting\n", adj_minutes);
			printk("               old minutes west=%d, new minutes west=%d\n", old_minutes_west, local_minutes_west);
			
			/* this function is always called with absolute time, not time adjusted for timezone.  Correct that before adjusting */
			shift_timezone_current_time = now - local_seconds_west;
			apply_to_every_string_map_value(id_map, shift_timezone_of_id);

			old_minutes_west = local_minutes_west;


			if(already_locked == 0) { up(&userspace_lock); }
		}
	}
	if(already_locked == 0) { spin_unlock_bh(&bandwidth_lock); }
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
static unsigned char update_history(bw_history* history, ktime_t interval_start, ktime_t interval_end, struct nft_bandwidth_info* info)
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


static struct nft_bandwidth_info* do_reset_info = NULL;
static string_map* do_reset_ip_map = NULL;
static string_map* do_reset_delete_ips = NULL;
static ktime_t do_reset_interval_start = 0;
static ktime_t do_reset_interval_end = 0;
static void do_reset(char* key, void* value)
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
				set_string_map_element(do_reset_delete_ips, key, (void*)(history->history_data + history->current_index));
			}
		}
		else
		{
			set_string_map_element(do_reset_ip_map, key, (void*)(history->history_data + history->current_index) );
		}
	}
}

string_map* clear_ip_map = NULL;
string_map* clear_ip_history_map = NULL;
string_map* clear_ip_family_map = NULL;
static void clear_ips(char* key, void* value)
{
	if(clear_ip_history_map != NULL && clear_ip_map != NULL && clear_ip_family_map != NULL)
	{
		bw_history* history;
		
		#ifdef BANDWIDTH_DEBUG
			printk("clearing ip = %s\n", key);
		#endif

		remove_string_map_element(clear_ip_map, key);
		history = (bw_history*)remove_string_map_element(clear_ip_history_map, key);
		if(history != NULL)
		{
			kfree(history->history_data);
			kfree(history);
		}
		remove_string_map_element(clear_ip_family_map, key);
	}
}

static void set_bandwidth_to_zero(char* key, void* value)
{
	*((uint64_t*)value) = 0;
}


string_map* reset_histories_ip_map = NULL;
static void reset_histories(char* key, void* value)
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
		set_string_map_element(reset_histories_ip_map, key, bh->history_data);
	}
}


static void handle_interval_reset(info_and_maps* iam, ktime_t now)
{
	struct nft_bandwidth_info* info;

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

	info = iam->info;
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
		apply_to_every_string_map_value(iam->ip_map, set_bandwidth_to_zero);
	}
	else
	{
		unsigned long num_updates;
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
		clear_ip_family_map = iam->ip_family_map;
		

		/* 
		 * at most update as many times as we have intervals to save -- prevents
		 * rediculously long loop if interval length is 2 seconds and time was 
		 * reset to 5 years in the future
		 */
		num_updates = 0;
		while(info->next_reset <= now && num_updates < info->num_intervals_to_save)
		{
			do_reset_delete_ips = initialize_string_map(1);
			/* 
			 * don't check for malloc failure here -- we 
			 * include tests for whether do_reset_delete_ips 
			 * is null below (reset should still be able to procede)
			 */

			do_reset_interval_start = info->previous_reset;
			do_reset_interval_end = info->next_reset;
			
			apply_to_every_string_map_value(iam->ip_history_map, do_reset);
			

			info->previous_reset = info->next_reset;
			info->next_reset = get_next_reset_time(info, info->previous_reset, info->previous_reset);

			/* free all data for ips whose entire histories contain only zeros to conserve space */
			if(do_reset_delete_ips != NULL)
			{
				unsigned long num_destroyed;

				/* only clear ips if this is the last iteration of this update */
				if(info->next_reset >= now)
				{
					/* 
					 * no need to reset iam->info->combined_bw if it gets deleted here.
					 * below, at end of function it will get set to NULL if it gets wiped
					 */

					apply_to_every_string_map_value(do_reset_delete_ips, clear_ips);
				}

				/* but clear do_reset_delete_ips no matter what, values are just pointers to history data so we can ignore them */
				destroy_string_map(do_reset_delete_ips, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				do_reset_delete_ips = NULL;
			}
			num_updates++;
		}
		do_reset_info = NULL;
		do_reset_ip_map = NULL;
		clear_ip_map = NULL;
		clear_ip_history_map = NULL;
		clear_ip_family_map = NULL;

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
			apply_to_every_string_map_value(iam->ip_history_map, reset_histories);
			reset_histories_ip_map = NULL;

			info->previous_reset = now;
			info->next_reset = get_next_reset_time(info, now, info->previous_reset);
		}
	}
	info->combined_bw = (uint64_t*)get_string_map_element(iam->ip_map, "0.0.0.0");
	if(iam->other_info != NULL)
	{
		iam->other_info->combined_bw = info->combined_bw;
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


#define ADD_UP_TO_MAX(original,add,is_check) (bandwidth_record_max - original > add && is_check== 0) ? original+add : (is_check ? original : bandwidth_record_max);


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


static ktime_t get_nominal_previous_reset_time(struct nft_bandwidth_info *info, ktime_t current_next_reset)
{
	ktime_t previous_reset = current_next_reset;
	if(info->reset_is_constant_interval == 0)
	{
		/* skip backwards in halves of interval after next, until  */
		ktime_t next = get_next_reset_time(info, current_next_reset, 0);
		ktime_t half_interval = div_s64((next-current_next_reset),2);
		ktime_t half_count, tmp;
		half_interval = half_interval == 0 ? 1 : half_interval; /* must be at least one second, otherwise we loop forever*/
	
		half_count = 1;
		tmp = get_next_reset_time(info, (current_next_reset-(half_count*half_interval)),0);
		while(previous_reset >= current_next_reset)
		{
			previous_reset = tmp;
			half_count++;
			tmp = get_next_reset_time(info, (current_next_reset-(half_count*half_interval)),0);
		}
	}
	else
	{
		previous_reset = current_next_reset - info->reset_interval;
	}
	return previous_reset;
}


static ktime_t get_next_reset_time(struct nft_bandwidth_info *info, ktime_t now, ktime_t previous_reset)
{
	//first calculate when next reset would be if reset_time is 0 (which it may be)
	ktime_t next_reset = 0;
	s64 weeks_since_epoch;
	if(info->reset_is_constant_interval == 0)
	{
		if(info->reset_interval == BANDWIDTH_MINUTE)
		{
			next_reset = (div_s64(now,60) + 1)*60;
			if(info->reset_time > 0)
			{
				ktime_t alt_reset = next_reset + info->reset_time - 60;
				next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
			}
		}
		else if(info->reset_interval == BANDWIDTH_HOUR)
		{
			next_reset = (div_s64(now,(60*60)) + 1)*60*60;
			if(info->reset_time > 0)
			{
				ktime_t alt_reset = next_reset + info->reset_time - (60*60);
				next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
			}
		}
		else if(info->reset_interval == BANDWIDTH_DAY)
		{
			next_reset = (div_s64(now,(60*60*24)) + 1)*60*60*24;
			if(info->reset_time > 0)
			{
				ktime_t alt_reset = next_reset + info->reset_time - (60*60*24);
				next_reset = alt_reset > now ? alt_reset : next_reset+info->reset_time;
			}
		}	
		else if(info->reset_interval == BANDWIDTH_WEEK)
		{
			int current_weekday;
			s64 days_since_epoch = div_s64(now,(60*60*24));
			weeks_since_epoch = div_s64_rem((4 + days_since_epoch),7,&current_weekday);
			next_reset = (days_since_epoch + (7-current_weekday) )*(60*60*24);
			if(info->reset_time > 0)
			{
				ktime_t alt_reset = next_reset + info->reset_time - (60*60*24*7);
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
			s64 days_since_epoch = div_s64(now,(60*60*24));
			uint16_t* month_start_days;	
			ktime_t alt_reset;

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
			
			alt_reset = (days_since_epoch_for_each_year_start[year_index] + month_start_days[month])*(60*60*24) + info->reset_time;
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
			unsigned long adj_reset_time = info->reset_time;
			unsigned long tz_secs = 60 * local_minutes_west;
			if(adj_reset_time < tz_secs)
			{
				unsigned long interval_multiple = 1+(tz_secs/info->reset_interval);
				adj_reset_time = adj_reset_time + (interval_multiple*info->reset_interval);
			}
			adj_reset_time = adj_reset_time - tz_secs;
			
			if(info->reset_time > now)
			{
				s64 whole_intervals = div_s64((info->reset_time - now),info->reset_interval) + 1; /* add one to make sure integer gets rounded UP (since we're subtracting) */
				next_reset = info->reset_time - (whole_intervals*info->reset_interval);
				while(next_reset <= now)
				{
					next_reset = next_reset + info->reset_interval;
				}
				
			}
			else /* info->reset_time <= now */
			{
				s64 whole_intervals = div_s64((now-info->reset_time),info->reset_interval); /* integer gets rounded down */
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
				s64 whole_intervals = div_s64((now-next_reset),info->reset_interval); /* integer gets rounded down */
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

static uint64_t* initialize_map_entries_for_ip(info_and_maps* iam, char* ip, uint64_t initial_bandwidth, uint32_t family)
{
	uint64_t* new_bw = NULL;
	uint32_t* fam = NULL;

	#ifdef BANDWIDTH_DEBUG
		printk("initializing entry for ip: %s, bw=%lld\n", ip, initial_bandwidth);
	#endif
	
	#ifdef BANDWIDTH_DEBUG
		if(iam == NULL){ printk("error in initialization: iam is null!\n"); }
	#endif

	if(iam != NULL) /* should never happen, but let's be certain */
	{
		struct nft_bandwidth_info *info = iam->info;
		string_map* ip_map = iam->ip_map;
		string_map* ip_history_map = iam->ip_history_map;
		string_map* ip_family_map = iam->ip_family_map;

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
				bw_history *new_history;
				#ifdef BANDWIDTH_DEBUG
					printk("  initializing entry for ip with history\n");
				#endif

				new_history = initialize_history(info->num_intervals_to_save);
				if(new_history != NULL) /* check for kmalloc failure */
				{
					bw_history* old_history;
					#ifdef BANDWIDTH_DEBUG
						printk("  malloc succeeded, new history is non-null\n");
					#endif

					new_bw = (uint64_t*)(new_history->history_data + new_history->current_index);
					old_history = set_string_map_element(ip_history_map, ip, (void*)new_history);
					if(old_history != NULL)
					{
						#ifdef BANDWIDTH_DEBUG
							printk("  after initialization old_history not null!  (something is FUBAR)\n");
						#endif
						kfree(old_history->history_data);
						kfree(old_history);
					}
				}
			}
			fam = (uint32_t*)kmalloc(sizeof(uint32_t), GFP_ATOMIC);
			if(new_bw != NULL && fam != NULL) /* check for kmalloc failure */
			{
				uint64_t* old_bw;
				*new_bw = initial_bandwidth;
				old_bw = set_string_map_element(ip_map, ip, (void*)new_bw );
				*fam = family;
				set_string_map_element(ip_family_map, ip, fam);
				
				/* only free old_bw if num_intervals_to_save is zero -- otherwise it already got freed above when we wiped the old history */
				if(old_bw != NULL && info->num_intervals_to_save == 0)
				{
					free(old_bw);
				}

				if(strcmp(ip, "0.0.0.0") == 0)
				{
					info->combined_bw = new_bw;
					if(iam->other_info != NULL)
					{
						iam->other_info->combined_bw = info->combined_bw;
					}
				}

				#ifdef BANDWIDTH_DEBUG
					if(1)
					{
						uint64_t *test = (uint64_t*)get_string_map_element(ip_map, ip);
						if(test == NULL)
						{
							printk("  after initialization bw is null!\n");
						}
						else
						{
							printk("  after initialization bw is %lld\n", *new_bw);
							printk("  after initialization test is %lld\n", *test);
						}
					}
				#endif
			}
		}
	}

	return new_bw;
}

static bool bandwidth_mt4(struct nft_bandwidth_info *priv, const struct sk_buff *skb)
{
	ktime_t now;
	int match_found;
	struct nft_bandwidth_info *rule_priv = priv;
	unsigned char is_check = priv->cmp == BANDWIDTH_CHECK ? 1 : 0;
	unsigned char do_src_dst_swap = 0;
	info_and_maps* iam = NULL;
	string_map* ip_map = NULL;
	int family = NFPROTO_IPV4;
	
	uint64_t* bws[2] = {NULL, NULL};

	/* if we're currently setting this id, ignore new data until set is complete */
	if(set_in_progress == 1)
	{
		if(strcmp(priv->id, set_id) == 0)
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
	now = ktime_get_real_seconds();
	

	if(now != last_local_mw_update )
	{
		check_for_timezone_shift(now, 0);
		check_for_backwards_time_shift(now);
	}
	now = now -  local_seconds_west;  /* Adjust for local timezone */

	spin_lock_bh(&bandwidth_lock);
	
	if(is_check)
	{
		info_and_maps* check_iam;
		do_src_dst_swap = priv->check_type == BANDWIDTH_CHECK_SWAP ? 1 : 0;
		check_iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
		if(check_iam == NULL)
		{
			spin_unlock_bh(&bandwidth_lock);
			return 0;
		}
		priv = check_iam->info;
	}
	else
	{
		// Fetch the master_priv which has everything up to date, instead of this impostor...
		priv = rule_priv->non_const_self;
	}

	if(priv->reset_interval != BANDWIDTH_NEVER)
	{
		if(priv->next_reset < now)
		{
			//do reset
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL) /* should never be null, but let's be sure */
			{
				handle_interval_reset(iam, now);
				ip_map = iam->ip_map;
			}
			else
			{
				/* even in case of malloc failure or weird error we can update these params */
				priv->current_bandwidth = 0;
				priv->next_reset = get_next_reset_time(priv, now, priv->previous_reset);
			}
		}
	}

	if(priv->type == BANDWIDTH_COMBINED)
	{
		if(iam == NULL)
		{
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
			}
		}
		if(ip_map != NULL) /* if this ip_map != NULL iam can never be NULL, so we don't need to check this */
		{
			if(priv->combined_bw == NULL)
			{
				bws[0] = initialize_map_entries_for_ip(iam, "0.0.0.0", skb->len, family);
			}
			else
			{
				bws[0] = priv->combined_bw;
				*(bws[0]) = ADD_UP_TO_MAX(*(bws[0]), (uint64_t)skb->len, is_check);
			}
		}
		else
		{
			#ifdef BANDWIDTH_DEBUG
				printk("error: ip_map is null in match!\n");
			#endif
		}
		priv->current_bandwidth = ADD_UP_TO_MAX(priv->current_bandwidth, (uint64_t)skb->len, is_check);
	}
	else
	{
		struct iphdr* iph = (struct iphdr*)(skb_network_header(skb));
		uint32_t bw_ip_index;
		char* bw_ip = NULL;
		char bw_ips[2][INET_ADDRSTRLEN];
		strcpy(bw_ips[0], "0.0.0.0");
		strcpy(bw_ips[1], "0.0.0.0");
		if(priv->type == BANDWIDTH_INDIVIDUAL_SRC)
		{
			//src ip
			sprintf(bw_ips[0], "%pI4", &iph->saddr);
			if(do_src_dst_swap)
			{
				sprintf(bw_ips[0], "%pI4", &iph->daddr);
			}
		}
		else if (priv->type == BANDWIDTH_INDIVIDUAL_DST)
		{
			//dst ip
			sprintf(bw_ips[0], "%pI4", &iph->daddr);
			if(do_src_dst_swap)
			{
				sprintf(bw_ips[0], "%pI4", &iph->saddr);
			}
		}
		else if(priv->type ==  BANDWIDTH_INDIVIDUAL_LOCAL ||  priv->type == BANDWIDTH_INDIVIDUAL_REMOTE)
		{
			//remote or local ip -- need to test both src && dst
			uint32_t src_ip = iph->saddr;
			uint32_t dst_ip = iph->daddr;
			uint32_t tsrc_ip = 0;
			uint32_t tdst_ip = 0;
			if(priv->type == BANDWIDTH_INDIVIDUAL_LOCAL)
			{
				tsrc_ip = ((priv->local_subnet_mask.s_addr & src_ip) == priv->local_subnet.s_addr) ? src_ip : 0;
				tdst_ip = ((priv->local_subnet_mask.s_addr & dst_ip) == priv->local_subnet.s_addr) ? dst_ip : 0;
				sprintf(bw_ips[0], "%pI4", &tsrc_ip);
				sprintf(bw_ips[1], "%pI4", &tdst_ip);
			}
			else if(priv->type == BANDWIDTH_INDIVIDUAL_REMOTE)
			{
				tsrc_ip = ((priv->local_subnet_mask.s_addr & src_ip) != priv->local_subnet.s_addr) ? src_ip : 0;
				tdst_ip = ((priv->local_subnet_mask.s_addr & dst_ip) != priv->local_subnet.s_addr) ? dst_ip : 0;
				sprintf(bw_ips[0], "%pI4", &tsrc_ip);
				sprintf(bw_ips[1], "%pI4", &tdst_ip);
			}
		}
		
		if(ip_map == NULL)
		{
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
			}	
		}
		if(!is_check && priv->cmp == BANDWIDTH_MONITOR)
		{
			uint64_t* combined_oldval = priv->combined_bw;
			if(combined_oldval == NULL)
			{
				combined_oldval = initialize_map_entries_for_ip(iam, "0.0.0.0", (uint64_t)skb->len, family);
			}
			else
			{
				*combined_oldval = ADD_UP_TO_MAX(*combined_oldval, (uint64_t)skb->len, is_check);
			}
		}
		bw_ip_index = strcmp(bw_ips[0], "0.0.0.0") == 0 ? 1 : 0;
		bw_ip = bw_ips[bw_ip_index];
		if(strcmp(bw_ip, "0.0.0.0") != 0 && ip_map != NULL)
		{
			uint64_t* oldval = get_string_map_element(ip_map, bw_ip);
			if(oldval == NULL)
			{
				if(!is_check)
				{
					/* may return NULL on malloc failure but that's ok */
					oldval = initialize_map_entries_for_ip(iam, bw_ip, (uint64_t)skb->len, family);
				}
			}
			else
			{
				*oldval = ADD_UP_TO_MAX(*oldval, (uint64_t)skb->len, is_check);
			}
			
			/* this is fine, setting bws[bw_ip_index] to NULL on check for undefined value or kmalloc failure won't crash anything */
			bws[bw_ip_index] = oldval;
		}
		
	}

	match_found = 0;
	if(priv->cmp != BANDWIDTH_MONITOR)
	{
		if(priv->cmp == BANDWIDTH_GT)
		{
			match_found = bws[0] != NULL ? ( *(bws[0]) > priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = bws[1] != NULL ? ( *(bws[1]) > priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = priv->current_bandwidth > priv->bandwidth_cutoff ? 1 : match_found;
		}
		else if(priv->cmp == BANDWIDTH_LT)
		{
			match_found = bws[0] != NULL ? ( *(bws[0]) < priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = bws[1] != NULL ? ( *(bws[1]) < priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = priv->current_bandwidth < priv->bandwidth_cutoff ? 1 : match_found;
		}
	}

	spin_unlock_bh(&bandwidth_lock);

	return match_found;
}

static bool bandwidth_mt6(struct nft_bandwidth_info *priv, const struct sk_buff *skb)
{
	ktime_t now;
	int match_found;
	struct nft_bandwidth_info *rule_priv = priv;
	unsigned char is_check = priv->cmp == BANDWIDTH_CHECK ? 1 : 0;
	unsigned char do_src_dst_swap = 0;
	info_and_maps* iam = NULL;
	string_map* ip_map = NULL;
	int family = NFPROTO_IPV6;
	
	uint64_t* bws[2] = {NULL, NULL};

	/* if we're currently setting this id, ignore new data until set is complete */
	if(set_in_progress == 1)
	{
		if(strcmp(priv->id, set_id) == 0)
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
	now = ktime_get_real_seconds();
	

	if(now != last_local_mw_update )
	{
		check_for_timezone_shift(now, 0);
		check_for_backwards_time_shift(now);
	}
	now = now -  local_seconds_west;  /* Adjust for local timezone */

	spin_lock_bh(&bandwidth_lock);
	
	if(is_check)
	{
		info_and_maps* check_iam;
		do_src_dst_swap = priv->check_type == BANDWIDTH_CHECK_SWAP ? 1 : 0;
		check_iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
		if(check_iam == NULL)
		{
			spin_unlock_bh(&bandwidth_lock);
			return 0;
		}
		priv = check_iam->info;
	}
	else
	{
		// Fetch the master_priv which has everything up to date, instead of this impostor...
		priv = rule_priv->non_const_self;
	}

	if(priv->reset_interval != BANDWIDTH_NEVER)
	{
		if(priv->next_reset < now)
		{
			//do reset
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL) /* should never be null, but let's be sure */
			{
				handle_interval_reset(iam, now);
				ip_map = iam->ip_map;
			}
			else
			{
				/* even in case of malloc failure or weird error we can update these params */
				priv->current_bandwidth = 0;
				priv->next_reset = get_next_reset_time(priv, now, priv->previous_reset);
			}
		}
	}

	if(priv->type == BANDWIDTH_COMBINED)
	{
		if(iam == NULL)
		{
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
			}
		}
		if(ip_map != NULL) /* if this ip_map != NULL iam can never be NULL, so we don't need to check this */
		{
			if(priv->combined_bw == NULL)
			{
				bws[0] = initialize_map_entries_for_ip(iam, "0.0.0.0", skb->len, NFPROTO_IPV4);
			}
			else
			{
				bws[0] = priv->combined_bw;
				*(bws[0]) = ADD_UP_TO_MAX(*(bws[0]), (uint64_t)skb->len, is_check);
			}
		}
		else
		{
			#ifdef BANDWIDTH_DEBUG
				printk("error: ip_map is null in match!\n");
			#endif
		}
		priv->current_bandwidth = ADD_UP_TO_MAX(priv->current_bandwidth, (uint64_t)skb->len, is_check);
	}
	else
	{
		struct ipv6hdr* iph = (struct ipv6hdr*)(skb_network_header(skb));
		uint32_t bw_ip_index;
		char* bw_ip = NULL;
		char bw_ips[2][INET6_ADDRSTRLEN];
		strcpy(bw_ips[0], "0.0.0.0");
		strcpy(bw_ips[1], "0.0.0.0");
		if(priv->type == BANDWIDTH_INDIVIDUAL_SRC)
		{
			//src ip
			sprintf(bw_ips[0], "%pI6c", &iph->saddr.s6_addr);
			if(do_src_dst_swap)
			{
				sprintf(bw_ips[0], "%pI6c", &iph->daddr.s6_addr);
			}
		}
		else if (priv->type == BANDWIDTH_INDIVIDUAL_DST)
		{
			//dst ip
			sprintf(bw_ips[0], "%pI6c", &iph->daddr.s6_addr);
			if(do_src_dst_swap)
			{
				sprintf(bw_ips[0], "%pI6c", &iph->saddr.s6_addr);
			}
		}
		else if(priv->type ==  BANDWIDTH_INDIVIDUAL_LOCAL ||  priv->type == BANDWIDTH_INDIVIDUAL_REMOTE)
		{
			//remote or local ip -- need to test both src && dst
			struct in6_addr src_ip = iph->saddr;
			struct in6_addr dst_ip = iph->daddr;
			struct in6_addr tsrc_ip;
			struct in6_addr tdst_ip;
			unsigned int x;
			if(priv->type == BANDWIDTH_INDIVIDUAL_LOCAL)
			{
				for(x = 0; x < 16; x++)
				{
					tsrc_ip.s6_addr[x] = (src_ip.s6_addr[x] & priv->local_subnet6_mask.s6_addr[x]);
					tdst_ip.s6_addr[x] = (dst_ip.s6_addr[x] & priv->local_subnet6_mask.s6_addr[x]);
				}
				if(memcmp(tsrc_ip.s6_addr,priv->local_subnet6.s6_addr,sizeof(unsigned char)*16) == 0)
				{
					tsrc_ip = src_ip;
				}
				else
				{
					memset(tsrc_ip.s6_addr,0,sizeof(unsigned char)*16);
				}
				if(memcmp(tdst_ip.s6_addr,priv->local_subnet6.s6_addr,sizeof(unsigned char)*16) == 0)
				{
					tdst_ip = dst_ip;
				}
				else
				{
					memset(tdst_ip.s6_addr,0,sizeof(unsigned char)*16);
				}
				sprintf(bw_ips[0], "%pI6c", &tsrc_ip.s6_addr);
				sprintf(bw_ips[1], "%pI6c", &tdst_ip.s6_addr);
			}
			else if(priv->type == BANDWIDTH_INDIVIDUAL_REMOTE)
			{
				for(x = 0; x < 16; x++)
				{
					tsrc_ip.s6_addr[x] = (src_ip.s6_addr[x] & priv->local_subnet6_mask.s6_addr[x]);
					tdst_ip.s6_addr[x] = (dst_ip.s6_addr[x] & priv->local_subnet6_mask.s6_addr[x]);
				}
				if(memcmp(tsrc_ip.s6_addr,priv->local_subnet6.s6_addr,sizeof(unsigned char)*16) != 0)
				{
					sprintf(bw_ips[0], "%pI6c", &src_ip.s6_addr);
				}
				else
				{
					sprintf(bw_ips[0], "%s", "0.0.0.0");
				}
				if(memcmp(tdst_ip.s6_addr,priv->local_subnet6.s6_addr,sizeof(unsigned char)*16) != 0)
				{
					sprintf(bw_ips[1], "%pI6c", &dst_ip.s6_addr);
				}
				else
				{
					sprintf(bw_ips[1], "%s", "0.0.0.0");
				}
			}
		}
		
		if(ip_map == NULL)
		{
			//iam = (info_and_maps*)get_string_map_element_with_hashed_key(id_map, priv->hashed_id);
			iam = (info_and_maps*)priv->iam;
			if(iam != NULL)
			{
				ip_map = iam->ip_map;
			}	
		}
		if(!is_check && priv->cmp == BANDWIDTH_MONITOR)
		{
			uint64_t* combined_oldval = priv->combined_bw;
			if(combined_oldval == NULL)
			{
				combined_oldval = initialize_map_entries_for_ip(iam, "0.0.0.0", (uint64_t)skb->len, NFPROTO_IPV4);
			}
			else
			{
				*combined_oldval = ADD_UP_TO_MAX(*combined_oldval, (uint64_t)skb->len, is_check);
			}
		}
		bw_ip_index = strcmp(bw_ips[0], "0.0.0.0") == 0 ? 1 : 0;
		bw_ip = bw_ips[bw_ip_index];
		if(strcmp(bw_ip, "0.0.0.0") != 0 && ip_map != NULL)
		{
			uint64_t* oldval = get_string_map_element(ip_map, bw_ip);
			if(oldval == NULL)
			{
				if(!is_check)
				{
					/* may return NULL on malloc failure but that's ok */
					oldval = initialize_map_entries_for_ip(iam, bw_ip, (uint64_t)skb->len, family);
				}
			}
			else
			{
				*oldval = ADD_UP_TO_MAX(*oldval, (uint64_t)skb->len, is_check);
			}
			
			/* this is fine, setting bws[bw_ip_index] to NULL on check for undefined value or kmalloc failure won't crash anything */
			bws[bw_ip_index] = oldval;
		}
		
	}

	match_found = 0;
	if(priv->cmp != BANDWIDTH_MONITOR)
	{
		if(priv->cmp == BANDWIDTH_GT)
		{
			match_found = bws[0] != NULL ? ( *(bws[0]) > priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = bws[1] != NULL ? ( *(bws[1]) > priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = priv->current_bandwidth > priv->bandwidth_cutoff ? 1 : match_found;
		}
		else if(priv->cmp == BANDWIDTH_LT)
		{
			match_found = bws[0] != NULL ? ( *(bws[0]) < priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = bws[1] != NULL ? ( *(bws[1]) < priv->bandwidth_cutoff ? 1 : match_found ) : match_found;
			match_found = priv->current_bandwidth < priv->bandwidth_cutoff ? 1 : match_found;
		}
	}

	spin_unlock_bh(&bandwidth_lock);

	return match_found;
}

/**********************
 * Get functions
 *********************/
#define ERROR_NONE 0
#define ERROR_NO_ID 1
#define ERROR_BUFFER_TOO_SHORT 2
#define ERROR_NO_HISTORY 3
#define ERROR_UNKNOWN 4
typedef struct get_req_struct 
{
	uint32_t family;
	uint32_t ip[4];
	uint32_t next_ip_index;
	unsigned char return_history;
	char id[BANDWIDTH_MAX_ID_LENGTH];
} get_request;

static char** output_ip_list = NULL;
static unsigned long output_ip_list_length = 0;

static char add_ip_block(uint32_t family,
			uint32_t* ip, 
			unsigned char full_history_requested,
			info_and_maps* iam,
			unsigned char* output_buffer, 
			uint32_t* current_output_index, 
			uint32_t buffer_length 
			);
static void parse_get_request(unsigned char* request_buffer, get_request* parsed_request);
static int handle_get_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, unsigned char error_code, unsigned char* out_buffer, unsigned char* free_buffer );

/* 
 * returns whether we succeeded in adding ip block, 0= success, 
 * otherwise error code of problem that we found
 */
static char add_ip_block(uint32_t family,
				uint32_t* ip, 
				unsigned char full_history_requested,
				info_and_maps* iam,
				unsigned char* output_buffer, 
				uint32_t* current_output_index, 
				uint32_t output_buffer_length 
				)
{
	char ipstr[INET6_ADDRSTRLEN];
	if(family == NFPROTO_IPV4)
	{
		sprintf(ipstr, "%pI4", ip);
	}
	else
	{
		sprintf(ipstr, "%pI6c", ip);
	}
	#ifdef BANDWIDTH_DEBUG
		printk("doing output for ip = %s\n", ipstr);
	#endif

	if(full_history_requested)
	{
		bw_history* history = NULL;
		if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map != NULL)
		{
			history = (bw_history*)get_string_map_element(iam->ip_history_map, ipstr);
		}
		if(history == NULL)
		{
			uint32_t block_length = (2*4) + (3*8) + (1*16);
			uint64_t *bw;

			#ifdef BANDWIDTH_DEBUG
				printk("  no history map for ip, dumping latest value in history format\n" );
			#endif

			if(*current_output_index + block_length > output_buffer_length)
			{
				return ERROR_BUFFER_TOO_SHORT;
			}
			*( (uint32_t*)(output_buffer + *current_output_index) ) = family;
			*current_output_index = *current_output_index + 4;
			
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *ip;
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+1);
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+2);
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+3);
			*current_output_index = *current_output_index + 4;
	
			*( (uint32_t*)(output_buffer + *current_output_index) ) = 1;
			*current_output_index = *current_output_index + 4;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * local_minutes_west);
			*current_output_index = *current_output_index + 8;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * local_minutes_west);
			*current_output_index = *current_output_index + 8;

			*( (uint64_t*)(output_buffer + *current_output_index) ) = (uint64_t)iam->info->previous_reset + (60 * local_minutes_west);
			*current_output_index = *current_output_index + 8;

			bw = (uint64_t*)get_string_map_element(iam->ip_map, ipstr);
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
			uint32_t block_length = (2*4) + (3*8) + (1*16) + (8*history->num_nodes);
			uint64_t last_reset;
			uint32_t node_num;
			uint32_t next_index;

			if(*current_output_index + block_length > output_buffer_length)
			{
				return ERROR_BUFFER_TOO_SHORT;
			}
		
			*( (uint32_t*)(output_buffer + *current_output_index) ) = family;
			*current_output_index = *current_output_index + 4;
			
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *ip;
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+1);
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+2);
			*current_output_index = *current_output_index + 4;
			*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+3);
			*current_output_index = *current_output_index + 4;
	
			*( (uint32_t*)(output_buffer + *current_output_index) )= history->num_nodes;
			*current_output_index = *current_output_index + 4;

			
			
			/* need to return times in regular UTC not the UTC - minutes west, which is useful for processing */
			last_reset = (uint64_t)iam->info->previous_reset + (60 * local_minutes_west);
			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->first_start > 0 ? (uint64_t)history->first_start + (60 * local_minutes_west) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping first start = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->first_end > 0 ?   (uint64_t)history->first_end + (60 * local_minutes_west) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping first end   = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			*( (uint64_t*)(output_buffer + *current_output_index) ) = history->last_end > 0 ?    (uint64_t)history->last_end + (60 * local_minutes_west) : last_reset;
			#ifdef BANDWIDTH_DEBUG
				printk("  dumping last end    = %lld\n", *( (uint64_t*)(output_buffer + *current_output_index) )   );
			#endif
			*current_output_index = *current_output_index + 8;



			node_num = 0;
			next_index = history->num_nodes == history->max_nodes ? history->current_index+1 : 0;
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
		uint64_t *bw;
		if(*current_output_index + 28 > output_buffer_length)
		{
			return ERROR_BUFFER_TOO_SHORT;
		}

		*( (uint32_t*)(output_buffer + *current_output_index) ) = family;
		*current_output_index = *current_output_index + 4;
		
		*( (uint32_t*)(output_buffer + *current_output_index) ) = *ip;
		*current_output_index = *current_output_index + 4;
		*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+1);
		*current_output_index = *current_output_index + 4;
		*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+2);
		*current_output_index = *current_output_index + 4;
		*( (uint32_t*)(output_buffer + *current_output_index) ) = *(ip+3);
		*current_output_index = *current_output_index + 4;

		bw = (uint64_t*)get_string_map_element(iam->ip_map, ipstr);
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
static int handle_get_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, unsigned char error_code, unsigned char* out_buffer, unsigned char* free_buffer )
{
	unsigned long retval;
	retval = copy_to_user(out_buffer, &error_code, 1);
	if( free_buffer != NULL ) { kfree(free_buffer); }
	if(unlock_bandwidth_spin) { spin_unlock_bh(&bandwidth_lock); }
	if(unlock_user_sem) { up(&userspace_lock); }
	return ret_value;
}

/* 
 * request structure: 
 * bytes 1:4 is family (uint32_t)
 * bytes 5:20 is ip (uint32_t * 4) (left aligned i.e. ipv4 takes up 5:8, not 17:20)
 * bytes 21:24 is the next ip index (uint32_t)
 * byte  25   is whether to return full history or just current usage (unsigned char)
 * bytes 26:26+MAX_ID_LENGTH are the id (a string)
 */
static void parse_get_request(unsigned char* request_buffer, get_request* parsed_request)
{
	uint32_t* family = (uint32_t*)(request_buffer+0);
	uint32_t* ip = (uint32_t*)(request_buffer+4);
	uint32_t* next_ip_index = (uint32_t*)(request_buffer+20);
	unsigned char* return_history = (unsigned char*)(request_buffer+24);

	parsed_request->family = *family;
	if(parsed_request->family == NFPROTO_IPV4)
	{
		parsed_request->ip[0] = *ip;
		parsed_request->ip[1] = 0;
		parsed_request->ip[2] = 0;
		parsed_request->ip[3] = 0;
	}
	else
	{
		parsed_request->ip[0] = *ip;
		parsed_request->ip[1] = *(ip+1);
		parsed_request->ip[2] = *(ip+2);
		parsed_request->ip[3] = *(ip+3);
	}
	parsed_request->next_ip_index = *next_ip_index;
	parsed_request->return_history = *return_history;
	memcpy(parsed_request->id, request_buffer+25, BANDWIDTH_MAX_ID_LENGTH);
	(parsed_request->id)[BANDWIDTH_MAX_ID_LENGTH-1] = '\0'; /* make sure id is null terminated no matter what */
	
	#ifdef BANDWIDTH_DEBUG
		if(parsed_request->family == NFPROTO_IPV4)
		{
			printk("ip = %pI4\n", ip);
			printk("ip = %pI4\n", parsed_request->ip);
		}
		else
		{
			printk("ip6 = %pI6c\n", ip);
			printk("ip6 = %pI6c\n", parsed_request->ip);
		}
		printk("next ip index = %d\n", *next_ip_index);
		printk("return_history = %d\n", *return_history);
	#endif
}

static int nft_bandwidth_get_ctl(struct sock *sk, int cmd, void *user, int *len)
{
	/* check for timezone shift & adjust if necessary */
	char* buffer;
	get_request query;
	info_and_maps* iam;

	uint32_t testblk[4];
	unsigned char* error;
	uint32_t* total_ips;
	uint32_t* start_index;
	uint32_t* num_ips_in_response;
	uint64_t* reset_interval;
	uint64_t* reset_time;
	unsigned char* reset_is_constant_interval;
	uint32_t  current_output_index;
	unsigned long retval;
	ktime_t now = ktime_get_real_seconds();
	check_for_timezone_shift(now, 0);
	check_for_backwards_time_shift(now);
	now = now -  local_seconds_west;  /* Adjust for local timezone */

	down(&userspace_lock);

	/* first check that query buffer is big enough to hold the info needed to parse the query */
	if(*len < BANDWIDTH_MAX_ID_LENGTH + 25)
	{

		return handle_get_failure(0, 1, 0, ERROR_BUFFER_TOO_SHORT, user, NULL);
	}

	/* copy the query from userspace to kernel space & parse */
	buffer = kmalloc(*len, GFP_ATOMIC);
	if(buffer == NULL) /* check for malloc failure */
	{
		return handle_get_failure(0, 1, 0, ERROR_UNKNOWN, user, NULL);
	}
	retval = copy_from_user(buffer, user, *len);
	parse_get_request(buffer, &query);

	/* 
	 * retrieve data for this id and verify all variables are properly defined, just to be sure
	 * this is a kernel module -- it pays to be paranoid! 
	 */
	spin_lock_bh(&bandwidth_lock);
	
	iam = (info_and_maps*)get_string_map_element(id_map, query.id);
	
	if(iam == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	if(iam->info == NULL || iam->ip_map == NULL || iam->ip_family_map == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map == NULL)
	{
		return handle_get_failure(0, 1, 1, ERROR_NO_ID, user, buffer);
	}
	
	/* allocate ip list if this is first query */
	memset(testblk, 0, sizeof(uint32_t)*4);
	if((query.next_ip_index == 0 || query.next_ip_index == __UINT32_MAX__) && memcmp(testblk, query.ip, sizeof(uint32_t)*4) == 0)
	{
		if(output_ip_list != NULL)
		{
			free_null_terminated_string_array(output_ip_list);
		}
		if(iam->info->type == BANDWIDTH_COMBINED || query.next_ip_index == __UINT32_MAX__)
		{
			output_ip_list_length = 1;
			output_ip_list = (char**)kmalloc(sizeof(char*)*2, GFP_ATOMIC);
			if(output_ip_list != NULL) { output_ip_list[0] = strdup("0.0.0.0"); output_ip_list[1] = NULL; }
			// We set next_ip_index to a very large number to indicate that we only want the COMBINED (0.0.0.0) use case only.
			// Reset the variable here to a sensible value.
			query.next_ip_index = 0;
		}
		else
		{
			output_ip_list = get_string_map_keys(iam->ip_map, &output_ip_list_length);
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
	 * bytes 1-4 : family
	 * bytes 5-20 : ip
	 * bytes 21-28 : bandwidth
	 *
	 * if history WAS queried we have
	 *   (note we are using 64 bit integers for time here
	 *   even though ktime_t is 32 bits on most 32 bit systems
	 *   just to be on the safe side)
	 * bytes 1-4 : family
	 * bytes 5-20 : ip
	 * bytes 21-24 : history_length number of history values (including current)
	 * bytes 25-32 : first start
	 * bytes 33-40 : first end
	 * bytes 41-48 : recent end 
	 * 49 onward : list of 64 bit integers of length history_length
	 *
	 */
	error = buffer;
	total_ips = (uint32_t*)(buffer+1);
	start_index = (uint32_t*)(buffer+5);
	num_ips_in_response = (uint32_t*)(buffer+9);
	reset_interval = (uint64_t*)(buffer+13);
	reset_time = (uint64_t*)(buffer+21);
	reset_is_constant_interval = (char*)(buffer+29);

	*reset_interval = (uint64_t)iam->info->reset_interval;
	*reset_time = (uint64_t)iam->info->reset_time;
	*reset_is_constant_interval = iam->info->reset_is_constant_interval;

	current_output_index = 30;
	if(memcmp(testblk, query.ip, sizeof(uint32_t)*4) != 0)
	{
		*error = add_ip_block(query.family,
					query.ip, 
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
			uint32_t next_ip[4] = {0};
			uint32_t family = NFPROTO_IPV4;
			int ret;
			ret = in4_pton(output_ip_list[next_index], -1, (u8 *)next_ip, '\0', NULL);
			if(ret == 0)
			{
				family = NFPROTO_IPV6;
				ret = in6_pton(output_ip_list[next_index], -1, (u8 *)next_ip, '\0', NULL);
			}
			if(ret == 0)
			{
				*error = ERROR_UNKNOWN;
			}
			else
			{
				*error = add_ip_block(family,
						next_ip, 
						query.return_history,
						iam,
						buffer, 
						&current_output_index, 
						*len
						);
			}
			
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
			free_null_terminated_string_array(output_ip_list);
			output_ip_list = NULL;
			output_ip_list_length = 0;
		}
	}

	spin_unlock_bh(&bandwidth_lock);
	
	retval = copy_to_user(user, buffer, *len);
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
	ktime_t last_backup;
	char id[BANDWIDTH_MAX_ID_LENGTH];
} set_header;

static int handle_set_failure(int ret_value, int unlock_user_sem, int unlock_bandwidth_spin, unsigned char* free_buffer );
static void parse_set_header(unsigned char* input_buffer, set_header* header);
static void set_single_ip_data(unsigned char history_included, info_and_maps* iam, unsigned char* buffer, uint32_t* buffer_index, ktime_t now);

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
	header->last_backup = (ktime_t)*last_backup;
	memcpy(header->id, input_buffer+22, BANDWIDTH_MAX_ID_LENGTH);
	(header->id)[BANDWIDTH_MAX_ID_LENGTH-1] = '\0'; /* make sure id is null terminated no matter what */

	#ifdef BANDWIDTH_DEBUG
		printk("parsed set header:\n");
		printk("  total_ips         = %d\n", header->total_ips);
		printk("  next_ip_index     = %d\n", header->next_ip_index);
		printk("  num_ips_in_buffer = %d\n", header->num_ips_in_buffer);
		printk("  zero_unset_ips    = %d\n", header->zero_unset_ips);
		printk("  last_backup       = %lld\n", ktime_to_ns(header->last_backup));
		printk("  id                = %s\n", header->id);
	#endif
}
static void set_single_ip_data(unsigned char history_included, info_and_maps* iam, unsigned char* buffer, uint32_t* buffer_index, ktime_t now)
{
	/* 
	 * note that times stored within the module are adjusted so they are equal to seconds 
	 * since unix epoch that corrosponds to the UTC wall-clock time (timezone offset 0) 
	 * that is equal to the wall-clock time in the current time-zone.  Incoming values must 
	 * be adjusted similarly
	 */
	char ipstr[INET6_ADDRSTRLEN];
	uint32_t testblk[4];
	uint32_t family = *( (uint32_t*)(buffer + *buffer_index) );
	uint32_t ip[4];
	ip[0] = *( (uint32_t*)(buffer + *buffer_index+4) );
	ip[1] = *( (uint32_t*)(buffer + *buffer_index+8) );
	ip[2] = *( (uint32_t*)(buffer + *buffer_index+12) );
	ip[3] = *( (uint32_t*)(buffer + *buffer_index+16) );
	if(family == NFPROTO_IPV4)
	{
		sprintf(ipstr, "%pI4", &ip);
	}
	else
	{
		sprintf(ipstr, "%pI6c", &ip);
	}
	
	// We only ever want to use 0.0.0.0 for COMBINED bw. If an attempt is made to set the ipv6 equivalent, silently redirect it
	if(strcmp(ipstr,"::") == 0)
	{
		#ifdef BANDWIDTH_DEBUG
			printk("found combined ipv6 data, redirecting to ipv4\n");
		#endif
		strcpy(ipstr, "0.0.0.0");
		family = NFPROTO_IPV4;
	}
	
	#ifdef BANDWIDTH_DEBUG
		printk("doing set for ip = %s, family = %d\n", ipstr, family);
		printk("ip index = %d\n", *buffer_index);
	#endif
	
	memset(testblk, 0, sizeof(uint32_t)*4);

	if(history_included)
	{
		uint32_t num_history_nodes = *( (uint32_t*)(buffer + *buffer_index+20));
		if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map != NULL)
		{
			ktime_t first_start = (ktime_t) *( (uint64_t*)(buffer + *buffer_index+24));
			ktime_t next_start;
			ktime_t next_end;
			uint32_t node_index;
			uint32_t zero_count;
			bw_history* history;


			#ifdef BANDWIDTH_DEBUG
				printk("setting history with first start = %lld, now = %lld\n", ktime_to_ns(first_start), ktime_to_ns(now));
			#endif


			*buffer_index = *buffer_index + (2*4) + (1*16) + (3*8);
			
			/* adjust for timezone */
			next_start = first_start - (60 * local_minutes_west);
			next_end = get_next_reset_time(iam->info, next_start, next_start);
			node_index=0;
			zero_count=0;
			history = NULL;
			while(next_start < now)
			{
				uint64_t next_bw = 0;
				if(node_index < num_history_nodes)
				{
					next_bw = *( (uint64_t*)(buffer + *buffer_index));
					*buffer_index = *buffer_index + 8;
				}
				zero_count = next_bw == 0 ? zero_count+1 : 0;
				
				if(node_index == 0 || history == NULL)
				{
					initialize_map_entries_for_ip(iam, ipstr, next_bw, family);
					history = get_string_map_element(iam->ip_history_map, ipstr);
				}
				else if(next_end < now) /* if this is most recent node, don't do update since last node is current bandwidth */ 
				{
					update_history(history, next_start, next_end, iam->info);
					(history->history_data)[ history->current_index ] = next_bw;
					if(zero_count < history->max_nodes +2)
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
				set_string_map_element(iam->ip_map, ipstr, (history->history_data + history->current_index) );
				iam->info->previous_reset = next_start;
				iam->info->next_reset = next_end;
				if(memcmp(testblk, ip, sizeof(uint32_t)*4) == 0)
				{
					iam->info->current_bandwidth = (history->history_data)[history->current_index];
				}
			}
		}
		else
		{
			uint64_t bw;
			*buffer_index = *buffer_index + (2*4) + (1*16) + (3*8) + ((num_history_nodes-1)*8);
			bw = *( (uint64_t*)(buffer + *buffer_index));
			initialize_map_entries_for_ip(iam, ipstr, bw, family); /* automatically frees existing values if they exist */
			*buffer_index = *buffer_index + 8;
			if(memcmp(testblk, ip, sizeof(uint32_t)*4) == 0)
			{
				iam->info->current_bandwidth = bw;
			}
		}

	}
	else
	{
		uint64_t bw = *( (uint64_t*)(buffer + *buffer_index+20) );
		#ifdef BANDWIDTH_DEBUG
			printk("  setting bw to %lld\n", bw );
		#endif

		
		initialize_map_entries_for_ip(iam, ipstr, bw, family); /* automatically frees existing values if they exist */
		*buffer_index = *buffer_index + 28;

		if(memcmp(testblk, ip, sizeof(uint32_t)*4) == 0)
		{
			iam->info->current_bandwidth = bw;
		}
	}
}

static int nft_bandwidth_set_ctl(struct sock *sk, int cmd, sockptr_t arg, u_int32_t len)
{
	/* check for timezone shift & adjust if necessary */
	char* buffer;
	set_header header;
	info_and_maps* iam;
	uint32_t buffer_index;
	uint32_t next_ip_index;
	ktime_t now = ktime_get_real_seconds();
	check_for_timezone_shift(now, 0);
	check_for_backwards_time_shift(now);
	now = now -  local_seconds_west;  /* Adjust for local timezone */

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
	
	buffer = kmalloc(len, GFP_ATOMIC);
	if(buffer == NULL) /* check for malloc failure */
	{
		return handle_set_failure(0, 1, 0, NULL);
	}
	copy_from_sockptr(buffer, arg, len);
	parse_set_header(buffer, &header);

	/* 
	 * retrieve data for this id and verify all variables are properly defined, just to be sure
	 * this is a kernel module -- it pays to be paranoid! 
	 */
	spin_lock_bh(&bandwidth_lock);

	iam = (info_and_maps*)get_string_map_element(id_map, header.id);
	if(iam == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}
	if(iam->info == NULL || iam->ip_map == NULL || iam->ip_family_map == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}
	if(iam->info->num_intervals_to_save > 0 && iam->ip_history_map == NULL)
	{
		return handle_set_failure(0, 1, 1, buffer);
	}

	/* 
	 * during set unconditionally set combined_bw to NULL 
	 * if combined data (ip=0) exists after set exits cleanly, we will restore it
	 */
	iam->info->combined_bw = NULL;
	if(iam->other_info != NULL)
	{
		iam->other_info->combined_bw = NULL;
	}

	//if zero_unset_ips == 1 && next_ip_index == 0
	//then clear data for all ips for this id
	if(header.zero_unset_ips && header.next_ip_index == 0)
	{
		//clear data
		if(iam->info->num_intervals_to_save > 0)
		{
			if(iam->ip_map->num_elements > 0)
			{
				unsigned long num_ips = 0;
				unsigned long ip_index = 0;
				char** iplist = (char**)get_string_map_keys(iam->ip_map, &num_ips);
				for(ip_index = 0; ip_index < num_ips; ip_index++)
				{
					uint32_t* fam = NULL;
					remove_string_map_element(iam->ip_map, iplist[ip_index]);
					fam = remove_string_map_element(iam->ip_family_map, iplist[ip_index]);
					kfree(fam);
				}
				/* ignore return value for bw -- it's actually malloced in history, not here */
				free_null_terminated_string_array(iplist);
			}
			if(iam->ip_history_map->num_elements > 0)
			{
				unsigned long num_history = 0;
				unsigned long history_index = 0;
				char** historylist = (char**)get_string_map_keys(iam->ip_history_map, &num_history);
				for(history_index = 0; history_index < num_history; history_index++)
				{
					bw_history* history = remove_string_map_element(iam->ip_history_map, historylist[history_index]);
					kfree(history->history_data);
					kfree(history);
				}
				free_null_terminated_string_array(historylist);
			}
		}
		else
		{
			if(iam->ip_map->num_elements > 0)
			{
				unsigned long num_ips = 0;
				unsigned long ip_index = 0;
				char** iplist = (char**)get_string_map_keys(iam->ip_map, &num_ips);
				for(ip_index = 0; ip_index < num_ips; ip_index++)
				{
					uint32_t* fam = NULL;
					uint64_t *bw = remove_string_map_element(iam->ip_map, iplist[ip_index]);
					kfree(bw);
					fam = remove_string_map_element(iam->ip_family_map, iplist[ip_index]);
					kfree(fam);
				}
				free_null_terminated_string_array(iplist);
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
		ktime_t adjusted_last_backup_time = header.last_backup - (60 * local_minutes_west); 
		ktime_t next_reset_of_last_backup = get_next_reset_time(iam->info, adjusted_last_backup_time, adjusted_last_backup_time);
		if(next_reset_of_last_backup != iam->info->next_reset)
		{
			return handle_set_failure(0, 1, 1, buffer);
		}
	}

	/*
	 * iterate over each ip block in buffer, 
	 * loading data into necessary kerenel-space data structures
	*/
	buffer_index = (3*4) + 1 + 1 + 8 + BANDWIDTH_MAX_ID_LENGTH;
	next_ip_index = header.next_ip_index;
	
	while(next_ip_index < header.num_ips_in_buffer)
	{
		set_single_ip_data(header.history_included, iam, buffer, &buffer_index, now);
		next_ip_index++;
	}

	if (next_ip_index == header.total_ips)
	{
		set_in_progress = 0;
	}

	/* set combined_bw */
	iam->info->combined_bw = (uint64_t*)get_string_map_element(iam->ip_map, "0.0.0.0");
	if(iam->other_info != NULL)
	{
		iam->other_info->combined_bw = iam->info->combined_bw;
	}

	kfree(buffer);
	spin_unlock_bh(&bandwidth_lock);
	up(&userspace_lock);
	return 0;
}

static void nft_bandwidth_eval(const struct nft_expr *expr, struct nft_regs *regs, const struct nft_pktinfo *pkt) {
	struct nft_bandwidth_info *priv = nft_expr_priv(expr);
	struct ethhdr *eth = eth_hdr(pkt->skb);
	struct sk_buff *skb = pkt->skb;
	
	switch (eth->h_proto) {
	case htons(ETH_P_IP):
		if(!bandwidth_mt4(priv, skb))
			regs->verdict.code = NFT_BREAK;
		break;
	case htons(ETH_P_IPV6):
		if(!bandwidth_mt6(priv, skb))
			regs->verdict.code = NFT_BREAK;
		break;
	default:
		break;
	}
}

static void* pton_guess_family(char* ipstr, int* family)
{
	unsigned char* buf;
	const char* end;
	int ret = 0;

	buf = kcalloc(1,sizeof(struct in6_addr),GFP_ATOMIC);
	if(buf == NULL) return buf;

	// Try IPv4
	ret = in4_pton(ipstr, -1, buf, -1, &end);
	if(ret == 1)
	{
		*family = AF_INET;
	}
	else
	{
		// Try IPv6
		memset(buf,0,sizeof(struct in6_addr));
		ret = in6_pton(ipstr, -1, buf, -1, &end);
		if(ret == 0)
		{
			free(buf);
			buf = NULL;
		}
		else if(ret == 1)
		{
			*family = AF_INET6;
		}
	}

	return buf;
}

/*
 * line is the line to be parsed -- it is not modified in any way
 * max_pieces indicates number of pieces to return, if negative this is determined dynamically
 * include_remainder_at_max indicates whether the last piece, when max pieces are reached, 
 * 	should be what it would normally be (0) or the entire remainder of the line (1)
 * 	if max_pieces < 0 this parameter is ignored
 *
 *
 * returns all non-separator pieces in a line
 * result is dynamically allocated, MUST be freed after call-- even if 
 * line is empty (you still get a valid char** pointer to to a NULL char*)
 */
char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max, unsigned long *num_pieces)
{
	char** split;
	
	*num_pieces = 0;
	if(line != NULL)
	{
		int split_index;
		int non_separator_found;
		char* dup_line;
		char* start;

		if(max_pieces < 0)
		{
			/* count number of separator characters in line -- this count + 1 is an upperbound on number of pieces */
			int separator_count = 0;
			int line_index;
			for(line_index = 0; line[line_index] != '\0'; line_index++)
			{
				int sep_index;
				int found = 0;
				for(sep_index =0; found == 0 && sep_index < num_separators; sep_index++)
				{
					found = separators[sep_index] == line[line_index] ? 1 : 0;
				}
				separator_count = separator_count+ found;
			}
			max_pieces = separator_count + 1;
		}
		split = (char**)malloc((1+max_pieces)*sizeof(char*));
		split_index = 0;
		split[split_index] = NULL;


		dup_line = strdup(line);
		start = dup_line;
		non_separator_found = 0;
		while(non_separator_found == 0)
		{
			int matches = 0;
			int sep_index;
			for(sep_index =0; sep_index < num_separators; sep_index++)
			{
				matches = matches == 1 || separators[sep_index] == start[0] ? 1 : 0;
			}
			non_separator_found = matches==0 || start[0] == '\0' ? 1 : 0;
			if(non_separator_found == 0)
			{
				start++;
			}
		}

		while(start[0] != '\0' && split_index < max_pieces)
		{
			/* find first separator index */
			int first_separator_index = 0;
			int separator_found = 0;
			while(	separator_found == 0 )
			{
				int sep_index;
				for(sep_index =0; separator_found == 0 && sep_index < num_separators; sep_index++)
				{
					separator_found = separators[sep_index] == start[first_separator_index] || start[first_separator_index] == '\0' ? 1 : 0;
				}
				if(separator_found == 0)
				{
					first_separator_index++;
				}
			}
			
			/* copy next piece to split array */
			if(first_separator_index > 0)
			{
				char* next_piece = NULL;
				if(split_index +1 < max_pieces || include_remainder_at_max <= 0)
				{
					next_piece = (char*)malloc((first_separator_index+1)*sizeof(char));
					memcpy(next_piece, start, first_separator_index);
					next_piece[first_separator_index] = '\0';
				}
				else
				{
					next_piece = strdup(start);
				}
				split[split_index] = next_piece;
				split[split_index+1] = NULL;
				split_index++;
			}


			/* find next non-separator index, indicating start of next piece */
			start = start+ first_separator_index;
			non_separator_found = 0;
			while(non_separator_found == 0)
			{
				int matches = 0;
				int sep_index;
				for(sep_index =0; sep_index < num_separators; sep_index++)
				{
					matches = matches == 1 || separators[sep_index] == start[0] ? 1 : 0;
				}
				non_separator_found = matches==0 || start[0] == '\0' ? 1 : 0;
				if(non_separator_found == 0)
				{
					start++;
				}
			}
		}
		free(dup_line);
		*num_pieces = split_index;
	}
	else
	{
		split = (char**)malloc((1)*sizeof(char*));
		split[0] = NULL;
	}
	return split;
}

char* trim_flanking_whitespace(char* str)
{
	int new_start = 0;
	int new_length = 0;

	char whitespace[5] = { ' ', '\t', '\n', '\r', '\0' };
	int num_whitespace_chars = 4;
	
	
	int str_index = 0;
	int is_whitespace = 1;
	int test;
	while( (test = str[str_index]) != '\0' && is_whitespace == 1)
	{
		int whitespace_index;
		is_whitespace = 0;
		for(whitespace_index = 0; whitespace_index < num_whitespace_chars && is_whitespace == 0; whitespace_index++)
		{
			is_whitespace = test == whitespace[whitespace_index] ? 1 : 0;
		}
		str_index = is_whitespace == 1 ? str_index+1 : str_index;
	}
	new_start = str_index;


	str_index = strlen(str) - 1;
	is_whitespace = 1;
	while( str_index >= new_start && is_whitespace == 1)
	{
		int whitespace_index;
		is_whitespace = 0;
		for(whitespace_index = 0; whitespace_index < num_whitespace_chars && is_whitespace == 0; whitespace_index++)
		{
			is_whitespace = str[str_index] == whitespace[whitespace_index] ? 1 : 0;
		}
		str_index = is_whitespace == 1 ? str_index-1 : str_index;
	}
	new_length = str[new_start] == '\0' ? 0 : str_index + 1 - new_start;
	

	if(new_start > 0)
	{
		for(str_index = 0; str_index < new_length; str_index++)
		{
			str[str_index] = str[str_index+new_start];
		}
	}
	str[new_length] = 0;
	return str;
}

void parse_ips_and_ranges(char* addr_str, struct nft_bandwidth_info *priv)
{
	int family = 0;
	unsigned long num_pieces;
	void* addr = NULL;

	if(addr_str != NULL && strchr(addr_str, '/') != NULL)
	{
		char** range_parts = split_on_separators(addr_str, "/", 1, 2, 1, &num_pieces);
		char* start = trim_flanking_whitespace(range_parts[0]);
		char* end = trim_flanking_whitespace(range_parts[1]);
		struct in_addr bip;
		struct in6_addr bip6;

       addr = pton_guess_family(start, &family);
		if(family == NFPROTO_IPV4)
		{
			if(addr != NULL)
			{
				int mask_valid = 0;
				uint32_t mask;
				bip = *((struct in_addr*)addr);
				kfree(addr);
				if(strchr(end, '.') != NULL)
				{
					addr = pton_guess_family(end, &family);

					if(addr != NULL)
					{
						mask = (uint32_t)(((struct in_addr*)addr)->s_addr);
						mask_valid = 1;
						kfree(addr);
					}
				}
				else
				{
					int mask_bits;
					if(sscanf(end, "%d", &mask_bits) > 0)
					{
						if(mask_bits >=0 && mask_bits <= 32)
						{
							mask = 0;
							mask = htonl(0xFFFFFFFF << (32 - mask_bits));
							mask_valid = 1;
						}
					}
				}
				if(mask_valid)
				{
					priv->local_subnet.s_addr = ( ((uint32_t)bip.s_addr) & mask );
					priv->local_subnet_mask.s_addr = mask;
				}
			}
		}
		else
		{
			if(addr != NULL)
			{
				int mask_valid = 0;
				struct in6_addr mask_add;
				bip6 = *((struct in6_addr*)addr);
				kfree(addr);
				if(strchr(end, ':') != NULL)
				{
					addr = pton_guess_family(end, &family);
					if(addr != NULL)
					{
						mask_add = *((struct in6_addr*)addr);
						kfree(addr);
						mask_valid = 1;
					}
				}
				else
				{
					int mask_bits;
					if(sscanf(end, "%d", &mask_bits) > 0)
					{
						if(mask_bits >=0 && mask_bits <= 128)
						{
							char* p = (void *)&mask_add;
							memset(p, 0xff, mask_bits/8);
							memset(p + ((mask_bits+7)/8), 0, (128-mask_bits)/8);
							if(mask_bits < 128)
							{
								p[mask_bits/8] = 0xff << (8-(mask_bits & 7));
							}
							mask_valid = 1;
						}
					}
				}
				if(mask_valid)
				{
					priv->local_subnet6 = bip6;
					priv->local_subnet6_mask = mask_add;
					for(unsigned int x = 0; x < 16; x++)
					{
						priv->local_subnet6.s6_addr[x] = ( priv->local_subnet6.s6_addr[x] & mask_add.s6_addr[x] );
					}
				}
			}
		}

		free(start);
		free(end);	
		free(range_parts);
	}
}

static int nft_bandwidth_init(const struct nft_ctx *ctx, const struct nft_expr *expr, const struct nlattr * const tb[]) {
	struct nft_bandwidth_info *priv = nft_expr_priv(expr);
	int valid_arg = 0;
	uint8_t family = ctx->family;

	#ifdef BANDWIDTH_DEBUG
		printk("nft_bandwidth_init called\n");	
	#endif
	
	if (tb[NFTA_BANDWIDTH_ID] == NULL)
		return -EINVAL;

	if(priv->ref_count == NULL) /* first instance, we're inserting rule */
	{
		char* subnet = kcalloc(BANDWIDTH_SUBNET_STR_SIZE,sizeof(char),GFP_ATOMIC);
		char* subnet6 = kcalloc(BANDWIDTH_SUBNET_STR_SIZE,sizeof(char),GFP_ATOMIC);
		struct nft_bandwidth_info *master_priv = (struct nft_bandwidth_info*)kmalloc(sizeof(struct nft_bandwidth_info), GFP_ATOMIC);
		priv->ref_count = (unsigned long*)kmalloc(sizeof(unsigned long), GFP_ATOMIC);

		if(priv->ref_count == NULL || subnet == NULL || subnet6 == NULL) /* deal with kmalloc failure */
		{
			printk("nft_bandwidth: kmalloc failure in nft_bandwidth_init!\n");
			return -ENOMEM;
		}
		*(priv->ref_count) = 1;

		nla_strscpy(priv->id, tb[NFTA_BANDWIDTH_ID], BANDWIDTH_MAX_ID_LENGTH);
		priv->type = nla_get_u8(tb[NFTA_BANDWIDTH_TYPE]);
		priv->check_type = nla_get_u8(tb[NFTA_BANDWIDTH_CHECKTYPE]);
		if(tb[NFTA_BANDWIDTH_SUBNET] != NULL) nla_strscpy(subnet, tb[NFTA_BANDWIDTH_SUBNET], BANDWIDTH_SUBNET_STR_SIZE);
		if(tb[NFTA_BANDWIDTH_SUBNET6] != NULL) nla_strscpy(subnet6, tb[NFTA_BANDWIDTH_SUBNET6], BANDWIDTH_SUBNET_STR_SIZE);
		memset(&priv->local_subnet, 0, sizeof(struct in_addr));
		memset(&priv->local_subnet_mask, 0, sizeof(struct in_addr));
		memset(&priv->local_subnet6, 0, sizeof(struct in6_addr));
		memset(&priv->local_subnet6_mask, 0, sizeof(struct in6_addr));
		parse_ips_and_ranges(subnet, priv);
		parse_ips_and_ranges(subnet6, priv);
		priv->cmp = nla_get_u8(tb[NFTA_BANDWIDTH_CMP]);
		priv->reset_is_constant_interval = nla_get_u8(tb[NFTA_BANDWIDTH_RSTINTVLCONST]);
		priv->reset_interval = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_RSTINTVL]));
		priv->reset_time = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_RSTTIME]));
		priv->bandwidth_cutoff = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_BWCUTOFF]));
		priv->current_bandwidth = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_CURRENTBW]));
		priv->next_reset = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_NEXTRESET]));
		priv->previous_reset = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_PREVRESET]));
		priv->last_backup_time = be64_to_cpu(nla_get_be64(tb[NFTA_BANDWIDTH_LASTBACKUPTIME]));
		priv->num_intervals_to_save = ntohl(nla_get_be32(tb[NFTA_BANDWIDTH_NUMINTVLSTOSAVE]));

		priv->non_const_self = master_priv;
		priv->hashed_id = sdbm_string_hash(priv->id);
		priv->iam = NULL;
		priv->combined_bw = NULL;

		memcpy(master_priv->id, priv->id, BANDWIDTH_MAX_ID_LENGTH);
		master_priv->type                       = priv->type;
		master_priv->check_type                 = priv->check_type;
		master_priv->local_subnet               = priv->local_subnet;
		master_priv->local_subnet_mask          = priv->local_subnet_mask;
		master_priv->local_subnet6               = priv->local_subnet6;
		master_priv->local_subnet6_mask          = priv->local_subnet6_mask;
		master_priv->cmp                        = priv->cmp;
		master_priv->reset_is_constant_interval = priv->reset_is_constant_interval;
		master_priv->reset_interval             = priv->reset_interval;
		master_priv->reset_time                 = priv->reset_time;
		master_priv->bandwidth_cutoff           = priv->bandwidth_cutoff;
		master_priv->current_bandwidth          = priv->current_bandwidth;
		master_priv->next_reset                 = priv->next_reset;
		master_priv->previous_reset             = priv->previous_reset;
		master_priv->last_backup_time           = priv->last_backup_time;
		master_priv->num_intervals_to_save      = priv->num_intervals_to_save;
		
		master_priv->hashed_id                  = priv->hashed_id;
		master_priv->iam                        = priv->iam;
		master_priv->combined_bw                = priv->combined_bw;
		master_priv->non_const_self             = priv->non_const_self;
		master_priv->ref_count                  = priv->ref_count;

		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(priv->ref_count) );
		#endif

		if(priv->cmp != BANDWIDTH_CHECK)
		{
			info_and_maps *iam;
		
			down(&userspace_lock);
			spin_lock_bh(&bandwidth_lock);

			iam = (info_and_maps*)get_string_map_element(id_map, priv->id);
			if(iam != NULL)
			{
				// Duplicate ID, we will allow this only if one references IPv4 and the other is IPv6
				#ifdef BANDWIDTH_DEBUG
					printk("iam is not null during nft_bandwidth_init!\n");
				#endif
				// We can only have 1 in each family NFPROTO_IPV4/IPV6. NFPROTO_INET conflicts with both of these
				// For NFPROTO_INET, we can have up to 2 of these
				if((family != NFPROTO_INET && (iam->info_family == NFPROTO_INET || iam->info_family == family)) || (family == NFPROTO_INET && iam->info_family != NFPROTO_INET) || (family == NFPROTO_INET && iam->ref_count > 1))
				{
					printk("nft_bandwidth: error, \"%s\" is a duplicate id in this IP family, OR, id referenced more than twice in INET\n", priv->id); 
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return -EINVAL;
				}
				
				#ifdef BANDWIDTH_DEBUG
					printk("not a duplicate, other protocol\n");
				#endif
				// Check that they are the exact same rule (except for some allowed differences)
				// Otherwise, we don't want to allow this
				if(iam->info->type != master_priv->type ||
					iam->info->check_type != master_priv->check_type ||
					iam->info->cmp != master_priv->cmp ||
					iam->info->reset_is_constant_interval != master_priv->reset_is_constant_interval ||
					iam->info->reset_interval != master_priv->reset_interval ||
					iam->info->reset_time != master_priv->reset_time ||
					iam->info->bandwidth_cutoff != master_priv->bandwidth_cutoff ||
					iam->info->num_intervals_to_save != master_priv->num_intervals_to_save
				)
				{
					printk("nft_bandwidth: error, \"%s\" is already used in the other IP family, but this rule is not substantially the same\n", priv->id); 
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return -EINVAL;
				}
				
				iam->other_info = master_priv;
				iam->other_info_family = family;
				master_priv->combined_bw = iam->info->combined_bw;
				iam->ref_count += 1;
			}
			else
			{
				iam = (info_and_maps*)kmalloc( sizeof(info_and_maps), GFP_ATOMIC);
				if(iam == NULL) /* handle kmalloc failure */
				{
					printk("nft_bandwidth: kmalloc failure in nft_bandwidth_init!\n");
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return -ENOMEM;
				}
				iam->ip_map = initialize_string_map(1);
				if(iam->ip_map == NULL) /* handle kmalloc failure */
				{
					printk("nft_bandwidth: kmalloc failure in nft_bandwidth_init!\n");
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return -ENOMEM;
				}
				iam->ip_history_map = NULL;
				if(priv->num_intervals_to_save > 0)
				{
					iam->ip_history_map = initialize_string_map(1);
					if(iam->ip_history_map == NULL) /* handle kmalloc failure */
					{
						printk("nft_bandwidth: kmalloc failure in nft_bandwidth_init!\n");
						spin_unlock_bh(&bandwidth_lock);
						up(&userspace_lock);
						return -ENOMEM;
					}
				}
				iam->ip_family_map = initialize_string_map(1);
				if(iam->ip_family_map == NULL) /* handle kmalloc failure */
				{
					printk("nft_bandwidth: kmalloc failure in nft_bandwidth_init!\n");
					spin_unlock_bh(&bandwidth_lock);
					up(&userspace_lock);
					return -ENOMEM;
				}
				
				iam->info = master_priv;
				set_string_map_element(id_map, priv->id, iam);
				iam->info_family = family;
				iam->other_info = NULL;
				iam->other_info_family = 0;
				iam->ref_count = 1;
			}

			if(priv->reset_interval != BANDWIDTH_NEVER)
			{
				ktime_t now = ktime_get_real_seconds();
				if(now != last_local_mw_update )
				{
					check_for_timezone_shift(now, 1);
				}
				
				
				now = now -  (60 * local_minutes_west);  /* Adjust for local timezone */
				priv->previous_reset = now;
				master_priv->previous_reset = now;
				if(priv->next_reset == 0)
				{
					priv->next_reset = get_next_reset_time(priv, now, now);
					master_priv->next_reset = priv->next_reset;
					/* 
					 * if we specify last backup time, check that next reset is consistent, 
					 * otherwise reset current_bandwidth to 0 
					 * 
					 * only applies to combined type -- otherwise we need to handle setting bandwidth
					 * through userspace library
					 */
					if(priv->last_backup_time != 0 && priv->type == BANDWIDTH_COMBINED)
					{
						ktime_t adjusted_last_backup_time = priv->last_backup_time - (60 * local_minutes_west); 
						ktime_t next_reset_of_last_backup = get_next_reset_time(priv, adjusted_last_backup_time, adjusted_last_backup_time);
						if(next_reset_of_last_backup != priv->next_reset)
						{
							priv->current_bandwidth = 0;
							master_priv->current_bandwidth = 0;
						}
						priv->last_backup_time = 0;
						master_priv->last_backup_time = 0;
					}
				}
			}

			priv->iam = (void*)iam;
			master_priv->iam = (void*)iam;

			spin_unlock_bh(&bandwidth_lock);
			up(&userspace_lock);
		}
		kfree(subnet);
		kfree(subnet6);
	}
	else
	{
		/* priv->non_const_self = priv; */

		*(priv->ref_count) = *(priv->ref_count) + 1;
		#ifdef BANDWIDTH_DEBUG
			printk("   after increment, ref count = %ld\n", *(priv->ref_count) );
		#endif
		
		/*
		if(priv->cmp != BANDWIDTH_CHECK)
		{
			info_and_maps* iam;
			down(&userspace_lock);
			spin_lock_bh(&bandwidth_lock);
			iam = (info_and_maps*)get_string_map_element(id_map, priv->id);
			if(iam != NULL)
			{
				iam->info = priv;
			}
			spin_unlock_bh(&bandwidth_lock);
			up(&userspace_lock);
		}
		*/
	}
	
	#ifdef BANDWIDTH_DEBUG
		printk("nft_bandwidth_init complete\n");
	#endif
	valid_arg = 1;

	return (valid_arg ? 0 : -EINVAL);
}

static int nft_bandwidth_dump(struct sk_buff *skb, const struct nft_expr *expr) {
	const struct nft_bandwidth_info *rule_priv = nft_expr_priv(expr);
	struct nft_bandwidth_info *priv = rule_priv->non_const_self;
	int retval = 0;
	char* subnetstr;
	char* subnet6str;
	struct in6_addr* subnettest;
	int ret;

	subnetstr = kcalloc(BANDWIDTH_SUBNET_STR_SIZE,sizeof(char),GFP_ATOMIC);
	subnet6str = kcalloc(BANDWIDTH_SUBNET_STR_SIZE,sizeof(char),GFP_ATOMIC);
	if (subnetstr == NULL || subnet6str == NULL)
		return -1;
	subnettest = kcalloc(1,sizeof(struct in6_addr),GFP_ATOMIC);
	if (subnettest == NULL)
		return -1;

	if (nla_put_string(skb, NFTA_BANDWIDTH_ID, priv->id))
	{
		retval = -1;
	}
	if (nla_put_u8(skb, NFTA_BANDWIDTH_TYPE, priv->type))
	{
		retval = -1;
	}
	if (nla_put_u8(skb, NFTA_BANDWIDTH_CHECKTYPE, priv->check_type))
	{
		retval = -1;
	}

	ret = snprintf(subnetstr, BANDWIDTH_SUBNET_STR_SIZE, "%pI4/%pI4", &priv->local_subnet, &priv->local_subnet_mask);
	ret = snprintf(subnet6str, BANDWIDTH_SUBNET_STR_SIZE, "%pI6c/%pI6c", &priv->local_subnet6, &priv->local_subnet6_mask);

	if (nla_put_string(skb, NFTA_BANDWIDTH_SUBNET, subnetstr))
	{
       retval = -1;
	}
	if (nla_put_string(skb, NFTA_BANDWIDTH_SUBNET6, subnet6str))
	{
		retval = -1;
	}

	if (nla_put_u8(skb, NFTA_BANDWIDTH_CMP, priv->cmp))
	{
		retval = -1;
	}
	if (nla_put_u8(skb, NFTA_BANDWIDTH_RSTINTVLCONST, priv->reset_is_constant_interval))
	{
		retval = -1;
	}

	if (nla_put_be64(skb, NFTA_BANDWIDTH_RSTINTVL, cpu_to_be64(priv->reset_interval), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_RSTTIME, cpu_to_be64(priv->reset_time), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_BWCUTOFF, cpu_to_be64(priv->bandwidth_cutoff), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_CURRENTBW, cpu_to_be64(priv->current_bandwidth), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_NEXTRESET, cpu_to_be64(priv->next_reset), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_PREVRESET, cpu_to_be64(priv->previous_reset), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}
	if (nla_put_be64(skb, NFTA_BANDWIDTH_LASTBACKUPTIME, cpu_to_be64(priv->last_backup_time), NFTA_BANDWIDTH_PAD))
	{
		retval = -1;
	}

	if (nla_put_be32(skb, NFTA_BANDWIDTH_NUMINTVLSTOSAVE, htonl(priv->num_intervals_to_save)))
	{
		retval = -1;
	}

	kfree(subnetstr);
	kfree(subnet6str);
	kfree(subnettest);
	
	return retval;
}

static void nft_bandwidth_destroy(const struct nft_ctx *ctx, const struct nft_expr *expr) {
	struct nft_bandwidth_info *priv = nft_expr_priv(expr);
	struct nft_bandwidth_info *other_priv = NULL;
	uint8_t family = ctx->family;

	#ifdef BANDWIDTH_DEBUG
		printk("destroy called\n");
	#endif
	
	*(priv->ref_count) = *(priv->ref_count) - 1;
	
	#ifdef BANDWIDTH_DEBUG
		printk("   after decrement refcount = %ld\n", *(priv->ref_count));
	#endif
	
	if(*(priv->ref_count) == 0)
	{
		int destroying_primary = 0;
		info_and_maps* iam;
		down(&userspace_lock);
		spin_lock_bh(&bandwidth_lock);
		
		// Check if we need to preserve iam due to other protocol rule
		iam = (info_and_maps*)get_string_map_element(id_map, priv->id);
		if(iam != NULL)
		{
			if(iam->info_family == family)
			{
				#ifdef BANDWIDTH_DEBUG
					printk("priv is primary priv\n");
				#endif
				destroying_primary = 1;
			}
			else
			{
				#ifdef BANDWIDTH_DEBUG
					printk("priv is other priv\n");
				#endif
			}
			other_priv = iam->other_info;
			iam->ref_count -= 1;
		}
		
		if(other_priv != NULL)
		{
			if(destroying_primary == 1)
			{
				#ifdef BANDWIDTH_DEBUG
					printk("destroying primary, copying data from priv to other_priv\n");
				#endif
				//We need to copy priv to it
				//current_bandwidth, next_reset, previous_reset, last_backup_time, combined_bw
				other_priv->current_bandwidth          = priv->non_const_self->current_bandwidth;
				other_priv->next_reset                 = priv->non_const_self->next_reset;
				other_priv->previous_reset             = priv->non_const_self->previous_reset;
				other_priv->last_backup_time           = priv->non_const_self->last_backup_time;
				other_priv->combined_bw                = priv->non_const_self->combined_bw;
				
				iam->info = other_priv;
				iam->other_info = NULL;
				iam->info_family = iam->other_info_family;
				iam->other_info_family = 0;
			}
			else
			{
				#ifdef BANDWIDTH_DEBUG
					printk("destroying other_priv\n");
				#endif

				iam->other_info = NULL;
				iam->other_info_family = 0;
			}
		}
		else
		{
			iam = (info_and_maps*)remove_string_map_element(id_map, priv->id);
			if(iam != NULL && priv->cmp != BANDWIDTH_CHECK)
			{
				unsigned long num_destroyed;
				if(iam->ip_map != NULL && iam->ip_history_map != NULL && iam->ip_family_map != NULL)
				{
					unsigned long history_index = 0;
					bw_history** histories_to_free;
					
					destroy_string_map(iam->ip_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
					destroy_string_map(iam->ip_family_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
					
					histories_to_free = (bw_history**)destroy_string_map(iam->ip_history_map, DESTROY_MODE_RETURN_VALUES, &num_destroyed);
					
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
				else if(iam->ip_map != NULL && iam->ip_family_map != NULL)
				{
					destroy_string_map(iam->ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
					destroy_string_map(iam->ip_family_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
				}
				kfree(iam);
				/* priv portion of iam gets taken care of automatically */
			}
		}
		
		priv->combined_bw = NULL;
		kfree(priv->ref_count);
		kfree(priv->non_const_self);

		spin_unlock_bh(&bandwidth_lock);
		up(&userspace_lock);
	}
	
	#ifdef BANDWIDTH_DEBUG
		printk("destroy complete\n");
	#endif
}

static struct nf_sockopt_ops nft_bandwidth_sockopts = 
{
	.pf = PF_INET,
	.set_optmin = BANDWIDTH_SET,
	.set_optmax = BANDWIDTH_SET+1,
	.set = nft_bandwidth_set_ctl,
	.get_optmin = BANDWIDTH_GET,
	.get_optmax = BANDWIDTH_GET+1,
	.get = nft_bandwidth_get_ctl
};

static struct nft_expr_type nft_bandwidth_type;
static const struct nft_expr_ops nft_bandwidth_op = {
	.eval = nft_bandwidth_eval,
	.size = NFT_EXPR_SIZE(sizeof(struct nft_bandwidth_info)),
	.init = nft_bandwidth_init,
	.destroy = nft_bandwidth_destroy,
	.dump = nft_bandwidth_dump,
	.type = &nft_bandwidth_type,
};
static struct nft_expr_type nft_bandwidth_type __read_mostly =  {
	.ops = &nft_bandwidth_op,
	.name = "bandwidth",
	.owner = THIS_MODULE,
	.policy = nft_bandwidth_policy,
	.maxattr = NFTA_BANDWIDTH_MAX,
};

static int __init init(void)
{
	/* Register setsockopt */
	if (nf_register_sockopt(&nft_bandwidth_sockopts) < 0)
	{
		printk("nft_bandwidth: Can't register sockopts. Aborting\n");
	}
	bandwidth_record_max = get_bw_record_max();
	local_minutes_west = old_minutes_west = sys_tz.tz_minuteswest;
	local_seconds_west = local_minutes_west*60;
	last_local_mw_update = ktime_get_real_seconds();
	if(local_seconds_west > last_local_mw_update)
	{
		/* we can't let adjusted time be < 0 -- pretend timezone is still UTC */
		local_minutes_west = 0;
		local_seconds_west = 0;
	}

	id_map = initialize_string_map(0);
	if(id_map == NULL) /* deal with kmalloc failure */
	{
		printk("id map is null, returning -1\n");
		return -1;
	}

	return nft_register_expr(&nft_bandwidth_type);
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
			string_map* ip_map = iam->ip_map;
			unsigned long num_destroyed;
			destroy_string_map(ip_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			kfree(iam);
			/* info portion of iam gets taken care of automatically */
		}
	}
	nf_unregister_sockopt(&nft_bandwidth_sockopts);
	nft_unregister_expr(&nft_bandwidth_type);
	spin_unlock_bh(&bandwidth_lock);
	up(&userspace_lock);
}

module_init(init);
module_exit(fini);

