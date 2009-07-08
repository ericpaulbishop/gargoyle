/*  bwmond / bw-stats -	A small bandwidth monitoring utility for linux that uses
 *  			iptables rules to monitor bandwidth usage (useful for monitoring QoS)
 *  			Originally created for Gargoyle Router Management Utility
 * 		
 * 			Created By Eric Bishop 
 * 			http://www.gargoyle-router.com
 * 		  
 *
 *  Copyright © 2008,2009 by Eric Bishop <eric@gargoyle-router.com>
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
#include "libiptc/libiptc.h"

#define MINUTE	101
#define HOUR	102
#define DAY	103
#define WEEK	104
#define	MONTH	105







typedef struct
{
	char* name;
	long interval_length;
	int interval_end;
	long history_length;
	char* table;
	char* chain;
	int num_marks;
	int *marks;
	int num_rules;
	int *rules;
	char* backup_dir;
	long backup_freq;
	long accumulator_freq;
	
	time_t  last_update;
	time_t  last_accumulator_update;
	time_t  last_backup;
	int64_t last_byte_count;
	int64_t accumulator_count;


	bw_history* history;

} bw_monitor;




typedef struct
{
	int monitor_index;
	time_t update_time;

	
	//0 = no update, 1 = accumulator update, 2= full update
	int update_type;
	
	//0 = don't backup, 1= do backup
	int backup;

} update_node;




//prototypes


bw_monitor** load_bwmon_config(char *filename);
char** parse_variable_definition_line(char* line);
int* parse_comma_list(char* list);
bw_monitor** initialize_monitors(bw_monitor** potential_monitors);
void load_monitor_history_from_file(bw_monitor* monitor);
FILE* open_monitor_backup_file(bw_monitor* monitor, const char* open_mode);

int get_minutes_west(void);


update_node get_next_update_time(bw_monitor** monitors, int monitor_index);
time_t get_next_interval_end(time_t current_time, int end_type);
void update_monitor(bw_monitor* monitor, update_node update, iptc_handle_t* recent_iptables_snapshots, char** recent_iptables_names);
void backup_monitor(bw_monitor* monitor, time_t current_time);

void handle_output_request(bw_monitor** monitors);

void daemonize(void);
void signal_handler(int sig);

int output_requested = 0;
int terminated = 0;

int main( int argc, char** argv )
{
	char* config_file_name = strdup("/etc/bwmond.conf");
	int run_in_foreground = 0;

	int c;
	while((c = getopt(argc, argv, "c:C:fFuU")) != -1)
	{	
		switch(c)
		{
			case 'C':
			case 'c':
				free(config_file_name);
				config_file_name = strdup(optarg);
				break;
			case 'F':
			case 'f':
				run_in_foreground = 1;
				break;
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS] [MONITOR NAMES]\n", argv[0]);
				printf("\t-C [MONITOR_CONFIG_FILE] specifies location of file containing monitor configuration information\n");
				printf("\t-f forces program to run in foreground and not daemonize\n");
				printf("\t-u print usage information and exit\n");
				return 0;
		}
	}


	output_requested = 0;
	terminated = 0;

	bw_monitor** potential_monitors = load_bwmon_config(config_file_name);  
	bw_monitor** monitors = initialize_monitors(potential_monitors);

	if(monitors[0] == NULL)
	{
		printf("ERROR: no valid monitors defined in config file\n");
		return 0;
	}






	if(run_in_foreground > 0)
	{
		// record pid to lockfile
		int pid_file= open(PID_PATH,O_RDWR|O_CREAT,0644);
		if(pid_file<0) // exit if we can't open file
		{
			exit(1);
		}
		if(lockf(pid_file,F_TLOCK,0)<0) // try to lock file, exit if we can't
		{
			exit(1);
		}
		char pid_str[25];
		sprintf(pid_str,"%d\n",getpid());
		write(pid_file,pid_str,strlen(pid_str));

		//set signal handlers
		signal(SIGTERM,signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGUSR1,signal_handler);
	}
	else
	{
		daemonize();
	}

	//initialize reference time
	time_t reference_time = time(NULL);


	
	
	//load monitors from backup files (if they exist)
	int monitor_index;
	for(monitor_index=0; monitors[monitor_index] != NULL; monitor_index++)
	{
		bw_monitor* monitor = monitors[monitor_index];
		FILE* backup = open_monitor_backup_file(monitor, "r");
		if(backup != NULL)
		{
			fclose(backup);
			load_monitor_history_from_file(monitor);
		}
	}


	//initialize update queue
	priority_queue* update_queue = initialize_priority_queue();
	for(monitor_index=0; monitors[monitor_index] != NULL; monitor_index++)
	{
		bw_monitor* monitor = monitors[monitor_index];
		unsigned long priority;

		update_node* next_update = (update_node*)malloc(sizeof(update_node));
		*next_update = get_next_update_time(monitors, monitor_index);
		priority= next_update->update_time > reference_time ?  next_update->update_time - reference_time : 0;
		push_priority_queue(update_queue, priority, monitor->name, next_update);
		
		//time_t next = next_update->update_time;
		//struct tm* detailedTime = localtime(&next);
		//printf( "scheduled update for %s at: %s", monitor->name, asctime(detailedTime));
		//printf( "type = %d, backup = %d\n\n", next_update->update_type, next_update->backup);
	}
	



	//loop
	time_t last_checked = time(NULL);
	long queue_test_counter = 0;
	while(terminated == 0)
	{
		time_t current_time = time(NULL);
		int current_minutes_west = get_minutes_west();
		if(current_time != last_checked)
		{
			last_checked = current_time;
			priority_queue_node *check = peek_priority_queue_node(update_queue);
			if(current_time >= ((update_node*)check->value)->update_time)
			{
				// we need to save iptables snapshots so we don't make extra work
				// retrieving the same data every time
				// don't bother doing anything fancy
				// there definitely aren't more than 25 tables! 
				// (In fact that's WAY more than we'll ever see!)
				iptc_handle_t recent_iptables_snapshots[25];
				char* recent_iptables_names[25];
				recent_iptables_snapshots[0] = NULL;
				recent_iptables_names[0] = NULL;

				while(current_time >= ((update_node*)check->value)->update_time)
				{
					unsigned long priority;

					//pop next update from priority queue
					priority_queue_node *p = shift_priority_queue_node(update_queue);
					update_node* next_update = (update_node*)free_priority_queue_node(p);
					next_update->update_time = current_time;
					
					

					//do update
					int monitor_index = next_update->monitor_index;
					bw_monitor* monitor = monitors[ monitor_index ];
					update_monitor(monitor, *next_update, recent_iptables_snapshots, recent_iptables_names);
			
					//printf("updating %s at %ld\n", monitor->name, current_time);	

					//schedule next update for this monitor & return to queue
					*next_update = get_next_update_time(monitors, monitor_index);
					priority= next_update->update_time > reference_time ?  next_update->update_time - reference_time : 0;
					push_priority_queue(update_queue,  next_update->update_time-reference_time, monitor->name, next_update);
				

					//make sure there's not another monitor we need to update
					check = peek_priority_queue_node(update_queue);
				}

				//free iptables snapshots
				int snapshot_index;
				for(snapshot_index = 0; recent_iptables_names[snapshot_index] != NULL; snapshot_index++)
				{
					free(recent_iptables_names[snapshot_index]);
					iptc_free(&(recent_iptables_snapshots[snapshot_index]));
				}
			}
		}
		
		//sleep for 400 milliseconds, or until signal is caught
		usleep(400*1000); 

	

		queue_test_counter = queue_test_counter + 400;
		if(output_requested > 0)
		{
			handle_output_request(monitors);
			output_requested = 0;
		}
		else if (queue_test_counter > 5000)
		{
			//periodically ensure that message queue hasn't 
			//failed to be destroyed properly
			queue_test_counter = 0;
			int q_test = msgget(ftok(PID_PATH, MSG_ID), 0777 );
			if(q_test >= 0)
			{
				usleep(250);
				if(output_requested == 0)
				{
					struct msqid_ds queue_data;
					msgctl(q_test, IPC_RMID, &queue_data);
				}
				else
				{
					handle_output_request(monitors);
					output_requested = 0;
				}
			}
		}

		//if we just jumped more than 25 minutes (i.e. date/time was reset dramatically) , reset all times
		time_t old_current_time = current_time;
		current_time = time(NULL);
		if( old_current_time - current_time > 25*60 || current_time - old_current_time > 25*60)
		{
			long adjustment_seconds = (long)(current_time - old_current_time);
			reference_time = reference_time + adjustment_seconds;

			priority_queue* new_update_queue = initialize_priority_queue();
			while(update_queue->length > 0)
			{
				unsigned long priority;
				
				priority_queue_node *p = shift_priority_queue_node(update_queue);
				update_node* next_update = (update_node*)p->value;
				bw_monitor* monitor = monitors[ next_update->monitor_index ];
				if(monitor->interval_end <= 0 ) //handle case of fixed interval length
				{
					monitor->last_update = monitor->last_update + adjustment_seconds;
					monitor->last_accumulator_update = monitor->last_accumulator_update > 0 ? monitor->last_accumulator_update + adjustment_seconds : 0;
					monitor->last_backup = monitor->last_backup + adjustment_seconds;
				
					bw_history* history = monitor->history;
					history->oldest_interval_start = history->oldest_interval_start + adjustment_seconds;
					history->oldest_interval_end   = history->oldest_interval_end + adjustment_seconds;
					history->recent_interval_end   = history->recent_interval_end + adjustment_seconds;
				
				}
				else //handle case of fixed interval end (MUCH harder)
				{
					if(monitor->history->first == NULL) //if nothing in the history, just update last update / last backup simply
					{
						monitor->last_update = monitor->last_update + adjustment_seconds;
						monitor->last_accumulator_update = monitor->last_accumulator_update > 0 ? monitor->last_accumulator_update + adjustment_seconds : 0;
						monitor->last_backup = monitor->last_backup + adjustment_seconds;
					}
					else //the ugly case...
					{
						//find out when last update should have been given the new time scheme
						time_t int_test1 = get_next_interval_end(0, monitor->interval_end);
						time_t int_test2 = get_next_interval_end(int_test1, monitor->interval_end);
						long interval_length = int_test2 - int_test1;

						time_t new_next = get_next_interval_end(current_time, monitor->interval_end);
						time_t new_previous = new_next - interval_length;
						time_t old_previous = monitor->history->recent_interval_end;
						long next_adjustment = new_previous - old_previous;

						//if it's not when it was supposed to be, shift to make it so, otherwise do nothing
						if(next_adjustment != 0)
						{
							monitor->last_update = monitor->last_update + next_adjustment;
							monitor->last_accumulator_update = monitor->last_accumulator_update > 0 ? monitor->last_accumulator_update + adjustment_seconds : 0;
							monitor->last_backup = monitor->last_backup + next_adjustment;
				
							bw_history* history = monitor->history;
							history->oldest_interval_start = history->oldest_interval_start + next_adjustment;
							history->oldest_interval_end   = history->oldest_interval_end + next_adjustment;
							history->recent_interval_end   = history->recent_interval_end + next_adjustment;
						}
					}
				}
				*next_update = get_next_update_time(monitors, next_update->monitor_index);
				priority = next_update->update_time > reference_time ?  next_update->update_time - reference_time : 0;
				//printf("ADJUSTING FOR %s, current_time = %ld, next_update_time = %ld\n", monitor->name, current_time, next_update->update_time);

				p->priority = priority;
				push_priority_queue_node(new_update_queue, p);
			}
			unsigned long num_destroyed;
			destroy_priority_queue(update_queue, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			update_queue = new_update_queue;
		}
	

		//if timezone was just reset, deal with it
		int new_minutes_west = get_minutes_west();
		if(new_minutes_west != current_minutes_west)
		{
			//printf("adjusting for timezone, time being adjusted by %d minutes\n", (new_minutes_west - current_minutes_west) );

			long adjustment_seconds = (new_minutes_west - current_minutes_west)*60;
			priority_queue* new_update_queue = initialize_priority_queue();
			while(update_queue->length > 0)
			{
				unsigned long priority;
				priority_queue_node *p = shift_priority_queue_node(update_queue);
				update_node* next_update = (update_node*)p->value;
				bw_monitor* monitor = monitors[ next_update->monitor_index ];
				if(monitor->interval_end > 0 ) //only out of whack if we update with specific interval_end, otherwise non-timezone-specific
				{
					/* CASE1: the correct time is now after it was (e.g. it was 2, it is now 4, we measure at 3 -> we SKIPPED a measurement
					 *        solution: add a measurement, must still add a measurement if no measurements exist
					 * CASE2: the correct time is now before it was (e.g. it was 4, it is now 2, we measure at 3 -> TOO MANY measurements
					 *        solution: remove a measurement, if no measurements exist don't bother
					 */
					
					//printf("For %s, old next_update=%d\n", monitor->name, p->priority + reference_time);	
					
					/* let's pretend we measured at the right time points instead of the wrong ones for this time zone*/	
					bw_history* history = monitor->history;
					time_t oldest_start = history->oldest_interval_start == 0 ? monitor->last_update + adjustment_seconds : history->oldest_interval_start + adjustment_seconds ;
					time_t oldest_end = history->oldest_interval_end == 0 ? get_next_interval_end(monitor->last_update +adjustment_seconds, monitor->interval_end) : history->oldest_interval_end + adjustment_seconds;
					if(oldest_start > current_time)
					{
						oldest_start = current_time;
						oldest_end = get_next_interval_end(current_time, monitor->interval_end);
					}
					//printf("current_time = %d\n", current_time);
					//printf("history->oldest_start = %d, monitor->last_update = %d, adjustment_seconds = %d, oldest_start = %d\n", history->oldest_interval_start, monitor->last_update, adjustment_seconds, oldest_start);
					//printf("history->oldest_end = %d, monitor->last_update = %d, adjustment_seconds = %d, oldest_end = %d\n", history->oldest_interval_end, monitor->last_update, adjustment_seconds, oldest_end);
					
					int increment;
					if(monitor->interval_end > 0)
					{
						time_t time1 = get_next_interval_end(monitor->last_update, monitor->interval_end);
						time_t time2 = get_next_interval_end(time1, monitor->interval_end);
						increment = time2 - time1;
					}
					else //shouldn't be possible... but code was copied & pasted, and it works, so what the hell?
					{
						increment = monitor->interval_length;
					}


					bw_history* new_history = initialize_history();
					int node_num = 0;
					time_t next_start = oldest_start;
					time_t next_end = oldest_end;
					
					
					if(monitor->last_update + adjustment_seconds <= current_time)
					{	
						monitor->last_update = monitor->last_update + adjustment_seconds;
					}
					else
					{
						monitor->last_update = current_time;
					}
					while(history->length > 0)
					{
						history_node* old_node = pop_history(history);						
						if(next_end < current_time)
						{
							//printf("Adjusting for %s, next_start=%d, next_end=%d\n", monitor->name, next_start, next_end);	
							push_history(new_history, old_node, next_start, next_end);
							monitor->last_update = next_end;
						}
						else 
						{
							if(next_start < current_time)
							{
								monitor->last_update = next_start;
								monitor->accumulator_count = old_node->bandwidth;
							}
							free(old_node);
						}
						node_num++;
						next_start = oldest_end +((node_num-1)*increment);
						next_end = oldest_end + (node_num*increment);
					}

					//printf("After adjust, before add for %s, next_start=%d, next_end=%d\n", monitor->name, next_start, next_end);	
					
					while(next_end < current_time)
					{
						//printf("Adding for %s, next_start=%d, next_end=%d\n", monitor->name, next_start, next_end);	
						history_node* next_node = (history_node*)malloc(sizeof(history_node));
						next_node->bandwidth = monitor->accumulator_count > 0 ? monitor->accumulator_count : -1;
						monitor->accumulator_count = 0;
						push_history(new_history, next_node, next_start, next_end);
						monitor->accumulator_count = 0;
						
						if(next_end > monitor->last_update)
						{
							monitor->last_update = next_end;
						}

						//pop off & free nodes that are too old
						while(new_history->length > monitor->history_length)
						{
							history_node* old_node = pop_history(new_history);
							free(old_node);
						}

						node_num++;
						next_start = oldest_end +((node_num-1)*increment);
						next_end = oldest_end + (node_num*increment);
					}

					free(history);
					monitor->history = new_history;
					if(monitor->accumulator_freq > 0)
					{
						monitor->last_accumulator_update = monitor->last_update;
					}

				
					
					*next_update = get_next_update_time(monitors, next_update->monitor_index); //gets calculated correctly since we set monitor->last_update correctly above
					priority = next_update->update_time > reference_time ?  next_update->update_time - reference_time : 0;
					p->priority = priority;
					
					//printf("For %s, new next_update=%d\n", monitor->name, priority + reference_time);
					//printf("For %s next update type = %d\n", monitor->name, next_update->update_type);
					//printf("For %s, history length = %d\n", monitor->name, new_history->length);
						
				}
				push_priority_queue_node(new_update_queue, p);
			}
			
			long num_destroyed;
			destroy_priority_queue(update_queue, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			update_queue = new_update_queue;
			//printf("\n\n\n");
		}
	}

	// term signal received
	// back up monitors that should be backed up (onces with backup_dir defined) before exiting
	time_t current_time = time(NULL);
	for(monitor_index=0; monitors[monitor_index] != NULL; monitor_index++)
	{
		bw_monitor* monitor = monitors[monitor_index];
		if(monitor->backup_dir != NULL)
		{
			backup_monitor(monitor, current_time);
		}
	}

	//remove pid file
	unlink(PID_PATH);

	return(0);
}



