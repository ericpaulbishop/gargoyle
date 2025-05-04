/*  timerange --	An nftables extension to match multiple timeranges within a week
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

#include <linux/netfilter/nf_tables.h>
#include <net/netfilter/nf_tables.h>
#include <linux/netfilter/nft_timerange.h>
#include <linux/math64.h>
#include <linux/ktime.h>

#include <linux/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Gray");
MODULE_DESCRIPTION("Match time ranges, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS_NFT_EXPR("timerange");

extern struct timezone sys_tz;

#define TIMERANGE_TEXT_SIZE 1024
static const struct nla_policy nft_timerange_policy[NFTA_TIMERANGE_MAX + 1] = {
	[NFTA_TIMERANGE_FLAGS]			= { .type = NLA_U32 },
	[NFTA_TIMERANGE_HOURS]			= { .type = NLA_STRING, .len = TIMERANGE_TEXT_SIZE },
	[NFTA_TIMERANGE_WEEKDAYS]		= { .type = NLA_STRING, .len = TIMERANGE_TEXT_SIZE },
	[NFTA_TIMERANGE_WEEKLYRANGES]	= { .type = NLA_STRING, .len = TIMERANGE_TEXT_SIZE },
};

static bool timerange_mt(struct nft_timerange_info *priv, struct sk_buff *skb)
{
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

	/*printk("time=%d, since midnight = %d, day=%d, minuteswest=%d\n", stamp_time, seconds_since_midnight, weekday, sys_tz.tz_minuteswest);*/

	match_found = 0;
	if(priv->type == HOURS)
	{
		for(test_index=0; priv->ranges[test_index] != -1 && match_found == 0 && seconds_since_midnight >= priv->ranges[test_index]; test_index=test_index+2)
		{
			match_found = seconds_since_midnight >= priv->ranges[test_index] && seconds_since_midnight <= priv->ranges[test_index+1] ? 1 : match_found;
		}
	}
	else if(priv->type == WEEKDAYS)
	{
		match_found = priv->days[weekday];
	}
	else if(priv->type == DAYS_HOURS)
	{
		match_found = priv->days[weekday];
		if(match_found == 1)
		{
			match_found = 0;
			for(test_index=0; priv->ranges[test_index] != -1 && match_found == 0 && seconds_since_midnight >= priv->ranges[test_index]; test_index=test_index+2)
			{
				match_found = seconds_since_midnight >= priv->ranges[test_index] && seconds_since_midnight <= priv->ranges[test_index+1] ? 1 : match_found;
			}
		}
	}
	else if(priv->type == WEEKLY_RANGE)
	{
		ktime_t seconds_since_sunday_midnight = seconds_since_midnight + (weekday*86400);
		for(test_index=0; priv->ranges[test_index] != -1 && match_found == 0 && seconds_since_sunday_midnight >= priv->ranges[test_index]; test_index=test_index+2)
		{
			match_found = seconds_since_sunday_midnight >= priv->ranges[test_index] && seconds_since_sunday_midnight <= priv->ranges[test_index+1] ? 1 : match_found;
		}
		
	}
	
	match_found ^= priv->invert;
	return match_found;
}

static void nft_timerange_eval(const struct nft_expr *expr, struct nft_regs *regs, const struct nft_pktinfo *pkt) {
	struct nft_timerange_info *priv = nft_expr_priv(expr);
	struct sk_buff *skb = pkt->skb;
	if(!timerange_mt(priv, skb))
		regs->verdict.code = NFT_BREAK;
}

/* takes a string of days e.g. "Monday, Tuesday, Friday", and turns into an array of 7 longs
 * each 0 or 1, one for each weekday starting with sunday, e.g. [0,1,1,0,0,1,0] for our example 
 */
