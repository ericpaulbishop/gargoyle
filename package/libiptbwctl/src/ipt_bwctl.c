#include "ipt_bwctl.h"

unsigned char* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips)
{
	unsigned char* ret = NULL;
	uint32_t ret_length = 0;
	uint32_t ret_index = 0;
	
	unsigned char buf[QUERY_BUFFER_LENGTH];
	int sockfd;
	int done = 0;
	
	*num_ips = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	sprintf(buf, "%s %s", id, "ALL");
	while(!done && sockfd >= 0)
	{
		int buf_index;
		int size = QUERY_BUFFER_LENGTH;
		

		getsockopt(sockfd, IPPROTO_IP, BANDWIDTH_GET, buf, &size);
		if(ret_length == 0)
		{
			ret_length =  *( (uint32_t*)(buf) );
			ret = (unsigned char*)malloc(ret_length);
		}
		for(buf_index=4; buf_index+BANDWIDTH_ENTRY_LENGTH < QUERY_BUFFER_LENGTH && ret_index < ret_length; buf_index=buf_index+BANDWIDTH_ENTRY_LENGTH)
		{
			uint32_t ip = *((uint32_t*)(buf + buf_index));
			uint64_t bw = *((uint64_t*)(buf + buf_index + 4));
			*( (uint32_t*)(ret + ret_index) ) = ip;
			*( (uint64_t*)(ret + ret_index + 4) ) = bw;

			ret_index=ret_index+BANDWIDTH_ENTRY_LENGTH;
		}
		memset(buf, 0, QUERY_BUFFER_LENGTH); //re-zero to indicate we're doing a follow-up query
		done = ret_index >= ret_length ? 1 : 0;
	}
	if(sockfd >= 0)
	{
		close(sockfd);
	}
	*num_ips = ret_length/BANDWIDTH_ENTRY_LENGTH;
	return ret;
}