void daemonize(void)
{
	//fork and end parent process
	int i=fork();
	if (i != 0)
	{	
		if(i < 0) //exit on fork error
		{
			exit(1);
		}
		else //this is parent, exit cleanly
		{
			exit(0);
		}
	}
	
	/********************************
	* child continues as a daemon after parent exits
	********************************/
	// obtain a new process group & close all file descriptors 
	setsid();
	for(i=getdtablesize();i>=0;--i)
	{
		close(i);
	}

	// close standard i/o  
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDERR_FILENO);
	

	// record pid to lockfile
	int pid_file= open(PID_PATH,O_RDWR|O_CREAT,0644);
	if(pid_file<0) // exit if we can't open file
	{
		exit(1);
	}
	if(lockf(pid_file,F_TLOCK,0)<0) // try to lock file, exit if we can't
	{
		exit(1);
	}
	char pid_str[25];
	sprintf(pid_str,"%d\n",getpid());
	write(pid_file,pid_str,strlen(pid_str));


	//set signal handlers
	signal(SIGTERM,signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGUSR1,signal_handler);
}

void signal_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT )
	{
		terminated = 1; //exit cleanly on SIGTERM signal
	}
	else if(sig == SIGUSR1)
	{
		output_requested = 1; //do output
		//printf("output signal received\n");
	}
	else
	{
		//ignore other signals
	}
}