long* parse_weekdays(char* wd_str)
{
	long* weekdays = (long*)kmalloc(7*sizeof(long),GFP_ATOMIC);
	char** days = split_on_separators(wd_str, ",", 1, -1, 0);
	int day_index;
	int found = 0;
	weekdays[0] = weekdays[1] = weekdays[2] = weekdays[3] = weekdays[4] = weekdays[5] = weekdays[6] = 0;

	for(day_index=0; days[day_index] != NULL; day_index++)
	{
		char day[4];
		trim_flanking_whitespace(days[day_index]);
		memcpy(day, days[day_index], 3);
		kfree(days[day_index]);
		day[3] = '\0';
		to_lowercase(day);
		if(strcmp(day, "sun") == 0)
		{
			weekdays[0] = 1;
			found = 1;
		}
		else if(strcmp(day, "mon") ==0)
		{
			weekdays[1] = 1;
			found = 1;
		}
		else if(strcmp(day, "tue") ==0)
		{
			weekdays[2] = 1;
			found = 1;
		}
		else if(strcmp(day, "wed") ==0)
		{
			weekdays[3] = 1;
			found = 1;
		}	
		else if(strcmp(day, "thu") ==0)
		{
			weekdays[4] = 1;
			found = 1;
		}
		else if(strcmp(day, "fri") ==0)
		{
			weekdays[5] = 1;
			found = 1;
		}
		else if(strcmp(day, "sat") ==0)
		{
			weekdays[6] = 1;
			found = 1;
		}
		else if(strcmp(day, "all") ==0)
		{
			weekdays[0] = weekdays[1] = weekdays[2] = weekdays[3] = weekdays[4] = weekdays[5] = weekdays[6] = 1;
			found = 1;
		}
	}
	kfree(days);
	if(found == 0)
	{
		kfree(weekdays);
		weekdays = NULL;
	}
	return weekdays;	
}

/* is_weekly_range indicates whether we're parsing hours within a single day or a range over a whole week */
long* parse_time_ranges(char* time_ranges, unsigned char is_weekly_range)
{
	int num_pieces = 0;
	int range_index = 0;
	char overlap_found = 0;
	int piece_index = 0;
	int max_multiple = is_weekly_range ? 7 : 1;
	long *parsed = NULL;
	char** pieces = split_on_separators(time_ranges, ",", 1, -1, 0);
	int num_range_indices=0;
	long* adjusted_range = NULL;
	int ar_index = 0;
	int old_index = 0;
	for(num_pieces = 0; pieces[num_pieces] != NULL; num_pieces++) {};
	parsed = (long*)kcalloc((1+(num_pieces*2)),sizeof(long),GFP_ATOMIC);

	for(piece_index = 0; pieces[piece_index] != NULL; piece_index++)
	{
		char** times;
		int time_count = 0;
		trim_flanking_whitespace(pieces[piece_index]);
		times=split_on_separators(pieces[piece_index], "-", 1, 2, 0);
		for(time_count = 0; times[time_count] != 0 ; time_count++){}
		if( time_count == 2 )
		{
			unsigned long  start = parse_time(trim_flanking_whitespace(times[0]));
			unsigned long end = parse_time(trim_flanking_whitespace(times[1]));
			parsed[ piece_index*2 ] = (long)start;
			parsed[ (piece_index*2)+1 ] = (long)end;

			kfree( times[1] );
		}
		if( time_count > 0) { kfree(times[0]); }

		kfree(times);
		kfree(pieces[piece_index]);
	}
	kfree(pieces);
	parsed[ (num_pieces*2) ] = -1; // terminated with -1 

	// make sure there is no overlap -- this will invalidate ranges 
	for(range_index = 0; range_index < num_pieces; range_index++)
	{
		int range_index2 = 0;
		// now test for overlap 
		long start1 = parsed[ (range_index*2) ];
		long end1 = parsed[ (range_index*2)+1 ];
		end1= end1 < start1 ? end1 + (max_multiple*24*60*60) : end1;
		
		for(range_index2 = 0; range_index2 < num_pieces; range_index2++)
		{
			if(range_index2 != range_index)
			{
				long start2 = parsed[ (range_index2*2) ];
				long end2 = parsed[ (range_index2*2)+1 ];
				end2= end2 < start2 ? end2 + (max_multiple*24*60*60) : end2;
				overlap_found = overlap_found || (start1 < end2 && end1 > start2 );
			}
		}
	}

	if(!overlap_found)
	{
		// sort ranges 
		int sorted_index = 0;
		while(parsed[sorted_index] != -1)
		{
			int next_start=-1;
			int next_start_index=-1;
			int test_index;
			long tmp1;
			long tmp2;
			for(test_index=sorted_index; parsed[test_index] != -1; test_index=test_index+2)
			{
				next_start_index = next_start < 0 || next_start > parsed[test_index] ? test_index : next_start_index;
				next_start = next_start < 0 || next_start > parsed[test_index] ? parsed[test_index] : next_start;
			}
			tmp1 = parsed[next_start_index];
			tmp2 = parsed[next_start_index+1];
			if(tmp1 == tmp2)
			{
				// de-allocate parsed, set to NULL
				kfree(parsed);
				parsed = NULL;
				return parsed;
			}
			parsed[next_start_index] = parsed[sorted_index];
			parsed[next_start_index+1] = parsed[sorted_index+1];
			parsed[sorted_index] = 	tmp1;
			parsed[sorted_index+1] = tmp2;
			sorted_index = sorted_index + 2;
		}
	}
	else
	{
		// de-allocate parsed, set to NULL 
		kfree(parsed);
		parsed = NULL;
		return parsed;
	}

	// merge time ranges where end of first = start of second 
	merge_adjacent_time_ranges(parsed, is_weekly_range);

	// if always active, free & return NULL 
	if(parsed[0] == 0 && parsed[1] == max_multiple*24*60*60)
	{
		kfree(parsed);
		parsed = NULL;
		return parsed;
	}

	//adjust so any range that crosses end of range is split in two
	for(num_range_indices=0; parsed[num_range_indices] != -1; num_range_indices++){}
	adjusted_range = (long*)kcalloc((3+num_range_indices),sizeof(long),GFP_ATOMIC);
	if(parsed[num_range_indices-1] < parsed[0])
	{
		adjusted_range[0] = 0;
		adjusted_range[1] = parsed[num_range_indices-1];
		ar_index = ar_index + 2;
		parsed[num_range_indices-1] = -1;
	}
	for(old_index=0; parsed[old_index] != -1; old_index++)
	{
		adjusted_range[ar_index] = parsed[old_index];
		ar_index++;
	}

	if(ar_index % 2 == 1 )
	{
		adjusted_range[ar_index] = max_multiple*24*60*60;
		ar_index++;
	}
	adjusted_range[ar_index] = -1;
	kfree(parsed);

	return adjusted_range;
}

