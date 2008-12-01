/*  restricter -	A small daemon for inserting/removing rules
 *  			from iptables to impose access restrictions
 *  			at specific times
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#include <math.h>

#include "erics_tools.h"
#include <libiptc/libiptc.h>

/* directions */
#define DIRECTION_EGRESS	100
#define DIRECTION_INGRESS	200

/* update_types */
#define UPDATE_QUOTA_TEST	30
#define UPDATE_QUOTA_RESET	40
#define UPDATE_BLOCK_INSERT	50
#define UPDATE_BLOCK_REMOVE	60
#define UPDATE_SAVE_QUOTA_DATA	70


/* quota reset intervals */
#define INTERVAL_MINUTE	101
#define INTERVAL_HOUR	102
#define INTERVAL_DAY	103
#define INTERVAL_WEEK	104
#define	INTERVAL_MONTH	105

/* pid */
#define PID_PATH	"/var/run/restricterd.pid"

/* useful for message queue */
#define MSG_ID			148
#define REQUEST_MSG_TYPE 	160
#define RESPONSE_MSG_TYPE	172
#define MAX_MSG_LINE		250

/* output request types */
#define OUTPUT_REQUEST_ALL_QUOTAS	110
#define OUTPUT_REQUEST_ALL_BLOCKS	111
#define OUTPUT_REQUEST_ALL		112
#define OUTPUT_REQUEST_INDIVIDUAL	113

/* tells client if more than one update is scheduled */
#define OUTPUT_RETURNED_MULTIPLE	114
#define OUTPUT_RETURNED_SUCCESS		115
#define OUTPUT_RETURNED_FAILURE		116

typedef struct
{
	long int msg_type;
	char msg_line[MAX_MSG_LINE];
} message_t;



typedef struct u_n
{
	time_t update_time;
	unsigned char update_type;
	char* table;
	char* chain;
	long rule_index1;
	long rule_index2;
	long rule_index3;
	string_map* definition;
	
	/* 
	used only by quota updates
	include this here so we don't
	have to do lookup O(log(n)) in
	definition map when doing
	quota tests, which will be most 
	frequent type of update. Instead
	we just access structure -- O(1)
	*/
	time_t next_reset_time;
	int64_t counter;
	int64_t reference_count; 
	int64_t max_count;
} update_node;


#define IP_PARTS_NATIVE(n)      \
	(unsigned long)((n)>>24)&0xFF,   \
	(unsigned long)((n)>>16)&0xFF,   \
	(unsigned long)((n)>>8)&0xFF,    \
	(unsigned long)((n)&0xFF)
#define IP_PARTS(n) IP_PARTS_NATIVE(ntohl(n))
char* get_ip_str(struct in_addr ip);

void** load_restricterd_config(char* filename);
char** parse_variable_definition_line(char* line);



void get_ip_range_strs(int* start, int* end, char* prefix, int list_length, list* range_strs);
char** parse_ips(char* ip_str);
char** parse_ports(char* port_str);
char** parse_marks(char* list_str, unsigned long max_mask);
char** parse_quoted_list(char* list_str, char quote_char, char escape_char, char add_remainder_if_uneven_quotes);


long* combine_daily_and_hourly(char** daily, long* hourly);
long* parse_time_ranges(char* time_ranges, unsigned char is_weekly_range);
void merge_adjacent_time_ranges(long* time_ranges, unsigned char is_weekly_range);
unsigned long parse_time(char* time_str);
string_map* initialize_weekday_hash(void);
string_map* weekdays;

void load_quota_data(string_map* global_data, update_node* template_node, time_t current_time);
void save_quota_data(string_map* global_data, update_node* quota_update_node);
void save_all_quota_data(string_map* global_data, priority_queue* update_queue);
int create_path(const char *name, int mode);

void flush_iptables_chain(char* table, char* chain);
update_node* initialize_generic_update_node(string_map* def, string_map* global_data, int is_block);

int count_rules_in_chain(char* table, char* chain);
void adjust_update_rule_numbers(priority_queue** update_queue, string_map* global_data, char* table, char* chain, int first_removed_rule, int num_removed_rules);

time_t get_next_block_update_time(update_node *last_update, unsigned char* update_type);
time_t get_next_interval_end(time_t current_time, int end_type);

void initialize_quota_rules_for_direction(list** quota_rules, int direction, string_map* global_data, priority_queue** update_queue, time_t reference_time, time_t current_time);


void compute_block_rules(update_node* unode);
int compute_multi_rules(char** def, list* multi_rules, char** single_check, int never_single, char* rule_prefix, char* test_prefix1, char* test_prefix2, int is_negation, int mask_byte_index, char* proto, int requires_proto, int quoted_args);


void quota_test(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time);
void quota_reset(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time);
void update_block(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time);

void daemonize(int background);
void signal_handler(int sig);

char* get_output_for_update(update_node* update, char* update_name, char output_format);
void request_output(char** request_args, char output_format);
void run_daemon(char* config_file_path);

int terminated;
int output_requested;





int main (int argc, char **argv)
{
	/*
	 * 1) load config file
	 * 2) load data from saved config files if they exist, initializing quota monitoring data
	 * 3) test if quota ip bandwith monitoring rules exist, otherwise add them
	 * 4) flush block rule chains (eliminate all rules)
	 * 4) initialize quota counters to current values in monitoring rules
	 * 5) schedule next block rule updates, active rules get scheduled for right now
	 * 6) begin polling loop to continuously test if quotas have been exceeded or need to be reset, and whether
	 * 	we need to add/remove block rule for time frame
	 * 	6a) when a rule matches, deal with it
	 * 7) On kill signal, save quota data to file so we can easily resume
	 */

	/* initialize weekdays hash */
	weekdays = initialize_weekday_hash();

	int is_daemon = 0;
	char output_format = 'h';
	int next_opt;
	char* config_file_path = strdup("/etc/restricterd.conf");
	while((next_opt = getopt(argc, argv, "c:C:dDmMhHuU")) != -1)
	{	
		switch(next_opt)
		{
			case 'c':
			case 'C':
				config_file_path = strdup(optarg);
				break;
			case 'd':
			case 'D':
				is_daemon = 1;
				break;
			case 'm':
			case 'M':
				output_format = 'm';
				break;
			case 'h':
			case 'H':
				output_format = 'h';
				break;
			case 'u':
			case 'U':
			default:
				printf("USAGE: %s -C [config_file_path]\n", argv[0]);
				return 0;
		}
	}

	char** request_args = NULL;
	if(optind < argc)
	{
		int arg_length = argc-optind;
		request_args = (char**)malloc((1+arg_length)*sizeof(char*));
		int arg_index;
		for(arg_index =optind; arg_index < argc; arg_index++)
		{
			request_args[arg_index-optind] = strdup(argv[arg_index]);
		}
		request_args[arg_length] = NULL;
	}


	//test if a daemon is already running by trying to open the message queue (which daemon creates)
	int testq = msgget(ftok(PID_PATH, MSG_ID), 0777);
	int daemon_running = testq < 0 ? 0 : 1;
	

	if(is_daemon)
	{
		if(daemon_running)
		{
			printf("ERROR: daemon is already running, exiting.\n");
		}
		else
		{
			run_daemon(config_file_path);
		}
	}
	else
	{
		request_output(request_args, output_format); 
	}

	return 0;
}

void request_output(char** request_args, char output_format)
{
	// get queue
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 );
	if(mq < 0)
	{
		printf("ERROR: Could not open message queue.  Exiting.\n");
		return;
	}

	//get pid of daemon
	FILE* pid_file = fopen(PID_PATH, "r");
	int daemon_pid = -1;
	if(pid_file != NULL)
	{
		char newline_terminator[3] = { '\r', '\n' };
		dyn_read_t pid_read = dynamic_read(pid_file, newline_terminator, 2);
		if(pid_read.str != NULL)
		{
			if(sscanf(pid_read.str, "%d", &daemon_pid) != 1)
			{
				daemon_pid = -1;
			}
			free(pid_read.str);
		}
	}

	unsigned long responses_expected;
	message_t req_msg;
	req_msg.msg_type = REQUEST_MSG_TYPE;
	if(request_args == NULL)
	{
		req_msg.msg_line[0] = OUTPUT_REQUEST_ALL;
		req_msg.msg_line[1] = output_format;
		msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, 0);
		responses_expected = 1;
	}
	else
	{
		for(responses_expected = 0; request_args[responses_expected] != NULL; responses_expected++)
		{
			char* req_name = request_args[responses_expected];
			if(safe_strcmp(req_name, "ALL") == 0)
			{
				req_msg.msg_line[0] = OUTPUT_REQUEST_ALL;
			}
			else if(safe_strcmp(req_name, "ALL_QUOTAS") == 0)
			{
				req_msg.msg_line[0] = OUTPUT_REQUEST_ALL_QUOTAS;
			}
			else if(safe_strcmp(req_name, "ALL_BLOCKS") == 0)
			{
				req_msg.msg_line[0] = OUTPUT_REQUEST_ALL_BLOCKS;
			}
			else
			{
				req_msg.msg_line[0] = OUTPUT_REQUEST_INDIVIDUAL;
				sprintf(req_msg.msg_line + 2, "%s", req_name);
			}
			req_msg.msg_line[1] = output_format;
			msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, 0);
		}
	}

	// signal daemon that we've sent requests
	kill((pid_t)daemon_pid, SIGUSR1);


	message_t resp_msg;
	unsigned long messages_read = 0;
	while(messages_read < responses_expected) //blocking read
	{
		int q_read_status = msgrcv(mq, (void *)&resp_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 0);
		if(q_read_status > 0)
		{
			char update_status = resp_msg.msg_line[0];
			responses_expected = responses_expected + ( update_status == OUTPUT_RETURNED_MULTIPLE ? *((unsigned long*)(resp_msg.msg_line + 1)) : 0);
			if(update_status == OUTPUT_RETURNED_SUCCESS || update_status == OUTPUT_RETURNED_FAILURE)
			{
				printf("%s", resp_msg.msg_line + 1);
			}
			messages_read++;
		}
	}
	printf("\n");
}