void handle_output_request(bw_monitor** monitors)
{
	//open message queue 
	//NOTE:	do not allow queue creation, this should be
	//	done by program that is requesting data
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 );

	//read from queue (identifier of monitor to print)	
	message_t next_req_message;
	next_req_message.msg_line[0] = '\0';

	if(mq >= 0)
	{
		if( msgrcv(mq, (void *)&next_req_message, MAX_MSG_LINE, REQUEST_MSG_TYPE, IPC_NOWAIT) < 0)
		{
			next_req_message.msg_line[0] = '\0';
		}
	}

	//if we have successfully read message, do output
	while( next_req_message.msg_line[0] != '\0' )
	{
		message_t next_response_message;
		next_response_message.msg_type = RESPONSE_MSG_TYPE;
		
		//printf("received: \"%s\"\n", next_req_message.msg_line);


		//output for each monitor consists of four lines
		//first line = monitor name
		//second line = first interval start
		//third line = first interval end
		//third line = last interval end
		//fourth line = comma separated values for all time points (one message per each value), oldest first, most recent last
		//newlines/commas are included in messages sent to queue so they can be printed as-is
		int monitor_index;
		for(monitor_index = 0; monitors[monitor_index] != NULL; monitor_index++)
		{
			bw_monitor* monitor = monitors[monitor_index];
			if((strcmp(next_req_message.msg_line, "ALL") == 0 || strcmp(next_req_message.msg_line, monitor->name) == 0) && monitor->history->last != NULL)
			{

				sprintf(next_response_message.msg_line, "%s\n", monitor->name);
				send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);
			
				
				sprintf(next_response_message.msg_line, "%d\n", (int)monitor->history->oldest_interval_start);
				send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);
				sprintf(next_response_message.msg_line, "%d\n", (int)monitor->history->oldest_interval_end);
				send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);
				sprintf(next_response_message.msg_line, "%d\n", (int)monitor->history->recent_interval_end);
				send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);



				//print oldest node first, most recent last
				history_node* next_node = monitor->history->last;
				while(next_node != NULL)
				{
					if(next_node != monitor->history->first)
					{
						sprintf(next_response_message.msg_line, "%lld,", (long long int)next_node->bandwidth);
					}
					else
					{
						sprintf(next_response_message.msg_line, "%lld\n", (long long int)next_node->bandwidth);
					}
					send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);
					next_node = next_node->previous;
				}
			}
		}


		if( msgrcv(mq, (void *)&next_req_message, MAX_MSG_LINE, REQUEST_MSG_TYPE, IPC_NOWAIT) < 0)
		{
			next_req_message.msg_line[0] = '\0';
			sprintf(next_response_message.msg_line, "END");
			send_next_message(mq, (void*)&next_response_message, MAX_MSG_LINE, 500);
		}
	}
}




