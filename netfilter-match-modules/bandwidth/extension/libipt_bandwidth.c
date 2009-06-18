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
#include <limits.h>

//in iptables 1.4.0 and higher, iptables.h includes xtables.h, which
//we can use to check whether we need to deal with the new requirements
//in pre-processor directives below
#include <iptables.h>  
#include <linux/netfilter_ipv4/ipt_bandwidth.h>





/* Function which prints out usage message. */
static void help(void)
{
	printf(	"bandwidth options:\n  --id [unique identifier for querying bandwidth]\n  --type [combined|individual_src|individual_dst|individual_local|individual_remote]\n  --subnet [a.b.c.d/mask] (0 < mask < 32)\n  --greater_than [BYTES]\n  --less_than [BYTES]\n  --current_bandwidth [BYTES]\n  --reset_interval [minute|hour|day|week|month]\n  --reset_time [OFFSET IN SECONDS]\n  --last_backup_time [UTC SECONDS SINCE 1970]\n");
}

static struct option opts[] = 
{
	{ .name = "id", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_ID },	
	{ .name = "type", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_TYPE },	
	{ .name = "subnet", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_SUBNET },	
	{ .name = "less_than", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_LT },	
	{ .name = "greater_than", 	.has_arg = 1, .flag = 0, .val = BANDWIDTH_GT },
	{ .name = "current_bandwidth",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_CURRENT },	
	{ .name = "reset_interval",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET_INTERVAL },
	{ .name = "reset_time",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET_TIME },
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
	uint64_t read_64;
	time_t read_time;
	unsigned int read,A,B,C,D,mask;

	/* set defaults first time we get here */
	if(*flags == 0)
	{
		/* generate random id */
		srand ( time(NULL) );
		unsigned long id_num = rand();
		sprintf(info->id, "%lu", id_num);

		info->type = BANDWIDTH_COMBINED;
		info->local_subnet = 0;
		info->local_subnet_mask = 0;
		info->current_bandwidth = 0;
		info->reset_interval = BANDWIDTH_NEVER;
		info->reset_time=0;
		info->last_backup_time = 0;
		info->next_reset = 0;

		*flags = *flags + BANDWIDTH_INITIALIZED;
	}

