/*  weburl --	An iptables extension to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
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

#include <iptables.h>
#include <linux/netfilter_ipv4/ipt_weburl.h>


/* Function which prints out usage message. */
static void help(void)
{
	printf(	"weburl options:\n  --contains [!] [STRING]\n  --contains_regex [!] [REGEX]\n");
}

static struct option opts[] = 
{
	{ .name = "contains", 		.has_arg = 1, .flag = 0, .val = 's' }, //string
	{ .name = "contains_regex", 	.has_arg = 1, .flag = 0, .val = 'r' },  //regex
	{ .name = 0 }
};


/* Function which parses command options; returns true if it
   ate an option */
static int parse(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
#ifdef ipt_entry_match
			const void *entry,
#else
			const struct ipt_entry *entry,
			unsigned int *nfcache,
#endif			
			struct ipt_entry_match **match
			)
{
	struct ipt_weburl_info *info = (struct ipt_weburl_info *)(*match)->data;

	switch (c)
	{
		case 's':
			//use simple string match, not regex
			info->use_regex = 0;	
			break;

		case 'r':
			
			//use regex instead of simple string match
			info->use_regex = 1;
			break;
		default:
			return 0;
	}
	
	//test whether to invert rule
	check_inverse(optarg, &invert, &optind, 0);
	info->invert = invert ? 1 : 0;
	
	//test that test string is reasonable length, then to info
	int testlen = strlen(argv[optind-1]);
	if(testlen > 0 && testlen < MAX_TEST_STR)
	{
		strcpy(info->test_str, argv[optind-1]);
	}
	else if(testlen >= MAX_TEST_STR)
	{
		exit_error(PARAMETER_PROBLEM, "Parameter definition is too long, must be less than %d characters", MAX_TEST_STR);
	}
	else
	{
		exit_error(PARAMETER_PROBLEM, "Parameter definition is incomplete");
	}

	if(*flags == 1)
	{
		exit_error(PARAMETER_PROBLEM, "You may only specify one string/pattern to match");
	}	
	*flags = 1;

	return 1;
}


	
static void print_weburl_args(	struct ipt_weburl_info* info )
{
	//invert
	if(info->invert > 0)
	{
		printf("! ");
	}
	//match type
	if(info->use_regex > 0)
	{
		printf("--contains_regex ");
	}
	else
	{
		printf("--contains ");
	}
	//test string
	printf("%s ", info->test_str);
	
}

/* Final check; must have specified a test string with either --contains or --contains_regex. */
static void final_check(unsigned int flags)
{
	if (!flags)
	{
		exit_error(PARAMETER_PROBLEM, "You must specify '--contains' or '--contains_regex'");
	}
}

/* Prints out the matchinfo. */
#ifdef ipt_entry_match
static void print(const void *ip, const struct xt_entry_match *match, int numeric)
#else	
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
#endif
{
	printf("WEBURL ");
	struct ipt_weburl_info *info = (struct ipt_weburl_info *)match->data;

	print_weburl_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
#ifdef ipt_entry_match
static void save(const void *ip, const struct xt_entry_match *match)
#else
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
#endif
{
	struct ipt_weburl_info *info = (struct ipt_weburl_info *)match->data;
	print_weburl_args(info);
}

static struct iptables_match weburl = 
{ 
	.next		= NULL,
 	.name		= "weburl",
	.version	= IPTABLES_VERSION,
	.size		= IPT_ALIGN(sizeof(struct ipt_weburl_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_weburl_info)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&weburl);
}
