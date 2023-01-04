/*  bandwidth --	An xtables extension for bandwidth monitoring/control
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


#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#include <arpa/inet.h>

#ifndef ktime_t
#define ktime_t time_t
#endif

#include <xtables.h>
#include <linux/netfilter.h>
#include <linux/netfilter/xt_bandwidth.h>

typedef union
{
	struct in_addr ip4;
	struct in6_addr ip6;
} ipany;

int get_minutes_west(void);
void set_kernel_timezone(void);
int parse_sub(char* subnet_string, ipany* subnet, ipany* subnet_mask, int family);
static void param_problem_exit_error(char* msg);
char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
char* trim_flanking_whitespace(char* str);


/* Function which prints out usage message. */
static void help(void)
{
	printf("bandwidth options:\n");
	printf("  --id [unique identifier for querying bandwidth]\n");
	printf("  --type [combined|individual_src|individual_dst|individual_local|individual_remote]\n");
	printf("  --subnet [a.b.c.d/mask] (0 < mask < 32) OR [a:b:c:d:e:f:g:h/mask] (0 < mask < 128)\n");
	printf("  --greater_than [BYTES]\n");
	printf("  --less_than [BYTES]\n");
	printf("  --current_bandwidth [BYTES]\n");
	printf("  --reset_interval [minute|hour|day|week|month]\n");
	printf("  --reset_time [OFFSET IN SECONDS]\n");
	printf("  --intervals_to_save [NUMBER OF PREVIOUS INTERVALS TO STORE IN MEMORY]\n");
	printf("  --last_backup_time [UTC SECONDS SINCE 1970]\n");
	printf("  --bcheck Check another bandwidth rule without incrementing it\n");
	printf("  --bcheck_with_src_dst_swap Check another bandwidth rule without incrementing it, swapping src & dst ips for check\n");
}

static struct option opts[] = 
{
	{ .name = "id", 			.has_arg = 1, .flag = 0, .val = BANDWIDTH_ID },	
	{ .name = "type", 			.has_arg = 1, .flag = 0, .val = BANDWIDTH_TYPE },	
	{ .name = "subnet", 			.has_arg = 1, .flag = 0, .val = BANDWIDTH_SUBNET },	
	{ .name = "greater_than", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_GT },
	{ .name = "less_than", 			.has_arg = 1, .flag = 0, .val = BANDWIDTH_LT },	
	{ .name = "current_bandwidth",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_CURRENT },	
	{ .name = "reset_interval",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET_INTERVAL },
	{ .name = "reset_time",			.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET_TIME },
	{ .name = "intervals_to_save",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_NUM_INTERVALS },
	{ .name = "last_backup_time",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_LAST_BACKUP},
	{ .name = "bcheck",	 		.has_arg = 0, .flag = 0, .val = BANDWIDTH_CHECK_NOSWAP },
	{ .name = "bcheck_with_src_dst_swap",	.has_arg = 0, .flag = 0, .val = BANDWIDTH_CHECK_SWAP },
	{ .name = 0 }
};


/* Function which parses command options; returns true if it
   ate an option */
