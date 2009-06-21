#include "ipt_bwctl.h"

static uint32_t bandwidth_entry_length = sizeof(ip_bw);

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
	sprintf(buf, "%s %s", id, "ALL");
	while(!done && sockfd >= 0)
	{
		int buf_index;
		int size = BANDWIDTH_QUERY_LENGTH;
		

		getsockopt(sockfd, IPPROTO_IP, BANDWIDTH_GET, buf, &size);
		if(ret_length == 0)
		{
			ret_length =  *( (uint32_t*)(buf) );
			ret = (ip_bw*)malloc(ret_length);
		}
		for(buf_index=4; buf_index+bandwidth_entry_length < BANDWIDTH_QUERY_LENGTH && ret_index < ret_length; buf_index=buf_index+bandwidth_entry_length)
		{
			*( (ip_bw*)(ret + ret_index) )  = *( (ip_bw*)(buf + buf_index) );
			ret_index=ret_index+bandwidth_entry_length;
		}
		memset(buf, 0, BANDWIDTH_QUERY_LENGTH); //re-zero to indicate we're doing a follow-up query
		done = ret_index >= ret_length ? 1 : 0;
	}
	if(sockfd >= 0)
	{
		close(sockfd);
	}
	*num_ips = ret_length/bandwidth_entry_length;
	return ret;
}

void set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, ip_bw* data)
{
	uint32_t data_index = 0;
	uint32_t data_length = (uint32_t)(num_ips*bandwidth_entry_length);
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int sockfd;
	int done = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	while(!done && sockfd >= 0)
	{
		int buf_index = 0;
		int size = BANDWIDTH_QUERY_LENGTH;
		if(data_index == 0)
		{
			*( (uint32_t*)(buf) ) = data_length;
			buf_index = sizeof(uint32_t);
			sprintf( (buf + buf_index), "%s", id);
			buf_index = buf_index + BANDWIDTH_MAX_ID_LENGTH;
		}
		else
		{
			*( (uint32_t*)(buf) ) = 0;
			buf_index = sizeof(uint32_t);
		}
		
		for(buf_index; buf_index+bandwidth_entry_length < BANDWIDTH_QUERY_LENGTH && data_index < data_length; buf_index=buf_index+bandwidth_entry_length)
		{
			*( (ip_bw*)(buf + buf_index) )  = *( (ip_bw*)(data + data_index) );
			data_index=data_index+bandwidth_entry_length;
		}
		
		setsockopt(sockfd, IPPROTO_IP, BANDWIDTH_SET, buf, BANDWIDTH_QUERY_LENGTH);

		done = data_index >= data_length ? 1 : 0;
	}

}
