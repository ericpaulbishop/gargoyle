/*  bwmond / bw-stats -	A small bandwidth monitoring utility for linux that uses
 *  			iptables rules to monitor bandwidth usage (useful for monitoring QoS)
 *  			Originally created for Gargoyle Router Management Utility
 * 		
 * 			Created By Eric Bishop 
 * 			http://www.gargoyle-router.com
 * 		  
 *
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
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

#ifndef BWMON_H
#define BWMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

#include <getopt.h>
#include <sys/errno.h>
#include <dlfcn.h>


#include "erics_tools.h"


#define MSG_ID 12
#define REQUEST_MSG_TYPE 24
#define RESPONSE_MSG_TYPE 36
#define MAX_MSG_LINE 75
#define PID_PATH	"/var/run/bwmond.pid"

#define MINUTE	101
#define HOUR	102
#define DAY	103
#define WEEK	104
#define	MONTH	105
#define FIXED_INTERVAL   -1

typedef struct
{
	long int msg_type;
	char msg_line[MAX_MSG_LINE];
} message_t;



typedef struct 
{
	void* next;
	void* previous;
	int64_t bandwidth;
} history_node;

typedef struct
{
	history_node* first;
	history_node* last;
	int length;
	time_t oldest_interval_start;
	time_t oldest_interval_end;
	time_t recent_interval_end;
	
} bw_history;


bw_history* initialize_history(void);
void push_history(bw_history* history, history_node* new_node, time_t interval_start, time_t interval_end);
history_node* shift_history(bw_history* history);
int get_interval_length(bw_history* history);
time_t get_next_interval_end(time_t current_time, int end_type);
void print_history(bw_history* history, int interval_end);
int get_next_message(int queue, void* message_data, size_t message_size, long message_type, unsigned long max_wait_milliseconds);
int send_next_message(int queue, void* message_data, size_t message_size, unsigned long max_wait_milliseconds);


bw_history* initialize_history(void)
{
	bw_history* history = (bw_history*)malloc(sizeof(bw_history));
	history->first = NULL;
	history->last = NULL;
	history->length = 0;

	history->oldest_interval_start = 0;
	history->oldest_interval_end = 0;
	history->recent_interval_end = 0;
	return history;
}

void push_history(bw_history* history, history_node* new_node, time_t interval_start, time_t interval_end)
{
	new_node->next = history->first;
	new_node->previous = NULL;
	
	if(history->first != NULL) //history length > 0
	{
		history->first->previous = new_node;
	}
	else //history length == 0
	{
		history->last = new_node;
		history->oldest_interval_start = interval_start;
		history->oldest_interval_end = interval_end;
	}
	history->first = new_node;
	history->recent_interval_end = interval_end;
	history->length = history->length+1;
	
}

history_node* shift_history(bw_history* history)
{
	history_node* old_node = history->last;
	if(history->length > 1)
	{
		history->oldest_interval_start = history->oldest_interval_end;
		history->oldest_interval_end = history->oldest_interval_start+get_interval_length(history);
	}
	else
	{
		history->oldest_interval_start = 0;
		history->oldest_interval_end = 0;
		history->recent_interval_end = 0;
	}
	if(old_node != NULL)
	{
		history->last = old_node->previous;
		if(history->first == old_node) //old_node is only node in history
		{
			history->first = NULL;
		}
		else // old_node is not the only node in history
		{
			history->last->next = NULL;
		}
		old_node->previous = NULL;
		history->length = history->length-1;
	}
	return old_node;
}

int get_interval_length(bw_history* history)
{
	int interval_length;
	if(history->length > 1)
	{
		interval_length = (history->recent_interval_end - history->oldest_interval_end)/(history->length-1);
	}
	else if(history->length == 1)
	{
		interval_length = (history->oldest_interval_end-history->oldest_interval_start);
	}
	else
	{
		interval_length = 0;
	}
	return interval_length;
}

#ifndef BWSIM
time_t get_next_interval_end(time_t current_time, int end_type)
{
	time_t next_end;
	time_t ct = current_time;
	struct tm* curr = localtime(&ct);
	if(end_type == MINUTE)
	{
		curr->tm_sec = 0;
		curr->tm_min = curr->tm_min+1;
		next_end = mktime(curr);
	}
	else if(end_type == HOUR)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = curr->tm_hour+1;
		next_end = mktime(curr);
	}
	else if(end_type == DAY)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = 0;
		curr->tm_mday = curr->tm_mday+1;
		next_end = mktime(curr);
	}
	else if(end_type == WEEK)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = 0;
		curr->tm_mday = curr->tm_mday+1;
		time_t tmp = mktime(curr);
		curr = localtime(&tmp);
		while(curr->tm_wday != 0)
		{
			curr->tm_mday=curr->tm_mday+1;
			tmp = mktime(curr);
			curr = localtime(&tmp);
		}
		next_end = mktime(curr);
	}
	else if(end_type == MONTH)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = 0;
		curr->tm_mday = 1;
		curr->tm_mon  = curr->tm_mon+1;
		next_end = mktime(curr);
	}
	return next_end;
}



void print_history(bw_history* history, int interval_end)
{
	history_node* next_node = history->last;
	if(next_node == NULL)
	{
		printf("history is blank\n");
	}

	time_t t1 = history->oldest_interval_start;
	time_t t2 = history->oldest_interval_end;
	int interval_length = get_interval_length(history);

	/*
	printf("interval length = %d\n", interval_length);
	printf("recent end = %d\n", history->recent_interval_end);
	printf("old end = %d\n", history->oldest_interval_end);
	printf("history length = %d\n", history->length);
	*/

	while(next_node != NULL)
	{
		struct tm* detailed_time1 = localtime(&t1);
		char* start_time_str = strdup(asctime(detailed_time1));

		struct tm* detailed_time2 = localtime(&t2);
		char* end_time_str = strdup(asctime(detailed_time2));

		
		int64_t b = next_node->bandwidth;
		char byte_str[50];
		sprintf(byte_str, "%lld.0 \n", b);
		double kb;
		sscanf(byte_str, "%lf\n", &kb);
		kb=kb/1024.0;
		sprintf(byte_str, "%.3lf kb", kb);
		

		printf("%-20s%s\t%s\n", byte_str, trim_flanking_whitespace(start_time_str), trim_flanking_whitespace(end_time_str));


		free(start_time_str);
		free(end_time_str);

		next_node = next_node->previous;
		t1 = t2;
		t2 = (interval_end == FIXED_INTERVAL) ? t2+interval_length : get_next_interval_end(t2, interval_end);
	}
	printf("\n\n\n");
}
#endif



int get_next_message(int queue, void* message_data, size_t message_size, long message_type, unsigned long max_wait_milliseconds)
{
	int iteration = 0;
	int got_data = -1;
	while(iteration*25 < max_wait_milliseconds  && got_data < 0)
	{
		if(iteration > 0)
		{
			usleep(25*1000); //wait 25 milliseconds & try again
		}
		got_data = msgrcv(queue, message_data, message_size, message_type, IPC_NOWAIT);
		iteration++;
	}
	return got_data;
}
int send_next_message(int queue, void* message_data, size_t message_size, unsigned long max_wait_milliseconds)
{
	int iteration = 0;
	int sent = -1;
	while(iteration*25 < max_wait_milliseconds  && sent < 0)
	{
		if(iteration > 0)
		{
			usleep(25*1000); //wait 25 milliseconds & try again
		}
		sent = msgsnd(queue, message_data, message_size, IPC_NOWAIT);
		iteration++;
	}
	return sent;
}


#endif //BWMON_H