void update_monitor(bw_monitor* monitor, update_node update, iptc_handle_t* recent_iptables_snapshots, char** recent_iptables_names)
{
	//printf("updating monitor %s\n", monitor->name);
	if(update.update_type > 0)
	{
		int found = 0;
		int snapshot_index = 0;
		while(recent_iptables_names[snapshot_index] != NULL && found == 0)
		{
			if(strcmp(recent_iptables_names[snapshot_index], monitor->table) == 0)
			{
				found = 1;
			}
			else
			{
				snapshot_index++;
			}
		}

		iptc_handle_t iptables_snapshot;
		if(found == 0)
		{
			iptables_snapshot = iptc_init(monitor->table);
			if(iptables_snapshot == NULL)
			{
				printf("ERROR, iptables snapshot could not be initialized\n error code = %s\n",iptc_strerror(errno));
				exit(0);
			}
			else
			{
				recent_iptables_snapshots[snapshot_index] = iptables_snapshot;
				recent_iptables_names[snapshot_index] = strdup(monitor->table);
				recent_iptables_snapshots[snapshot_index+1] = NULL;
				recent_iptables_names[snapshot_index+1] = NULL;
			}
		}
		else
		{
			iptables_snapshot = recent_iptables_snapshots[snapshot_index];
		}
		

		//get new byte count from ip tables	
		int64_t new_byte_count = 0;
		int rule_index;
		struct ipt_counters *rule_counter;
		int* rules = monitor->rules;
		for(rule_index =0; rule_index < monitor->num_rules; rule_index++)
		{
       			rule_counter = iptc_read_counter(monitor->chain, rules[rule_index], &iptables_snapshot);
			if(rule_counter != NULL)
			{
				new_byte_count = new_byte_count + rule_counter->bcnt;
			}
		}
		

		// snapshot will be freed after all necessary updates are done
		// this allows us to use the snapshots over again, without 
		// re-initialization if we need to
		//iptc_free(&iptables_snapshot);

	
		//calc bandwidth for this timepoint
		int64_t new_bandwidth = new_byte_count - monitor->last_byte_count;

		monitor->accumulator_count = monitor->accumulator_count + new_bandwidth;
		monitor->last_accumulator_update = update.update_time;
		monitor->last_byte_count = new_byte_count;
		
		
		if(update.update_type > 1) //do full update?
		{
			//update monitor history
			history_node* new_node;
			if(monitor->history->length < monitor->history_length)
			{
				new_node = (history_node*)malloc(sizeof(history_node));
			}
			else
			{
				new_node = pop_history(monitor->history);
			}
			
			time_t interval_start = monitor->last_update;
			time_t nominal_end = (monitor->interval_end > 0) ? get_next_interval_end( monitor->last_update, monitor->interval_end) : monitor->last_update + monitor->interval_length;
			unsigned long nominal_length = (monitor->interval_end > 0) ? get_next_interval_end( nominal_end, monitor->interval_end) - nominal_end : monitor->interval_length;
			time_t actual_end = update.update_time;
			
			//if current time is way ahead of where it should be in series, insert intermediate history nodes
			while(actual_end > nominal_end+nominal_length)
			{
				new_node->bandwidth = -1;
				push_history(monitor->history, new_node, interval_start, nominal_end);

				nominal_end = (monitor->interval_end > 0) ? get_next_interval_end( nominal_end, monitor->interval_end) : nominal_end + monitor->interval_length;
				if(monitor->history->length < monitor->history_length)
				{
					new_node = (history_node*)malloc(sizeof(history_node));
				}
				else
				{
					new_node = pop_history(monitor->history);
				}
			}
			
			new_node->bandwidth = monitor->accumulator_count;
			push_history(monitor->history, new_node, interval_start, nominal_end);
			
			//set update_time to nominal_end so next is scheduled on time
			update.update_time = nominal_end;
			monitor->last_update = nominal_end;
			monitor->accumulator_count = 0;

			//print_history(monitor->history);
		}
	}
	if(update.backup > 0)
	{
		backup_monitor(monitor, update.update_time);
		monitor->last_backup = update.update_time;
	}
	//printf("done updating monitor %s\n", monitor->name);
}