void run_daemon(char* config_file_path)
{
	daemonize(1);
	
	//open message queue 
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 | IPC_CREAT );
	if(mq < 0)
	{
		exit(0);
	}

	/* load data */
	void** data = load_restricterd_config(config_file_path);
	priority_queue* update_queue = initialize_priority_queue();


	/* flush block rule chains (if they exist)*/
	string_map* global_data = (string_map*)data[0];
	if(global_data != NULL)
	{
		flush_iptables_chain( get_map_element(global_data, "ingress_filter_table"), get_map_element(global_data, "ingress_filter_chain"));
		flush_iptables_chain( get_map_element(global_data, "egress_filter_table"), get_map_element(global_data, "egress_filter_chain"));
	}

	/* initialize reference time */
	time_t reference_time = time(NULL);
	time_t current_time = time(NULL);
	
	
	/* initialize quota rules */
	initialize_quota_rules_for_direction( (list**)(&(data[2])), DIRECTION_EGRESS, global_data, &update_queue, reference_time, current_time);
	initialize_quota_rules_for_direction( (list**)(&(data[2])), DIRECTION_INGRESS, global_data, &update_queue, reference_time, current_time);
	

	/* initialize block rules */
	list* block_list = (list*)data[1];
	long block_index = 0;
	while(block_list->length >0)
	{
		string_map *next_block_def = (string_map*)shift_list(block_list);
		update_node* next_update = initialize_generic_update_node(next_block_def, global_data, 1);
		if(next_update != NULL)
		{
			compute_block_rules(next_update);
	
			/* we initialize by performing whatever update is OPPOSITE from what NEXT action is */	
			next_update->update_time = current_time;
			
			unsigned char next_update_type;
			get_next_block_update_time(next_update, &next_update_type);
			
			/*
			time_t next_time = get_next_block_update_time(next_update, &next_update_type);
			if(next_update_type == UPDATE_BLOCK_REMOVE) { printf("next block update is remove\n"); } else { printf("next block update is insert\n"); };
			printf("next time is %ld, current time is %ld\n", next_time, current_time);	
			*/

			char *priority_id = get_map_element( next_block_def, "name");
			if(priority_id == NULL)
			{
				char tmp[20];
				sprintf(tmp, "%ld", block_index);
				priority_id = dynamic_strcat(5, next_update->table, "-", next_update->chain, "-", tmp);
				set_map_element(next_block_def, "name", priority_id);
			}
			next_update->update_type = next_update_type == UPDATE_BLOCK_INSERT ? UPDATE_BLOCK_REMOVE : UPDATE_BLOCK_INSERT;
			update_block(next_update, &update_queue, global_data, priority_id, reference_time);
		}	
		block_index++;
	}

		

	/* intitialize save data update, save will take place every 24 hours */
	unsigned long save_quota_interval = 24*60*60;
	char* save_quota_interval_str = (char*)get_map_element(global_data, "quota_backup_interval");
	if(save_quota_interval_str != NULL)
	{
		sscanf(save_quota_interval_str, "%ld", &save_quota_interval);
	}
	
	update_node* save_update = (update_node*)malloc(sizeof(update_node));
	save_update->definition = NULL;
	save_update->table = NULL;
	save_update->chain = NULL;
	save_update->update_type = UPDATE_SAVE_QUOTA_DATA;
	save_update->update_time = current_time + save_quota_interval;
	push_priority_queue(update_queue, (unsigned long)(save_update->update_time - reference_time), " QUOTA_SAVE_UPDATE_QUEUE_NODE ", save_update);

	

	/* update loop */
	terminated = 0;
	output_requested = 0;	
	while( !terminated )
	{
		time_t current_time = time(NULL);
		priority_queue_node* next_pq = peek_priority_queue_node(update_queue);
		while(next_pq->priority <= (unsigned long)(current_time - reference_time) )
		{
			next_pq = shift_priority_queue_node(update_queue);
			char* priority_id = strdup(next_pq->id);
			update_node* next_update = (update_node*)free_priority_queue_node(next_pq);
			

			if(next_update->update_type == UPDATE_BLOCK_INSERT || next_update->update_type == UPDATE_BLOCK_REMOVE)
			{
				update_block(next_update, &update_queue, global_data, priority_id, reference_time);
			}
			else if(next_update->update_type == UPDATE_QUOTA_TEST)
			{
				quota_test(next_update, &update_queue, global_data, priority_id, reference_time);
			}
			else if(next_update->update_type == UPDATE_QUOTA_RESET)
			{
				quota_reset(next_update, &update_queue, global_data, priority_id, reference_time);
			}
			else if(next_update->update_type == UPDATE_SAVE_QUOTA_DATA)
			{
				save_all_quota_data(global_data, update_queue);
				next_update->update_time = current_time + save_quota_interval;
				push_priority_queue(update_queue, (unsigned long)(next_update->update_time - reference_time), priority_id, next_update);
			}
			free(priority_id);

			next_pq = peek_priority_queue_node(update_queue);
			
		}
		//sleep for 400 milliseconds, or until signal is caught
		usleep(400*1000); 

		if(output_requested)
		{
			message_t next_req_message;
			next_req_message.msg_line[0] = '\0';
			while(msgrcv(mq, (void *)&next_req_message, MAX_MSG_LINE, REQUEST_MSG_TYPE, IPC_NOWAIT) > 0)
			{
				char request_type = next_req_message.msg_line[0];
				char output_format = next_req_message.msg_line[1];
				if(request_type == OUTPUT_REQUEST_INDIVIDUAL)
				{
					message_t resp_msg;
					resp_msg.msg_type = RESPONSE_MSG_TYPE;
					
					char* request_name = next_req_message.msg_line + 2;
					printf("output requested, trying to get node with name = \"%s\"\n", request_name);
					priority_queue_node *p = get_priority_queue_node_with_id(update_queue, request_name);
					printf("%s\n", p==NULL ? "got null node with id" : "got valid node with id" );
					if(p != NULL)
					{
						update_node* u = (update_node*)p->value;
						char* output = get_output_for_update(u, p->id,  output_format);
						resp_msg.msg_line[0] = OUTPUT_RETURNED_SUCCESS;
						sprintf(resp_msg.msg_line + 1, "%s", output);
						free(output);
					}
					else
					{
						resp_msg.msg_line[0] = OUTPUT_RETURNED_FAILURE;
						if(output_format == 'm')
						{
							sprintf(resp_msg.msg_line + 1, "-1,-1 ");
						}
						else
						{
							sprintf(resp_msg.msg_line + 1, "No quotas or block rules defined with id %s\n", request_name);
						}
					}
					msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);

				}
				else
				{
					message_t resp_msg;
					resp_msg.msg_type = RESPONSE_MSG_TYPE;

					unsigned long num_to_send = 0;
					priority_queue* tmp_queue = initialize_priority_queue();
					while(update_queue->length > 0)
					{
						priority_queue_node* p = shift_priority_queue_node(update_queue);
						update_node* u = (update_node*)p->value;
						if( 	(u->update_type == UPDATE_QUOTA_RESET || u->update_type == UPDATE_QUOTA_TEST) && 
							(request_type == OUTPUT_REQUEST_ALL_QUOTAS || request_type == OUTPUT_REQUEST_ALL)
						  	)
						{
							num_to_send++;
						}
						if( 	(u->update_type == UPDATE_BLOCK_INSERT || u->update_type == UPDATE_BLOCK_REMOVE) && 
							(request_type == OUTPUT_REQUEST_ALL_BLOCKS || request_type == OUTPUT_REQUEST_ALL)
						  	)
						{
							num_to_send++;
						}
						push_priority_queue_node(tmp_queue, p);
					}
					resp_msg.msg_line[0] = OUTPUT_RETURNED_MULTIPLE;
					*((unsigned long*)(resp_msg.msg_line + 1)) = num_to_send;
					msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
					
					while(tmp_queue->length > 0)
					{
						priority_queue_node* p= shift_priority_queue_node(tmp_queue);
						update_node* u = (update_node*)p->value;
						if( 	(u->update_type == UPDATE_QUOTA_RESET || u->update_type == UPDATE_QUOTA_TEST) && 
							(request_type == OUTPUT_REQUEST_ALL_QUOTAS || request_type == OUTPUT_REQUEST_ALL)
						  	)
						{
							char* output = get_output_for_update(u, p->id,  output_format);
							resp_msg.msg_line[0] = OUTPUT_RETURNED_SUCCESS;
							sprintf(resp_msg.msg_line + 1, "%s", output);
							free(output);
							msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
						
						}
						if( 	(u->update_type == UPDATE_BLOCK_INSERT || u->update_type == UPDATE_BLOCK_REMOVE) && 
							(request_type == OUTPUT_REQUEST_ALL_BLOCKS || request_type == OUTPUT_REQUEST_ALL)
						  	)
						{
							char* output = get_output_for_update(u, p->id,  output_format);
							resp_msg.msg_line[0] = OUTPUT_RETURNED_SUCCESS;
							sprintf(resp_msg.msg_line + 1, "%s", output);
							free(output);
							msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);

						}


						push_priority_queue_node(update_queue, p);
					}
					
					unsigned long num_destroyed;
					destroy_priority_queue(tmp_queue, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				}
			}
			output_requested = 0;
		}
	}


	/* on kill signal, flush filter chains */	
	if(global_data != NULL)
	{
		flush_iptables_chain( get_map_element(global_data, "ingress_filter_table"), get_map_element(global_data, "ingress_filter_chain"));
		flush_iptables_chain( get_map_element(global_data, "egress_filter_table"), get_map_element(global_data, "egress_filter_chain"));
	}

	/* save quota data */
	save_all_quota_data(global_data, update_queue);


	/* remove pid file */
	unlink(PID_PATH);

	/* destroy message queue */
	struct msqid_ds queue_data;
	msgctl(mq, IPC_RMID, &queue_data);

}



char* get_output_for_update(update_node* update, char* update_name,  char output_format)
{
	char* output_str = NULL;
	if(update->update_type == UPDATE_QUOTA_RESET || update->update_type == UPDATE_QUOTA_TEST)
	{
		char counter_str[25];
		char max_count_str[25];
		sprintf(counter_str, "%lld", (long long int)(update->counter <= update->max_count ? update->counter : update->max_count) );
		sprintf(max_count_str, "%lld", (long long int)update->max_count);

		if(output_format == 'h')
		{
			double c;
			double m;
			sscanf(counter_str, "%lf", &c);
			sscanf(max_count_str, "%lf", &m);
			double percent = 100*(c/m);
			char percent_str[6];
			sprintf(percent_str, "%5.2lf", percent);
			trim_flanking_whitespace(percent_str);
			output_str = dynamic_strcat(9, "quota ", update_name, " is ", percent_str, "\% used (", counter_str, " of ", max_count_str, " bytes)\n");
		}
		else
		{
			output_str = dynamic_strcat(4, counter_str, ",", max_count_str, " ");
		}
	}
	else if(update->update_type == UPDATE_BLOCK_INSERT || update->update_type == UPDATE_BLOCK_REMOVE)
	{
		time_t next_time_active = update->update_time;
		if(output_format == 'h')
		{
			struct tm* next_time_info = localtime( &next_time_active );
			
			
			if(next_time_active > 0)
			{
				char* status_str = update->update_type == UPDATE_BLOCK_REMOVE ? " is currently active, and will next be deactivated " : " is not currently active, and will next be active ";
				output_str = dynamic_strcat(4, "block rule ", update_name, status_str, asctime(next_time_info));
			}
			else
			{
				char* status_str = update->update_type == UPDATE_BLOCK_REMOVE ? " is currently active, and will never be deactivated " : " is not currently active and will never be active";
				output_str = dynamic_strcat(3, "block rule ", update_name, status_str);
			}
		}
		else
		{
			char tmp[25];
			sprintf(tmp, "%d,%ld ",  update->update_type == UPDATE_BLOCK_REMOVE ? 1 : 0, (unsigned long)next_time_active);
			output_str = strdup(tmp);
		}
	}
	return output_str;
}


void daemonize(int background) //background variable is useful for debugging, causes program to run in foreground if 0
{

	if(background != 0)
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
	}


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
		//ignore other signals
	}
	if(sig == SIGUSR1)
	{
		output_requested = 1;
	}
}

void quota_test(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time)
{
	//printf("quota test %s, seconds to next reset = %ld\n", priority_id, (long)(next_update->next_reset_time - next_update->update_time)); 
	
	/*
	 * 1) if current time is past next reset time, schedule a quota reset for right now & return, otherwise...
	 * 2) look up rule & update byte counters
	 * 3) if byte count is above allowed maximum, schedule next quota reset & insert block rules
	 * 	3a) if ingress_filter_chain defined, add a rule there, set to rule_index2
	 *	3b) if egress_filter_chain defined, and a rule there, set to rule_index3
	 * 4) otherwise schedule anpther quota_test in 1 second
	 */
	if( next_update->update_time >= next_update->next_reset_time )
	{
		//printf("quota test time-out reset\n");
		//schedule reset for right now
		next_update->update_type = UPDATE_QUOTA_RESET;
		push_priority_queue(*update_queue,  (unsigned long)(next_update->update_time - reference_time), priority_id, (void*)next_update );
	}
	else
	{
		//update byte count
		iptc_handle_t iptables_snapshot = iptc_init(next_update->table);
		if(iptables_snapshot)
		{
			if(iptc_is_chain(next_update->chain, iptables_snapshot))
			{
				struct ipt_counters* rule_counter = iptc_read_counter(next_update->chain, next_update->rule_index1, &iptables_snapshot);
				int64_t new_reference_count = rule_counter->bcnt;	
				int64_t next_increment = new_reference_count - next_update->reference_count;
				next_increment = next_increment >= 0 ? next_increment : 0;

				//printf("quota test old_count = %lld, increment = %lld, max count = %lld\n", (long long int)next_update->counter, (long long int)next_increment, (long long int)next_update->max_count);
				
				next_update->counter = next_update->counter + next_increment;
				next_update->reference_count = new_reference_count;
			}
		}
		iptc_free(&iptables_snapshot);

		
		if( next_update->counter >= next_update->max_count )
		{
			//printf("quota test limit reached, scheduling next reset\n");
			//insert block rules & schedule reset
			char* ingress_filter_table = get_map_element(global_data, "ingress_filter_table");
			char* ingress_filter_chain = get_map_element(global_data, "ingress_filter_chain");
			char* egress_filter_table  = get_map_element(global_data, "egress_filter_table");
			char* egress_filter_chain  = get_map_element(global_data, "egress_filter_chain");

			if( ingress_filter_table != NULL && ingress_filter_chain != NULL )
			{
				long current_num_rules = count_rules_in_chain(ingress_filter_table, ingress_filter_chain);
				next_update->rule_index2 = current_num_rules + 1;
				
				char* ip = get_map_element(next_update->definition, "ip");
				char* rule_end =  ip != NULL ? dynamic_strcat(3, " -d ", ip, " -j REJECT") : strdup(" -j REJECT");
				char* rule = dynamic_strcat(6, "iptables -t ", ingress_filter_table, " -A ", ingress_filter_chain, " ", rule_end);
				system( rule );
				free(rule_end);
				free(rule);
			}
			if( egress_filter_table != NULL && egress_filter_chain != NULL )
			{
				long current_num_rules = count_rules_in_chain(egress_filter_table, egress_filter_chain);
				next_update->rule_index3 = current_num_rules + 1;
				
				char* ip = get_map_element(next_update->definition, "ip");
				char* rule_end =  ip != NULL ? dynamic_strcat(3, " -s ", ip, " -j REJECT") : strdup(" -j REJECT");
				char* rule = dynamic_strcat(6, "iptables -t ", egress_filter_table, " -A ", egress_filter_chain, " ", rule_end);
				system( rule );
				free(rule_end);
				free(rule);
			}	
			
			next_update->update_type = UPDATE_QUOTA_RESET;
			next_update->update_time = next_update->next_reset_time;
			push_priority_queue(*update_queue, (unsigned long)(next_update->update_time - reference_time), priority_id, (void*)next_update );
		}
		else
		{
			//printf("quota test, scheduling next test\n");
			
			//schedule another quota_test
			next_update->update_time = next_update->update_time + 1;
			push_priority_queue(*update_queue, (unsigned long)(next_update->update_time - reference_time), priority_id, (void*)next_update );
		}
	}

}



