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


#include "bwmon.h"

void signal_handler(int sig);

// message queue is a global variable so
// that if we get a term signal we can make sure
// it gets destroyed properly
int mq;

int main( int argc, char** argv )
{
	//set signal handlers
	mq = -1;
	signal(SIGTERM,signal_handler);
	signal(SIGINT, signal_handler);
	

	char **monitor_names = (char**)malloc(1*sizeof(char*));
	monitor_names[0] = NULL;

	char format = 'm';
	int c;
	while((c = getopt(argc, argv, "HhMmTtUu")) != -1)
	{
		switch(c)
		{
			case 'H':
			case 'h':
				format = 'h'; //human readable
				break;
			case 'M':
			case 'm':
				format = 'm'; //machine readable (most simplistic, minimizes output length)
				break;
			case 'T':
			case 't':
				format = 't'; //table of comma separated values, on each line: monitor_name,start,end,bandwidth
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS] [MONITOR NAMES]\n", argv[0]);
				printf("\t-h display output in human readable format\n");
				printf("\t-m display output in minimalistic format\n");
				printf("\t-u print usage and exit\n");
				return 0;

		}		
	}
	if(optind < argc)
	{
		free(monitor_names);
		int monitor_length = argc-optind;
		monitor_names = (char**)malloc((1+monitor_length)*sizeof(char*));
		
		int arg_index;
		for(arg_index =optind; arg_index < argc; arg_index++)
		{
			monitor_names[arg_index-optind] = strdup(argv[arg_index]);
		}
		monitor_names[monitor_length] = NULL;
	}


	


	//get pid of bwmond
	FILE* pid_file = fopen(PID_PATH, "r");
	int bwmond_pid = -1;
	if(pid_file != NULL)
	{
		char newline_terminator[3];
		unsigned long read_length;
		newline_terminator[0] = '\n';
		newline_terminator[1] = '\r';
		dyn_read_t pid_read = dynamic_read(pid_file, newline_terminator, 2, &read_length);
		if(sscanf(pid_read.str, "%d", &bwmond_pid) == EOF)
		{
			bwmond_pid = -1;
		}
	}

	int queue_exists = 0;
	if(bwmond_pid > 0)
	{
		int testq = msgget(ftok(PID_PATH, MSG_ID), 0777); //should fail, since no IPC_CREAT
		int count = 0;
		while(testq >= 0 && count < 5)
		{
			usleep(500*1000);
			testq = msgget(ftok(PID_PATH, MSG_ID), 0777); //should fail, since no IPC_CREAT
			count++;
		}
		if(testq >= 0)
		{
			queue_exists = 1;
			printf("ERROR: Simultaneous queries not permitted\n");
			//struct msqid_ds queue_data;
			//msgctl(testq, IPC_RMID, &queue_data);

		}
	}
	else
	{
		printf("ERROR: PID file does not exist or you do not have permissions to read it\n");
	}




	if(bwmond_pid >= 0 && queue_exists == 0)
	{

		//open message queue 
		mq = msgget(ftok(PID_PATH, MSG_ID), 0777 | IPC_CREAT );

		//send request data	
		message_t req_msg;
		req_msg.msg_type = REQUEST_MSG_TYPE;
		if(monitor_names[0] == NULL)
		{
			sprintf(req_msg.msg_line,"ALL");
			msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, IPC_NOWAIT);
		}
		else
		{
			int monitor_index;
			for(monitor_index = 0; monitor_names[monitor_index] != NULL; monitor_index++)
			{
				sprintf(req_msg.msg_line, "%s", monitor_names[monitor_index]);
				msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, IPC_NOWAIT);
			}
		}


		//signal bwmond to send data
		kill((pid_t)bwmond_pid, SIGUSR1);

		message_t response_msg;
		sprintf(response_msg.msg_line,"initial");
		int read_valid = 1;
		while(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
		{
			if(format == 'm')
			{
				int read_num = 0;
				for(read_num = 0; read_num < 5 && read_valid >= 0 && strcmp(response_msg.msg_line, "END") != 0 ; read_num++)
				{
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
					if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0 && read_num != 1)
					{
						printf("%s", response_msg.msg_line);
					}
				}
				int end_found = 0;
				while(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0 && end_found == 0)
				{
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
					printf("%s", response_msg.msg_line);
					end_found = strstr(response_msg.msg_line, "\n") != NULL ? 1 : 0;
				}
			}
			else
			{
				//get name of monitor
				char monitor_name[MAX_MSG_LINE];
				read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					sprintf(monitor_name, "%s", response_msg.msg_line);
					monitor_name[ strlen(monitor_name) - 1] = '\0'; //get rid of newline
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
				}
			
				long interval_end;	
				time_t oldest_start;
				time_t oldest_end;
				time_t recent_end;
				long read;
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					sscanf(response_msg.msg_line, "%ld", &read);
					interval_end = read;
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
				}
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					sscanf(response_msg.msg_line, "%ld", &read);
					oldest_start = (time_t)read;
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
				}
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					sscanf(response_msg.msg_line, "%ld", &read);
					oldest_end = (time_t)read;
					read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
				}
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					sscanf(response_msg.msg_line, "%ld", &read);
					recent_end = (time_t)read;
				}


				//load time point data
				bw_history* history = initialize_history();
				if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					//load bandwidth usage at each time point
					response_msg.msg_line[ strlen(response_msg.msg_line) - 1 ] = ',';
					char next_end_char = ',';
					while( strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0 &&  next_end_char == ',' )
					{
						read_valid = get_next_message(mq, (void*)&response_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 500);
						if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0)
						{
							next_end_char = response_msg.msg_line[ strlen(response_msg.msg_line) - 1 ];
							response_msg.msg_line[ strlen(response_msg.msg_line) - 1 ] = '\0';

							int64_t bw_value;
							sscanf(response_msg.msg_line, "%lld", ((long long int*)(&bw_value)) );
						
							history_node* hnode = (history_node*)malloc(sizeof(history_node));
							hnode->next = NULL;
							hnode->previous = NULL;
							hnode->bandwidth = bw_value;
							
							if(history->length == 0)
							{
								push_history(history, hnode, oldest_start, oldest_end);
							}
							else
							{
								//in this case the interval start parameter (parameter three) gets ignored, so this is fine
								push_history(history, hnode, recent_end, recent_end);
							}
						}
						else
						{
							next_end_char = '\n';
						}
					}



					if(strcmp(response_msg.msg_line, "END") != 0 && read_valid >= 0 && history->length > 0)
					{
						if(format == 'h')
						{
							printf("%s\n", monitor_name);
							print_history(history, interval_end);
						}
						if(format == 't')
						{
							print_history_as_csv(history, interval_end, monitor_name);
						}
					}
					
					//free history
					while(history->length > 0)
					{
						history_node* old_node = shift_history(history, FIXED_INTERVAL);
						free(old_node);
					}
					free(history);
				}
			}
		}
		struct msqid_ds queue_data;
		msgctl(mq, IPC_RMID, &queue_data);
	}
	return 0;
}


void signal_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT )
	{
		//destroy message queue
		if(mq >= 0)
		{
			struct msqid_ds queue_data;
			msgctl(mq, IPC_RMID, &queue_data);
		}
		exit(0);
	}
}


