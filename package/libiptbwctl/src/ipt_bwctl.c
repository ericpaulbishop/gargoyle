/*  libiptbwctl --	A userspace library for querying the bandwidth iptables module
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009 by Eric Bishop <eric@gargoyle-router.com>
 *
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "ipt_bwctl.h"
#define malloc ipt_bwctl_safe_malloc



static int bandwidth_semaphore = -1;

union semun 
{
	int val; /* Value for SETVAL */
	struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */
	unsigned short *array; /* Array for GETALL, SETALL */
	struct seminfo *__buf; /* Buffer for IPC_INFO (Linux specific) */
};



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

	
	int success = ((*sid = semget(key, members, IPC_CREAT|IPC_EXCL|0777))== -1) ? 0 : 1;
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
		success = ((*sid = semget(key, members, 0777)) == -1) ? 0 : 1;
	}
	return success;
}


static int lock_sem(int sid)
{
	int member = 0;
        struct sembuf sem_lock={ 0, -1, IPC_NOWAIT};
	int success = 0;
	
	//printf("locking sem, member count = %d\n", get_sem_member_count(sid)  );

       	/* Attempt to lock the semaphore set */
	int semval = get_sem_val(sid, member);
	if(semval > 0)
       	{
       		sem_lock.sem_num = member;
       		if((semop(sid, &sem_lock, 1)) != -1)
		{
			success = 1;
		}
	}

	return success;
}

static int unlock_sem(int sid)
{
	int member = 0;
        struct sembuf sem_unlock={ member, 1, IPC_NOWAIT};
	
	/* will fail if we can't can't unlock semaphore for some reason, 
	 * will NOT fail if semaphore is already unlocked
	 */
	int success = 1; 


	/* Is the semaphore set locked? */
	int semval = get_sem_val(sid, member);
	if(semval == 0)
	{
		/* it's locked, unlock it */
		sem_unlock.sem_num = member;
        	success = ((semop(sid, &sem_unlock, 1)) == -1) ? 0 : 1;
	}
	return success;
}



static int lock(unsigned long max_wait_milliseconds)
{
	int locked = 0;
	if(bandwidth_semaphore == -1)
	{
		get_sem(&bandwidth_semaphore, (key_t)(BANDWIDTH_SEMAPHORE_KEY) );
	}
	if(bandwidth_semaphore != -1)
	{
		do
		{
			locked = lock_sem(bandwidth_semaphore);
			if(locked == 0 && max_wait_milliseconds > 25)
			{
				usleep(1000*25);
			}
			max_wait_milliseconds = max_wait_milliseconds > 25 ? max_wait_milliseconds - 25 : 0;
		} while(locked == 0 && max_wait_milliseconds > 0);
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



static int get_all_bandwidth_usage(char* id, char* type, unsigned long* num_ips, ip_bw** data, unsigned long max_wait_milliseconds)
{	
	uint32_t ret_length = 0;
	uint32_t ret_index = 0;
	
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int done = 0;
	
	ip_bw *ret = *data = NULL;
	*num_ips = 0;


	int got_lock = lock(max_wait_milliseconds);
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


int get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips, ip_bw** data, unsigned long max_wait_milliseconds)
{
	return get_all_bandwidth_usage(id, "ALL", num_ips, data, max_wait_milliseconds);
}

int get_ip_bandwidth_usage_for_rule_id(char* id, char* ip, ip_bw** data, unsigned long max_wait_milliseconds)
{
	unsigned long num_ips;
	return get_all_bandwidth_usage(id, ip, &num_ips, data, max_wait_milliseconds);
}


int set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data, unsigned long max_wait_milliseconds)
{
	uint32_t data_index = 0;
	unsigned char buf[BANDWIDTH_QUERY_LENGTH];
	int done = 0;

		
	int got_lock = lock(max_wait_milliseconds);
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

void set_kernel_timezone(void)
{
	time_t now;
	struct tm* utc_info;
	struct tm* tz_info;
	int utc_day;
	int utc_hour;
	int utc_minute;
	int tz_day;
	int tz_hour;
	int tz_minute;
	int minuteswest;

	struct timeval tv;
	struct timezone old_tz;
	struct timezone new_tz;

	time(&now);
	utc_info = gmtime(&now);
	utc_day = utc_info->tm_mday;
	utc_hour = utc_info->tm_hour;
	utc_minute = utc_info->tm_min;
	tz_info = localtime(&now);
	tz_day = tz_info->tm_mday;
	tz_hour = tz_info->tm_hour;
	tz_minute = tz_info->tm_min;

	utc_day = utc_day < tz_day  - 1 ? tz_day  + 1 : utc_day;
	tz_day =  tz_day  < utc_day - 1 ? utc_day + 1 : tz_day;
	
	minuteswest = (24*60*utc_day + 60*utc_hour + utc_minute) - (24*60*tz_day + 60*tz_hour + tz_minute) ;
	new_tz.tz_minuteswest = minuteswest;
	new_tz.tz_dsttime = 0;

	/* Get tv to pass to settimeofday(2) to be sure we avoid hour-sized warp */
	/* (see gettimeofday(2) man page, or /usr/src/linux/kernel/time.c) */
	gettimeofday(&tv, &old_tz);

	/* set timezone */
	settimeofday(&tv, &new_tz);

}