void quota_reset(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time)
{
	/*
	* 1) if rule_index2 > 0, remove rule from ingress_filter_chain & update rule numbers in update_queue
	* 2) if rule_index3 > 0, remove rule from egress_filter_chain & update rule numbers in update_queue
	* 3) compute next reset time
	* 4) reset counter to 0 & reset reference counter
	* 5) schedule next quota_test in 1 second
	*/
	
	//update reference count (in case any packets got through)
	iptc_handle_t iptables_snapshot = iptc_init(next_update->table);
	if(iptables_snapshot)
	{
		if(iptc_is_chain(next_update->chain, iptables_snapshot))
		{
			struct ipt_counters* rule_counter = iptc_read_counter(next_update->chain, next_update->rule_index1, &iptables_snapshot);
			next_update->reference_count = rule_counter->bcnt;	
			next_update->counter = 0;
		}
	}
	iptc_free(&iptables_snapshot);
	
	
	char* ingress_filter_table = get_map_element(global_data, "ingress_filter_table");
	char* ingress_filter_chain = get_map_element(global_data, "ingress_filter_chain");
	char* egress_filter_table  = get_map_element(global_data, "egress_filter_table");
	char* egress_filter_chain  = get_map_element(global_data, "egress_filter_chain");
	if(next_update->rule_index2 > 0)
	{
		char tmp[20];
		sprintf(tmp, "%ld", next_update->rule_index2);
		char* delete = dynamic_strcat(6, "iptables -t ", ingress_filter_table, " -D ", ingress_filter_chain, " ", tmp);
		system( delete );
		adjust_update_rule_numbers(update_queue, global_data, ingress_filter_table, ingress_filter_chain, next_update->rule_index2,1);
		next_update->rule_index2 = -1;
	}
	if(next_update->rule_index3 > 0)
	{
		char tmp[20];
		sprintf(tmp, "%ld", next_update->rule_index3);
		char* delete = dynamic_strcat(6, "iptables -t ", egress_filter_table, " -D ", egress_filter_chain, " ", tmp);
		system( delete );
		adjust_update_rule_numbers(update_queue, global_data, egress_filter_table, egress_filter_chain, next_update->rule_index3,1);
		next_update->rule_index3 = -1;
	}

	//compute next reset time
	long* reset_interval = (long*)get_map_element(next_update->definition, "reset_interval");
	if(reset_interval[0] >= 0)
	{
		next_update->next_reset_time =  get_next_interval_end(next_update->update_time, reset_interval[0]);
	}
	else
	{
		next_update->next_reset_time = next_update->update_time + reset_interval[1];
	}


	next_update->update_type = UPDATE_QUOTA_TEST;
	next_update->update_time = next_update->update_time + 1;
	push_priority_queue(*update_queue, (unsigned long)(next_update->update_time - reference_time), priority_id,  (void*)next_update );

}
void update_block(update_node *next_update, priority_queue** update_queue, string_map* global_data, char* priority_id, time_t reference_time)
{
	/*
	* 1) if this is a remove, remove the rules & update rule indices in rest of update_queue
	* 2) if this is an insert, insert the rules
	* 3) calculate time of next block event (remove/insert) 
	* 4) set update node to reflect next event, and re-insert into update_queue
	*/
	//printf("DOING  BLOCK UPDATE, OF TYPE %s, TIME= %ld, FIRST RULE INDEX = %ld, \n", next_update->update_type == UPDATE_BLOCK_REMOVE ? "REMOVE" : "INSERT", next_update->update_time, next_update->rule_index1 );
	char** rule_list = (char**)get_map_element(next_update->definition, "iptables_rule_list");
	long* rule_list_length = (long*)get_map_element(next_update->definition, "iptables_rule_list_length");
	if(next_update->update_type == UPDATE_BLOCK_REMOVE && next_update->rule_index1 > 0)
	{
		//remove rules & update rule indices, set rule indices to negative values in update node
		long iptables_index = next_update->rule_index1;
		char iptables_index_str[25];
		sprintf(iptables_index_str, "%ld", iptables_index);
		char* delete_command = dynamic_strcat(6, "iptables -t ", next_update->table, " -D ", next_update->chain, " ", iptables_index_str);

		long rule_list_index;
		for(rule_list_index = 0; rule_list_index < *rule_list_length; rule_list_index++)
		{
			system( delete_command );
		}
		free(delete_command);
		
		adjust_update_rule_numbers(update_queue, global_data, next_update->table, next_update->chain, next_update->rule_index1,*rule_list_length);
		
		
		next_update->rule_index1 = -1;
		next_update->rule_index2 = -1;

	}
	else if(next_update->update_type == UPDATE_BLOCK_INSERT)
	{
		//insert rules & set rule indices in update node
		long current_num_rules = count_rules_in_chain(next_update->table, next_update->chain);
		long start_rule_num = current_num_rules + 1;
		long end_rule_num = current_num_rules + *rule_list_length;

		long rule_list_index;
		for(rule_list_index = 0; rule_list[rule_list_index] != NULL; rule_list_index++)
		{
			//printf("INSERTING:'%s'\n", rule_list[rule_list_index] );
			system( rule_list[rule_list_index] );
		}
		next_update->rule_index1 = start_rule_num;
		next_update->rule_index2 = end_rule_num;
	}


	unsigned char next_update_type;
	next_update->update_time = get_next_block_update_time(next_update, &next_update_type);	
	next_update->update_type = next_update_type;
	if(next_update_type == UPDATE_BLOCK_REMOVE)
	{
		//printf("remove of %s scheduled for %ld\n", priority_id, next_update->update_time);
	}
	
	push_priority_queue(*update_queue, (unsigned long)(next_update->update_time - reference_time), priority_id, (void*)next_update );
}


time_t get_next_block_update_time(update_node *last_update, unsigned char* update_type)
{
	string_map* rule_def = last_update->definition;
	long* weekly_block_times = (long*)get_map_element(rule_def, "weekly_block_times");

	time_t last_update_time = last_update->update_time;
	struct tm* last_update_time_info = localtime( &(last_update->update_time) );
	long seconds_since_sunday_midnight =	(24*60*60*(long)last_update_time_info->tm_wday) + 
						(60*60*(long)last_update_time_info->tm_hour) + 
						(60*(long)last_update_time_info->tm_min) + 
						(long)last_update_time_info->tm_sec ;


	time_t next_update_time;
	if(weekly_block_times != NULL)
	{
		/* compute number of elements in weekly_block_times array */
		int wbt_length = 0;
		for(wbt_length = 0; weekly_block_times[wbt_length] > 0; wbt_length++){}

		
		/* compute seconds since last sunday midnight rule will next be active */		
		int last_before_first = weekly_block_times[wbt_length-1] < weekly_block_times[0];
		int next_event_index = last_before_first ? wbt_length-1 : 0;
		long week_count = 0;
		long next_event_seconds_since_sunday_midnight = week_count + weekly_block_times[next_event_index];
		while( next_event_seconds_since_sunday_midnight <= seconds_since_sunday_midnight)
		{
			
			next_event_index++;
			week_count = week_count + (	
							(next_event_index == wbt_length && (!last_before_first)) || 
							(next_event_index == wbt_length-1 && last_before_first) ?
							7*24*60*60 : 0
							);
			next_event_index = next_event_index == wbt_length ? 0 : next_event_index;
			next_event_seconds_since_sunday_midnight = week_count + weekly_block_times[next_event_index];
		}


		/* compute update time and return */
		next_update_time = last_update_time - seconds_since_sunday_midnight + next_event_seconds_since_sunday_midnight;
		*update_type = next_event_index % 2 == 0 ? UPDATE_BLOCK_INSERT : UPDATE_BLOCK_REMOVE;
	
		/*
		printf(	"\tlast update time = %ld, seconds since sunday midnight = %ld, next_block_interval = %ld, next_update_time = %ld\n", 
			last_update_time, seconds_since_sunday_midnight, next_block_interval, next_update_time
			);
		*/
	}
	else
	{
		/* always active */
		next_update_time = (time_t)0;
		*update_type = UPDATE_BLOCK_REMOVE;	
	}
	return next_update_time;
}


time_t get_next_interval_end(time_t current_time, int end_type)
{
	time_t next_end = (time_t)0;
	struct tm* curr = localtime(&current_time);
	if(end_type == INTERVAL_MINUTE)
	{
		curr->tm_sec = 0;
		curr->tm_min = curr->tm_min+1;
		next_end = mktime(curr);
	}
	else if(end_type == INTERVAL_HOUR)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = curr->tm_hour+1;
		next_end = mktime(curr);
	}
	else if(end_type == INTERVAL_DAY)
	{
		curr->tm_sec  = 0;
		curr->tm_min  = 0;
		curr->tm_hour = 0;
		curr->tm_mday = curr->tm_mday+1;
		next_end = mktime(curr);
	}
	else if(end_type == INTERVAL_WEEK)
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
	else if(end_type == INTERVAL_MONTH)
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



/* 
 * Note we've currently maxed out out one whole byte of address space
 * in the connmark at this point.  If we want to match in
 * further dimensions, we will have to be greedy and take 
 * even more address space
 */
