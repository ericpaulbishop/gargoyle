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

#include <getopt.h>
#include <sys/errno.h>
#include <dlfcn.h>


#include <erics_tools.h>
#include "libiptc/libiptc.h"


#define MSG_ID 12
#define REQUEST_MSG_TYPE 24
#define RESPONSE_MSG_TYPE 36
#define MAX_MSG_LINE 75
#define PID_PATH	"/var/run/bwmond.pid"



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
history_node* pop_history(bw_history* history);
int get_interval_length(bw_history* history);
void print_history(bw_history* history);



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

history_node* pop_history(bw_history* history)
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

void print_history(bw_history* history)
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
		t2 = t1+interval_length;
	}
	printf("\n\n\n");
}

#endif //BWMON_H
