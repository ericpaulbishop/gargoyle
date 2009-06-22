#include "ipt_bwctl.h"


ip_bw* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips)
{
	ip_bw* ret = NULL;
	uint32_t ret_length = 0;
	uint32_t ret_index = 0;
	
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int sockfd;
	int done = 0;
	
	*num_ips = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	sprintf((char*)buf, "%s %s", id, "ALL");


	while(!done && sockfd >= 0)
	{
		int buf_index;
		uint32_t size = BANDWIDTH_QUERY_LENGTH;

		getsockopt(sockfd, IPPROTO_IP, BANDWIDTH_GET, buf, &size);
		if(ret_length == 0)
		{
			ret_length =  *( (uint32_t*)(buf) );
			*num_ips = ret_length/BANDWIDTH_ENTRY_LENGTH;
			ret = (ip_bw*)malloc( (*num_ips)*sizeof(ip_bw) );
			
			/* printf("ret length = %d, num ips = %ld\n", ret_length, *num_ips); */
		}
		for(buf_index=4; buf_index+BANDWIDTH_ENTRY_LENGTH < BANDWIDTH_QUERY_LENGTH && ret_index < *num_ips; buf_index=buf_index+BANDWIDTH_ENTRY_LENGTH)
		{
			(ret[ret_index]).ip  = *( (uint32_t*)(buf + buf_index) );
			(ret[ret_index]).bw  = *( (uint64_t*)(buf + 4 + buf_index) );
			ret_index++;
		}
		memset(buf, 0, BANDWIDTH_QUERY_LENGTH); /* re-zero to indicate we're doing a follow-up query */
		done = ret_index >= *num_ips ? 1 : 0;
	}
	if(sockfd >= 0)
	{
		close(sockfd);
	}
	return ret;
}

void set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data)
{
	uint32_t data_index = 0;
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int sockfd;
	int done = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	while(!done && sockfd >= 0)
	{
		int buf_index = 0;
		if(data_index == 0)
		{
			*( (uint32_t*)(buf) ) = (uint32_t)(num_ips*BANDWIDTH_ENTRY_LENGTH);
			buf_index = 4;
			sprintf( (char*)(buf + buf_index), "%s", id);
			buf_index = buf_index + BANDWIDTH_MAX_ID_LENGTH;
			*( (time_t*)(buf + buf_index) ) = last_backup;
			buf_index = buf_index + sizeof(time_t);
		}
		else
		{
			*( (uint32_t*)(buf) ) = 0;
			buf_index = 4;
		}
		
		for( ; buf_index+BANDWIDTH_ENTRY_LENGTH < BANDWIDTH_QUERY_LENGTH && data_index < num_ips; buf_index=buf_index+BANDWIDTH_ENTRY_LENGTH)
		{
			*( (uint32_t*)(buf + buf_index) ) = (data[data_index]).ip;
			*( (uint64_t*)(buf + 4 + buf_index) ) = (data[data_index]).bw;
			data_index++;
		}
		
		setsockopt(sockfd, IPPROTO_IP, BANDWIDTH_SET, buf, BANDWIDTH_QUERY_LENGTH);

		done = data_index >= num_ips ? 1 : 0;
	}

}