void merge_adjacent_time_ranges(long* time_ranges, unsigned char is_weekly_range)
{
	int range_length = 0;
	int* merged_indices;
	int merged_index=0;
	int next_index;
	while(time_ranges[range_length] != -1){ range_length++; }
	merged_indices = (int*)kmalloc((range_length+1)*sizeof(int),GFP_ATOMIC);
	
	for(next_index=0; time_ranges[next_index] != -1; next_index++)
	{
		if(next_index == 0)
		{
			merged_indices[merged_index] = next_index;
			merged_index++;
		}
		else if( time_ranges[next_index+1] == -1 )
		{
			merged_indices[merged_index] = next_index;
			merged_index++;
		}
		else if( time_ranges[next_index] != time_ranges[next_index-1] && time_ranges[next_index] != time_ranges[next_index+1] )
		{
			merged_indices[merged_index] = next_index;
			merged_index++;
		}
	}
	merged_indices[merged_index] = -1;

	for(next_index=0; merged_indices[next_index] != -1; next_index++)
	{
		time_ranges[next_index] = time_ranges[ merged_indices[next_index] ];
	}
	time_ranges[next_index] = -1;
	kfree(merged_indices);
}

/* 
 * assumes 24hr time, not am/pm, in format:
 * (Day of week) hours:minutes:seconds
 * if day of week is present, returns seconds since midnight on Sunday
 * otherwise, seconds since midnight
 */
unsigned long parse_time(char* time_str)
{
	int tp_index = 0;
	int weekday = -1;
	char** time_parts;
	unsigned long seconds = 0;
	unsigned long tmp = 0;
	unsigned long multiple = 60*60;
	while((*time_str == ' ' || *time_str == '\t') && *time_str != '\0') { time_str++; }

	if(strlen(time_str) > 3)
	{
		char wday_test[4];
		memcpy(wday_test, time_str, 3);
		wday_test[3] = '\0';
		to_lowercase(wday_test);
		if(strcmp(wday_test, "sun") == 0)
		{
			weekday = 0;
		}
		else if(strcmp(wday_test, "mon") == 0)
		{
			weekday = 1;
		}
		else if(strcmp(wday_test, "tue") == 0)
		{
			weekday = 2;
		}
		else if(strcmp(wday_test, "wed") == 0)
		{
			weekday = 3;
		}
		else if(strcmp(wday_test, "thu") == 0)
		{
			weekday = 4;
		}
		else if(strcmp(wday_test, "fri") == 0)
		{
			weekday = 5;
		}
		else if(strcmp(wday_test, "sat") == 0)
		{
			weekday = 6;
		}
	}

	if(weekday >= 0)
	{
		time_str = time_str + 3;
		while( (*time_str < 48 || *time_str > 57) && *time_str != '\0') { time_str++; }
	}

	time_parts=split_on_separators(time_str, ":", 1, -1, 0);
	seconds = weekday < 0 ? 0 : ( ((unsigned long)(weekday))*60*60*24 );

	for(tp_index=0; time_parts[tp_index] != NULL; tp_index++)
	{
		sscanf(time_parts[tp_index], "%lu", &tmp);
		seconds = seconds + (tmp*multiple);
		multiple = (unsigned long)(multiple/60);
		kfree(time_parts[tp_index]);
	}
	kfree(time_parts);

	return seconds;
}