void backup_monitor(bw_monitor* monitor, time_t current_time)
{
	//open file
	FILE* output = open_monitor_backup_file(monitor, "wb");

	//output backup time & accumulator count
	monitor->last_backup = current_time;
	fwrite(&(monitor->last_backup), sizeof(time_t), 1, output);
	fwrite(&(monitor->accumulator_count), sizeof(int64_t), 1, output);

	
	//output info for history timepoints
	fwrite(&(monitor->history->oldest_interval_start), sizeof(time_t), 1, output);
	fwrite(&(monitor->history->oldest_interval_end), sizeof(time_t), 1, output);
	fwrite(&(monitor->history->recent_interval_end), sizeof(time_t), 1, output);

	//test whether we can get away with writing 32 bit integers instead of 64
	// then save this to the file so when we load, we know how to parse the file
	char bw_bits = 32;
	history_node* next_node = monitor->history->last;
	while(next_node != NULL)
	{
		if(next_node->bandwidth > INT32_MAX)
		{
			bw_bits = 64;
		}
		next_node = next_node->previous;
	}	
	fwrite(&bw_bits, sizeof(char), 1, output);

	
	next_node = monitor->history->last;	
	while(next_node != NULL)
	{
		if(bw_bits == 64)
		{	
			fwrite(&(next_node->bandwidth), sizeof(int64_t), 1, output);
		}
		else
		{
			int32_t bw_32;
		       	char convert_str[25];
			sprintf(convert_str, "%lld", (long long int)next_node->bandwidth);
			sscanf(convert_str, "%d", &bw_32);
			fwrite(&bw_32, sizeof(int32_t), 1, output); 
		}
		next_node = next_node->previous;
	}

	//close file	
	fclose(output);
}

