/*  netset --	An iptables extension to match multiple netsets within a week
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009-2010 by Eric Bishop <eric@gargoyle-router.com>
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
#include <ctype.h>


/*
 * in iptables 1.4.0 and higher, iptables.h includes xtables.h, which
 * we can use to check whether we need to deal with the new requirements
 * in pre-processor directives below
 */
#include <iptables.h>  
#include <linux/netfilter_ipv4/ipt_netset.h>

#ifdef _XTABLES_H
	#define iptables_rule_match	xtables_rule_match
	#define iptables_match		xtables_match
	#define iptables_target		xtables_target
	#define ipt_tryload		xt_tryload
#endif

/* 
 * XTABLES_VERSION_CODE is only defined in versions 1.4.1 and later, which
 * also require the use of xtables_register_match
 * 
 * Version 1.4.0 uses register_match like previous versions
 */
#ifdef XTABLES_VERSION_CODE 
	#define register_match          xtables_register_match
#endif

/* utility functions necessary for module to work across multiple iptables versions */
static void param_problem_exit_error(char* msg);

/* Function which prints out usage message. */
static void help(void)
{
	printf(	"netset match options:\n  --id [SET_ID] --group [GROUP_ID]\n");
}

static struct option opts[] = 
{
	{ .name = "id",        .has_arg = 1, .flag = 0, .val = SET_ID },
	{ .name = "group",     .has_arg = 1, .flag = 0, .val = GROUP_ID },
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
	struct ipt_netset_info *info = (struct ipt_netset_info *)(*match)->data;
	int valid_arg = 0;

	switch (c)
	{
		case SET_ID:
			strcpy(info->id, argv[optind-1], MAX_ID_LENGTH);
			(info->id)[MAX_ID_LENGTH-1] = '\0';
			if(strlen(info->id) > 0)
			{
				valid_arg = 1;
				*flags = *flags + c;
				info->group_or_set = *flags;
			}
			break;
		case GROUP_ID:
			strcpy(info->group, argv[optind-1], MAX_GROUP_LENGTH);
			(info->group)[MAX_GROUP_LENGTH-1] = '\0';
			if(strlen(info->group) > 0)
			{
				valid_arg = 1;
				*flags = *flags + c;
				info->group_or_set = *flags;
			}
			break;
	}

	return valid_arg;
}


	
static void print_netset_args(	struct ipt_netset_info* info )
{

	if( (info->group_or_set | GROUP_ID) == GROUP_ID )
	{
		printf(" --group \"%s\"", info->group);
	}
	if( (info->group_or_set | SET_ID) == SET_ID )
	{
		printf(" --id \"%s\"", info->id);

	}
}

/* Final check; must have specified a test string with either --contains or --contains_regex. */
static void final_check(unsigned int flags)
{
	if(flags ==0)
	{
		param_problem_exit_error("Invalid arguments to netset");
	}
}

/* Prints out the matchinfo. */
#ifdef _XTABLES_H
static void print(const void *ip, const struct xt_entry_match *match, int numeric)
#else	
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
#endif
{
	printf("netset");
	struct ipt_netset_info *info = (struct ipt_netset_info *)match->data;

	print_netset_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
#ifdef _XTABLES_H
static void save(const void *ip, const struct xt_entry_match *match)
#else
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
#endif
{
	struct ipt_netset_info *info = (struct ipt_netset_info *)match->data;
	print_netset_args(info);
}

static struct iptables_match netset = 
{ 
	.next		= NULL,
 	.name		= "netset",
	#ifdef XTABLES_VERSION_CODE
		.version = XTABLES_VERSION, 
	#else
		.version = IPTABLES_VERSION,
	#endif
	.size		= IPT_ALIGN(sizeof(struct ipt_netset_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_netset_info)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&netset);
}

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static void param_problem_exit_error(char* msg)
{
	#ifdef xtables_error
		xtables_error(PARAMETER_PROBLEM, msg);
	#else
		exit_error(PARAMETER_PROBLEM, msg);
	#endif
}

