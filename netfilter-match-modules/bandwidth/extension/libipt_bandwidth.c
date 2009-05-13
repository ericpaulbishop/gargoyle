/* 
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


#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

//in iptables 1.4.0 and higher, iptables.h includes xtables.h, which
//we can use to check whether we need to deal with the new requirements
//in pre-processor directives below
#include <iptables.h>  
#include <linux/netfilter_ipv4/ipt_bandwidth.h>





/* Function which prints out usage message. */
static void help(void)
{
	printf(	"bandwidth options:\n  --greater_than [BYTES]\n  --less_than [BYTES]\n  --current_bandwidth [BYTES]\n  --reset_interval [minute|hour|day|week|month]\n  --last_backup_time [UTC SECONDS SINCE 1970]\n");
}

static struct option opts[] = 
{
	{ .name = "less_than", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_LT },	
	{ .name = "greater_than", 	.has_arg = 1, .flag = 0, .val = BANDWIDTH_GT },
	{ .name = "current_bandwidth",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_CURRENT },	
	{ .name = "reset_interval",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET },
	{ .name = "last_backup_time",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_LAST_BACKUP},
	{ .name = 0 }
};


/* Function which parses command options; returns true if it
   ate an option */
static int parse(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
#ifdef _XTABLES_H
			const void *entry,
#else
			const struct ipt_entry *entry,
			unsigned int *nfcache,
#endif			
			struct ipt_entry_match **match
			)
{
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)(*match)->data;
	int valid_arg = 0;
	int num_read;
	u_int64_t read_64;
	time_t read_time;
	
	int inc_flags = 1;
	
	switch (c)
	{
		case BANDWIDTH_LT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 && *flags % BANDWIDTH_CURRENT == 0)
			{
				info->gt_lt = BANDWIDTH_LT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_GT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 && *flags % BANDWIDTH_CURRENT == 0)
			{
				info->gt_lt = BANDWIDTH_GT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_CURRENT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 )
			{
				info->current_bandwidth = read_64;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_RESET:
			valid_arg = 1;
			*flags = *flags + BANDWIDTH_RESET;
			if(strcmp(argv[optind-1],"minute") ==0)
			{
				info->reset_interval = BANDWIDTH_MINUTE;
			}
			else if(strcmp(argv[optind-1],"hour") ==0)
			{
				info->reset_interval = BANDWIDTH_HOUR;
			}
			else if(strcmp(argv[optind-1],"day") ==0)
			{
				info->reset_interval = BANDWIDTH_DAY;
			}
			else if(strcmp(argv[optind-1],"week") ==0)
			{
				info->reset_interval = BANDWIDTH_WEEK;
			}
			else if(strcmp(argv[optind-1],"month") ==0)
			{
				info->reset_interval = BANDWIDTH_MONTH;
			}
			else if(strcmp(argv[optind-1],"never") ==0)
			{
				info->reset_interval = BANDWIDTH_NEVER;
			}
			else
			{
				inc_flags = 0;
				valid_arg = 0;
			}
			break;
		case BANDWIDTH_LAST_BACKUP:
			num_read = sscanf(argv[optind-1], "%ld", &read_time);
			if(num_read > 0 )
			{
				info->last_backup_time = read_time;
				valid_arg = 1;
			}
			valid_arg = 1;
			break;
	}


	if(inc_flags)
	{
		*flags = *flags + (unsigned int)c;
	}
	if(!(*flags & CURRENT_BANDWIDTH ))
	{
		info->current_bandwidth = 0;
	}
	if(!(*flags & BANDWIDTH_RESET))
	{
		info->reset_interval = BANDWIDTH_NEVER;
	}
	if(!(*flags & BANDWIDTH_LAST_BACKUP))
	{
		info->last_backup_time = 0;
	}
	info->next_reset = 0;

	return valid_arg;
}


	
static void print_bandwidth_args(	struct ipt_bandwidth_info* info )
{
	/* determine current time in seconds since epoch, with offset for current timezone */
	time_t now;
	struct tm* utc_info;
	struct tm* tz_info;
	int utc_day;
	int utc_hour;
	int utc_minute;
	int tz_day;
	int tz_hour;
	int tz_minute;
	int minuteswest;

	time(&now);
	utc_info = gmtime(&now);
	utc_day = utc_info->tm_mday;
	utc_hour = utc_info->tm_hour;
	utc_minute = utc_info->tm_min;
	tz_info = localtime(&now);
	tz_day = tz_info->tm_mday;
	tz_hour = tz_info->tm_hour;
	tz_minute = tz_info->tm_min;

	utc_day = utc_day < tz_day  - 1 ? tz_day  + 1 : utc_day;
	tz_day =  tz_day  < utc_day - 1 ? utc_day + 1 : tz_day;
	
	minuteswest = (24*60*utc_day + 60*utc_hour + utc_minute) - (24*60*tz_day + 60*tz_hour + tz_minute) ;

	now = now - (minuteswest*60);

	if(info->gt_lt == BANDWIDTH_GT)
	{
		printf(" --greater_than %lld ", info->bandwidth_cutoff);
	}
	if(info->gt_lt == BANDWIDTH_LT)
	{
		printf(" --less_than %lld ", info->bandwidth_cutoff);
	}
	if( info->reset_interval != BANDWIDTH_NEVER && info->next_reset != 0 && info->next_reset < now)
	{
		/* 
		 * current bandwidth only gets reset when first packet after reset interval arrives, so output
		 * zero if we're already past interval, but no packets have arrived 
		 */
		printf(" --current_bandwidth 0 ");
	}
	else
	{
		printf(" --current_bandwidth %lld ", info->current_bandwidth);
	}
	if(info->reset_interval == BANDWIDTH_MINUTE)
	{
		printf(" --reset_interval minute ");
	}
	else if(info->reset_interval == BANDWIDTH_HOUR)
	{
		printf(" --reset_interval hour ");
	}
	else if(info->reset_interval == BANDWIDTH_DAY)
	{
		printf(" --reset_interval day ");
	}
	else if(info->reset_interval == BANDWIDTH_WEEK)
	{
		printf(" --reset_interval week ");
	}
	else if(info->reset_interval == BANDWIDTH_MONTH)
	{
		printf(" --reset_interval month ");
	}
	
	/*
	if(info->reset_interval != BANDWIDTH_NEVER)
	{
		printf("now=%ld, reset=%ld\n", now, info->next_reset);
	}
	*/	
}

/* Final check; must have specified a test string with either --contains or --contains_regex. */
static void final_check(unsigned int flags)
{
	if(flags % BANDWIDTH_CURRENT != BANDWIDTH_LT && flags % BANDWIDTH_CURRENT != BANDWIDTH_GT )
	{
		exit_error(PARAMETER_PROBLEM, "You must specify '--greater_than' or '--less_than' '");
	}
}

/* Prints out the matchinfo. */
#ifdef _XTABLES_H
static void print(const void *ip, const struct xt_entry_match *match, int numeric)
#else	
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
#endif
{
	printf("bandwidth ");
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)match->data;

	print_bandwidth_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
#ifdef _XTABLES_H
static void save(const void *ip, const struct xt_entry_match *match)
#else
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
#endif
{
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)match->data;
	time_t now;
	
	print_bandwidth_args(info);
	
	time(&now);
	printf("--last_backup-time %ld ", now);
}

static struct iptables_match bandwidth = 
{ 
	.next		= NULL,
 	.name		= "bandwidth",
	.version	= IPTABLES_VERSION,
	.size		= IPT_ALIGN(sizeof(struct ipt_bandwidth_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_bandwidth_info)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&bandwidth);
}
