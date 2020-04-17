/*  webmon --	An iptables extension to match URLs in HTTP(S) requests
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008-2011 by Eric Bishop <eric@gargoyle-router.com>
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

#include <arpa/inet.h>

#include <xtables.h>
#include <linux/netfilter.h>
#include <linux/netfilter/xt_webmon.h>

/* utility functions necessary for module to work across multiple iptables versions */
static void param_problem_exit_error(char* msg);


void parse_ips_and_ranges(int family, char* addr_str, struct xt_webmon_info *info);

char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
char* trim_flanking_whitespace(char* str);
unsigned char* read_entire_file(FILE* in, unsigned long read_block_size, unsigned long *length);

#define DEFAULT_MAX      300

#define SEARCH_LOAD_FILE 100
#define DOMAIN_LOAD_FILE 101
#define CLEAR_SEARCH     102
#define CLEAR_DOMAIN     103

static char* domain_load_file = NULL;
static char* search_load_file = NULL;
static uint32_t global_max_domains  = DEFAULT_MAX;
static uint32_t global_max_searches = DEFAULT_MAX;

/* Function which prints out usage message. */
static void help(void)
{
	printf(	"webmon options:\n --[exclude|include]_ips [ip[,ip,ip...]|ipstart-ipend|ip/mask]\n --max_domains [num]\n --max_searches [num]\n --search_load_file [path]\n --domain_load_file [path]\n --clear_search\n --clear_domain");
}

static struct option opts[] = 
{
	{ .name = "exclude_ips",        .has_arg = 1, .flag = 0, .val = WEBMON_EXCLUDE },
	{ .name = "include_ips",        .has_arg = 1, .flag = 0, .val = WEBMON_INCLUDE },
	{ .name = "max_domains",        .has_arg = 1, .flag = 0, .val = WEBMON_MAXDOMAIN },
	{ .name = "max_searches",       .has_arg = 1, .flag = 0, .val = WEBMON_MAXSEARCH },
	{ .name = "search_load_file",   .has_arg = 1, .flag = 0, .val = SEARCH_LOAD_FILE },
	{ .name = "domain_load_file",   .has_arg = 1, .flag = 0, .val = DOMAIN_LOAD_FILE },
	{ .name = "clear_search",       .has_arg = 0, .flag = 0, .val = CLEAR_SEARCH },
	{ .name = "clear_domain",       .has_arg = 0, .flag = 0, .val = CLEAR_DOMAIN },

	{ .name = 0 }
};

static void webmon_init(
	struct xt_entry_match *match
	)
{
	struct xt_webmon_info *info = (struct xt_webmon_info *)match->data;
	info->max_domains=DEFAULT_MAX;
	info->max_searches=DEFAULT_MAX;
	info->num_exclude_ips=0;
	info->num_exclude_ranges=0;
	info->exclude_type = WEBMON_EXCLUDE;
	info->ref_count = NULL;
}


