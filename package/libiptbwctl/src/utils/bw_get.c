#include <ipt_bwctl.h>


int main(int argc, char **argv)
{
	char *id = NULL;
	FILE* out_file = NULL;;
	char *address = NULL;

	unsigned long num_ips;
	ip_bw *ip_buf;
	unsigned long out_index;
	int query_succeeded;

	int c;
	struct in_addr read_addr;
	while((c = getopt(argc, argv, "i:I:a:A:f:F:uUhH")) != -1)
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
				out_file = fopen(optarg, "w");
				if(out_file == NULL)
				{
					fprintf(stderr, "ERROR: cannot open specified file for writing\n");
					exit(0);
				}
				break;
			case 'u':
			case 'U':
			case 'h':
			case 'H':
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
	if(address == NULL)
	{
		query_succeeded = get_all_bandwidth_usage_for_rule_id(id, &num_ips, &ip_buf, 1000);
	}
	else
	{
		num_ips = 1;
		query_succeeded = get_ip_bandwidth_usage_for_rule_id(id, address, &ip_buf, 1000);
	}
	if(!query_succeeded)
	{
		fprintf(stderr, "ERROR: bandwidth module does not allow simultaneous queries.  Please try again.\n\n");
		exit(0);
	}

	/* printf("num ips = %ld\n", num_ips); */

	time_t now;
	time(&now);
	fprintf(out_file, "%-15ld\n", now);


	for(out_index=0; out_index < num_ips; out_index++)
	{
		ip_bw next = ip_buf[out_index];
		if(  next.ip != 0 )
		{
			struct in_addr ipaddr;
			ipaddr.s_addr = next.ip;
			if(out_file == NULL)
			{
				printf("%-15s\t%lld\n", inet_ntoa(ipaddr), (long long int)next.bw);
			}
			else
			{
				fprintf(out_file, "%-15s\t%lld\n", inet_ntoa(ipaddr), (long long int)next.bw);

			}
		}
		else
		{
			if(out_file == NULL)
			{
				printf("%-15s\t%lld\n", "COMBINED", (long long int)next.bw);
			}
			else
			{
				fprintf(out_file, "%-15s\t%lld\n", "COMBINED", (long long int)next.bw);

			}
		}
	}
	if(num_ips == 0)
	{
		fprintf(stderr, "No data available for id \"%s\"\n", id);
	}
	if(out_file != NULL)
	{
		fclose(out_file);
	}
	printf("\n");

	return 0;
}
