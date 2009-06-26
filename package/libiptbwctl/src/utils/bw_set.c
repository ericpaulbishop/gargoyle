/*  libiptbwctl --	A userspace library for querying the bandwidth iptables module
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

#include <ipt_bwctl.h>

static char* read_entire_file(FILE* in, int read_block_size);
static char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max, unsigned long *pieces_read);

int main(int argc, char **argv)
{
	char *id = NULL;
	FILE* in_file = NULL;
	time_t last_backup = 0;
	int last_backup_from_cl = 0;


	int c;
	while((c = getopt(argc, argv, "i:I:b:B:f:F:UuHh")) != -1)
	{	
		switch(c)
		{
			case 'i':
			case 'I':
				if(strlen(optarg) < BANDWIDTH_MAX_ID_LENGTH)
				{
					id = strdup(optarg);
				}
				else
				{
					fprintf(stderr, "ERROR: ID length is improper length.\n");
					exit(0);
				}

				break;
			case 'b':
			case 'B':
				if(sscanf(optarg, "%ld", &last_backup) == 0)
				{
					fprintf(stderr, "ERROR: invalid backup time specified. Should be unix epoch seconds -- number of seconds since 1970 (UTC)\n");
					exit(0);
				}
				last_backup_from_cl = 1;
				break;
			case 'f':
			case 'F':
				in_file = fopen(optarg, "r");
				if(in_file == NULL)
				{
					fprintf(stderr, "ERROR: cannot open specified file for reading\n");
					exit(0);
				}
				break;
			case 'u':
			case 'U':
			case 'h':
			case 'H':
			default:
				fprintf(stderr, "USAGE:\n\t%s -i [ID] -b [LAST_BACKUP_TIME] -f [IN_FILE_NAME] [ IP BANDWIDTH PAIRS, IF -f NOT SPECIFIED ]\n", argv[0]);
				exit(0);

		}
	}

	if(id == NULL)
	{
		fprintf(stderr, "ERROR: you must specify an id for which to set data\n\n");
		exit(0);
	}

	char** data_parts;
	unsigned long num_data_parts;
	if(in_file != NULL)
	{
		char* file_data = read_entire_file(in_file, 4086);
		char whitespace[] =  {'\n', '\r', '\t', ' '};
		data_parts = split_on_separators(file_data, whitespace, 4, -1, 0, &num_data_parts);
		free(file_data);
	}
	else
	{
		data_parts = argv+optind;
		num_data_parts = argc - optind;
	}
	
	
	unsigned long num_ips = num_data_parts/2;
       	ip_bw* buffer = (ip_bw*)malloc(BANDWIDTH_ENTRY_LENGTH*num_ips);
	unsigned long data_index = 0;
	unsigned long buffer_index = 0;
	while(data_index < num_data_parts)
	{
		ip_bw next;
		struct in_addr ipaddr;
		int valid = inet_aton(data_parts[data_index], &ipaddr);
		if((!valid) && (!last_backup_from_cl))
		{
			sscanf(data_parts[data_index], "%ld", &last_backup);
		}
		data_index++;

		if(valid && data_index < num_data_parts)
		{
			next.ip = ipaddr.s_addr;
			valid = sscanf(data_parts[data_index], "%lld", (long long int*)&(next.bw) );
			data_index++;
		}
		else
		{
			valid = 0;
		}

		if(valid)
		{
			/* printf("ip=%d, bw=%lld\n", next.ip, (long long int)next.bw); */
			buffer[buffer_index] = next;
			buffer_index++;
		}
	}
	set_kernel_timezone();
	unlock_bandwidth_semaphore_on_exit();
	num_ips = buffer_index; /* number that were successfully read */
	int query_succeeded = set_bandwidth_usage_for_rule_id(id, num_ips, last_backup, buffer, 1000);
	if(!query_succeeded)
	{
		fprintf(stderr, "ERROR: Could not set data. Please try again.\n\n");
	}
	else
	{
		fprintf(stderr, "Data set successfully\n\n");
	}

	return 0;
}


static char* read_entire_file(FILE* in, int read_block_size)
{
	int max_read_size = read_block_size;
	char* read_string = (char*)malloc(max_read_size+1);
	int bytes_read = 0;
	int end_found = 0;
	while(end_found == 0)
	{
		int nextch = '?';
		while(nextch != EOF && bytes_read < max_read_size)
		{
			nextch = fgetc(in);
			if(nextch != EOF)
			{
				read_string[bytes_read] = (char)nextch;
				bytes_read++;
			}
		}
		read_string[bytes_read] = '\0';
		end_found = (nextch == EOF) ? 1 : 0;
		if(end_found == 0)
		{
			char *new_str;
			max_read_size = max_read_size + read_block_size;
		       	new_str = (char*)malloc(max_read_size+1);
			memcpy(new_str, read_string, bytes_read);
			free(read_string);
			read_string = new_str;
		}
	}
	return read_string;
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
static char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max, unsigned long *pieces_read)
{
	char** split;
	*pieces_read = 0;

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
				*pieces_read = split_index;
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