/* Function which parses command options; returns true if it ate an option */
static int parse_mt4(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
			const void *entry,			
			struct xt_entry_match **match
			)
{
	struct xt_webmon_info *info = (struct xt_webmon_info *)(*match)->data;
	int valid_arg = 1;
	long max;
	switch (c)
	{
		case WEBMON_EXCLUDE:
			parse_ips_and_ranges(NFPROTO_IPV4, optarg, info);
			info->exclude_type = WEBMON_EXCLUDE;
			break;
		case WEBMON_INCLUDE:
			parse_ips_and_ranges(NFPROTO_IPV4, optarg, info);
			info->exclude_type = WEBMON_INCLUDE;
			break;
		case WEBMON_MAXSEARCH:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_searches = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_searches = (uint32_t)max;
				global_max_searches = info->max_searches;
			}
			break;
		case WEBMON_MAXDOMAIN:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_domains = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_domains = (uint32_t)max;
				global_max_domains = info->max_domains;
			}
			break;
		case SEARCH_LOAD_FILE:
			search_load_file = strdup(optarg);
			break;
		case DOMAIN_LOAD_FILE:
			domain_load_file = strdup(optarg);
			break;
		case CLEAR_SEARCH:
			search_load_file = strdup("/dev/null");
			break;
		case CLEAR_DOMAIN:
			domain_load_file = strdup("/dev/null");
			break;
		default:
			valid_arg = 0;
	}
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
	struct xt_webmon_info *info = (struct xt_webmon_info *)(*match)->data;
	int valid_arg = 1;
	long max;
	switch (c)
	{
		case WEBMON_EXCLUDE:
			parse_ips_and_ranges(NFPROTO_IPV6, optarg, info);
			info->exclude_type = WEBMON_EXCLUDE;
			break;
		case WEBMON_INCLUDE:
			parse_ips_and_ranges(NFPROTO_IPV6, optarg, info);
			info->exclude_type = WEBMON_INCLUDE;
			break;
		case WEBMON_MAXSEARCH:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_searches = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_searches = (uint32_t)max;
				global_max_searches = info->max_searches;
			}
			break;
		case WEBMON_MAXDOMAIN:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_domains = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_domains = (uint32_t)max;
				global_max_domains = info->max_domains;
			}
			break;
		case SEARCH_LOAD_FILE:
			search_load_file = strdup(optarg);
			break;
		case DOMAIN_LOAD_FILE:
			domain_load_file = strdup(optarg);
			break;
		case CLEAR_SEARCH:
			search_load_file = strdup("/dev/null");
			break;
		case CLEAR_DOMAIN:
			domain_load_file = strdup("/dev/null");
			break;
		default:
			valid_arg = 0;
	}
	return valid_arg;

}


	
static void print_webmon_args(int family,	struct xt_webmon_info* info)
{
	printf("--max_domains %ld ", (unsigned long int)info->max_domains);
	printf("--max_searches %ld ", (unsigned long int)info->max_searches);
	if(info->num_exclude_ips > 0 || info->num_exclude_ranges > 0)
	{
		int ip_index = 0;
		char comma[3] = "";
		printf("--%s ", (info->exclude_type == WEBMON_EXCLUDE ? "exclude_ips" : "include_ips"));
		for(ip_index=0; ip_index < info->num_exclude_ips; ip_index++)
		{
      if(family == NFPROTO_IPV4)
      {
        printf("%s%s", comma, xtables_ipaddr_to_numeric(&(((info->exclude_ips)[ip_index]).ip4)));
      }
      else
      {
        printf("%s%s", comma, xtables_ip6addr_to_numeric(&(((info->exclude_ips)[ip_index]).ip6)));
      }
			sprintf(comma, ",");
		}
		for(ip_index=0; ip_index < info->num_exclude_ranges; ip_index++)
		{
			struct xt_webmon_ip_range r = (info->exclude_ranges)[ip_index];
      if(family == NFPROTO_IPV4)
      {
			  printf("%s%s-", comma, xtables_ipaddr_to_numeric(&(r.start.ip4)) );
        printf("%s", xtables_ipaddr_to_numeric(&(r.end.ip4)) );
      }
      else
      {
        printf("%s%s-", comma, xtables_ip6addr_to_numeric(&(r.start.ip6)) );
        printf("%s", xtables_ip6addr_to_numeric(&(r.end.ip6)) );
      }
			sprintf(comma, ",");
		}
		printf(" ");
	}
}