static int parse(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
			const void *entry,		
			struct xt_entry_match **match,
			int family
			)
{
	struct xt_bandwidth_info *info = (struct xt_bandwidth_info *)(*match)->data;
	int valid_arg = 0;
	long int num_read;
	uint64_t read_64;
	time_t read_time;

	/* set defaults first time we get here */
	if(*flags == 0)
	{
		/* generate random id */
		srand ( time(NULL) );
		unsigned long id_num = rand();
		sprintf(info->id, "%lu", id_num);

		info->type = BANDWIDTH_COMBINED;
		info->check_type = BANDWIDTH_CHECK_NOSWAP;
		memset(&info->local_subnet, 0, sizeof(info->local_subnet));
		memset(&info->local_subnet_mask, 0, sizeof(info->local_subnet_mask));
		info->cmp = BANDWIDTH_MONITOR; /* don't test greater/less than, just monitor bandwidth */
		info->current_bandwidth = 0;
		info->reset_is_constant_interval = 0;
		info->reset_interval = BANDWIDTH_NEVER;
		info->reset_time=0;
		info->last_backup_time = 0;
		info->next_reset = 0;
		
		info->num_intervals_to_save=0;

		info->non_const_self = NULL;
		info->ref_count = NULL;

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
			if(family == NFPROTO_IPV4)
			{
				valid_arg =  parse_sub(optarg, &(info->local_subnet), &(info->local_subnet_mask), NFPROTO_IPV4);
			}
			else
			{
				valid_arg =  parse_sub(optarg, &(info->local_subnet), &(info->local_subnet_mask), NFPROTO_IPV6);
			}
			break;
		case BANDWIDTH_LT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0 && (*flags & BANDWIDTH_CMP) == 0)
			{
				info->cmp = BANDWIDTH_LT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP; //only need one flag for less_than/greater_than
			break;
		case BANDWIDTH_GT:
			num_read = sscanf(argv[optind-1], "%lld", &read_64);
			if(num_read > 0  && (*flags & BANDWIDTH_CMP) == 0)
			{
				info->cmp = BANDWIDTH_GT;
				info->bandwidth_cutoff = read_64;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP; //only need one flag for less_than/greater_than
			break;
		case BANDWIDTH_CHECK_NOSWAP:
			if(  (*flags & BANDWIDTH_CMP) == 0 )
			{
				info->cmp = BANDWIDTH_CHECK;
				info->check_type = BANDWIDTH_CHECK_NOSWAP;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP;
			break;
		case BANDWIDTH_CHECK_SWAP:
			if(  (*flags & BANDWIDTH_CMP) == 0 )
			{
				info->cmp = BANDWIDTH_CHECK;
				info->check_type = BANDWIDTH_CHECK_SWAP;
				valid_arg = 1;
			}
			c = BANDWIDTH_CMP;
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
				info->reset_is_constant_interval = 0;
			}
			else if(strcmp(argv[optind-1],"hour") ==0)
			{
				info->reset_interval = BANDWIDTH_HOUR;
				info->reset_is_constant_interval = 0;
			}
			else if(strcmp(argv[optind-1],"day") ==0)
			{
				info->reset_interval = BANDWIDTH_DAY;
				info->reset_is_constant_interval = 0;
			}
			else if(strcmp(argv[optind-1],"week") ==0)
			{
				info->reset_interval = BANDWIDTH_WEEK;
				info->reset_is_constant_interval = 0;
			}
			else if(strcmp(argv[optind-1],"month") ==0)
			{
				info->reset_interval = BANDWIDTH_MONTH;
				info->reset_is_constant_interval = 0;
			}
			else if(strcmp(argv[optind-1],"never") ==0)
			{
				info->reset_interval = BANDWIDTH_NEVER;
			}
			else if(sscanf(argv[optind-1], "%lld", &read_time) > 0)
			{
				info->reset_interval = read_time;
				info->reset_is_constant_interval = 1;
			}
			else
			{
				valid_arg = 0;
			}
			break;
		case BANDWIDTH_NUM_INTERVALS:
			if( sscanf(argv[optind-1], "%ld", &num_read) > 0)
			{
				info->num_intervals_to_save = num_read;
				valid_arg=1;
			}
			c=0;
			break;
		case BANDWIDTH_RESET_TIME:
			num_read = sscanf(argv[optind-1], "%lld", &read_time);
			if(num_read > 0 )
			{
				info->reset_time = read_time;
				valid_arg = 1;
			}	
			break;
		case BANDWIDTH_LAST_BACKUP:
			num_read = sscanf(argv[optind-1], "%lld", &read_time);
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
			param_problem_exit_error("Parameter for '--reset_time' is not in valid range");
		}
	}
	if(info->type != BANDWIDTH_COMBINED && (*flags & BANDWIDTH_CURRENT) == BANDWIDTH_CURRENT)
	{
		valid_arg = 0;
		param_problem_exit_error("You may only specify current bandwidth for combined type\n  Use user-space library for setting bandwidth for individual types");
	}

	return valid_arg;
}

