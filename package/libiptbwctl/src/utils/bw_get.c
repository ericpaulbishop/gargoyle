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
#define malloc ipt_bwctl_safe_malloc
#define strdup ipt_bwctl_safe_strdup


int main(int argc, char **argv)
{
	char *id = NULL;
	char* out_file_path = NULL;;
	FILE* out_file = NULL;
	char *address = NULL;

	unsigned long num_ips;
	void *ip_buf;
	unsigned long out_index;
	int query_succeeded;
	int get_history = 0;
	int machine_time = 0;

	int c;
	struct in_addr read_addr;
	while((c = getopt(argc, argv, "i:I:a:A:f:F:hHmMuU")) != -1)
	{	
		switch(c)
		{
			case 'i':
			case 'I':
				if(strlen(optarg) < BANDWIDTH_MAX_ID_LENGTH && strlen(optarg) > 0)
				{
					id = strdup(optarg);
				}
				else
				{
					fprintf(stderr, "ERROR: ID length is improper length.\n");
					exit(0);
				}
				break;
			case 'a':
			case 'A':
				if(strcmp(optarg, "combined") == 0 || strcmp(optarg, "COMBINED") == 0)
				{
					address = strdup("0.0.0.0");
				}
				else if( inet_aton(optarg, &read_addr) )
				{
					address = strdup(optarg);
				}
				else
				{
					fprintf(stderr, "ERROR: invalid IP address specified\n");
					exit(0);
				}

				break;
			case 'f':
			case 'F':
				out_file_path = strdup(optarg);
				out_file = fopen(out_file_path, "w");
				if(out_file == NULL)
				{
					fprintf(stderr, "ERROR: cannot open specified file for writing\n");
					exit(0);
				}
				fclose (out_file);
				break;
			case 'h':
			case 'H':
				get_history = 1;
				break;
			case 'm':
			case 'M':
				machine_time = 1;
				break;
			case 'u':
			case 'U':
			default:
				fprintf(stderr, "USAGE:\n\t%s -i [ID] -a [IP ADDRESS] -f [OUT_FILE_NAME]\n", argv[0]);
				exit(0);
		}
	}


	if(id == NULL)
	{
		fprintf(stderr, "ERROR: you must specify an id to query\n\n");
		exit(0);
	}
	
	set_kernel_timezone();	
	unlock_bandwidth_semaphore_on_exit();
	
	if(get_history == 0)
	{
		if(address == NULL)
		{
			query_succeeded = get_all_bandwidth_usage_for_rule_id(id, &num_ips, (ip_bw**)&ip_buf, 1000);
		}
		else
		{
			num_ips = 1;
			query_succeeded = get_ip_bandwidth_usage_for_rule_id(id, address, (ip_bw**)&ip_buf, 1000);
		}
	}
	else
	{
		if(address == NULL)
		{
			query_succeeded = get_all_bandwidth_history_for_rule_id(id, &num_ips, (ip_bw_history**)&ip_buf, 1000);
		}
		else
		{
			num_ips = 1;
			query_succeeded = get_ip_bandwidth_history_for_rule_id(id, address, (ip_bw_history**)&ip_buf, 1000);
		}
	}
	if(!query_succeeded)
	{
		fprintf(stderr, "ERROR: bandwidth module does not allow simultaneous queries.  Please try again.\n\n");
		exit(0);
	}


	if(out_file_path != NULL)
	{
		if(get_history == 0)
		{
			save_usage_to_file( (ip_bw*)ip_buf, num_ips, out_file_path);
		}
		else
		{
			save_history_to_file( (ip_bw_history*)ip_buf, num_ips, out_file_path);
		}
	}
	else
	{
		if(get_history == 0)
		{
			print_usage(stdout, (ip_bw*)ip_buf, num_ips);
		}
		else
		{
			print_histories(stdout, (ip_bw_history*)ip_buf, num_ips, !machine_time );
		}
	}
	if(num_ips == 0)
	{
		fprintf(stderr, "No data available for id \"%s\"\n", id);
	}
	printf("\n");

	if(out_file_path != NULL)
	{
		free(out_file_path);
	}

	return 0;
}
