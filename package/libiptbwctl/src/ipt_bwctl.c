#include "ipt_bwctl.h"

static int bandwidth_semaphore = -1;


static union semun 
{
	int val; /* Value for SETVAL */
	struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */
	unsigned short *array; /* Array for GETALL, SETALL */
	struct seminfo *__buf; /* Buffer for IPC_INFO (Linux specific) */
};


static unsigned short get_sem_member_count(int sid)
{
        union semun semopts;
        struct semid_ds mysemds;

        semopts.buf = &mysemds;

        /* Return number of members in the semaphore set */
        return(semopts.buf->sem_nsems);
}
static int get_sem_val(int sid, int member)
{
        int semval;
        semval = semctl(sid, member, GETVAL, 0);
        return(semval);
}

static int get_sem(int *sid, key_t key)
{
        int cntr;
        union semun semopts;
	int members = 1;


	
	int success = ((*sid = semget(key, members, IPC_CREAT|IPC_EXCL|0666))== -1) ? 0 : 1;
	if(success)
	{
		semopts.val = 1;
        	/* Initialize all members (could be done with SETALL) */        
        	for(cntr=0; cntr<members; cntr++)
		{
			semctl(*sid, cntr, SETVAL, semopts);
		}
	}
	else
	{
		success = ((*sid = semget(key, 0, 0666)) == -1) ? 0 : 1;
	}
	return success;
}


static int lock_sem(int sid)
{
	int member = 0;
        struct sembuf sem_lock={ 0, -1, IPC_NOWAIT};
	int success = 0;

	if(member >= 0 && member < (get_sem_member_count(sid)-1))
	{
        	/* Attempt to lock the semaphore set */
        	if(get_sem_val(sid, member))
        	{
        		sem_lock.sem_num = member;
        		if((semop(sid, &sem_lock, 1)) != -1)
			{
				success = 1;
			}
		}
	}
	return success;
}

static int unlock_sem(int sid)
{
	int member = 0;
        struct sembuf sem_unlock={ member, 1, IPC_NOWAIT};
        int semval;
	int success = 0; 
	/* will fail if we can't get semaphore or for some reason we can't unlock it, 
	 * will NOT fail if semaphore is already unlocked
	 */

	if(member >= 0 && member < (get_sem_member_count(sid)-1))
	{
		success = 1;

		/* Is the semaphore set locked? */
		semval = get_sem_val(sid, member);
		if(semval == 0)
		{
			/* it's locked, unlock it */
			sem_unlock.sem_num = member;
        		success = ((semop(sid, &sem_unlock, 1)) == -1) ? 0 : 1;
		}
	}
	return success;
}



static int lock(void)
{
	int locked = 0;
	if(bandwidth_semaphore == -1)
	{
		get_sem(&bandwidth_semaphore, (key_t)(BANDWIDTH_SEMAPHORE_KEY) );
	}
	if(bandwidth_semaphore != -1)
	{
		locked = lock_sem(bandwidth_semaphore);
	}
	return locked;
}

static int unlock(void)
{
	int unlocked = 0;
	if(bandwidth_semaphore == -1)
	{
		get_sem(&bandwidth_semaphore, (key_t)(BANDWIDTH_SEMAPHORE_KEY) );
	}
	if(bandwidth_semaphore != -1)
	{
		unlocked = unlock_sem(bandwidth_semaphore);
	}
	return unlocked;
	
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