static void do_load(char* file, uint32_t max, unsigned char type)
{
	if(file != NULL)
	{
		unsigned char* data = NULL;
		unsigned long data_length = 0;
		char* file_data = NULL;
		if(strcmp(file, "/dev/null") != 0)
		{
			FILE* in = fopen(file, "r");
			if(in != NULL)
			{
				file_data = (char*)read_entire_file(in, 4096, &data_length);
				fclose(in);
			}
		}
		if(file_data == NULL)
		{
			file_data=strdup("");
		}
		
		if(file_data != NULL)
		{
			data_length = strlen(file_data) + sizeof(uint32_t)+2;
			data = (unsigned char*)malloc(data_length);
			if(data != NULL)
			{
				int sockfd = -1;
				uint32_t* maxp = (uint32_t*)(data+1);
				data[0] = type;
				*maxp = max;
				sprintf( (data+1+sizeof(uint32_t)),  "%s", file_data);
			
				sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
				if(sockfd >= 0)
				{
					setsockopt(sockfd, IPPROTO_IP, WEBMON_SET, data, data_length);
					close(sockfd);
				}
				free(data);
			}
			free(file_data);
		}
	}

}


static void final_check(unsigned int flags)
{
	do_load(domain_load_file, global_max_domains,  WEBMON_DOMAIN);
	do_load(search_load_file, global_max_searches, WEBMON_SEARCH);
}

/* Prints out the matchinfo. */
static void print_mt4(const void *ip, const struct xt_entry_match *match, int numeric)
{
	printf("WEBMON ");
	struct xt_webmon_info *info = (struct xt_webmon_info *)match->data;

	print_webmon_args(NFPROTO_IPV4, info);
}

static void print_mt6(const void *ip, const struct xt_entry_match *match, int numeric)
{
	printf("WEBMON ");
	struct xt_webmon_info *info = (struct xt_webmon_info *)match->data;

	print_webmon_args(NFPROTO_IPV6, info);
}

/* Saves the union xt_matchinfo in parsable form to stdout. */
static void save_mt4(const void *ip, const struct xt_entry_match *match)
{
	struct xt_webmon_info *info = (struct xt_webmon_info *)match->data;
	print_webmon_args(NFPROTO_IPV4, info);
}

static void save_mt6(const void *ip, const struct xt_entry_match *match)
{
	struct xt_webmon_info *info = (struct xt_webmon_info *)match->data;
	print_webmon_args(NFPROTO_IPV6, info);
}

static struct xtables_match webmon_mt_reg[] = 
{
	{
		.next		= NULL,
	 	.name		= "webmon",
		.family		= NFPROTO_IPV4,
		.version	= XTABLES_VERSION,
		.size		= XT_ALIGN(sizeof(struct xt_webmon_info)),
		.userspacesize	= XT_ALIGN(sizeof(struct xt_webmon_info)),
		.help		= &help,
		.init           = &webmon_init,
		.parse		= &parse_mt4,
		.final_check	= &final_check,
		.print		= &print_mt4,
		.save		= &save_mt4,
		.extra_opts	= opts
	},
  {
		.next		= NULL,
	 	.name		= "webmon",
		.family		= NFPROTO_IPV6,
		.version	= XTABLES_VERSION,
		.size		= XT_ALIGN(sizeof(struct xt_webmon_info)),
		.userspacesize	= XT_ALIGN(sizeof(struct xt_webmon_info)),
		.help		= &help,
		.init           = &webmon_init,
		.parse		= &parse_mt6,
		.final_check	= &final_check,
		.print		= &print_mt6,
		.save		= &save_mt6,
		.extra_opts	= opts
	},
};

void _init(void)
{
	xtables_register_matches(webmon_mt_reg, ARRAY_SIZE(webmon_mt_reg));
}


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static void param_problem_exit_error(char* msg)
{
	xtables_error(PARAMETER_PROBLEM, "%s", msg);
}