void load_monitor_history_from_file(bw_monitor* monitor)
{
	//open file
	FILE* input = open_monitor_backup_file(monitor, "rb");

	
	// load backup time and accumulator data
	time_t  last_backup;
	int64_t accumulator_count;
	fread(&last_backup, sizeof(time_t), 1, input);
	fread(&accumulator_count, sizeof(int64_t), 1, input);

	time_t oldest_start;
	time_t oldest_end;
	time_t recent_end;
	char bw_bits;

	bw_history* history = initialize_history();
	fread(&oldest_start, sizeof(time_t), 1, input);	
	fread(&oldest_end, sizeof(time_t), 1, input);	
	fread(&recent_end, sizeof(time_t), 1, input);	
	fread(&bw_bits, sizeof(char), 1, input);

	
	// read in history
	// oldest will be first in file, entries will be in chronological order	
	int64_t bandwidth_64;
	int32_t bandwidth_32;
	void* bw_pointer= bw_bits == 32 ? (void*)&bandwidth_32 : (void*)&bandwidth_64;
	int read_size = bw_bits == 32 ? sizeof(int32_t) : sizeof(int64_t);
	while( fread(bw_pointer, read_size, 1, input) > 0)
	{
		char convert_str[25];
		if(bw_bits == 32)
		{
			sprintf(convert_str, "%d", bandwidth_32);
		}
		else
		{
			sprintf(convert_str, "%lld", (long long int)bandwidth_64);
		}
		
		history_node* next_node = (history_node*)malloc(sizeof(history_node));
		sscanf(convert_str, "%lld", (long long int*)&(next_node->bandwidth));
		next_node->next = NULL;
		next_node->previous = NULL;
		if(history->length == 0)
		{
			push_history(history, next_node, oldest_start, oldest_end);
		}
		else
		{
			//in this case the interval start parameter (parameter three) gets ignored, so this is fine
			push_history(history, next_node, recent_end, recent_end);
		}
	}	
	fclose(input);

	time_t current_time = time(NULL);

	//insert gaps in history as necessary & initialize next update time
	//to proper value to synchronize with history
	if(history->first != NULL)
	{
		time_t last_update = history->recent_interval_end;

		int increment;
		if(monitor->interval_end > 0)
		{
			time_t time1 = get_next_interval_end(last_update, monitor->interval_end);
			time_t time2 = get_next_interval_end(time1, monitor->interval_end);
			increment = time2 - time1;
		}
		else
		{
			increment = monitor->interval_length;
		}
		if(current_time - last_update > monitor->history_length * increment || current_time < oldest_start)
		{
			//don't actually save any of the loaded data to the monitor -- it's too old / too new to be useful
			//just free any history data we've allocated
			while(history->length > 0)
			{
				history_node* old_node = pop_history(history);
				free(old_node);
			}
			free(history);
		}
		else if(current_time < last_update)
		{
			int increment;
			if(monitor->interval_end > 0)
			{
				time_t time1 = get_next_interval_end(0, monitor->interval_end);
				time_t time2 = get_next_interval_end(time1, monitor->interval_end);
				increment = time2 - time1;
			}
			else
			{
				increment = monitor->interval_length;
			}

			//we don't want to completely whack all old data, but something is definitely FUBAR if we're getting data in the future
			//hope it's vaguely correct, and delete data after current time
			bw_history* new_history = initialize_history();
			monitor->accumulator_count = 0;
			int node_num = 0;
			while(history->length > 0)
			{
				history_node* old_node = pop_history(history);
				int next_start;
				int next_end;
				if(node_num == 0)
				{
					next_start = oldest_start;
					next_end = oldest_end;
				}
				else
				{
					next_start = oldest_end +((node_num-1)*increment);
					next_end = oldest_end + (node_num*increment);
				}
				if(next_end < current_time)
				{
					push_history(new_history, old_node, next_start, next_end);
				}
				else 
				{
					//uncomment the following to over-report, instead of under-report at boundry
					/*
					if(next_start < current_time)
					{
						monitor->accumulator_count = old_node->bandwidth;
					}
					*/
					free(old_node);
				}
				node_num++;
			}
			free(history);
			
			bw_history* old_history = monitor->history;
			monitor->history = new_history;
			free(old_history);
			
			monitor->last_update = new_history->recent_interval_end;
			monitor->last_backup = time(NULL);

		}
		else
		{
			//add nodes with undefined (-1) bandwidth until we're up to date
			time_t last_time_point = last_update;
			time_t next_time_point = monitor->interval_end > 0 ? get_next_interval_end(last_time_point, monitor->interval_end) : last_time_point + monitor->interval_length;
			while(next_time_point < current_time)
			{
				history_node* next_node = (history_node*)malloc(sizeof(history_node));
				next_node->bandwidth = -1;
				push_history(history, next_node, last_time_point, next_time_point);
				last_time_point = next_time_point;
				next_time_point = monitor->interval_end > 0 ? get_next_interval_end(last_time_point, monitor->interval_end) : last_time_point + monitor->interval_length;
			}

			//pop off & free nodes that are too old
			while(history->length > monitor->history_length)
			{
				history_node* old_node = pop_history(history);
				free(old_node);
			}

			
			//update monitor
			monitor->last_update = history->recent_interval_end;
			monitor->last_backup = last_backup;
			
			if(last_update + monitor->interval_end < next_time_point)
			{
				monitor->accumulator_count = accumulator_count;
			}
			bw_history* old_history = monitor->history;
			monitor->history = history;
			free(old_history);
		}
	}

	/* 
	 * Deal with possibility of timezone shift 
	 * This doesn't affect case where update is done every interval,
	 * But where there is a fixed interval end, things need to line up
	 * or everything gets out of whack.
	 */
	if(monitor->interval_end > 0 && monitor->history->first != NULL)
	{
		time_t oldest_start = monitor->history->oldest_interval_start;
		time_t oldest_end = monitor->history->oldest_interval_end;
		time_t expected_oldest_end = get_next_interval_end(oldest_start, monitor->interval_end);
		if(expected_oldest_end != oldest_end)
		{
			long adj_seconds = (long)(expected_oldest_end - oldest_end);
			monitor->history->oldest_interval_start = monitor->history->oldest_interval_start + adj_seconds;
			monitor->history->oldest_interval_end = monitor->history->oldest_interval_end + adj_seconds;
			monitor->history->recent_interval_end = monitor->history->recent_interval_end + adj_seconds;
			monitor->last_update = monitor->history->recent_interval_end;
		
			if(monitor->history->recent_interval_end > current_time)
			{
				bw_history* new_history = initialize_history();
				bw_history* old_history = monitor->history;
				time_t next_start = old_history->oldest_interval_start;
				while(old_history->length > 0)
				{
					history_node* old_node = pop_history(old_history);
					time_t next_end = get_next_interval_end(next_start, monitor->interval_end);
					if(next_end <= current_time)
					{
						push_history(new_history, old_node, next_start, next_end);
					}
					else if(old_node->bandwidth > 0)
					{
						monitor->accumulator_count = old_node->bandwidth;
						free(old_node);
					}
					next_start = next_end;
				}
				monitor->history = new_history;
				free(old_history);
			}
		}
	}
}


FILE* open_monitor_backup_file(bw_monitor* monitor, const char* open_mode)
{
	FILE* open_file = NULL;
	if(monitor->backup_dir != NULL)
	{
		char* filename1;
       		if( monitor->backup_dir[ strlen(monitor->backup_dir)-1 ] != '/')
		{		
			filename1 = dynamic_strcat(2, monitor->backup_dir, "/");
		}
		else
		{
			filename1 = strdup(monitor->backup_dir);
		}
		char* filename = dynamic_strcat(2, filename1, monitor->name);
		open_file = fopen( filename, open_mode);
		free(filename1);
		free(filename);
	}
	return open_file;
}


//returns next accumulator update, full update or backup time (whichever is first)
update_node get_next_update_time(bw_monitor** monitors, int monitor_index)
{
	bw_monitor* monitor = monitors[monitor_index];
	
	time_t next_full_update;
	if(monitor->interval_end > 0)
	{
		next_full_update = get_next_interval_end( monitor->last_update, monitor->interval_end);	
	}
	else
	{
		next_full_update = monitor->last_update + monitor->interval_length;
	}

	time_t next_accumulator_update;
	if(monitor->accumulator_freq > 0)
	{
		next_accumulator_update = monitor->last_accumulator_update + monitor->accumulator_freq;
	}
	else
	{
		next_accumulator_update = next_full_update;
	}

	time_t next_backup;
	if(monitor->backup_freq > 0)
	{
		next_backup = monitor->last_backup + monitor->backup_freq;
	}
	else
	{
		next_backup = next_full_update + 100;
	}

	update_node next_update;
	next_update.monitor_index = monitor_index;
	if(next_accumulator_update < next_full_update && next_accumulator_update <= next_backup)
	{
		next_update.update_time = next_accumulator_update;
		next_update.update_type = 1;
		next_update.backup = next_accumulator_update == next_backup ? 1 : 0;
	}
	else if(next_full_update <= next_backup)
	{
		next_update.update_time = next_full_update;
		next_update.update_type = 2;
		next_update.backup = next_full_update == next_backup ? 1 : 0;
	}
	else
	{
		next_update.update_time = next_backup;
		next_update.update_type = 0;
		next_update.backup = 1;
	}
	
	
	return next_update;
}