void to_lowercase(char* str)
{
	int i;
	for(i = 0; str[i] != '\0'; i++)
	{
		str[i] = tolower(str[i]);
	}
}

/*
 * line_str is the line to be parsed -- it is not modified in any way
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
char** split_on_separators(char* line_str, char* separators, int num_separators, int max_pieces, int include_remainder_at_max)
{
	char** split;

	if(line_str != NULL)
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
			for(line_index = 0; line_str[line_index] != '\0'; line_index++)
			{
				int sep_index;
				int found = 0;
				for(sep_index =0; found == 0 && sep_index < num_separators; sep_index++)
				{
					found = separators[sep_index] == line_str[line_index] ? 1 : 0;
				}
				separator_count = separator_count+ found;
			}
			max_pieces = separator_count + 1;
		}
		split = (char**)kmalloc((1+max_pieces)*sizeof(char*),GFP_ATOMIC);
		split_index = 0;
		split[split_index] = NULL;

		dup_line = kstrdup(line_str,GFP_ATOMIC);
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
					next_piece = (char*)kmalloc((first_separator_index+1)*sizeof(char),GFP_ATOMIC);
					memcpy(next_piece, start, first_separator_index);
					next_piece[first_separator_index] = '\0';
				}
				else
				{
					next_piece = kstrdup(start,GFP_ATOMIC);
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
		kfree(dup_line);
	}
	else
	{
		split = (char**)kmalloc((1)*sizeof(char*),GFP_ATOMIC);
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

static int nft_timerange_init(const struct nft_ctx *ctx, const struct nft_expr *expr, const struct nlattr * const tb[])
{
	struct nft_timerange_info *priv = nft_expr_priv(expr);
	char *hours, *weekdays, *weeklyranges;
	bool invert = false;
	int valid_arg = 0;
	long* parsed = NULL;
	unsigned int flags = 0;

	if (tb[NFTA_TIMERANGE_HOURS] == NULL && tb[NFTA_TIMERANGE_WEEKDAYS] == NULL && tb[NFTA_TIMERANGE_WEEKLYRANGES] == NULL)
		return -EINVAL;

	hours = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	weekdays = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	weeklyranges = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	if (hours == NULL || weekdays == NULL || weeklyranges == NULL)
		return -EINVAL;

	if(tb[NFTA_TIMERANGE_FLAGS])
	{
		u32 flag = ntohl(nla_get_be32(tb[NFTA_TIMERANGE_FLAGS]));
		if(flag & ~NFT_TIMERANGE_F_INV)
			return -EOPNOTSUPP;

		if(flag & NFT_TIMERANGE_F_INV)
			invert = true;
	}
	if(tb[NFTA_TIMERANGE_HOURS] != NULL) nla_strscpy(hours, tb[NFTA_TIMERANGE_HOURS], TIMERANGE_TEXT_SIZE);
	if(tb[NFTA_TIMERANGE_WEEKDAYS] != NULL) nla_strscpy(weekdays, tb[NFTA_TIMERANGE_WEEKDAYS], TIMERANGE_TEXT_SIZE);
	if(tb[NFTA_TIMERANGE_WEEKLYRANGES] != NULL) nla_strscpy(weeklyranges, tb[NFTA_TIMERANGE_WEEKLYRANGES], TIMERANGE_TEXT_SIZE);

	priv->invert = invert;

	if(strlen(hours) > 0)
	{
		parsed = parse_time_ranges(hours, 0);
		if(parsed != NULL && (flags & HOURS) == 0 && (flags & WEEKLY_RANGE) == 0)
		{
			int range_index = 0;
			for(range_index = 0; parsed[range_index] != -1; range_index++)
			{
				if(range_index > 100)
				{
					valid_arg = 0;
					goto PARSE_OUT;
				}
				priv->ranges[range_index] = parsed[range_index];
			}
			priv->ranges[range_index] = -1;
			kfree(parsed);


			valid_arg = 1;
			flags = flags+ HOURS;
			priv->type = flags;
		}
	}

	if(strlen(weekdays) > 0)
	{
		parsed = parse_weekdays(weekdays);
		if(parsed != NULL && (flags & WEEKDAYS) == 0 && (flags & WEEKLY_RANGE) == 0)
		{
			int day_index;
			for(day_index=0; day_index < 7; day_index++)
			{
				priv->days[day_index] = parsed[day_index];
			}
			kfree(parsed);

			valid_arg = 1 ;
			flags = flags + WEEKDAYS;
			priv->type = flags;
		}
	}

	if(strlen(weeklyranges) > 0)
	{
		parsed = parse_time_ranges(weeklyranges, 1);
		if(parsed != NULL && (flags & HOURS) == 0 && (flags & WEEKDAYS) == 0 && (flags & WEEKLY_RANGE) == 0 )
		{
			int range_index = 0;
			for(range_index = 0; parsed[range_index] != -1; range_index++)
			{
				if(range_index > 100)
				{
					valid_arg = 0;
					goto PARSE_OUT;
				}
				priv->ranges[range_index] = parsed[range_index];
			}
			priv->ranges[range_index] = -1;
			kfree(parsed);

			valid_arg = 1;
			flags = flags+WEEKLY_RANGE;
			priv->type = flags;
		}
	}

PARSE_OUT:
	kfree(hours);
	kfree(weekdays);
	kfree(weeklyranges);

	return (valid_arg ? 0 : -EINVAL);
}

static int nft_timerange_dump(struct sk_buff *skb, const struct nft_expr *expr)
{
	const struct nft_timerange_info *priv = nft_expr_priv(expr);
	int i;
	int retval = 0;
	char* cur;
	char* end;
	char *hours, *weekdays, *weeklyranges;
	u32 flags = priv->invert ? NFT_TIMERANGE_F_INV : 0;
	hours = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	weekdays = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	weeklyranges = kcalloc(TIMERANGE_TEXT_SIZE,sizeof(char),GFP_ATOMIC);

	switch(priv->type)
	{
		case DAYS_HOURS:
		case HOURS:
			cur = hours;
			end = cur+TIMERANGE_TEXT_SIZE;
			for(i=0; priv->ranges[i] != -1; i++)
			{
				cur += snprintf(cur, end-cur, "%ld", priv->ranges[i]);
				if(priv->ranges[i+1] != -1)
					cur += snprintf(cur, end-cur, "%s", (i % 2 == 0 ? "-" : ","));
			}
			if(priv->type == HOURS)
				break;
           fallthrough;
		case WEEKDAYS:
			cur = weekdays;
			end = cur+TIMERANGE_TEXT_SIZE;
			for(i=0; i<7; i++)
			{
				cur += snprintf(cur, end-cur, "%d", priv->days[i]);
				if(i != 6)
					cur += snprintf(cur, end-cur, "%s", ",");
			}
			break;
		case WEEKLY_RANGE:
			cur = weeklyranges;
			end = cur+TIMERANGE_TEXT_SIZE;
			for(i=0; priv->ranges[i] != -1; i++)
			{
				cur += snprintf(cur, end-cur, "%ld", priv->ranges[i]);
				if(priv->ranges[i+1] != -1)
					cur += snprintf(cur, end-cur, "%s", (i % 2 == 0 ? "-" : ","));
			}
			break;
	}

	if (nla_put_be32(skb, NFTA_TIMERANGE_FLAGS, htonl(flags)))
	{
		retval = -1;
		goto DUMP_OUT;
	}
	if (nla_put_string(skb, NFTA_TIMERANGE_HOURS, hours))
	{
		retval = -1;
		goto DUMP_OUT;
	}
	if (nla_put_string(skb, NFTA_TIMERANGE_WEEKDAYS, weekdays))
	{
		retval = -1;
		goto DUMP_OUT;
	}
	if (nla_put_string(skb, NFTA_TIMERANGE_WEEKLYRANGES, weeklyranges))
	{
		retval = -1;
		goto DUMP_OUT;
	}

DUMP_OUT:
	kfree(hours);
	kfree(weekdays);
	kfree(weeklyranges);

	return retval;
}

static struct nft_expr_type nft_timerange_type;
static const struct nft_expr_ops nft_timerange_op = {
	.eval = nft_timerange_eval,
	.size = NFT_EXPR_SIZE(sizeof(struct nft_timerange_info)),
	.init = nft_timerange_init,
	.dump = nft_timerange_dump,
	.type = &nft_timerange_type,
};
static struct nft_expr_type nft_timerange_type __read_mostly =  {
	.ops = &nft_timerange_op,
	.name = "timerange",
	.owner = THIS_MODULE,
	.policy = nft_timerange_policy,
	.maxattr = NFTA_TIMERANGE_MAX,
};

static int __init init(void)
{
	return nft_register_expr(&nft_timerange_type);
}

static void __exit fini(void)
{
	nft_unregister_expr(&nft_timerange_type);
}

module_init(init);
module_exit(fini);