void parse_ips_and_ranges(int family, char* addr_str, struct xt_webmon_info *info)
{
	char** addr_parts = split_on_separators(addr_str, ",", 1, -1, 0);

	info->num_exclude_ips=0;
	info->num_exclude_ranges = 0;

	int ip_part_index;
	for(ip_part_index=0; addr_parts[ip_part_index] != NULL; ip_part_index++)
	{
		char* next_str = addr_parts[ip_part_index];
		if(strchr(next_str, '-') != NULL)
		{
			char** range_parts = split_on_separators(next_str, "-", 1, 2, 1);
			char* start = trim_flanking_whitespace(range_parts[0]);
			char* end = trim_flanking_whitespace(range_parts[1]);
      struct in_addr sip, eip;
      struct in6_addr sip6, eip6;
      
      if(family == NFPROTO_IPV4)
      {
        struct in_addr* tsip = NULL;
        struct in_addr* teip = NULL;
        tsip = xtables_numeric_to_ipaddr(start);
        if(tsip != NULL)
        {
          sip = *tsip;
        }
        teip = xtables_numeric_to_ipaddr(end);
        if(teip != NULL)
        {
          eip = *teip;
        }
        
        if(tsip != NULL && teip != NULL)
  			{
  				struct xt_webmon_ip_range r;
  				r.start.ip4 = sip;
  				r.end.ip4   = eip;
  
  				if(info->num_exclude_ranges <  WEBMON_MAX_IP_RANGES  && (unsigned long)ntohl(r.start.ip4.s_addr) < (unsigned long)ntohl(r.end.ip4.s_addr) )
  				{
  					(info->exclude_ranges)[ info->num_exclude_ranges ] = r;
  					info->num_exclude_ranges = info->num_exclude_ranges + 1;
  				}
  			}
      }
      else
      {
        struct in6_addr* tsip6 = NULL;
        struct in6_addr* teip6 = NULL;
        tsip6 = xtables_numeric_to_ip6addr(start);
        if(tsip6 != NULL)
        {
          sip6 = *tsip6;
        }
        teip6 = xtables_numeric_to_ip6addr(end);
        if(teip6 != NULL)
        {
          eip6 = *teip6;
        }
        
        if(tsip6 != NULL && teip6 != NULL)
  			{
  				struct xt_webmon_ip_range r;
          r.start.ip6 = sip6;
          r.end.ip6 = eip6;
  
  				if(info->num_exclude_ranges <  WEBMON_MAX_IP_RANGES  && (memcmp(&(r.start.ip6.s6_addr), &(r.end.ip6.s6_addr), sizeof(unsigned char)*16) < 0))
  				{
  					(info->exclude_ranges)[ info->num_exclude_ranges ] = r;
  					info->num_exclude_ranges = info->num_exclude_ranges + 1;
  				}
  			}
      }

			free(start);
			free(end);	
			free(range_parts);
		}
		else if(strchr(next_str, '/') != NULL)
		{
			char** range_parts = split_on_separators(next_str, "/", 1, 2, 1);
			char* start = trim_flanking_whitespace(range_parts[0]);
			char* end = trim_flanking_whitespace(range_parts[1]);
      struct in_addr bip;
      struct in6_addr bip6;
      
      if(family == NFPROTO_IPV4)
      {
        struct in_addr* tbip = NULL;
        tbip = xtables_numeric_to_ipaddr(start);
        if(tbip != NULL)
        {
          bip = *tbip;
          int mask_valid = 0;
  				uint32_t mask;
  				if(strchr(end, '.') != NULL)
  				{
            struct in_addr* mask_add;
            mask_add = xtables_numeric_to_ipaddr(end);
  
  					if(mask_add != NULL)
  					{
  						mask = (uint32_t)mask_add->s_addr;
  						mask_valid = 1;
  					}
  				}
  				else
  				{
  					int mask_bits;
  					if( sscanf(end, "%d", &mask_bits) > 0)
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
            struct xt_webmon_ip_range r;
  
  					r.start.ip4.s_addr = ( ((uint32_t)bip.s_addr) & mask );
  					r.end.ip4.s_addr   = ( ((uint32_t)bip.s_addr) | (~mask) );
  					if(info->num_exclude_ranges <  WEBMON_MAX_IP_RANGES && ntohl(r.start.ip4.s_addr) <= ntohl(r.end.ip4.s_addr) )
  					{
  						(info->exclude_ranges)[ info->num_exclude_ranges ] = r;
  						info->num_exclude_ranges = info->num_exclude_ranges + 1;
  					}           
  				}
  			}
      }
      else
      {
        struct in6_addr* tbip6 = NULL;
        tbip6 = xtables_numeric_to_ip6addr(start);
        if(tbip6 != NULL)
        {
          bip6 = *tbip6;
          int mask_valid = 0;
          struct in6_addr mask_add;
          
  				if(strchr(end, ':') != NULL)
  				{
            struct in6_addr* tmask_add = NULL;
            tmask_add = xtables_numeric_to_ip6addr(end);
            if(tmask_add != NULL)
            {
              mask_add = *tmask_add;
              mask_valid = 1;
            }
  				}
  				else
  				{
  					int mask_bits;
  					if( sscanf(end, "%d", &mask_bits) > 0)
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
            struct xt_webmon_ip_range r;
            r.start.ip6 = bip6;
            r.end.ip6 = bip6;
            for(unsigned int x = 0; x < 16; x++)
            {
              r.start.ip6.s6_addr[x] = ( r.start.ip6.s6_addr[x] & mask_add.s6_addr[x] );
              r.end.ip6.s6_addr[x] = ( r.start.ip6.s6_addr[x] | (~mask_add.s6_addr[x]) );
            }
  					if(info->num_exclude_ranges <  WEBMON_MAX_IP_RANGES && (memcmp(&(r.start.ip6.s6_addr), &(r.end.ip6.s6_addr), sizeof(unsigned char)*16) < 0))
  					{
  						(info->exclude_ranges)[ info->num_exclude_ranges ] = r;
  						info->num_exclude_ranges = info->num_exclude_ranges + 1;
  					}
  				}
  			}
      }
      
			free(start);
			free(end);	
			free(range_parts);
		}
		else
		{
      struct in_addr* ip = NULL;
      struct in6_addr* ip6 = NULL;
      trim_flanking_whitespace(next_str);
      
      if(family == NFPROTO_IPV4)
      {
        ip = xtables_numeric_to_ipaddr(next_str);
        if(ip != NULL)
  			{
  				if(info->num_exclude_ranges <  WEBMON_MAX_IPS)
  				{
  					((info->exclude_ips)[ info->num_exclude_ips ]).ip4 = *ip;
  					info->num_exclude_ips = info->num_exclude_ips + 1;
  				}
  			}
      }
      else
      {
        ip6 = xtables_numeric_to_ip6addr(next_str);
        if(ip6 != NULL)
  			{
  				if(info->num_exclude_ranges <  WEBMON_MAX_IPS)
  				{
            ((info->exclude_ips)[ info->num_exclude_ips ]).ip6 = *ip6;
  					info->num_exclude_ips = info->num_exclude_ips + 1;
  				}
  			}
      }		
		}
		free(next_str);
	}
	free(addr_parts);
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


unsigned char* read_entire_file(FILE* in, unsigned long read_block_size, unsigned long *length)
{
	int max_read_size = read_block_size;
	unsigned char* read_string = (unsigned char*)malloc(max_read_size+1);
	unsigned long bytes_read = 0;
	int end_found = 0;
	while(end_found == 0)
	{
		int nextch = '?';
		while(nextch != EOF && bytes_read < max_read_size)
		{
			nextch = fgetc(in);
			if(nextch != EOF)
			{
				read_string[bytes_read] = (unsigned char)nextch;
				bytes_read++;
			}
		}
		read_string[bytes_read] = '\0';
		end_found = (nextch == EOF) ? 1 : 0;
		if(end_found == 0)
		{
			unsigned char *new_str;
			max_read_size = max_read_size + read_block_size;
		       	new_str = (unsigned char*)malloc(max_read_size+1);
			memcpy(new_str, read_string, bytes_read);
			free(read_string);
			read_string = new_str;
		}
	}
	*length = bytes_read;
	return read_string;
}

