/* Shared library add-on to iptables to add MAC address support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <iptables.h>
#include <linux/netfilter_ipv4/ipt_url.h>

/*
#if defined(__GLIBC__) && __GLIBC__ == 2
#include <net/ethernet.h>
#else
#include <linux/if_ether.h>
#endif
*/

/* Function which prints out usage message. */
static void help(void)
{
	printf(	"url options:\n  --contains [!] [STRING]\n  --contains_regex [!] [REGEX]\n");
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
			const struct ipt_entry *entry,
			unsigned int *nfcache,
			struct ipt_entry_match **match
			)
{
	struct ipt_url_info *info = (struct ipt_url_info *)(*match)->data;

	int testlen;
	switch (c)
	{
		case 's':
			//use simple string match, not regex
			info->use_regex = 0;
			
			//test whether to invert rule
			check_inverse(optarg, &invert, &optind, 0);
			info->invert = invert ? 1 : 0;

			//test that test string is reasonable length, then to info
			testlen = strlen(argv[optind-1]);
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
			break;
			
		case 'r':
			
			//use regex instead of simple string match
			info->use_regex = 1;
			
			//test whether to invert rule
			check_inverse(optarg, &invert, &optind, 0);
			info->invert = invert ? 1 : 0;

			//test that test string is reasonable length, then to info
			testlen = strlen(argv[optind-1]);
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
			break;

		default:
			return 0;
	}

	return 1;
}


static void print_url_args(struct ipt_url_info* info)
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
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
{
	printf("URL ");
	struct ipt_url_info *info = (struct ipt_url_info *)match->data;

	print_url_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
{
	struct ipt_url_info *info = (struct ipt_url_info *)match->data;
	print_url_args(info);
}

static struct iptables_match url = 
{ 
	.next		= NULL,
 	.name		= "url",
	.version	= IPTABLES_VERSION,
	.size		= IPT_ALIGN(sizeof(struct ipt_url_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_url_info)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&url);
}