	switch (c)
	{
		case BANDWIDTH_ID:
			if(strlen(optarg) < BANDWIDTH_MAX_ID_LENGTH)
			{
				sprintf(info->id, "%s", optarg);
				valid_arg = 1;
			}
			c=0;
			break;
		case BANDWIDTH_TYPE:
			valid_arg = 1;
			if(strcmp(optarg, "combined") == 0)
			{
				info->type = BANDWIDTH_COMBINED;
			}
			else if(strcmp(optarg, "individual_src") == 0)
			{
				info->type = BANDWIDTH_INDIVIDUAL_SRC;
			}
			else if(strcmp(optarg, "individual_dst") == 0)
			{
				info->type = BANDWIDTH_INDIVIDUAL_DST;
			}
			else if(strcmp(optarg, "individual_local") == 0)
			{
				info->type = BANDWIDTH_INDIVIDUAL_LOCAL;
				*flags = *flags + BANDWIDTH_REQUIRES_SUBNET;
			}
			else if(strcmp(optarg, "individual_remote") == 0)
			{
				info->type = BANDWIDTH_INDIVIDUAL_REMOTE;
				*flags = *flags + BANDWIDTH_REQUIRES_SUBNET;
			}
			else
			{
				valid_arg = 0;
			}

			c=0;
			break;
		case BANDWIDTH_SUBNET:
			read = sscanf(optarg, "%u.%u.%u.%u/%u", &A, &B, &C, &D, &mask);
			if(read == 5)
			{
				if( A <= 255 && B <= 255 && C <= 255 && D <= 255 && mask <= 32)
				{
					int pow_index;
					unsigned char* sub = (unsigned char*)(&(info->local_subnet));
					*( sub ) = (unsigned char)A;
					*( sub + 1 ) = (unsigned char)B;
					*( sub + 2 ) = (unsigned char)C;
					*( sub + 3 ) = (unsigned char)D;

					unsigned char* msk = (unsigned char*)(&(info->local_subnet_mask));
					int msk_index;
					for(msk_index=0; msk_index*8 < mask; msk_index++)
					{
						int bit_index;
						msk[msk_index] = 0;
						for(bit_index=0; msk_index*8 + bit_index < mask && bit_index < 8; bit_index++)
						{
							msk[msk_index] = msk[msk_index] + get_pow(2, 7-bit_index);
						}
					}

					info->local_subnet = (info->local_subnet & info->local_subnet_mask );
					valid_arg = 1;
				}
			}
			break;
		case BANDWIDTH_LT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 && (*flags & BANDWIDTH_LT) == 0 && (*flags & BANDWIDTH_GT) == 0)
			{
				info->cmp = BANDWIDTH_LT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP; //only need one flag for less_than/greater_than
			break;
		case BANDWIDTH_GT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 && (*flags & BANDWIDTH_LT) == 0 && (*flags & BANDWIDTH_GT) == 0)
			{
				info->cmp = BANDWIDTH_GT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP; //only need one flag for less_than/greater_than
			break;
		case BANDWIDTH_CURRENT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 )
			{
				info->current_bandwidth = read_64;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_RESET_INTERVAL:
			valid_arg = 1;
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
				valid_arg = 0;
			}
			break;
		case BANDWIDTH_RESET_TIME:
			num_read = sscanf(argv[optind-1], "%ld", &read_time);
			if(num_read > 0 )
			{
				info->reset_time = read_time;
				valid_arg = 1;
			}	
			break;
		case BANDWIDTH_LAST_BACKUP:
			num_read = sscanf(argv[optind-1], "%ld", &read_time);
			if(num_read > 0 )
			{
				info->last_backup_time = read_time;
				valid_arg = 1;
			}
			break;
	}
	*flags = *flags + (unsigned int)c;


	//if we have both reset_interval & reset_time, check reset_time is in valid range
	if((*flags & BANDWIDTH_RESET_TIME) == BANDWIDTH_RESET_TIME && (*flags & BANDWIDTH_RESET_INTERVAL) == BANDWIDTH_RESET_INTERVAL)
	{
		if(	(info->reset_interval == BANDWIDTH_NEVER) ||
			(info->reset_interval == BANDWIDTH_MONTH && info->reset_time >= 60*60*24*28) ||
			(info->reset_interval == BANDWIDTH_WEEK && info->reset_time >= 60*60*24*7) ||
			(info->reset_interval == BANDWIDTH_DAY && info->reset_time >= 60*60*24) ||
			(info->reset_interval == BANDWIDTH_HOUR && info->reset_time >= 60*60) ||
			(info->reset_interval == BANDWIDTH_MINUTE && info->reset_time >= 60) 
		  )
		{
			valid_arg = 0;
		}
	}


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

	if(info->cmp == BANDWIDTH_GT)
	{
		printf(" --greater_than %lld ", info->bandwidth_cutoff);
	}
	if(info->cmp == BANDWIDTH_LT)
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
	if(info->reset_time > 0)
	{
		printf(" --reset_time %ld ", info->reset_time);
	}	
	/*
	if(info->reset_interval != BANDWIDTH_NEVER)
	{
		printf("now=%ld, reset=%ld\n", now, info->next_reset);
	}
	*/	
}

/* 
 * Final check, we can't have reset_time without reset_interval
 */
static void final_check(unsigned int flags)
{
	if( (flags & BANDWIDTH_RESET_INTERVAL) == 0 && (flags & BANDWIDTH_RESET_TIME) != 0)
	{
		exit_error(PARAMETER_PROBLEM, "You may not specify '--reset_time' without '--reset_interval' ");
	}
	if( (flags & BANDWIDTH_REQUIRES_SUBNET) == BANDWIDTH_REQUIRES_SUBNET && (flags & BANDWIDTH_SUBNET) == 0 )
	{
		exit_error(PARAMETER_PROBLEM, "You must specify a subnet (--subnet a.b.c.d/mask) to match individual local/remote IPs ");
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
