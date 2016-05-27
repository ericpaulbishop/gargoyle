/* 
   Shared library add-on to iptables for layer 7 matching support. 
  
   By Matthew Strait <quadong@users.sf.net>, Oct 2003-Aug 2008.

   http://l7-filter.sf.net 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
   http://www.gnu.org/licenses/gpl.txt
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <dirent.h>

#include <xtables.h>
#include <linux/netfilter_ipv4/ipt_layer7.h>

#define MAX_FN_LEN 256

static char l7dir[MAX_FN_LEN] = "\0";

/* Function which prints out usage message. */
static void help(void)
{
	printf(
	"layer7 match options:\n"
	"    --l7dir <directory> : Look for patterns here instead of /etc/l7-protocols/\n"
	"                          (--l7dir must be specified before --l7proto if used)\n"
	"[!] --l7proto <name>: Match named protocol using /etc/l7-protocols/.../name.pat\n");
}

static const struct option opts[] = {
	{ .name = "l7proto", .has_arg = 1, .val = 'p' },
	{ .name = "l7dir",   .has_arg = 1, .val = 'd' },
	{ .name = NULL }
};

/* reads filename, puts protocol info into layer7_protocol_info, number of protocols to numprotos */
static int parse_protocol_file(char * filename, const char * protoname, struct xt_layer7_info *info)
{
	FILE * f;
	char * line = NULL;
	size_t len = 0;

	enum { protocol, pattern, done } datatype = protocol;

	f = fopen(filename, "r");

	if(!f)
		return 0;

	while(getline(&line, &len, f) != -1)
	{
		if(strlen(line) < 2 || line[0] == '#')
			continue;

		/* strip the pesky newline... */
		if(line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

		if(datatype == protocol)
		{
			/* Ignore everything on the line beginning with the 
			first space or tab . For instance, this allows the 
			protocol line in http.pat to be "http " (or 
			"http I am so cool") instead of just "http". */
			if(strchr(line, ' ')){
				char * space = strchr(line, ' ');
				space[0] = '\0';
			}
			if(strchr(line, '\t')){
				char * space = strchr(line, '\t');
				space[0] = '\0';
			}

			/* sanity check.  First non-comment non-blank 
			line must be the same as the file name. */
			if(strcmp(line, protoname))
				xtables_error(OTHER_PROBLEM, 
					"Protocol name (%s) doesn't match file name (%s).  Bailing out\n",
					line, filename);

			if(strlen(line) >= MAX_PROTOCOL_LEN)
				 xtables_error(PARAMETER_PROBLEM, 
					"Protocol name in %s too long!", filename);
			strncpy(info->protocol, line, MAX_PROTOCOL_LEN);

			datatype = pattern; 
		}
		else if(datatype == pattern)
		{
			if(strlen(line) >= MAX_PATTERN_LEN)
				 xtables_error(PARAMETER_PROBLEM, "Pattern in %s too long!", filename);
			strncpy(info->pattern, line, MAX_PATTERN_LEN);
			
			datatype = done;			
			break;
		}
		else
			xtables_error(OTHER_PROBLEM, "Internal error");
	}

	if(datatype != done)
		xtables_error(OTHER_PROBLEM, "Failed to get all needed data from %s", filename);

	if(line) free(line);
	fclose(f);

	return 1;
}

static int hex2dec(char c)
{
        switch (c)
        {
                case '0' ... '9':
                        return c - '0';
                case 'a' ... 'f':
                        return c - 'a' + 10;
                case 'A' ... 'F':
                        return c - 'A' + 10;
                default:
                        xtables_error(OTHER_PROBLEM, "hex2dec: bad value!\n");
                        return 0;
        }
}

/* takes a string with \xHH escapes and returns one with the characters 
they stand for */
static char * pre_process(char * s)
{
	char * result = malloc(strlen(s) + 1);
	int sindex = 0, rrindex = 0;
        while( sindex < strlen(s) )
        {
            if( sindex + 3 < strlen(s) &&
                s[sindex] == '\\' && s[sindex+1] == 'x' && 
                isxdigit(s[sindex + 2]) && isxdigit(s[sindex + 3]) ) 
                {
                        /* carefully remember to call tolower here... */
                        result[rrindex] = tolower( hex2dec(s[sindex + 2])*16 +
                                                  hex2dec(s[sindex + 3] ) );

			switch ( result[rrindex] )
			{
			case 0x24:
			case 0x28:
			case 0x29:
			case 0x2a:
			case 0x2b:
			case 0x2e:
			case 0x3f:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x7c:
				fprintf(stderr, 
					"Warning: layer7 regexp contains a control character, %c, in hex (\\x%c%c).\n"
					"I recommend that you write this as %c or \\%c, depending on what you meant.\n",
					result[rrindex], s[sindex + 2], s[sindex + 3], result[rrindex], result[rrindex]);
				break;
			case 0x00:
				fprintf(stderr, 
					"Warning: null (\\x00) in layer7 regexp.  A null terminates the regexp string!\n");
				break;
			default:
				break;
			}


                        sindex += 3; /* 4 total */
                }
                else
                        result[rrindex] = tolower(s[sindex]);

		sindex++; 
		rrindex++;
        }
	result[rrindex] = '\0';

	return result;
}

#define MAX_SUBDIRS 128
static char ** readl7dir(char * dirname)
{
        DIR             * scratchdir;
        struct dirent   ** namelist;
	char ** subdirs = malloc(MAX_SUBDIRS * sizeof(char *));

        int n, d = 1;
	subdirs[0] = "";

        n = scandir(dirname, &namelist, 0, alphasort);

	if (n < 0)
	{
            perror("scandir");
	    xtables_error(OTHER_PROBLEM, "Couldn't open %s\n", dirname);
	}
        else 
	{
            	while(n--) 
		{
			char fulldirname[MAX_FN_LEN];

			snprintf(fulldirname, MAX_FN_LEN, "%s/%s", dirname, namelist[n]->d_name);

                	if((scratchdir = opendir(fulldirname)) != NULL)
			{
				closedir(scratchdir);

				if(!strcmp(namelist[n]->d_name, ".") || 
				   !strcmp(namelist[n]->d_name, ".."))
					/* do nothing */ ;
				else
				{
					subdirs[d] = malloc(strlen(namelist[n]->d_name) + 1);
					strcpy(subdirs[d], namelist[n]->d_name);
					d++;
					if(d >= MAX_SUBDIRS - 1)
					{
						fprintf(stderr, 
						  "Too many subdirectories, skipping the rest!\n");
						break;
					}
				}
			}
                	free(namelist[n]);
            	}
            	free(namelist);
        }
	
	subdirs[d] = NULL;

	return subdirs;
}

static void parse_layer7_protocol(const char *s, struct xt_layer7_info *info)
{
	char filename[MAX_FN_LEN];
	char * dir = NULL;
	char ** subdirs;
	int n = 0, done = 0;

	if(strlen(l7dir) > 0) dir = l7dir;
	else                  dir = "/etc/l7-protocols";

	subdirs = readl7dir(dir);

	while(subdirs[n] != NULL)
	{
		int c = snprintf(filename, MAX_FN_LEN, "%s/%s/%s.pat", dir, subdirs[n], s);

		if(c > MAX_FN_LEN)
			xtables_error(OTHER_PROBLEM, 
				"Filename beginning with %s is too long!\n", filename);

		/* read in the pattern from the file */
		if(parse_protocol_file(filename, s, info)){
			done = 1;
			break;
		}
		
		n++;
	}

	if(!done)
		xtables_error(OTHER_PROBLEM, 
			"Couldn't find a pattern definition file for %s.\n", s);

	/* process \xHH escapes and tolower everything. (our regex lib has no
	case insensitivity option.) */
	strncpy(info->pattern, pre_process(info->pattern), MAX_PATTERN_LEN);
}

/* Function which parses command options; returns true if it ate an option */
static int parse(int c, char **argv, int invert, unsigned int *flags,
      const void *entry, struct xt_entry_match **match)
{
	struct xt_layer7_info *layer7info = 
		(struct xt_layer7_info *)(*match)->data;

	switch (c) {
	case 'p':
		parse_layer7_protocol(argv[optind-1], layer7info);
		if (invert)
			layer7info->invert = true;
		*flags = 1;
		break;

	case 'd':
		if(strlen(argv[optind-1]) >= MAX_FN_LEN)
			xtables_error(PARAMETER_PROBLEM, "directory name too long\n");

		strncpy(l7dir, argv[optind-1], MAX_FN_LEN);

		*flags = 1;
		break;

	default:
		return 0;
	}

	return 1;
}

/* Final check; must have specified --l7proto */
static void final_check(unsigned int flags)
{
	if (!flags)
		xtables_error(PARAMETER_PROBLEM,
			   "LAYER7 match: You must specify `--l7proto'");
}

static void print_protocol(char s[], int invert, int numeric)
{
	fputs("l7proto ", stdout);
	if (invert) fputc('!', stdout);
	printf("%s ", s);
}

/* Prints out the matchinfo. */
static void print(const void *ip,
      const struct xt_entry_match *match,
      int numeric)
{
	printf("LAYER7 ");
	print_protocol(((struct xt_layer7_info *)match->data)->protocol,
		  ((struct xt_layer7_info *)match->data)->invert, numeric);
}
/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void save(const void *ip, const struct xt_entry_match *match)
{
        const struct xt_layer7_info *info =
            (const struct xt_layer7_info*) match->data;

        printf("--l7proto %s%s ", (info->invert)? "! ":"", info->protocol);
}

static struct xtables_match layer7 = { 
    .family        = AF_INET,
    .name          = "layer7",
    .version       = XTABLES_VERSION,
    .size          = XT_ALIGN(sizeof(struct xt_layer7_info)),
    .userspacesize = XT_ALIGN(sizeof(struct xt_layer7_info)),
    .help          = &help,
    .parse         = &parse,
    .final_check   = &final_check,
    .print         = &print,
    .save          = &save,
    .extra_opts    = opts
};

void _init(void)
{
	xtables_register_match(&layer7);
}