void compute_block_rules(update_node* unode)
{
	string_map* rule_def = unode->definition;
		
	char* is_ingress_str = get_map_element(rule_def, "is_ingress");
	int is_ingress = (is_ingress_str == NULL || safe_strcmp(is_ingress_str, "0") == 0 ) ? 0 : 1;


	list* multi_rules = initialize_list();
	char* single_check = strdup("");
	char* rule_prefix = dynamic_strcat(5, "iptables -t ", unode->table, " -A ", unode->chain, " ");
	
	/*
	 * layer7 && ipp2p can not be negated.  To negate them
	 * set a mark/connmark and negate that
	 */
	char* layer7_def = get_map_element(rule_def, "layer7");
	if(layer7_def != NULL)
	{
		char* tmp = dynamic_strcat(2, " -m layer7 --l7proto ", layer7_def );
		dcat_and_free(&single_check, &tmp, 1, 1);
	}
	char* ipp2p_def = get_map_element(rule_def, "ipp2p");
	if(ipp2p_def != NULL)
	{
		char* tmp = dynamic_strcat(2, " -m ipp2p --", ipp2p_def );
		dcat_and_free(&single_check, &tmp, 1, 1);
	}




	/* make sure proto is lower case */
	char* proto = get_map_element(rule_def, "proto");
	if(proto == NULL)
	{
		proto = strdup("both");
		set_map_element(rule_def, "proto", proto);

	}
	to_lowercase(proto);
	if( safe_strcmp(proto, "udp") != 0 && safe_strcmp(proto, "tcp") != 0 && safe_strcmp(proto, "both") != 0 )
	{
		char* tmp;
		tmp = set_map_element(rule_def, "proto", strdup("both"));
		free(tmp);
		proto = (char*)get_map_element(rule_def, "proto");
	}
	int include_proto = strcmp(proto, "both") == 0 ? 0 : 1;


	/* parse multi rules */
	int mask_byte_index = 0;
	list* initial_mask_list = initialize_list();
	list* final_mask_list = initialize_list();


	
	
	/* url matches are a bit of a special case, handle them first */
	/* we have to save this mask_byte_index specially, because it must be set separately, so it only gets set if packet is http request */
	int url_mask_byte_index = mask_byte_index; 
	char** url_match_def = (char**)get_map_element(rule_def, "url");
	char** url_regex_def = (char**)get_map_element(rule_def, "url_regex");
	int url_is_negated = url_match_def == NULL && url_regex_def == NULL ? 1 : 0;
	url_match_def = url_is_negated ? (char**)get_map_element(rule_def, "not_url") : url_match_def;
	url_regex_def = url_is_negated ? (char**)get_map_element(rule_def, "not_url_regex") : url_regex_def;
	proto = url_match_def == NULL && url_regex_def == NULL ? proto : "tcp";
	int url_rule_count = 0;
	int url_rule_index = 0;
	if(url_match_def != NULL){ for(url_rule_index=0; url_match_def[url_rule_index] != NULL; url_rule_index++){ url_rule_count++;} }
	if(url_regex_def != NULL){ for(url_rule_index=0; url_regex_def[url_rule_index] != NULL; url_rule_index++){ url_rule_count++;} }
	int url_is_multi = url_rule_count <= 1 ? 0 : 1;
	compute_multi_rules( url_match_def, multi_rules, &single_check, url_is_multi, rule_prefix, " -m weburl --contains ", "",  url_is_negated, mask_byte_index, proto, include_proto, 1 );
	compute_multi_rules( url_regex_def, multi_rules, &single_check, url_is_multi, rule_prefix, " -m weburl --contains_regex ", "",  url_is_negated, mask_byte_index, proto, include_proto, 1 );
	push_list(initial_mask_list, (void*)&url_is_negated);
	push_list(final_mask_list, (void*)&url_is_multi);
	mask_byte_index++;

	/* mark matches */
	char** mark_def = get_map_element(rule_def, "mark");
	int mark_is_negated = mark_def == NULL ? 1 : 0;
	mark_def = mark_def == NULL ? get_map_element(rule_def, "not_mark") : mark_def;
	/* we can't do single negation with mark match, so always add seperate multi-match if mark is negated */
	int mark_is_multi = compute_multi_rules(mark_def, multi_rules, &single_check, mark_is_negated, rule_prefix, " -m mark ", " --mark ", mark_is_negated, mask_byte_index, proto, include_proto, 0) == 2;
	push_list(initial_mask_list, (void*)&mark_is_negated);
	push_list(final_mask_list, (void*)&mark_is_multi);
	mask_byte_index++;	

	/* connmark matches */
	char** connmark_def = get_map_element(rule_def, "connmark");
	int connmark_is_negated = connmark_def == NULL ? 1 : 0;
	connmark_def = connmark_def == NULL ? get_map_element(rule_def, "not_connmark") : connmark_def;
	int connmark_is_multi = compute_multi_rules(connmark_def, multi_rules, &single_check, 0, rule_prefix, " -m connmark ", " --mark ", connmark_is_negated, mask_byte_index, proto, include_proto, 0) == 2;
	push_list(initial_mask_list, (void*)&connmark_is_negated);
	push_list(final_mask_list, (void*)&connmark_is_multi);
	mask_byte_index++;	


	/*
	 * for ingress source = remote, destination = local 
	 * for egress source = local, destination = remote
	 */
	char** src_def = get_map_element(rule_def, (is_ingress ? "remote_addr" : "local_addr"));
	int src_is_negated = src_def == NULL ? 1 : 0;
	src_def = src_def == NULL ? get_map_element(rule_def, (is_ingress ? "not_remote_addr" : "not_local_addr")) : src_def;
	int src_is_multi = compute_multi_rules(src_def, multi_rules, &single_check, 0, rule_prefix, " -s ", "",  src_is_negated, mask_byte_index, proto, include_proto, 0) == 2;
	push_list(initial_mask_list, (void*)&src_is_negated);
	push_list(final_mask_list, (void*)&src_is_multi);
	mask_byte_index++;	

	char** dst_def = get_map_element(rule_def, (is_ingress ? "local_addr"  : "remote_addr"));
	int dst_is_negated = dst_def == NULL ? 1 : 0;
	dst_def = dst_def == NULL ? get_map_element(rule_def, (is_ingress ? "not_local_addr" : "not_remote_addr")) : dst_def;
	int dst_is_multi = compute_multi_rules(dst_def, multi_rules, &single_check, 0, rule_prefix, " -d ", "",  dst_is_negated, mask_byte_index, proto, include_proto, 0) == 2;
	push_list(initial_mask_list, (void*)&dst_is_negated);
	push_list(final_mask_list, (void*)&dst_is_multi);
	mask_byte_index++;



	char** sport_def = get_map_element(rule_def, (is_ingress ? "remote_port" : "local_port"));
	int sport_is_negated = sport_def == NULL ? 1 : 0;
	sport_def = sport_def == NULL ? get_map_element(rule_def, (is_ingress ? "not_remote_port" : "not_local_port")) : sport_def;
	int sport_is_multi = compute_multi_rules(sport_def, multi_rules, &single_check, 0, rule_prefix, " --sport ", "", sport_is_negated, mask_byte_index, proto, 1, 0) == 2;
	push_list(initial_mask_list, (void*)&sport_is_negated);
	push_list(final_mask_list, (void*)&sport_is_multi);
	mask_byte_index++;		


	char** dport_def = get_map_element(rule_def, (is_ingress ? "local_port"  : "remote_port"));
	int dport_is_negated = dport_def == NULL ? 1 : 0;
	dport_def = dport_def == NULL ? get_map_element(rule_def, (is_ingress ? "not_local_port" : "not_remote_port")) : dport_def;
	int dport_is_multi = compute_multi_rules(dport_def, multi_rules, &single_check, 0, rule_prefix, " --dport ", "", dport_is_negated, mask_byte_index, proto, 1, 0) == 2;
	push_list(initial_mask_list, (void*)&dport_is_negated);
	push_list(final_mask_list, (void*)&dport_is_multi);
	mask_byte_index++;	

	list* all_rules = initialize_list();
	if(multi_rules->length > 0)
	{
		if(strlen(single_check) > 0)
		{
			char* dummy_multi[] = { single_check, NULL };
			int requires_proto = strcmp(proto, "both") == 0 && sport_def == NULL && dport_def == NULL ? 0 : 1;
			compute_multi_rules(dummy_multi, multi_rules, &single_check, 1, rule_prefix, " ", "", 0, mask_byte_index, proto, requires_proto, 0);
			mask_byte_index++;
		}
	

		/*
		printf("final mask length = %ld\n", final_mask_list->length);
		printf("src is multi = %d\n", src_is_multi);
		unsigned long mi;
		int* one = shift_list(final_mask_list);
		int* two = shift_list(final_mask_list);
		printf("one = %d, two = %d\n", *one, *two);
		unshift_list(final_mask_list, two);
		unshift_list(final_mask_list, one);
		*/

		unsigned long initial_url_mark = 0x01000000 * url_is_negated * url_is_multi;
		unsigned long initial_main_mark = 0;
		unsigned long final_match = 0;
		int next_mask_index;
		for(next_mask_index = 0; next_mask_index <mask_byte_index ; next_mask_index++)
		{
			int tmp  = 1;
			int* next_is_multi = &tmp;
			if(final_mask_list->length > 0)
			{
				next_is_multi = shift_list(final_mask_list);
			}
			else
			{
				*next_is_multi = 1;
			}
			

			unsigned long next_mark_bit = 0x01000000 * (unsigned long)pow(2, next_mask_index) * (*next_is_multi);
			final_match = final_match + next_mark_bit;
			if(initial_mask_list->length > 0)
			{
				int* is_negation = (int*)shift_list(initial_mask_list);
				if(*is_negation == 1 && next_mask_index != url_mask_byte_index )
				{
					initial_main_mark = initial_main_mark + next_mark_bit;
				}
			}
			/* else it's last single_check mark which is never initialized to one */
		
		}

		if(initial_main_mark > 0)
		{
			//set main_mark unconditionally
			char mark[12];
			sprintf(mark, "0x%lX", initial_main_mark);
			push_list(all_rules, dynamic_strcat(4, rule_prefix, " -j CONNMARK --set-mark ", mark, "/0xFF000000" ));
		}
		if(initial_url_mark > 0) //do url_mark second since because in order to set main mark we use full mask of 0xFF000000
		{
			//set proper mark if this is an http request
			char mark[12];
			sprintf(mark, "0x%lX", initial_url_mark);
			push_list(all_rules, dynamic_strcat(5,  rule_prefix, " -p tcp  -m weburl --contains http -j CONNMARK --set-mark ", mark, "/", mark));
		}
		
		//put all rules in place from multi_rules list
		while(multi_rules->length > 0)
		{
			push_list(all_rules, shift_list(multi_rules));
		}
		unsigned long tmp_length;
		destroy_list(multi_rules, DESTROY_MODE_IGNORE_VALUES, &tmp_length);

		//if final mark matches perfectly with mask of 0xFF000000, REJECT
		char final_match_str[12];
		sprintf(final_match_str, "0x%lX", final_match);
		push_list(all_rules, dynamic_strcat(4, rule_prefix, " -m connmark --mark ", final_match_str, "/0xFF000000 -j REJECT" ));

		//if final mark does not match (i.e. we didn't reject), unconditionally reset mark to 0x0 with mask of 0xFF000000
		push_list(all_rules, dynamic_strcat(2, rule_prefix,  " -j CONNMARK --set-mark 0x0/0xFF000000" ));
	}
	else
	{
		if( strcmp(proto, "both") == 0 )
		{	
			if( dport_def == NULL && sport_def == NULL )
			{
				push_list(all_rules, dynamic_strcat(3, rule_prefix, single_check, " -j REJECT" ));
			}
			else
			{
				push_list(all_rules, dynamic_strcat(4, rule_prefix, " -p tcp ", single_check, " -j REJECT" ));
				push_list(all_rules, dynamic_strcat(4, rule_prefix, " -p udp ", single_check, " -j REJECT" ));
			}
		}
		else
		{
			push_list(all_rules, dynamic_strcat(6, rule_prefix, " -p ", proto, " ", single_check, " -j REJECT" ));
		}
	}


	unsigned long* num_rules = (unsigned long*)malloc(sizeof(unsigned long));
	char** block_rule_list = (char**) destroy_list( all_rules, DESTROY_MODE_RETURN_VALUES, num_rules);
	set_map_element(rule_def, "iptables_rule_list", block_rule_list);
	set_map_element(rule_def, "iptables_rule_list_length", num_rules);
	
	/*
	int i;
	for(i=0; block_rule_list[i] != NULL; i++)
	{
		printf("%s\n", block_rule_list[i]);
	}
	*/
}