time_t get_next_interval_end(time_t current_time, int end_type)
{
	time_t next_end;
	struct tm* curr = localtime(&current_time);
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


bw_monitor** initialize_monitors(bw_monitor** potential_monitors)
{
	int num_potential_monitors = 0;
	while( potential_monitors[num_potential_monitors] != NULL)
	{
		num_potential_monitors++;
	}
	
	bw_monitor** initialized_monitors = (bw_monitor**)malloc((num_potential_monitors+1)*sizeof(bw_monitor*));
	int initialized_monitor_index = 0;


	int pm_index;
	for(pm_index = 0; pm_index < num_potential_monitors; pm_index++)
	{
		bw_monitor* monitor = potential_monitors[pm_index];
		iptc_handle_t iptables_snapshot;
		iptables_snapshot = iptc_init(monitor->table);
		if(iptables_snapshot)
		{
			if(iptc_is_chain(monitor->chain, iptables_snapshot))
			{
				const struct ipt_entry *chain_rule = iptc_first_rule(monitor->chain, &iptables_snapshot);
				int num_rules = 0;
				while(chain_rule)
				{
					chain_rule = iptc_next_rule(chain_rule, &iptables_snapshot);
					num_rules++;
				}
				int* defined_rules = malloc( (num_rules+1)*sizeof(int));
				int defined_rule_index = 0;
				int64_t initial_count = 0;
				
				if(num_rules > 0)
				{
					chain_rule = iptc_first_rule(monitor->chain, &iptables_snapshot);
					int rule_num = 1;
					while(chain_rule != NULL)
					{
						if(monitor->num_marks > 0) //id marking rules & init counter
						{
							struct ipt_entry_target *t = ipt_get_target((struct ipt_entry *)chain_rule);
							if (t->u.user.name[0] && t->data)
							{
								if(strcmp(t->u.user.name,"MARK") == 0)
								{
									int* marks = monitor->marks;
									int mark_index = 0; 
									for(mark_index = 0; marks[mark_index] > 0; mark_index++)
									{
										if( *(int *)t->data == marks[mark_index])
										{
											defined_rules[defined_rule_index] = rule_num;
											defined_rule_index++;

											struct ipt_counters *count = iptc_read_counter(monitor->chain, rule_num, &iptables_snapshot);
											initial_count = initial_count + count->bcnt;
											/*
											printf("name = %s\n", t->u.user.name);
											printf("data = %d\n", *(int *)t->data);
											printf("count = %d\n", initial_count);
											*/
										}
									}
								}
							}
						}
						else //ensure specified rule number(s) exist & init counter
						{
							int* rules = monitor->rules;
							int rule_index = 0;
							for(rule_index = 0; rules[rule_index] > 0; rule_index++)
							{
								if(rule_num ==rules[rule_index])
								{
									defined_rules[defined_rule_index] = rule_num;
									defined_rule_index++;

									struct ipt_counters *count = iptc_read_counter(monitor->chain, rule_num, &iptables_snapshot);
									initial_count = initial_count + count->bcnt;
									//printf("rule count = %d\n", initial_count);
								}
							}	
						}
						rule_num++;
						chain_rule = iptc_next_rule(chain_rule, &iptables_snapshot);
					}

					defined_rules[defined_rule_index] = -1;
					if(defined_rule_index > 0)
					{
						if(monitor->rules != NULL)
						{
							free(monitor->rules);
						}
						monitor->rules = defined_rules;
						monitor->num_rules = defined_rule_index;
						monitor->last_byte_count = initial_count;
						monitor->history = initialize_history();
						monitor->last_update = time(NULL);
						monitor->last_accumulator_update = monitor->last_update;
						monitor->last_backup = monitor->last_update;
						
						initialized_monitors[initialized_monitor_index] = monitor;
						initialized_monitor_index++;
					}
					iptc_free(&iptables_snapshot);
				}
			}
		}

	
	}

	initialized_monitors[initialized_monitor_index] = NULL;
	return initialized_monitors;	
}





bw_monitor** load_bwmon_config(char *filename)
{
	FILE* config_file = fopen(filename, "r");
	
	char newline_terminator[3];
	newline_terminator[0] = '\n';
	newline_terminator[1] = '\r';


	//compute number of monitors to dynamically allocate array	
	int num_monitors = 0;
	if(config_file != NULL)
	{

		dyn_read_t next;
		unsigned long read_length;
		next = dynamic_read(config_file, newline_terminator, 2, &read_length);
		while(next.terminator != EOF)
		{
			if( strstr(next.str, "monitor ") == next.str || strstr(next.str, "monitor\t") == next.str)
			{
				num_monitors++;
			}
			free(next.str);
			next = dynamic_read(config_file, newline_terminator, 2, &read_length);
		}

		//if last line of file we can ignore data, since we need to have sub-data for each monitor
		if(next.str != NULL)
		{
			free(next.str);
		}	
		fclose(config_file);
	}

	
	//allocate monitors, array will be NULL terminated so we will know how many we have
	bw_monitor** monitors = (bw_monitor**)malloc((num_monitors+1)*sizeof(bw_monitor*));
	int monitor_index = 0;
	
	//parse the config file
	config_file = fopen(filename, "r");	
	if(config_file != NULL)
	{
		dyn_read_t next;
		unsigned long read_length;

		next = dynamic_read(config_file, newline_terminator, 2, &read_length);
		char** variable = parse_variable_definition_line(next.str);
		while(next.terminator != EOF)
		{
			//cycle past space outside monitor
			while(next.terminator != EOF && safe_strcmp(variable[0], "monitor") != 0)
			{
				if(variable[0] != NULL)
				{
					free(variable[0]);
					free(variable[1]);
				}
				free(variable);
				next = dynamic_read(config_file, newline_terminator, 2, &read_length);
				variable = parse_variable_definition_line(next.str);
			}
			
			//we've found a monitor section
			if(next.terminator != EOF)
			{
				bw_monitor* monitor = (bw_monitor*)malloc(sizeof(bw_monitor));
				monitor->name = variable[1];
			
				//initialize values of monitor to null values
				monitor->table = NULL;
				monitor->chain = NULL;
				monitor->interval_length = -1;
				monitor->interval_end = -1;
				monitor->history_length = -1;
				monitor->num_marks = 0;
				monitor->num_rules = 0;
				monitor->marks = NULL;
				monitor->rules = NULL;
				monitor->backup_dir = NULL;
				monitor->backup_freq = -1;
				monitor->accumulator_freq = -1;

		
				monitor->last_byte_count=-1;
				monitor->accumulator_count = 0;
				monitor->history = NULL;


				
				free(variable[0]);
				free(variable);
				next = dynamic_read(config_file, newline_terminator, 2, &read_length);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "monitor") != 0)
				{
					if(variable[0] != NULL)
					{
						int read; //used for parsing integers
						if(safe_strcmp(variable[0], "interval_length") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								monitor->interval_length = read;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "interval_end") == 0)
						{
							if(strcmp(variable[1], "minute") == 0)
							{
								monitor->interval_end = MINUTE;
							}
							else if(strcmp(variable[1], "hour") == 0)
							{
								monitor->interval_end = HOUR;
								
							}
							else if(strcmp(variable[1], "day") == 0)
							{
								monitor->interval_end = DAY;
							}
							else if(strcmp(variable[1], "week") == 0)
							{
								monitor->interval_end = WEEK;
							}
							else if(strcmp(variable[1], "month") == 0)
							{
								monitor->interval_end = MONTH;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "history_length") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								monitor->history_length = read;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "table") == 0)
						{
							monitor->table = variable[1];
						}
						else if(safe_strcmp(variable[0], "chain") == 0)
						{
							monitor->chain = variable[1];
						}
						else if(safe_strcmp(variable[0], "mark") == 0)
						{

							int *comma_list = parse_comma_list(variable[1]);
							if(comma_list[0] >= 0)
							{
								int comma_list_length = 0;
								while(comma_list[comma_list_length] >= 0)
								{
									comma_list_length++;
								}
								monitor->num_marks = comma_list_length;
								monitor->marks = comma_list;
							}
							else
							{
								free(comma_list);
							}
						}
						else if(safe_strcmp(variable[0], "rule") == 0)
						{
							int *comma_list = parse_comma_list(variable[1]);
							if(comma_list[0] >= 0)
							{
								int comma_list_length = 0;
								while(comma_list[comma_list_length] >= 0)
								{
									comma_list_length++;
								}
								monitor->num_rules = comma_list_length;
								monitor->rules = comma_list;
							}
							else
							{
								free(comma_list);
							}	
						
						}
						else if(safe_strcmp(variable[0], "backup_dir") == 0)
						{
							monitor->backup_dir = variable[1];
						}
						else if(safe_strcmp(variable[0], "backup_freq") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								monitor->backup_freq = read;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "accumulator_freq") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								monitor->accumulator_freq = read;
							}
							free(variable[1]);
						}

						else
						{
							free(variable[1]);
						}
						free(variable[0]);
					}
					free(variable);
					next = dynamic_read(config_file, newline_terminator, 2, &read_length);
					variable = parse_variable_definition_line(next.str);
				}
		
				/*	
				printf("table = %s\n", monitor->table);
				printf("chain = %s\n", monitor->chain);
				printf("history = %d\n", monitor->history_length);
				printf("interval_length = %d\n", monitor->interval_length);
				printf("interval_end = %d\n", monitor->interval_end);
				printf("num marks = %d\n", monitor->num_marks);
				printf("num rules = %d\n", monitor->num_rules);
				*/

				
				//test if we have all needed components of monitor
				if(	monitor->table != NULL &&
					monitor->chain != NULL &&
					monitor->history_length > 0 &&
					(monitor->interval_length > 0 || monitor->interval_end > 0) &&
					(monitor->num_rules > 0 || monitor->num_marks > 0)
				  	)
				{
					monitors[monitor_index] = monitor;
					monitor_index++;
				}

				// NOTE: There's a potential memory leak here in that monitors that are not fully
				// initialized don't have memory reclaimed.  I'll deal with this later
				// since it's a minor bug (we only load the config file once)
			}
		}
		fclose(config_file);
	}
	monitors[monitor_index] = NULL;
	
	return monitors;
}