static int parse_mt4(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
			const void *entry,		
			struct xt_entry_match **match
			)
{
	int valid_arg = 0;
	
	valid_arg = parse(c, argv, invert, flags, entry, match, NFPROTO_IPV4);

	return valid_arg;
}

static int parse_mt6(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
			const void *entry,		
			struct xt_entry_match **match
			)
{
	int valid_arg = 0;
	
	valid_arg = parse(c, argv, invert, flags, entry, match, NFPROTO_IPV6);

	return valid_arg;
}

static void print_bandwidth_args(struct xt_bandwidth_info* info, int family)
{
	if(info->cmp == BANDWIDTH_CHECK)
	{
		if(info->check_type == BANDWIDTH_CHECK_NOSWAP)
		{
			printf("--bcheck ");
		}
		else
		{
			printf("--bcheck_with_src_dst_swap ");
		}
	}
	printf("--id %s ", info->id);



	if(info->cmp != BANDWIDTH_CHECK)
	{
		/* determine current time in seconds since epoch, with offset for current timezone */
		int minuteswest = get_minutes_west();
		time_t now;
		time(&now);
		now = now - (minuteswest*60);

		if(info->type == BANDWIDTH_COMBINED)
		{
			printf("--type combined ");
		}
		if(info->type == BANDWIDTH_INDIVIDUAL_SRC)
		{
			printf("--type individual_src ");
		}
		if(info->type == BANDWIDTH_INDIVIDUAL_DST)
		{
			printf("--type individual_dst ");
		}
		if(info->type == BANDWIDTH_INDIVIDUAL_LOCAL)
		{
			printf("--type individual_local ");
		}
		if(info->type == BANDWIDTH_INDIVIDUAL_REMOTE)
		{
			printf("--type individual_remote ");
		}

		if(family == NFPROTO_IPV4)
		{
			struct in_addr testblk;
			memset(&testblk, 0, sizeof(struct in_addr));
			if(memcmp(&testblk, &(info->local_subnet.ip4), sizeof(struct in_addr)) != 0)
			{
				printf("--subnet %s/%d ", xtables_ipaddr_to_numeric(&(info->local_subnet.ip4)), xtables_ipmask_to_cidr(&(info->local_subnet_mask.ip4))); 
			}
		}
		else
		{
			struct in6_addr testblk;
			memset(&testblk, 0, sizeof(struct in6_addr));
			if(memcmp(&testblk, &(info->local_subnet.ip6), sizeof(struct in6_addr)) != 0)
			{
				printf("--subnet %s/%d ", xtables_ip6addr_to_numeric(&(info->local_subnet.ip6)), xtables_ip6mask_to_cidr(&(info->local_subnet_mask.ip6))); 
			}
		}
		
		if(info->cmp == BANDWIDTH_GT)
		{
			printf("--greater_than %lld ", info->bandwidth_cutoff);
		}
		if(info->cmp == BANDWIDTH_LT)
		{
			printf("--less_than %lld ", info->bandwidth_cutoff);
		}
		if (info->type == BANDWIDTH_COMBINED) /* too much data to print for multi types, have to use socket to get/set data */
		{
			if( info->reset_interval != BANDWIDTH_NEVER && info->next_reset != 0 && info->next_reset < now)
			{
				/* 
				 * current bandwidth only gets reset when first packet after reset interval arrives, so output
				 * zero if we're already past interval, but no packets have arrived 
				 */
				printf("--current_bandwidth 0 ");
			}
			else 
			{
				printf("--current_bandwidth %lld ", info->current_bandwidth);
			}
		}
		if(info->reset_is_constant_interval)
		{
			printf("--reset_interval %lld ", info->reset_interval);
		}
		else
		{
			if(info->reset_interval == BANDWIDTH_MINUTE)
			{
				printf("--reset_interval minute ");
			}
			else if(info->reset_interval == BANDWIDTH_HOUR)
			{
				printf("--reset_interval hour ");
			}
			else if(info->reset_interval == BANDWIDTH_DAY)
			{
				printf("--reset_interval day ");
			}
			else if(info->reset_interval == BANDWIDTH_WEEK)
			{
				printf("--reset_interval week ");
			}
			else if(info->reset_interval == BANDWIDTH_MONTH)
			{
				printf("--reset_interval month ");
			}
		}
		if(info->reset_time > 0)
		{
			printf("--reset_time %lld ", info->reset_time);
		}
		if(info->num_intervals_to_save > 0)
		{
			printf("--intervals_to_save %d ", info->num_intervals_to_save);
		}
	}
}