/* returns 0 if no rules found, 1 if one rule found AND included in single_check, otherwise 2 */
int compute_multi_rules(char** def, list* multi_rules, char** single_check, int never_single, char* rule_prefix, char* test_prefix1, char* test_prefix2, int is_negation, int mask_byte_index, char* proto, int requires_proto, int quoted_args)
{
	int parse_type = 0;
	if(def != NULL)
	{
		int num_rules; 
		for(num_rules=0; def[num_rules] != NULL; num_rules++){}
		if(num_rules == 1 && !never_single)
		{
			parse_type = 1;
			char* tmp = dynamic_strcat(7, " ", test_prefix1, (is_negation ? " ! " : " "), test_prefix2, (quoted_args ? " \"" : " "), def[0], (quoted_args ? "\" " : " ") );
			dcat_and_free(&tmp, single_check, 1, 1 );
		}
		else
		{
			parse_type = 2;
			unsigned long mask = 0x01000000 * (unsigned long)pow(2, mask_byte_index);
			char mask_str[12];
			sprintf(mask_str, "0x%lX", mask);
			char* connmark_part = dynamic_strcat(4, " -j CONNMARK --set-mark ", (is_negation ? "0x0" : mask_str), "/", mask_str);


			int rule_index =0;
			for(rule_index=0; def[rule_index] != NULL; rule_index++)
			{
				char* common_part = dynamic_strcat(7, test_prefix1, " ", test_prefix2, (quoted_args ? " \"" : " "),  def[rule_index], (quoted_args ? "\" " : " "), connmark_part);
				if(strcmp(proto, "both") == 0)
				{
					if(requires_proto)
					{
						push_list(multi_rules, dynamic_strcat(3, rule_prefix, " -p tcp ", common_part));
						push_list(multi_rules, dynamic_strcat(3, rule_prefix, " -p udp ", common_part));
					}
					else
					{
						push_list(multi_rules, dynamic_strcat(3, rule_prefix, " ", common_part ));
					}
				}
				else
				{
					push_list(multi_rules, dynamic_strcat(5, rule_prefix, " -p ", proto, " ", common_part));
				}
				free(common_part);
			}
			free(connmark_part);
		}
	}
	return parse_type;
}



/* 
 * parses list of quoted strings, ignoring escaped quote characters that are not themselves escaped 
 * Note that we don't de-escape anything here.  If necessary that should be done elsewhere.
 */ 
char** parse_quoted_list(char* list_str, char quote_char, char escape_char, char add_remainder_if_uneven_quotes)
{
	
	long num_quotes = 0;
	long list_index = 0;
	char previous_is_quoted = 0;
	for(list_index=0; list_str[list_index] != '\0'; list_index++)
	{
		num_quotes = num_quotes + ( list_str[list_index] == quote_char && !previous_is_quoted ? 1 : 0);
		previous_is_quoted = list_str[list_index] == escape_char && !previous_is_quoted ? 1 : 0;
	}
	
	char** pieces = (char**)malloc( ((long)(num_quotes/2)+2) * sizeof(char*) );
	long piece_index = 0;
	long next_start_index=-1;
	previous_is_quoted = 0;	
	for(list_index=0; list_str[list_index] != '\0'; list_index++)
	{
		if( list_str[list_index] == quote_char && !previous_is_quoted )
		{
			if(next_start_index < 0)
			{
				next_start_index = list_index+1;
			}
			else
			{
				long length = list_index-next_start_index;
				char* next_piece = (char*)malloc( (1+length)*sizeof(char) );
				memcpy(next_piece, list_str+next_start_index, length);
				next_piece[length] = '\0';
				pieces[piece_index] = next_piece;
				piece_index++;
				next_start_index = -1;
			}
		}
		previous_is_quoted = list_str[list_index] == escape_char && !previous_is_quoted ? 1 : 0;
	}
	if(add_remainder_if_uneven_quotes && next_start_index >= 0)
	{
		long length = 1+list_index-next_start_index;
		char* next_piece = (char*)malloc( (1+length)*sizeof(char) );
		memcpy(next_piece, list_str+next_start_index, length);
		next_piece[length] = '\0';
		pieces[piece_index] = next_piece;
		piece_index++;
	}
	pieces[piece_index] = NULL;


	return pieces;
}






/* daily is an array of 7 strings, each 0 or 1, or null
 * hourly is a list of start/stop ranges, listed in seconds 
 * 	from midnight.  first start in day is first, but last
 * 	stop may be before first start
 */
long* combine_daily_and_hourly(char** daily, long* hourly)
{
	if(daily == NULL && hourly == NULL)
	{
		return NULL;
	}

	char* daily_dummy[8] = { "1", "1", "1", "1", "1", "1", "1", NULL };
	long hourly_dummy[3] = { 0, 24*60*60, -1 };
	if(daily == NULL)
	{
		daily = daily_dummy;
	}
	if(hourly == NULL)
	{
		hourly = hourly_dummy;
	}
	
	int num_hourly = 0;
	for(num_hourly = 0; hourly[num_hourly] >= 0; num_hourly++){}


	//adjust hourly so any range that crosses midnight is split in two
	long* adjusted_hourly = (long*)malloc((3+num_hourly)*sizeof(long));
	int ah_index = 0;
	int hourly_index = 0;
	if(hourly[num_hourly-1] < hourly[0])
	{
		adjusted_hourly[0] = 0;
		adjusted_hourly[1] = hourly[num_hourly-1];
		ah_index = ah_index + 2;
	}
	for(hourly_index=0; hourly_index < num_hourly && ah_index < num_hourly+1; hourly_index++)
	{
		adjusted_hourly[ah_index] = hourly[hourly_index];
		ah_index++;
	}

	if(ah_index % 2 == 1 )
	{
		adjusted_hourly[ah_index] = 24*60*60;
		ah_index++;
	}
	adjusted_hourly[ah_index] = -1;

	/*
	int a=0;
	for(a=0; adjusted_hourly[a] >= 0 ; a++)
	{
		printf("%ld\n", adjusted_hourly[a]);
	}
	*/

	//go through hourly for each day of week, building weekly time ranges
	long* weekly = (long*)malloc(7*(ah_index+1)*sizeof(long));
	int weekly_index = 0;
	int day_index = 0;
	long previous_seconds = 0;
	
	for(day_index=0; day_index < 7; day_index++)
	{
		int is_active=daily[day_index][0] == '1' ? 1 : 0;
		for(hourly_index = 0; adjusted_hourly[hourly_index] >= 0 && is_active == 1; hourly_index++)
		{
			weekly[weekly_index] = previous_seconds + adjusted_hourly[hourly_index];
			weekly_index++;
		}
		previous_seconds = previous_seconds + (24*60*60);
	}
	weekly[weekly_index] = -1;

	/*
	int w=0;
	for(w=0; weekly[w] >= 0 ; w++)
	{
		printf("%ld\n", weekly[w]);
	}
	*/
	



	/* merge time ranges where end of first = start of second */	
	merge_adjacent_time_ranges(weekly, 1);
	

	/* if always active, free & return NULL */
	if(weekly[0] == 0 && weekly[1] == 7*24*60*60)
	{
		free(weekly);
		weekly = NULL;
	}
	free(adjusted_hourly);

	return weekly;	
}


