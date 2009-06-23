#include "ipt_bwctl.h"

static char bw_mutex_file_name[] = "/tmp/ipt_bandwidth_userspace_mutex";
static int bw_mutex_file = -1;

static int lock(void)
{
	int locked = 0;
	bw_mutex_file = open(bw_mutex_file_name,O_RDWR|O_CREAT,0644);
	if(bw_mutex_file >= 0)
	{
		locked = lockf(bw_mutex_file,F_TLOCK,0)<0 ? 0 : 1;
		if(!locked)
		{
			close(bw_mutex_file);
			bw_mutex_file = -1;
		}
	}
	return locked;
}

static int unlock(void)
{
	int success = 0;
	if(bw_mutex_file >=0)
	{
		success = lockf(bw_mutex_file,F_ULOCK,0) < 0 ? 0 : 1;
		close(bw_mutex_file);
		bw_mutex_file = -1;
	}
	return success;
}



static int get_all_bandwidth_usage(char* id, char* type, unsigned long* num_ips, ip_bw** data)
{	
	uint32_t ret_length = 0;
	uint32_t ret_index = 0;
	
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int done = 0;
	
	ip_bw *ret = *data = NULL;
	*num_ips = 0;


	int got_lock = lock();
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	
	sprintf((char*)buf, "%s %s", id, type);


	while(!done && sockfd >= 0 && got_lock)
	{
		int buf_index;
		uint32_t size = BANDWIDTH_QUERY_LENGTH;

		getsockopt(sockfd, IPPROTO_IP, BANDWIDTH_GET, buf, &size);
		if(ret_length == 0)
		{
			ret_length =  *( (uint32_t*)(buf) );
			*num_ips = ret_length/BANDWIDTH_ENTRY_LENGTH;
			*data = (ip_bw*)malloc( (*num_ips)*sizeof(ip_bw) );
			ret = *data;
			
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
	if(got_lock)
	{
		unlock();
	}
	return got_lock;
}


int get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips, ip_bw** data)
{
	return get_all_bandwidth_usage(id, "ALL", num_ips, data);
}

int get_ip_bandwidth_usage_for_rule_id(char* id, char* ip, ip_bw** data)
{
	unsigned long num_ips;
	return get_all_bandwidth_usage(id, ip, &num_ips, data);
}


int set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data)
{
	uint32_t data_index = 0;
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int done = 0;

		
	int got_lock = lock();
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	while(!done && sockfd >= 0 && got_lock)
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
	if(sockfd >= 0)
	{
		close(sockfd);
	}
	if(got_lock)
	{
		unlock();
	}
	return got_lock;
}

void unlock_bandwidth_semaphore(void)
{
	unlock();
}

void signal_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT )
	{
		unlock_bandwidth_semaphore();
		exit(0);
	}
}

void unlock_bandwidth_semaphore_on_exit(void)
{
	signal(SIGTERM,signal_handler);
	signal(SIGINT, signal_handler);
}

