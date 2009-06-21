#include "ipt_bwctl.h"

int main(int argc, char **argv)
{
	unsigned long num_ips;
	unsigned char *ip_buf;
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
		struct in_addr ip = *((struct in_addr*)(ip_buf + (out_index*BANDWIDTH_ENTRY_LENGTH) ));
		uint64_t bw = *((uint64_t*)(ip_buf + 4 + (out_index*BANDWIDTH_ENTRY_LENGTH) ));
		if(  *((uint32_t*)(&ip)) != 0 )
		{
			printf("%15s\t%lld\n", inet_ntoa(ip), (long long int)bw);
		}
		else
		{
			printf("%15s\t%lld\n", "COMBINED", (long long int)bw);
		}
	}
	if(num_ips == 0)
	{
		printf("No data available for id \"%s\"\n", id);
	}
	printf("\n");


	return 0;
}
