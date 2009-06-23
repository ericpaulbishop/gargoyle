#include "ipt_bwctl.h"


int main(int argc, char **argv)
{
	unsigned long num_ips;
	char id[BANDWIDTH_MAX_ID_LENGTH];

	unlock_bandwidth_semaphore_on_exit();

	if(argc > 1)
	{
		sprintf(id, "%s", argv[1]);
	}
	else
	{
		sprintf(id, "%s", "id_1");
	}
	
	num_ips = argc -2;
	if(num_ips > 0)
	{
       		ip_bw* data = (ip_bw*)malloc(BANDWIDTH_ENTRY_LENGTH*num_ips);
		int arg_index = 2;
		int data_index = 0;
		while(arg_index < argc)
		{
			ip_bw next;
			struct in_addr ipaddr;
			int valid = inet_aton(argv[arg_index], &ipaddr);
			arg_index++;
			
			if(valid && arg_index < argc)
			{
				next.ip = ipaddr.s_addr;
				valid = sscanf(argv[arg_index], "%lld", (long long int*)&(next.bw) );
			}
			else
			{
				valid = 0;
			}
			arg_index++;

			if(valid)
			{
				/* printf("ip=%d, bw=%lld\n", next.ip, (long long int)next.bw); */
				data[data_index] = next;
				data_index++;
			}	
		}
		num_ips = data_index; /* number that were successfully read */
		if(num_ips > 0)
		{
			set_bandwidth_usage_for_rule_id(id, num_ips, 0, data);
		}
	}
	

	return 0;
}