//variable name is first non-whitespace, definition is part of string after first whitespace up until first newline
//line is freed at end of function, but variable definition, if defined, must be freed
//comments are allowed & marked by '#' BUT these must be at start of line -- comments halfway through line are not supported
char** parse_variable_definition_line(char* line)
{

	char** variable_definition = (char**)malloc(2*sizeof(char*));
	variable_definition[0] = NULL;
	variable_definition[1] = NULL;

	if(line != NULL)
	{
		int is_comment = 0;
		char* trimmed_line = strdup(line);
		trimmed_line = trim_flanking_whitespace(trimmed_line);
		is_comment = trimmed_line[0] == '#' ? 1  : 0;

		if(is_comment != 1)
		{
			char whitespace_separators[] = {'\t', ' '};
			unsigned long num_pieces;
			char** split_line = split_on_separators(trimmed_line, whitespace_separators, 2, 2, 1, &num_pieces);
			if(split_line[0] != NULL)
			{
				variable_definition[0] = split_line[0];
				if(split_line[1] != NULL)
				{
					variable_definition[1] = split_line[1];
				}
			}
			free(split_line);
		}
		free(trimmed_line);
		free(line);  // we always call with dynamically allocated memory, and for convenience we free it here, so we don't have to do it elsewhere
	}
	return variable_definition;
}


int* parse_comma_list(char* list)
{
	int comma_count = 0;
	int index = 0;
	while(index < strlen(list))
	{
		if(list[index] == ',')
		{
			comma_count++;
		}
		index++;
	}
		
	
	int max_values = comma_count+1;
	int *values = (int*)malloc((max_values+1)*sizeof(int));
	int valueIndex = 0;

	int last_comma = -1;
	index = 0;
	while(index < strlen(list))
	{
		if(list[index] == ',')
		{
			int field_len = (index - last_comma)-1;
			char *field = (char*)malloc((field_len+1) * sizeof(char));
			memcpy(field, list+last_comma+1, field_len);
			field[ field_len ] = '\0';
			
			
			int read;			
			if(sscanf(field, "%d", &read) > 0)
			{
				values[valueIndex] = read;
				valueIndex++;
			}
			free(field);
			last_comma=index;
		}
		index++;
	}
	
	int field_len = (index - last_comma)-1;
	char *field = (char*)malloc((field_len+1) * sizeof(char));
	memcpy(field, list+last_comma+1, field_len);
	field[ field_len ] = '\0';
	
	int read;			
	if(sscanf(field, "%d", &read) > 0)
	{
		values[valueIndex] = read;
		valueIndex++;
	}
	free(field);

	//since we are only accepting positive values, terminate with -1
	//it's a hack, but for now it works	
	values[valueIndex] = -1;
	return values;
}


int get_minutes_west(void)
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
	return minuteswest;
}
