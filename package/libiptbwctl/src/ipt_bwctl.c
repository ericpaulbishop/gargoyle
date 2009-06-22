#include "ipt_bwctl.h"

static char bw_sem_name[] = "ipt_bandwidth_userspace_semaphore";

static ip_bw* get_all_bandwidth_usage(char* id, char* type, unsigned long* num_ips, unsigned long max_wait_milliseconds)
{
	sem_t *bw_sem = sem_open(bw_sem_name, O_CREAT);
	int got_lock = 0;
	
	ip_bw* ret = NULL;
	uint32_t ret_length = 0;
	uint32_t ret_index = 0;
	
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int sockfd;
	int done = 0;
	
	*num_ips = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
	struct timespec wait_time;
	wait_time.tv_sec = (long)(max_wait_milliseconds/1000);
	wait_time.tv_nsec = (max_wait_milliseconds % 1000 ) * 1000 *1000;


	
	
	
	
	sprintf((char*)buf, "%s %s", id, type);

	got_lock = sem_timedwait(bw_sem, &wait_time) == 0 ? 1 : 0;
	
	while(!done && sockfd >= 0 && got_lock)
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
	if(got_lock)
	{
		sem_post(bw_sem);
	}
	sem_close(bw_sem);
	return ret;
}


ip_bw* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips, unsigned long max_wait_milliseconds)
{
	return get_all_bandwidth_usage(id, "ALL", num_ips, max_wait_milliseconds);
}

ip_bw* get_ip_bandwidth_usage_for_rule_id(char* id, char* ip, unsigned long max_wait_milliseconds)
{
	unsigned long num_ips;
	return get_all_bandwidth_usage(id, ip, &num_ips, max_wait_milliseconds);
}


void set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data, unsigned long max_wait_milliseconds)
{
	uint32_t data_index = 0;
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int sockfd;
	int done = 0;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
	sem_t *bw_sem = sem_open(bw_sem_name, O_CREAT);
	int got_lock = 0;
	struct timespec wait_time;
	wait_time.tv_sec = (long)(max_wait_milliseconds/1000);
	wait_time.tv_nsec = (max_wait_milliseconds % 1000 ) * 1000 *1000;
	got_lock = sem_timedwait(bw_sem, &wait_time) == 0 ? 1 : 0;


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
		sem_post(bw_sem);
	}
	sem_close(bw_sem);
}

extern void unlock_bandwidth_semaphore(void)
{
	sem_t* bw_sem = sem_open(bw_sem_name, O_CREAT);
	sem_post(bw_sem);
	sem_close(bw_sem);
}

static void signal_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT )
	{
		unlock_bandwidth_semaphore();
		exit(0);
	}
}

extern void unlock_bandwidth_semaphore_on_exit(void)
{
	signal(SIGTERM,signal_handler);
	signal(SIGINT, signal_handler);
}