/* 
 * Final check, we can't have reset_time without reset_interval
 */
static void final_check(unsigned int flags)
{
	if (flags == 0)
	{
		param_problem_exit_error("You must specify at least one argument. ");
	}
	if( (flags & BANDWIDTH_RESET_INTERVAL) == 0 && (flags & BANDWIDTH_RESET_TIME) != 0)
	{
		param_problem_exit_error("You may not specify '--reset_time' without '--reset_interval' ");
	}
	if( (flags & BANDWIDTH_REQUIRES_SUBNET) == BANDWIDTH_REQUIRES_SUBNET && (flags & BANDWIDTH_SUBNET) == 0 )
	{
		param_problem_exit_error("You must specify a local subnet (--subnet a.b.c.d/mask) to match individual local/remote IPs ");
	}

	/* update timezone minutes_west in kernel to match userspace*/
	set_kernel_timezone();
}

/* Prints out the matchinfo. */
static void print_mt4(const void *ip, const struct xt_entry_match *match, int numeric)
{
	printf("bandwidth ");
	struct xt_bandwidth_info *info = (struct xt_bandwidth_info *)match->data;

	print_bandwidth_args(info, NFPROTO_IPV4);
}

static void print_mt6(const void *ip, const struct xt_entry_match *match, int numeric)
{
	printf("bandwidth ");
	struct xt_bandwidth_info *info = (struct xt_bandwidth_info *)match->data;

	print_bandwidth_args(info, NFPROTO_IPV6);
}

/* Saves the union xt_matchinfo in parsable form to stdout. */
static void save_mt4(const void *ip, const struct xt_entry_match *match)
{
	struct xt_bandwidth_info *info = (struct xt_bandwidth_info *)match->data;
	time_t now;
	
	print_bandwidth_args(info, NFPROTO_IPV4);
	
	time(&now);
	printf("--last_backup-time %lld ", now);
}

static void save_mt6(const void *ip, const struct xt_entry_match *match)
{
	struct xt_bandwidth_info *info = (struct xt_bandwidth_info *)match->data;
	time_t now;
	
	print_bandwidth_args(info, NFPROTO_IPV6);
	
	time(&now);
	printf("--last_backup-time %lld ", now);
}

static struct xtables_match bandwidth_mt_reg[] = 
{
	{
		.next		= NULL,
	 	.name		= "bandwidth",
		.family		= NFPROTO_IPV4,
		.version	= XTABLES_VERSION, 
		.size		= XT_ALIGN(sizeof(struct xt_bandwidth_info)),
		.userspacesize	= XT_ALIGN(sizeof(struct xt_bandwidth_info)),
		.help		= &help,
		.parse		= &parse_mt4,
		.final_check	= &final_check,
		.print		= &print_mt4,
		.save		= &save_mt4,
		.extra_opts	= opts
	},
	{
		.next		= NULL,
	 	.name		= "bandwidth",
		.family		= NFPROTO_IPV6,
		.version	= XTABLES_VERSION, 
		.size		= XT_ALIGN(sizeof(struct xt_bandwidth_info)),
		.userspacesize	= XT_ALIGN(sizeof(struct xt_bandwidth_info)),
		.help		= &help,
		.parse		= &parse_mt6,
		.final_check	= &final_check,
		.print		= &print_mt6,
		.save		= &save_mt6,
		.extra_opts	= opts
	},
};

void _init(void)
{
	xtables_register_matches(bandwidth_mt_reg, ARRAY_SIZE(bandwidth_mt_reg));
}