/* is_weekly_range indicates whether we're parsing hours within a single day or a range over a whole week */
long* parse_time_ranges(char* time_ranges, unsigned char is_weekly_range)
{
	char** pieces = split_on_separators(time_ranges, ",", 1, -1, 0);
	int num_pieces = 0;
	for(num_pieces = 0; pieces[num_pieces] != NULL; num_pieces++) {};
	long *parsed = (long*)malloc( (1+(num_pieces*2)) * sizeof(long));


	
	int piece_index = 0;
	for(piece_index = 0; pieces[piece_index] != NULL; piece_index++)
	{
		trim_flanking_whitespace(pieces[piece_index]);
		char** times=split_on_separators(pieces[piece_index], "-", 1, 2, 0);
		int time_count = 0;
		for(time_count = 0; times[time_count] != 0 ; time_count++){}
		if( time_count == 2 )
		{
			unsigned long  start = parse_time(trim_flanking_whitespace(times[0]));
			unsigned long end = parse_time(trim_flanking_whitespace(times[1]));
			parsed[ piece_index*2 ] = (long)start;
			parsed[ (piece_index*2)+1 ] = (long)end;

			free( times[1] );
		}
		if( time_count > 0) { free(times[0]); }

		free(times);
		free(pieces[piece_index]);
	}
	free(pieces);
	parsed[ (num_pieces*2) ] = -1; /* terminated with -1 */

	/* make sure there is no overlap -- this will invalidate ranges */
	int range_index = 0;
	char overlap_found = 0;
	long_map* range_sorter = initialize_long_map();
	for(range_index = 0; range_index < num_pieces; range_index++)
	{
		/* necessary for sorting ranges, which we do later if no overlap found*/
		int *index_ptr = (int*)malloc(sizeof(int));
		*index_ptr = range_index;
		set_long_map_element(range_sorter, parsed[ (range_index*2)], index_ptr);
		
		/* now test for overlap */
		long start1 = parsed[ (range_index*2) ];
		long end1 = parsed[ (range_index*2)+1 ];
		end1= end1 < start1 ? end1 + (is_weekly_range ? 7*24*60*60 : 24*60*60) : end1;
		
		int range_index2 = 0;
		for(range_index2 = 0; range_index2 < num_pieces; range_index2++)
		{
			if(range_index2 != range_index)
			{
				long start2 = parsed[ (range_index2*2) ];
				long end2 = parsed[ (range_index2*2)+1 ];
				end2= end2 < start2 ? end2 + (is_weekly_range ? 7*24*60*60 : 24*60*60) : end2;
				overlap_found = overlap_found || (start1 < end2 && end1 > start2 );
			}
		}
	}

	if(!overlap_found)
	{
		/* sort ranges */
		unsigned long sorted_length;
		int** sorted_range_indices = (int**)get_sorted_long_map_values(range_sorter, &sorted_length);
		long *sorted_parsed = (long*)malloc( (1+(sorted_length*2)) * sizeof(long));
		for(range_index = 0; range_index < sorted_length; range_index++)
		{
			int old_range_index = *(sorted_range_indices[range_index]);
			long start = parsed[ (2*old_range_index) ];
			long end = parsed[ (2*old_range_index)+1 ];
			sorted_parsed[ (2*range_index) ] = start;
			sorted_parsed[ (2*range_index)+1 ] = end;
		}
		sorted_parsed[ 2*range_index ] = -1;
		
		free(parsed);
		parsed = sorted_parsed;
	}
	else
	{
		/* de-allocate parsed, set to NULL */
		free(parsed);
		parsed = NULL;
	}
	unsigned long num_destroyed;
	destroy_long_map(range_sorter, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	

	/* merge time ranges where end of first = start of second */	
	merge_adjacent_time_ranges(parsed, is_weekly_range);


	/* if always active, free & return NULL */
	int max_multiple = is_weekly_range ? 7 : 1;
	if(parsed[0] == 0 && parsed[1] == max_multiple*24*60*60)
	{
		free(parsed);
		parsed = NULL;
	}

	return parsed;
}



void merge_adjacent_time_ranges(long* time_ranges, unsigned char is_weekly_range)
{
	list* merged_list = initialize_list();
	long* next = (long*)malloc(sizeof(long));
	*next = time_ranges[0];
	push_list(merged_list, next);
	
	int next_index = 1;
	while(time_ranges[next_index] >= 0)
	{
		long *previous = (long*)pop_list(merged_list);
		long prev_value;
		if(previous == NULL) { prev_value = -1; } else {prev_value = *previous; }
		if(prev_value != time_ranges[next_index] )
		{
			next = (long*)malloc(sizeof(long));
			*next = time_ranges[next_index];
			if(previous != NULL)
			{
				push_list(merged_list, previous);
			}
			push_list(merged_list, next);
		}
		else
		{
			free(previous);
		}
		next_index++;
	}
	
	long* first = shift_list(merged_list);
	long* last = pop_list(merged_list);
	long max_end = is_weekly_range ? 7*24*60*60 : 24*60*60;
	if(*first == 0 && *last == max_end && merged_list->length > 0)
	{
		push_list(merged_list, shift_list(merged_list));
		free(first);
		free(last);
	}
	else
	{
		unshift_list(merged_list, first);
		push_list(merged_list, last);
	}

	next_index=0;
	while(merged_list->length > 0)
	{
		next = shift_list(merged_list);
		time_ranges[next_index] = *next;
		free(next);
		next_index++;
	}
	time_ranges[next_index] = -1;
	
	destroy_list(merged_list, DESTROY_MODE_FREE_VALUES, next);
}





/* 
 * assumes 24hr time, not am/pm, in format:
 * (Day of week) hours:minutes:seconds
 * if day of week is present, returns seconds since midnight on Sunday
 * otherwise, seconds since midnight
 */
unsigned long parse_time(char* time_str)
{
	while((*time_str == ' ' || *time_str == '\t') && *time_str != '\0') { time_str++; }
	
	unsigned char *weekday = NULL;
	if(strlen(time_str) > 3)
	{
		char wday_test[4];
		memcpy(wday_test, time_str, 3);
		wday_test[4] = '\0';
		to_lowercase(wday_test);
		weekday = (unsigned char*)get_map_element(weekdays, wday_test);
	}

	if(weekday != NULL)
	{
		time_str = time_str + 3;
		while((*time_str == ' ' || *time_str == '\t') && *time_str != '\0') { time_str++; }
	}

	char** time_parts=split_on_separators(time_str, ":", 1, -1, 0);
	unsigned long seconds = weekday == NULL ? 0 : ( ((unsigned long)(*weekday))*60*60*24 );
	unsigned long tmp;
	unsigned long multiple = 60*60;

	int tp_index = 0;
	for(tp_index=0; time_parts[tp_index] != NULL; tp_index++)
	{
		sscanf(time_parts[tp_index], "%ld", &tmp);
		seconds = seconds + (tmp*multiple);
		multiple = (unsigned long)(multiple/60);
		free(time_parts[tp_index]);
	}
	free(time_parts);

	return seconds;
}

string_map* initialize_weekday_hash(void)
{
	string_map *w = initialize_map(0);
	unsigned char* new_ptrs[8]; //chars, but we're storing small integers (0-6)
	int i = 0;
	for(i = 0; i < 7; i++)
	{
		new_ptrs[i] = (unsigned char*)malloc(sizeof(unsigned char));
		*(new_ptrs[i]) = (unsigned char)i;
	}
	set_map_element(w, "sun", new_ptrs[0]);
	set_map_element(w, "mon", new_ptrs[1]);
	set_map_element(w, "tue", new_ptrs[2]);
	set_map_element(w, "wed", new_ptrs[3]);
	set_map_element(w, "thu", new_ptrs[4]);
	set_map_element(w, "fri", new_ptrs[5]);
	set_map_element(w, "sat", new_ptrs[6]);

	return w;
}




/*
 * parses a list of marks/connmarks
 * the max_mask parameter specfies a maximal mask that will be used
 * when matching marks/connmarks.  If a user-defined mask is specified
 * (by defining [mark]/[mask]) this is bitwise-anded with the maximum
 * mask to get the final mask.  This is especially necessary for
 * connmarks, because the mechanism to handle negation when multiple
 * test rules are needed uses the last (high) byte of the connmark 
 * address space, so this HAS to be masked out when matching 
 * connmarks, using max_mask=0x00FFFFFF
 */
char** parse_marks(char* list_str, unsigned long max_mask)
{
	char** marks = NULL;
	if(list_str != NULL)
	{
		marks = split_on_separators(list_str, ",", 1, -1, 0);
		if(marks[0] == NULL)
		{
			free(marks);
			marks = NULL;
		}
		else 
		{
			int mark_index;
			for(mark_index = 0; marks[mark_index] != NULL; mark_index++)
			{
				trim_flanking_whitespace(marks[mark_index]);
				if(max_mask != 0xFFFFFFFF)
				{

					char* m = marks[mark_index];
					char* mask_start;
					if( (mask_start = strchr(m, '/')) != NULL )
					{
						unsigned long mask = 0xFFFFFFFF;
						mask_start++;
						sscanf(mask_start, "%lX", &mask);
					
						mask = mask & max_mask;
					
						*(mask_start) = '\0';
						char new_mask_str[12];
						sprintf(new_mask_str, "0x%lX", mask);
						marks[mark_index] = dynamic_strcat(2, m, new_mask_str);
					}
					else
					{
						char new_mask_str[12];
						sprintf(new_mask_str, "0x%lX", max_mask);
						marks[mark_index] = dynamic_strcat(3, m, "/", new_mask_str);
					}
					free(m);
				}
			}
		}
	}
	return marks;
}









update_node* initialize_generic_update_node(string_map* def, string_map* global_data, int is_block)
{
	char* is_ingress_str = get_map_element(def, "is_ingress");
	int is_ingress = (is_ingress_str == NULL || safe_strcmp(is_ingress_str, "0") == 0 ) ? 0 : 1;

	char lookup_table_str[25];
	char lookup_chain_str[25];
	sprintf(lookup_table_str, "%s_%s_table", is_ingress ? "ingress" : "egress", is_block ? "filter" : "bandwidth");
	sprintf(lookup_chain_str, "%s_%s_chain", is_ingress ? "ingress" : "egress", is_block ? "filter" : "bandwidth");

	//printf("lookup_table_str = %s\n", lookup_table_str);
	//printf("lookup_chain_str = %s\n", lookup_chain_str);

	char* table = get_map_element(global_data, lookup_table_str);
	char* chain = get_map_element(global_data, lookup_chain_str);
	update_node* next_update= NULL;
	if(table != NULL && chain != NULL)
	{
		next_update = (update_node*)malloc(sizeof(update_node));
		
		next_update->update_time = (time_t)0;
		next_update->update_type = 0;
		next_update->table = strdup(table);
		next_update->chain = strdup(chain);
		next_update->update_type = 0;
		next_update->rule_index1 = -1;
		next_update->rule_index2 = -1;
		next_update->rule_index3 = -1;
		next_update->definition = def;
		
		next_update->next_reset_time = (time_t)0;
		next_update->counter = 0;
		next_update->reference_count = 0;
		next_update->max_count = 0;
	}
	return next_update;
}


void adjust_update_rule_numbers(priority_queue** update_queue, string_map* global_data, char* table, char* chain, int first_removed_rule, int num_removed_rules)
{
	/* each priority queue node contains an update_node, just cycle through list & update as necessary */
	priority_queue* tmp_queue = initialize_priority_queue();
	while( (*update_queue)->length > 0)
	{
		priority_queue_node* next = shift_priority_queue_node(*update_queue);
		update_node* u = (update_node*)(next->value);
		/* rule_index1 ALWAYS refers to first index associated with this rule in the table & chain listed */
		if(safe_strcmp(u->table, table) == 0 && safe_strcmp(u->chain, chain) == 0 && u->rule_index1 > first_removed_rule )
		{
			
			//printf("(1) WE ARE SHIFTING RULE INDEXES ! ----- BAAAAAAAAAAAAAAAAAAD\n");
			//printf("\t\trule chain=\"%s\", test chain = \"%s\", rule priority_id=%s, rule index = %ld, index of rule being removed = %ld\n", u->chain, chain, next->id, u->rule_index1, first_removed_rule);
		
			u->rule_index1 = u->rule_index1 - num_removed_rules;
			u->rule_index2 = u->update_type == UPDATE_BLOCK_INSERT || u->update_type == UPDATE_BLOCK_REMOVE ? u->rule_index2 - num_removed_rules : u->rule_index2;

		}
		if(u->update_type == UPDATE_QUOTA_RESET)
		{
			//index 2 may contain a rule in ingress filter table & index 3 may contain a rule in egress filter table
			if(	u->rule_index2 > 0 && u->rule_index2 > first_removed_rule && 
				safe_strcmp( (char*)get_map_element(global_data, "ingress_filter_table"), table) == 0 && 
				safe_strcmp( (char*)get_map_element(global_data, "ingress_filter_chain"), chain) == 0
			  	)
			{
				u->rule_index2 = u->rule_index2 - num_removed_rules;
				//printf("(2) WE ARE SHIFTING RULE INDEXES ! ----- BAAAAAAAAAAAAAAAAAAD\n");
			}
			if(	u->rule_index3 > 0 && u->rule_index3 > first_removed_rule &&
				safe_strcmp( (char*)get_map_element(global_data, "egress_filter_table"), table) == 0 && 
				safe_strcmp( (char*)get_map_element(global_data, "egress_filter_chain"), chain) == 0
			  	)
			{
				u->rule_index3 = u->rule_index3 - num_removed_rules;
				//printf("(3) WE ARE SHIFTING RULE INDEXES ! ----- BAAAAAAAAAAAAAAAAAAD\n");
			}
		}
		push_priority_queue_node(tmp_queue, next);
	}
	unsigned long tmp;
	destroy_priority_queue(*update_queue, DESTROY_MODE_IGNORE_VALUES, &tmp);
	*update_queue = tmp_queue;
}

int count_rules_in_chain(char* table, char* chain)
{
	int count = 0;
	if(table != NULL && chain != NULL)
	{
		iptc_handle_t iptables_snapshot = iptc_init(table);
		if(iptables_snapshot != NULL)
		{
			if(iptc_is_chain(chain, iptables_snapshot))
			{
				const struct ipt_entry *chain_rule = iptc_first_rule(chain, &iptables_snapshot);
				while(chain_rule)
				{
					count++;
					chain_rule = iptc_next_rule(chain_rule, &iptables_snapshot);
				}
			}
		}
		iptc_free(&iptables_snapshot);
	}
	return count;
}

void flush_iptables_chain(char* table, char* chain)
{
	if(table != NULL && chain != NULL)
	{
		iptc_handle_t iptables_snapshot = iptc_init(table);
		if(iptables_snapshot != NULL)
		{
			if(iptc_is_chain(chain, iptables_snapshot))
			{
				char *command = dynamic_strcat(4, "iptables -t ", table, " -F ", chain);
				system(command);
				free(command);
			}
			iptc_free(&iptables_snapshot);
		}
	}
}









//monitor rule will be added if it does not exist, otherwise use existing
// in each quota rule add new field chain_index, which gets set to an unsigned long value of what rule number this is
// also, initialize bandwidth_used (unsigned long) and next_reset (time_t)
void initialize_quota_rules_for_direction(list** quota_rules, int direction, string_map* global_data, priority_queue** update_queue, time_t reference_time, time_t current_time)
{
	int num_rules = 0;
	list* tmp_list = initialize_list();
	while((*quota_rules)->length > 0)
	{
		string_map *def = shift_list(*quota_rules);
		push_list(tmp_list, def);
		
		char* is_ingress_str = get_map_element(def, "is_ingress");
		int is_ingress = (is_ingress_str == NULL || safe_strcmp(is_ingress_str, "0") == 0 ) ? 0 : 1;
		
		if(is_ingress)
		{
			num_rules = num_rules + ( direction == DIRECTION_INGRESS ? 1 : 0 );
		}
		else
		{
			num_rules = num_rules + ( direction == DIRECTION_EGRESS ? 1 : 0 );
		}
	}

	unsigned long  tmp;
	destroy_list(*quota_rules, DESTROY_MODE_IGNORE_VALUES, &tmp);
	*quota_rules = tmp_list;

	//printf("num rules = %d\n", num_rules);
	if(num_rules == 0)
	{
		return;
	}

	//save directory should be in global_data
	//the first thing we should do is load saved data,
	//but implement that here later
	


	char *table;
	char *chain;
	if(direction == DIRECTION_EGRESS)
	{
		table = get_map_element(global_data, "egress_bandwidth_table");
		chain = get_map_element(global_data, "egress_bandwidth_chain");
	}
	else
	{
		table = get_map_element(global_data, "ingress_bandwidth_table");
		chain = get_map_element(global_data, "ingress_bandwidth_chain");
	}
	
	//printf("table = %s, chain = %s\n", table, chain);

	if(table == NULL || chain == NULL)
	{
		exit(1);
	}

	iptc_handle_t iptables_snapshot = iptc_init(table);
	if(iptables_snapshot)
	{
		if(iptc_is_chain(chain, iptables_snapshot))
		{
			//locate existing rules
			string_map* ip_monitor_indices = initialize_map(0);
			long rule_num = 0;
			const struct ipt_entry *chain_rule = iptc_first_rule(chain, &iptables_snapshot);
			while(chain_rule)
			{
				rule_num++;
				
				//look at dest addr in test (pretend this is ingress)	
				struct ipt_ip match_info = chain_rule->ip;
			
				char* ip;
				char* mask;
				if(direction == DIRECTION_INGRESS)
				{	
					ip = get_ip_str(match_info.dst);
					mask = get_ip_str(match_info.dmsk);
				}
				else
				{
					ip = get_ip_str(match_info.src);
					mask = get_ip_str(match_info.smsk);
				}


				long* i = (long*)malloc(sizeof(long));
				*i = rule_num;
				if(safe_strcmp(ip, "0.0.0.0") == 0 && safe_strcmp(mask, "0.0.0.0") == 0)
				{
					set_map_element(ip_monitor_indices, "ALL", (void*)i);
				}
				else if(safe_strcmp(mask, "255.255.255.255") == 0)
				{
					set_map_element(ip_monitor_indices, ip, (void*)i);
				}
				else
				{
					free(i);
				}
				
				//printf("loaded dst addr = %s, mask=%s\n", ip, mask); 
				
				free(ip);
				free(mask);

				chain_rule = iptc_next_rule(chain_rule, &iptables_snapshot);
			}
			
			//initialize updates, inserting new rules as necessary
			tmp_list = initialize_list();	
			while((*quota_rules)->length > 0)
			{
				string_map *def = (string_map*)shift_list(*quota_rules);
				push_list(tmp_list, def);

				char* is_ingress_str = get_map_element(def, "is_ingress");
				int is_ingress = (is_ingress_str == NULL || safe_strcmp(is_ingress_str, "0") == 0 ) ? 0 : 1;

				if((is_ingress == 0 && direction == DIRECTION_EGRESS) || (is_ingress == 1 && direction == DIRECTION_INGRESS))
				{
					//get the rule index, insert new rule if necessary
					char* ip = get_map_element(def, "ip");
					if(ip == NULL)
					{
						ip = strdup("ALL");
						set_map_element(def, "ip", ip);
					}
					to_uppercase(ip); 

					long* next_ip_index = get_map_element(ip_monitor_indices, ip);
					if(next_ip_index == NULL)
					{
						char command[50];
						char src_dst = direction == DIRECTION_EGRESS ? 's' : 'd';
						if(ip != NULL) { to_uppercase(ip); }
						if(safe_strcmp(ip, "ALL") != 0 && ip != NULL)
						{
							sprintf(command, "iptables -t %s -A %s -%c %s", table, chain, src_dst, ip);
						}
						else
						{
							sprintf(command, "iptables -t %s -A %s",table, chain);
						}
						system(command);
						
						rule_num++;
						next_ip_index = (long*)malloc(sizeof(long));
						*next_ip_index = rule_num;
						
						iptc_free(&iptables_snapshot);
						iptables_snapshot = iptc_init(table);
					}

					int64_t reference_count;
					struct ipt_counters* rule_counter = iptc_read_counter(chain, *next_ip_index, &iptables_snapshot);
					reference_count = rule_counter->bcnt;

					int64_t max_count; 
					char* max_count_str = get_map_element(def, "max_bandwidth");
					sscanf(max_count_str, "%lld", (long long int*)&max_count);
					
					
					//attempt to load old counters from save location here
					//implement when we get around to it



					//insert node into update queue
					//note, even if quota already has been reached we schedule a QUOTA_TEST
					//update, so block is inserted when we start updating  This eliminates redundant code
					
					update_node* next_update = initialize_generic_update_node(def, global_data, 0);
					next_update->update_time = current_time;
					next_update->update_type = UPDATE_QUOTA_TEST;
					next_update->rule_index1 = *next_ip_index;
					next_update->counter = 0;
					next_update->reference_count = reference_count;
					next_update->max_count = max_count;

					//compute next reset time
					long* reset_interval = (long*)get_map_element(next_update->definition, "reset_interval");
					if(reset_interval[0] >= 0)
					{
						next_update->next_reset_time =  get_next_interval_end(next_update->update_time, reset_interval[0]);
					}
					else
					{
						next_update->next_reset_time = next_update->update_time + reset_interval[1];
					}
					
						
					char* priority_id = get_map_element(def, "name");
					if(priority_id == NULL)
					{
						char tmp[20];
						sprintf(tmp, "%ld", *next_ip_index);
						priority_id = dynamic_strcat(5, next_update->table, "-", next_update->chain, "-", tmp);
						set_map_element(def, "name", priority_id);
					}
					
					load_quota_data(global_data, next_update, current_time);
					quota_test(next_update, update_queue, global_data, priority_id, reference_time);
				}
			}
			destroy_list(*quota_rules, DESTROY_MODE_IGNORE_VALUES, &tmp);
			*quota_rules = tmp_list;
		}
		iptc_free(&iptables_snapshot);
	}
}

char* get_ip_str(struct in_addr ip)
{
	char ip_str[20];
	sprintf(ip_str, "%ld.%ld.%ld.%ld", IP_PARTS(ip.s_addr));
	return( strdup(ip_str) );
}

void load_quota_data(string_map* global_data, update_node* template_node, time_t current_time)
{
	char* save_directory = get_map_element(global_data, "save_directory");
	if(save_directory != NULL)
	{	
		if(save_directory[ strlen(save_directory) -1 ] != '/')
		{
			char* new_dir = dynamic_strcat(2, save_directory, "/");
			set_map_element(global_data, "save_directory", new_dir);
			free(save_directory);
			save_directory = new_dir;
		}

		string_map* def = template_node->definition;
		char* name = get_map_element(def, "name");

		char* file_path = dynamic_strcat(3, save_directory, "/", name);
		FILE *in_file = fopen(file_path, "r");
		free(file_path);
		
		if(in_file != NULL)
		{
			int64_t counter;
			time_t next_reset_time;
			int definition_has_changed = 0;
			
			char newline_terminator[3] = "\n\r";
			
			dyn_read_t next = dynamic_read(in_file, newline_terminator, 2);
			while(next.terminator != EOF)
			{
				char** variable = parse_variable_definition_line(next.str);
				if(safe_strcmp(variable[0], "counter") == 0)
				{
					sscanf(variable[1], "%lld", (long long int*)(&counter) );
				}
				if(safe_strcmp(variable[0], "next_reset_time") == 0)
				{
					sscanf(variable[1], "%ld", &next_reset_time);
				}
				if(safe_strcmp(variable[0], "max_count") == 0)
				{
					int64_t max_count = 0;
					variable[1] = variable[1] == NULL ? strdup("") : variable[1];
					sscanf(variable[1], "%lld", (long long int*)(&max_count) );
					definition_has_changed = definition_has_changed || (max_count != template_node->max_count);
				}
				if(safe_strcmp(variable[0], "is_ingress") == 0)
				{
					char* is_ingress_def = (char*)get_map_element(def, "is_ingress");
					int template_is_in = safe_strcmp(is_ingress_def, "1") == 0;
					int loaded_is_in =   safe_strcmp(variable[1], "1") == 0;
					definition_has_changed = definition_has_changed || ( template_is_in != loaded_is_in );
				}
				if(safe_strcmp(variable[0], "ip") == 0)
				{
					char* ip_template =  (char*)get_map_element(def, "ip");
					variable[1] = variable[1] == NULL ? strdup("ALL") : variable[1];
					definition_has_changed = definition_has_changed || ( safe_strcmp(ip_template, variable[1] ) != 0 );
				}
				if(safe_strcmp(variable[0], "reset_interval") == 0)
				{
					long reset1;
					long reset2;
					long* template_reset = (long*)get_map_element(def, "reset_interval");
					sscanf(variable[1], "%ld\t%ld\n", &reset1, &reset2);
					definition_has_changed = definition_has_changed || (reset1 != template_reset[0] || reset2 != template_reset[1] );
				}

				free(variable[0]);
				free(variable[1]);

				next = dynamic_read(in_file, newline_terminator, 2);
			}
			fclose(in_file);

			if(!definition_has_changed)
			{
				if(next_reset_time > current_time)
				{
					template_node->next_reset_time = next_reset_time;
					template_node->counter = counter;
				}
			}
		}
	}
}


void save_all_quota_data(string_map* global_data, priority_queue* update_queue)
{
	priority_queue* tmp_queue = initialize_priority_queue();
	while(update_queue->length > 0)
	{
		priority_queue_node* pq = shift_priority_queue_node(update_queue);
		update_node* un = (update_node*)pq->value;
		if(un->update_type == UPDATE_QUOTA_RESET || un->update_type == UPDATE_QUOTA_TEST)
		{
			save_quota_data(global_data, un);
		}
		push_priority_queue_node(tmp_queue, pq);
	}

	unsigned long tmp;
	destroy_priority_queue(update_queue, DESTROY_MODE_IGNORE_VALUES, &tmp);
	update_queue = tmp_queue;
}

void save_quota_data(string_map* global_data, update_node* quota_update_node)
{
	char* save_directory = get_map_element(global_data, "save_directory");
	if(save_directory != NULL)
	{
		if(save_directory[ strlen(save_directory) -1 ] != '/')
		{
			char* new_dir = dynamic_strcat(2, save_directory, "/");
			set_map_element(global_data, "save_directory", new_dir);
			free(save_directory);
			save_directory = new_dir;
		}
		create_path(save_directory, 0700);
	

		string_map* def = quota_update_node->definition;
		char* name = get_map_element(def, "name");

		/* printf("DUMPING HERE, save_directory = %s\n", save_directory); */

		char* file_path = dynamic_strcat(2, save_directory,  name);
		FILE *out_file = fopen(file_path, "w");
		free(file_path);
		
		if(out_file != NULL)
		{
			/* variables necessary to load into update node*/
			fprintf(out_file, "counter\t%lld\n", (long long int)quota_update_node->counter);
			fprintf(out_file, "next_reset_time\t%ld\n", (long)quota_update_node->next_reset_time);
			
			/* variables below used to validate definition of quota hasn't changed */
			fprintf(out_file, "max_count\t%lld\n", (long long int)quota_update_node->max_count);
			fprintf(out_file, "is_ingress\t%s\n", (char*)get_map_element(def, "is_ingress"));
			fprintf(out_file, "ip\t%s\n", (char*)get_map_element(def, "ip"));
		
			long* reset_interval = (long*)get_map_element(def, "reset_interval");
			fprintf(out_file, "reset_interval\t%ld\t%ld\n", reset_interval[0], reset_interval[1]);	
			fclose(out_file);
		}
	}	
}


//yoinked from BusyBox source (hey, that's what open source is for!)
int create_path(const char *name, int mode)
{
	char *cp;
	char *cpOld;
	char buf[PATH_MAX + 1];
	int retVal = 0;

	strcpy(buf, name);
	for (cp = buf; *cp == '/'; cp++);
	cp = strchr(cp, '/');
	while (cp) 
	{
		cpOld = cp;
		cp = strchr(cp + 1, '/');
		*cpOld = '\0';
		retVal = mkdir(buf, cp ? 0777 : mode);
		if (retVal != 0 && errno != EEXIST)
		{
			/* perror(buf); */
			return 0;
		}
		*cpOld = '/';
	}
	return 1;
}





char** parse_ports(char* port_str)
{
	char** ports = split_on_separators(port_str, ",", 1, -1, 0);
	int port_index = 0;
	for(port_index=0; ports[port_index] != NULL; port_index++)
	{
		char* dash_ptr;
		while((dash_ptr=strchr(ports[port_index], '-')) != NULL)
		{
			dash_ptr[0] = ':';
		}
		trim_flanking_whitespace( ports[port_index] );
	}
	return ports;
}

char** parse_ips(char* ip_str)
{
	char** ip_parts = split_on_separators(ip_str, ",", 1, -1, 0);
	list* ip_list = initialize_list();
	
	int ip_part_index;
	for(ip_part_index=0; ip_parts[ip_part_index] != NULL; ip_part_index++)
	{
		char* next_str = ip_parts[ip_part_index];
		if(strchr(next_str, '-') != NULL)
		{
			char** range_parts = split_on_separators(next_str, "-", 1, 2, 1);
			char* start = trim_flanking_whitespace(range_parts[0]);
			char* end = trim_flanking_whitespace(range_parts[1]);
			int start_ip[4];
			int end_ip[4];
			int start_valid = sscanf(start, "%d.%d.%d.%d", start_ip, start_ip+1, start_ip+2, start_ip+3);
			int end_valid = sscanf(end, "%d.%d.%d.%d", end_ip, end_ip+1, end_ip+2, end_ip+3);
			if(start_valid == 4 && end_valid == 4)
			{
				get_ip_range_strs(start_ip, end_ip, "", 4, ip_list);
			}

			free(start);
			free(end);	
			free(range_parts);
			free(next_str);
		}
		else
		{
			push_list(ip_list, trim_flanking_whitespace(next_str));
		}
		
	}
	free(ip_parts);
	unsigned long num;
	return (char**)destroy_list(ip_list, DESTROY_MODE_RETURN_VALUES, &num);
}

void get_ip_range_strs(int* start, int* end, char* prefix, int list_length, list* range_strs)
{
	//mask = 8*(4-list_length)
	if(list_length > 1 )
	{
		if(start[0] == end[0])
		{
			char new_prefix[20];
			sprintf(new_prefix, "%s%d.", prefix, start[0]);
			get_ip_range_strs(start+1, end+1, new_prefix, list_length-1, range_strs);
		}
		else
		{
			int start_is_all_zeros = 1;
			int end_is_all_ones = 1;
			int test_index;
			for(test_index=0; test_index < list_length && start_is_all_zeros; test_index++){ start_is_all_zeros = start[test_index] == 0; }
			for(test_index=0; test_index < list_length && end_is_all_ones; test_index++){ end_is_all_ones = end[test_index] == 255; }


			int next;
			char new_prefix[20];
			sprintf(new_prefix, "%s%d.", prefix, start[0]);
			if(start_is_all_zeros)
			{
				next = start[0];
			}
			else
			{
				int all_ones[4] = {255, 255, 255, 255 };
				get_ip_range_strs(start+1, all_ones, new_prefix, list_length-1, range_strs);
				next = start[0] + 1;
			}
			
			char postfix[20] = "";
			int postfix_index;
			for(postfix_index=0; postfix_index < (list_length-1); postfix_index++){ strcat(postfix, ".0"); }
			int last = end_is_all_ones ? end[0] : end[0] -1;

			while(next <= last )
			{
				int max = 0;
				for(max = 0; next % ((int)pow(2,max)) == 0 && next + (int)pow(2,max) <= last+1 ; max++){}
				max = max > 0 ? max -1 : max;

				char next_str[25];
				sprintf(next_str, "%s%d%s/%d", prefix, next, postfix, (8*(4-list_length))+(8-max) );
				push_list(range_strs, strdup(next_str));

				next = next + (int)pow(2, max);
			}
		
			if(!end_is_all_ones)
			{	
				int all_zeros[4] = { 0, 0, 0, 0 };
				sprintf(new_prefix, "%s%d.", prefix, end[0]);
				get_ip_range_strs(all_zeros, end+1, new_prefix, list_length-1, range_strs);
			}
		}
	}
	else
	{
		int next = start[0];
		while(next <= end[0])
		{
			int max = 0;
			for(max = 0; next % ((int)pow(2,max)) == 0 && next + (int)pow(2,max) <= end[0] + 1; max++){}
			max = max > 0 ? max -1 : max;

			char next_str[25];
			if(max > 0)
			{
				sprintf(next_str, "%s%d/%d", prefix, next, 24+(8-max) );
			}
			else
			{
				sprintf(next_str, "%s%d", prefix,next );

			}
			push_list(range_strs, strdup(next_str));
			
			next = next + (int)pow(2, max);
		}
	}
}




//returns an array of length 3, first element is a pointer to a string_map containing global variables, second is 
//a pointer to a the first list_node in a list containing all block rules, the third the first list_node in a 
//list of quota rules
void** load_restricterd_config(char* filename)
{
	void** config = (void*)malloc(4*sizeof(void*));
	config[0] = NULL;
	config[1] = (void*)initialize_list();
	config[2] = (void*)initialize_list();
	config[3] = NULL;

	
	
				
	FILE *config_file = fopen(filename, "r");	
	char newline_terminator[3] = "\n\r";
	if(config_file != NULL)
	{
		dyn_read_t next;
		next = dynamic_read(config_file, newline_terminator, 2);
		char** variable = parse_variable_definition_line(next.str);
		int definition_number = 0;
		while(next.terminator != EOF)
		{
			//cycle past space outside definition line
			while(next.terminator != EOF && safe_strcmp(variable[0], "global") != 0 && safe_strcmp(variable[0], "quota") && safe_strcmp(variable[0], "block") )
			{
				if(variable[0] != NULL)
				{
					free(variable[0]);
					free(variable[1]);
				}
				free(variable);
				next = dynamic_read(config_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
			}
			
			//we've found a definition
			if(next.terminator != EOF)
			{
				definition_number++;
				string_map *definition = initialize_map(1);
				set_map_element(definition, "rule_type", variable[0]);
				if(variable[1] != NULL)
				{
					set_map_element(definition, "name", variable[1]);
				}
				else
				{
					char name[25];
					sprintf(name, "rule_%d", definition_number);
					set_map_element(definition, "name", name);
				}
				
				next = dynamic_read(config_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "global") != 0 && safe_strcmp(variable[0], "quota") && safe_strcmp(variable[0], "block") )
				{
					if(variable[0] != NULL && variable[1] != NULL)
					{
						
						if(safe_strcmp(variable[0], "hours_blocked") == 0 || safe_strcmp(variable[0], "weekly_block_times") == 0)
						{
							to_lowercase(variable[1]);
							if( safe_strcmp(variable[1], "all") != 0 )
							{
								unsigned char is_weekly_range = safe_strcmp(variable[0], "weekly_block_times") ? 1 : 0;
								long* blocked_time_ranges = parse_time_ranges(variable[1], is_weekly_range);
								set_map_element(definition, variable[0], blocked_time_ranges);
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "weekdays_blocked") == 0)
						{
							char** weekdays_blocked = split_on_separators(variable[1], ",", 1, -1, 0);
							if( safe_strcmp(variable[1], "all") != 0 )
							{
								int wdb_count = 0;
								for(wdb_count = 0; weekdays_blocked[wdb_count] != NULL; wdb_count++)
								{
									trim_flanking_whitespace(weekdays_blocked[wdb_count]);
								}
								if(wdb_count == 7)
								{
									set_map_element(definition, variable[0], weekdays_blocked);
								}
								else
								{
									for(wdb_count = 0; weekdays_blocked[wdb_count] != NULL; wdb_count++) { free(weekdays_blocked[wdb_count]); }
								}
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "reset_interval") == 0)
						{

							long reset_interval_type = -1;
							long reset_interval_seconds = -1;
							
							reset_interval_type = (safe_strcmp(variable[1], "minutely") == 0) ? INTERVAL_MINUTE : -1;  /* Doesn't 'minutely' mean small?  Does this make sense? Does anyone care? */
							reset_interval_type = (safe_strcmp(variable[1], "hourly") == 0) ? INTERVAL_HOUR : reset_interval_type;
							reset_interval_type = (safe_strcmp(variable[1], "daily") == 0) ? INTERVAL_DAY : reset_interval_type;
							reset_interval_type = (safe_strcmp(variable[1], "weekly") == 0) ? INTERVAL_WEEK : reset_interval_type;
							reset_interval_type = (safe_strcmp(variable[1], "monthly") == 0) ? INTERVAL_MONTH : reset_interval_type;
							
							sscanf(variable[1], "%ld", &reset_interval_seconds);	
						

							if(reset_interval_seconds >= 0 || reset_interval_type >= 0)
							{
								long* reset_interval=(long*)malloc(2*sizeof(long));
								reset_interval[0] = reset_interval_type;
								reset_interval[1] = reset_interval_seconds;
								set_map_element(definition, variable[0], reset_interval);
							}
							free(variable[1]);
						}
						else if(	safe_strcmp(variable[0], "url") == 0 ||
								safe_strcmp(variable[0], "url_regex") == 0 ||
								safe_strcmp(variable[0], "not_url") == 0 ||
								safe_strcmp(variable[0], "not_url_regex") == 0
								)
						{
							/*
							 * may be a quoted list of urls to block, so attempt to parse this
							 * if no quotes found, match on unquoted expresssion
							 * we don't need to de-escape quotes because when we define rule, 
							 * we call iptables from system, and through the shell, which will de-escape quotes for us
							 */
							char** parsed_quoted = parse_quoted_list(variable[1], '\"', '\\', 0);
							if(parsed_quoted[0] != NULL)
							{
								set_map_element(definition, variable[0], parsed_quoted);
								free(variable[1]);
							}
							else
							{
								char** url_list = (char**)malloc(2*sizeof(char*));
								url_list[0] = variable[1];
								url_list[1] = NULL;
								set_map_element(definition, variable[0], url_list);
							}
						}
						else if(	safe_strcmp(variable[0], "mark") == 0 ||
								safe_strcmp(variable[0], "not_mark") == 0
						       )
						{
							set_map_element(definition, variable[0], parse_marks(variable[1], 0xFFFFFFFF));
							free(variable[1]);
						}
						else if(	safe_strcmp(variable[0], "connmark") == 0 ||
								safe_strcmp(variable[0], "not_connmark") == 0
								)
						{
							set_map_element(definition, variable[0], parse_marks(variable[1], 0x00FFFFFF));
							free(variable[1]);
						}
						else if(	safe_strcmp(variable[0], "remote_addr") == 0 ||
								safe_strcmp(variable[0], "local_addr") == 0 ||
								safe_strcmp(variable[0], "not_remote_addr") == 0 ||
								safe_strcmp(variable[0], "not_local_addr") == 0 
						       		)
						{
							char** parsed_ips = parse_ips(variable[1]);
							if(parsed_ips != NULL)
							{
								set_map_element(definition, variable[0], parsed_ips);
							}
							free(variable[1]);
						}
						else if(	safe_strcmp(variable[0], "remote_port") == 0 ||
								safe_strcmp(variable[0], "local_port") == 0 ||
								safe_strcmp(variable[0], "not_remote_port") == 0 ||
								safe_strcmp(variable[0], "not_local_port") == 0 
								)
						{
							char** parsed_ports = parse_ports(variable[1]);
							if(parsed_ports != NULL)
							{
								set_map_element(definition, variable[0], parsed_ports);
							}
							free(variable[1]);
						}
						else
						{
							set_map_element(definition, variable[0], variable[1]);
						}
					}
					free(variable[0]);
					free(variable);
					next = dynamic_read(config_file, newline_terminator, 2);
					variable = parse_variable_definition_line(next.str);
				}
			
				//convert weekdays_blocked & hours_blocked to weekly_block_times if necessary
				char** weekdays_blocked = (char**)get_map_element(definition, "weekdays_blocked");
				long* hours_blocked = get_map_element(definition, "hours_blocked");
				if( (weekdays_blocked != NULL ||  hours_blocked != NULL ) && get_map_element(definition, "weekly_block_times") == NULL )
				{
					long* weekly_block_times = combine_daily_and_hourly(weekdays_blocked, hours_blocked);
					set_map_element(definition, "weekly_block_times", weekly_block_times);
				
					/*
					int combined_index;
					for(combined_index = 0; weekly_block_times[combined_index] >= 0; combined_index++)
					{
						printf("%ld, ", weekly_block_times[combined_index]);
					}
					printf("\n");
					*/
				}
				
				// do some more error checking here
				// ...
				// ...someday :-P
			


				//add definition to config
				int config_index = 99;
				if(strcmp("global", get_map_element(definition, "rule_type")) == 0)
				{
					config_index = 0;
				}
				if(strcmp("block", get_map_element(definition, "rule_type")) == 0)
				{
					config_index = 1;
				}
				else if(strcmp("quota", get_map_element(definition, "rule_type")) == 0)
				{
					config_index = 2;
				}

				if(config_index == 0)
				{
					config[0] = (void*)definition;
				}
				else if(config_index <= 2)
				{
					/* if enabled value is present, only load configuration if it is equal to one or "true"*/
					char* enabled_str = (char*)get_map_element(definition, "enabled");
					if( enabled_str == NULL || safe_strcmp(enabled_str, "1") == 0 || safe_strcmp(enabled_str, "true") == 0 )
					{
						list* l = (list*)config[config_index];
						push_list(l, definition);
					}
				}
			}
		}
	}

	return config;
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
			char** split_line = split_on_separators(trimmed_line, whitespace_separators, 2, 2, 1);
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



