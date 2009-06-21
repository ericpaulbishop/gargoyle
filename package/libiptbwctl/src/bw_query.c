#include "ipt_bwctl.h"

static uint32_t bandwidth_entry_length = sizeof(ip_bw);

int main(int argc, char **argv)
{
	unsigned long num_ips;
	ip_bw *ip_buf;
	char id[50];
	unsigned long out_index;

	if(argc > 1)
	{
		sprintf(id, "%s", argv[1]);
	}
	else
	{
		sprintf(id, "%s", "id_1");
	}
	ip_buf = get_all_bandwidth_usage_for_rule_id(id, &num_ips);

	for(out_index=0; out_index < num_ips; out_index++)
	{
		ip_bw next = ip_buf[out_index];
		if(  *((uint32_t*)(&(next.ip))) != 0 )
		{
			printf("%15s\t%lld\n", inet_ntoa(next.ip), (long long int)next.bw);
		}
		else
		{
			printf("%15s\t%lld\n", "COMBINED", (long long int)next.bw);
		}
	}
	if(num_ips == 0)
	{
		printf("No data available for id \"%s\"\n", id);
	}
	printf("\n");


	return 0;
}