static void param_problem_exit_error(char* msg)
{
	xtables_error(PARAMETER_PROBLEM, "%s", msg);
}

int parse_sub(char* subnet_string, ipany* subnet, ipany* subnet_mask, int family)
{
	char** sub_parts = split_on_separators(subnet_string,"/",1,2,1);
	char* substr = trim_flanking_whitespace(sub_parts[0]);
	char* mskstr = trim_flanking_whitespace(sub_parts[1]);
	int valid = 0;
	
	if(family == NFPROTO_IPV4)
	{
		struct in_addr* tsub = NULL;
		tsub = xtables_numeric_to_ipaddr(substr);
		if(tsub != NULL)
		{
			subnet->ip4 = *tsub;
			int mask_valid = 0;
			if(strchr(mskstr, '.') != NULL)
			{
				struct in_addr* mask_add;
				mask_add = xtables_numeric_to_ipaddr(mskstr);
				
				if(mask_add != NULL)
				{
					subnet_mask->ip4 = *mask_add;
					mask_valid = 1;
					free(mask_add);
				}
			}
			else
			{
				int mask_bits;
				if(sscanf(mskstr, "%d", &mask_bits) > 0)
				{
					if(mask_bits >= 0 && mask_bits <= 32)
					{
						subnet_mask->ip4.s_addr = 0;
						subnet_mask->ip4.s_addr = htonl(0xFFFFFFFF << (32 - mask_bits));
						mask_valid = 1;
					}
				}
			}
			
			if(mask_valid)
			{
				valid = 1;
			}
			free(tsub);
		}

		if(valid)
		{
			subnet->ip4.s_addr = (subnet->ip4.s_addr & subnet_mask->ip4.s_addr);
		}
	}
	else
	{
		struct in6_addr* tsub = NULL;
		tsub = xtables_numeric_to_ip6addr(substr);
		if(tsub != NULL)
		{
			subnet->ip6 = *tsub;
			int mask_valid = 0;
			if(strchr(mskstr, ':') != NULL)
			{
				struct in6_addr* mask_add;
				mask_add = xtables_numeric_to_ip6addr(mskstr);
				
				if(mask_add != NULL)
				{
					subnet_mask->ip6 = *mask_add;
					mask_valid = 1;
					free(mask_add);
				}
			}
			else
			{
				int mask_bits;
				struct in6_addr mask_add;
				if(sscanf(mskstr, "%d", &mask_bits) > 0)
				{
					char* p = (void *)&mask_add;
					memset(p, 0xff, mask_bits/8);
					memset(p+ ((mask_bits+7)/8), 0, (128-mask_bits)/8);
					if(mask_bits < 128)
					{
						p[mask_bits/8] = 0xff << (8-(mask_bits & 7));
					}
					subnet_mask->ip6 = mask_add;
					mask_valid = 1;
				}
			}
			
			if(mask_valid)
			{
				valid = 1;
			}
			free(tsub);
		}
		
		if(valid)
		{
			for(unsigned int x = 0; x < 16; x++)
			{
				subnet->ip6.s6_addr[x] = (subnet->ip6.s6_addr[x] & subnet_mask->ip6.s6_addr[x]);
			}
		}
	}
	
	return valid;
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
		split = (char**)malloc((1+max_pieces)*sizeof(char*));
		split_index = 0;
		split[split_index] = NULL;


		dup_line = strdup(line_str);
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

int get_minutes_west(void)
{
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

	return minuteswest;
}

void set_kernel_timezone(void)
{
	struct timeval tv;
	struct timezone old_tz;
	struct timezone new_tz;

	new_tz.tz_minuteswest = get_minutes_west();;
	new_tz.tz_dsttime = 0;

	/* Get tv to pass to settimeofday(2) to be sure we avoid hour-sized warp */
	/* (see gettimeofday(2) man page, or /usr/src/linux/kernel/time.c) */
	gettimeofday(&tv, &old_tz);

	/* set timezone */
	settimeofday(&tv, &new_tz);
}
