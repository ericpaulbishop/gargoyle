/*  ddns_updater -	A small tool for periodically performing dynamic DNS updates
 *  			Originally created for the Gargoyle Web Interface
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


#include "erics_tools.h"
#include "http_minimal_client.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>

#include <regex.h>

#include <dirent.h>
#include <sys/param.h>
#include <sys/errno.h>

#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <time.h>

#define INTERFACE	1
#define INTERNET	2

#define UPDATE_FAILED				3
#define UPDATE_NOT_NEEDED			4
#define UPDATE_SUCCESSFUL			5
#define UPDATE_OF_MULTIPLE_SERVICES_SCHEDULED	6


#define UPDATE_ALL_REQUEST_STR	"UPDATE_OF_ALL_DEFINED_DDNS_SERVICE_CONFIGURATIONS_REQUESTED"

#define UPDATE_INFO_DIR		"/var/last_ddns_updates/" 	//where to store files containing time of last updates
#define PID_PATH		"/var/run/ddns_updaterd.pid" 	//pid file for program when run in daemon mode
#define MSG_ID			112
#define REQUEST_MSG_TYPE 	124
#define RESPONSE_MSG_TYPE	136
#define MAX_MSG_LINE 		250


typedef struct
{
	char* name;
	char* url_template;	  	// contains vars that should be replaced, eg http://[USER]:[PASS]@dynservice.com
	char** variables;	 	// variable names for which to do replacement in url
	regex_t* success_regexp;	// update considered success only if result matches this regex
	regex_t* failure_regexp;	// update considered failure only if result matches this regex
					//
					// NOTE: only one of the two regexp variables should be defined, but in case 
					//       where both are present success_regexp has precedence 
					//       (even if failure_regexp, it will be success if success_regexp matches
} ddns_service_provider;

typedef struct
{
	char* name;
	char* service_provider;			//name of service from ddns_service_provider
	int check_interval; 			// seconds
	int force_interval; 			// seconds
	int ip_source; 			// INTERFACE or CHECK_URL
	char** ip_url;			// url or (whitespace separated) list of urls if ip_source is CHECK_URL
	char* ip_interface;			// name of an interface
	
	string_map* variable_definitions; 	// variable_id->variable_definition

} ddns_service_config;


typedef struct
{
	char* service_name;
	time_t next_time;
	time_t last_full_update;
} update_node;


typedef struct
{
	long int msg_type;
	char msg_line[MAX_MSG_LINE];
} message_t;

//global variables for daemon (used in signal handler) 
int terminated; 
int output_requested; 


string_map* load_service_providers(char* filename);
string_map* load_service_configurations(char* filename, string_map* service_providers);
char** parse_variable_definition_line(char* line);
int convert_to_regex(char* str, regex_t* p);
int get_multiple_for_unit(char* unit);

char* get_local_ip(int ip_source, void* check_parameter);
char* get_ip_from_url(char* url);
char* get_interface_ip(char* if_name);


char* do_url_substitution(ddns_service_provider* def, ddns_service_config* config, char* current_ip);
char *replace_str(char *s, char *old, char *new);


int do_single_update(ddns_service_config *service_config, string_map *service_providers, char* remote_ip, char* local_ip, int force_update, int verbose);
char* lookup_domain_ip(char* url_str);

void run_request(string_map *service_configs, string_map *service_providers, char **service_names, int force_update, char display_format, int verbose);
void run_request_through_daemon(char **service_names, int force_update, char display_format);
void run_daemon(string_map *service_configs, string_map *service_providers, int force_update);

void daemonize(int background);
void signal_handler(int sig);
int create_path(const char *name, int mode);



int main(int argc, char** argv)
{

	// -C [SERVICE_CONFIG_FILE]
	// -P [SERVICE_PROVIDER_FILE]
	// -d (start daemon mode)
	// -f (forces update for standalone or when daemon first starts -- default is just to check and update if necessary)
	// -h (for non-daemon mode display output in human readable format)
	// -m (for non-daemon mode display output in simple machine parsable format: a list of numbers, one for each service
	// 	where 0=failure to update, 1= update not needed, 2=success
	// -u print usage and exit
	// [SERVICE NAMES]
	
	char* config_file = strdup("/etc/ddns_updater.conf");
	char* service_provider_file = strdup("/etc/ddns_providers.conf");
	int is_daemon = 0;
	int force_update = 0;
	int verbose = 0;
	char display_format = 'h';
	char** service_names = (char**)malloc(sizeof(char*));
	service_names[0] = NULL;


	int c;
	while((c = getopt(argc, argv, "c:C:P:p:DdFfHhMmVvUu")) != -1)
	{	
		switch(c)
		{
			case 'C':
			case 'c':
				free(config_file);
				config_file = strdup(optarg);
				break;
			case 'P':
			case 'p':
				free(service_provider_file);
				service_provider_file = strdup(optarg);
				break;
			case 'D':
			case 'd':
				is_daemon = 1;
				break;
			case 'F':
			case 'f':
				force_update = 1;
				break;
			case 'H':
			case 'h':
				display_format = 'h';
				break;
			case 'M':
			case 'm':
				display_format = 'm';
				break;
			case 'V':
			case 'v':
				verbose = 1;
				break;
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS] [SERVICE NAMES]\n", argv[0]);
				printf("\t-C [SERVICE_CONFIG_FILE] specifies location of file containing configuration information\n");
				printf("\t-P [SERVICE_PROVIDER_FILE] specifies location of file containing service provider definitions\n");
				printf("\t-d start daemon mode\n");
				printf("\t-f forces update for standalone or when daemon first starts -- default is just to check and update if necessary\n");
				printf("\t-h for non-daemon mode display output in human readable format\n");
				printf("\t-m for non-daemon mode display output in simple machine parsable format: a list of numbers, one for each service\n");
				printf("\t\twhere 0=failure to update, 1= update not needed, 2=success\n");
				printf("\t-u print usage and exit\n");
				return 0;
		}
	}

	if(optind < argc)
	{
		free(service_names);
		int name_length = argc-optind;
		service_names = (char**)malloc((1+name_length)*sizeof(char*));
		int arg_index;
		for(arg_index =optind; arg_index < argc; arg_index++)
		{
			service_names[arg_index-optind] = strdup(argv[arg_index]);
		}
		service_names[name_length] = NULL;
	}



	//test if a daemon is already running by trying to open the message queue (which daemon creates)
	int testq = msgget(ftok(PID_PATH, MSG_ID), 0777);
	int daemon_running = testq < 0 ? 0 : 1;

	//if we're not running a request through the daemon, load config files
	if(daemon_running == 0)
	{
		string_map *service_providers = load_service_providers(service_provider_file);
		string_map *service_configs = load_service_configurations(config_file, service_providers);
		if(service_configs->num_elements == 0)
		{
			if(service_providers->num_elements == 0)
			{
				printf("ERROR: No dynamic DNS service definitions found\n(Did you specify correct definition file path?)\n");
			}
			else
			{
				printf("ERROR: No valid dynamic DNS service configurations defined\n(Did you specify correct configuration file path?)\n");
			}
		}
		else
		{

			if(is_daemon)
			{
				//start update daemon
				run_daemon(service_configs, service_providers, force_update);
			}
			else
			{
				run_request(service_configs, service_providers, service_names, force_update, display_format, verbose);
				// handle each service requested or all if none specified
			}
		}
	}	
	else
	{	
		if(is_daemon)
		{
			printf("ERROR: Daemon is already running, cannot start a new daemon.\n");
		}
		else
		{
			//send a request to daemon to check/update each service requested, or all if none specified
			run_request_through_daemon(service_names, force_update, display_format);
		}
	}


	return 0;
}


void run_request(string_map *service_configs, string_map* service_providers, char **service_names, int force_update, char display_format, int verbose)
{
	int name_index = 0;
	int all_found = service_names[name_index] == NULL ? 1 : 0;
	for(name_index = 0; service_names[name_index] != NULL && all_found == 0; name_index++)
	{
		all_found = safe_strcmp(service_names[name_index], "all") == 0 ? 1 : 0;
	}
	if(all_found == 1)
	{
		unsigned long num_keys;
		service_names = get_map_keys(service_configs, &num_keys);
	}
	
	
	for(name_index = 0; service_names[name_index] != NULL ; name_index++)
	{
		ddns_service_config* service_config = get_map_element(service_configs, service_names[name_index]);
		
		if(service_config != NULL)
		{
			
			char *local_ip = get_local_ip(service_config->ip_source, service_config->ip_source == INTERFACE ? (void*)service_config->ip_interface : (void*)service_config->ip_url);

			char* test_domain = get_map_element(service_config->variable_definitions, "domain");
			char* remote_ip = NULL;
			if(test_domain != NULL)
			{
				remote_ip = lookup_domain_ip(test_domain);
			}
			int update_status = do_single_update(service_config, service_providers, remote_ip, local_ip, force_update, verbose);
		
			if(update_status == UPDATE_SUCCESSFUL)
			{
				//update the last update time file
				time_t current_time = time(NULL);
				create_path(UPDATE_INFO_DIR, 0700);
				char* filename = dynamic_strcat(2, UPDATE_INFO_DIR, service_config->name);
				FILE* update_file = fopen(filename, "w");
				free(filename);
				if(update_file != NULL)
				{
					fprintf(update_file, "%ld", (long)current_time);
					fclose(update_file);
				}
			}


			if(test_domain == NULL)
			{
				test_domain = service_config->name;
			}
			if(display_format == 'h')
			{
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						printf("\nupdate of %s successful\n", test_domain);
						break;
					case UPDATE_NOT_NEEDED:
						printf("\nupdate of %s not needed\n", test_domain);
						break;
					case UPDATE_FAILED:
						printf("\nupdate of %s failed\n", test_domain);
						break;
				}
			}
			else
			{
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						printf("2 ");
						break;
					case UPDATE_NOT_NEEDED:
						printf("1 ");
						break;
					case UPDATE_FAILED:
						printf("0 ");
						break;
				}
			}
			free(local_ip);
			free(remote_ip);
		}
	}
	printf("\n");
}


void run_request_through_daemon(char** service_names, int force_update, char display_format)
{
	
	// get queue
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 );
	if(mq < 0)
	{
		printf("ERROR: Could not open message queue.  Exiting.\n");
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

	//send requests to queue
	int all_found = service_names[0] == NULL ? 1 : 0;
	int name_index = 0;
	for(name_index = 0; service_names[name_index] != NULL && all_found == 0; name_index++)
	{
		all_found = safe_strcmp(service_names[name_index], "all") == 0 ? 1 : 0;
	}
	
	unsigned long messages_expected;
	if(all_found == 1)
	{
		message_t req_msg;
		req_msg.msg_type = REQUEST_MSG_TYPE;
		req_msg.msg_line[0] = force_update == 1 ? 'f' : ' ';
		sprintf(req_msg.msg_line + 1, "%s", UPDATE_ALL_REQUEST_STR);
		msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, 0);
		messages_expected = 1;
	}
	else
	{
		for(name_index = 0; service_names[name_index] != NULL; name_index++)
		{
			message_t req_msg;
			req_msg.msg_type = REQUEST_MSG_TYPE;
			req_msg.msg_line[0] = force_update == 1 ? 'f' : ' ';
			sprintf(req_msg.msg_line + 1, "%s", service_names[name_index]);
			msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, 0);
		}
		messages_expected = name_index;
	}



	// signal daemon that we've sent requests
	kill((pid_t)daemon_pid, SIGUSR1);


	message_t resp_msg;
	unsigned long messages_read = 0;
	while(messages_read < messages_expected) //blocking read
	{
		int q_read_status = msgrcv(mq, (void *)&resp_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 0);
		if(q_read_status > 0)
		{
			int update_status = (int)resp_msg.msg_line[0];
			messages_expected = messages_expected + ( update_status == UPDATE_OF_MULTIPLE_SERVICES_SCHEDULED ? *((unsigned long*)(resp_msg.msg_line + 1)) : 0);

			if(display_format == 'h')
			{
				char *service_name = (char*)resp_msg.msg_line +1;
				char* test_domain = service_name;
				/*
				ddns_service_config* service_config = (ddns_service_config*)get_map_element(service_configs, service_name);
				if(service_config != NULL)
				{
					test_domain = get_map_element(service_config->variable_definitions, "domain");
				}
				*/
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						printf("\nupdate of %s successful\n", test_domain);
						break;
					case UPDATE_NOT_NEEDED:
						printf("\nupdate of %s not needed\n", test_domain);
						break;
					case UPDATE_FAILED:
						printf("\nupdate of %s failed\n", test_domain);
						break;
				}
			}
			else
			{
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						printf("2 ");
						break;
					case UPDATE_NOT_NEEDED:
						printf("1 ");
						break;
					case UPDATE_FAILED:
						printf("0 ");
						break;
				}
			}
			messages_read++;
		}
	}
	printf("\n");
}


void run_daemon(string_map *service_configs, string_map* service_providers, int force_update)
{


	daemonize(1); //call with 0 instead of 1 to run in foreground

	//open message queue 
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 | IPC_CREAT );
	if(mq < 0)
	{
		//printf("ERROR: Could not create message queue.  Exiting.\n");
		exit(0);
	}


	//we store times of last updates in a special directory
	//if it doesn't exist, create it now
	create_path(UPDATE_INFO_DIR, 0700);
	




	//printf("initial scheduling...\n");

	// load last update times, & do initial updates
	// based on this, initialize update queue
	// in the context of daemon, force_update indicates whether we
	// should force an update on initial check
	priority_queue* update_queue = initialize_priority_queue();
	priority_queue* request_queue = initialize_priority_queue();

	unsigned long num_keys;
	char** service_names = get_map_keys(service_configs, &num_keys);
	time_t reference_time = time(NULL);
	int name_index;
	for(name_index=0; service_names[name_index] != NULL; name_index++)
	{
		char* name = service_names[name_index];

		// since we're working off list we obtained from map, don't need to check for NULL
		ddns_service_config *service_config = get_map_element(service_configs, name); 	
	
	
		char* filename = dynamic_strcat(2, UPDATE_INFO_DIR, name);
		FILE* update_file = fopen(filename, "r");
		free(filename);

		time_t current_time = time(NULL);
		time_t last_update = current_time;
		if(update_file != NULL)
		{
			char newline_terminator[] = { '\n', '\r' };
			dyn_read_t next = dynamic_read(update_file, newline_terminator, 2);
			fclose(update_file);
			
			if(next.str != NULL)
			{
				trim_flanking_whitespace(next.str);
				if(sscanf(next.str, "%ld", &last_update) == 0)
				{
					//scanf failed
					last_update = current_time;
				}
				free(next.str);
			}
		}

		int time_since_update = current_time - last_update;
		int perform_force_update = time_since_update <= 0 || time_since_update >= service_config->force_interval || force_update==1 ? 1 : 0;
		
		//schedule updates
		update_node* next_update = (update_node*)malloc(sizeof(update_node));
		next_update->service_name = service_config->name;
		next_update->next_time = current_time; //we at least perform checks right away, as daemon starts
		if(perform_force_update)
		{
			next_update->last_full_update = 0; //last updated in 1970!
		}
		else
		{
			next_update->last_full_update = last_update;
		}
		push_priority_queue(update_queue,  next_update->next_time - reference_time, next_update->service_name, next_update); 
	}

	// store last update times for local ips from various interfaces & internet
	// and also store the ips themselves
	// if another check was performed within specified check interval
	// the old value will be used
	// also, store the current ips of the various domains (as we set them)
	//
	// local_ip_updates:	interface name/internet -> last check time (time_t*)
	// local_ips: 		interface name/internet -> last value (char*)
	// remote_ips: 		domain -> last value (char*)
	//
	string_map* local_ip_updates = initialize_map(1);
	string_map* local_ips = initialize_map(1);
	string_map* remote_ips = initialize_map(1);

	//printf("starting update loop\n");

	terminated = 0;
	output_requested = 0;
	time_t last_checked = time(NULL);
	while(terminated == 0)
	{
		time_t current_time = time(NULL);
		if(current_time != last_checked)
		{
			last_checked = current_time;
			priority_queue_node *check_rq = peek_priority_queue_node(request_queue);
			priority_queue_node *check_uq = check_rq == NULL ? peek_priority_queue_node(update_queue) : check_rq;
			while(current_time >= ((update_node*)check_uq->value)->next_time || check_rq != NULL)
			{
				//updates can take a bit of time since we're doing network i/o so constantly update current time
				current_time = time(NULL); 
				
				//de-queue the next update to do
				priority_queue_node *p;
			       	if(check_rq != NULL)
				{
					p = shift_priority_queue_node(request_queue);

				}
				else
				{
					p = shift_priority_queue_node(update_queue);
				}
				update_node* next_update = (update_node*)free_priority_queue_node(p);
				next_update->next_time = current_time;
			
				//determine whether to force an update or just check	
				ddns_service_config* service_config = get_map_element(service_configs, next_update->service_name);
				int perform_force_update = current_time - next_update->last_full_update >= service_config->force_interval ? 1 : 0;
			
				//printf("updating %s , forcing = %d\n", service_config->name, perform_force_update);	

				//determine remote ip
				char* test_domain = get_map_element(service_config->variable_definitions, "domain");
				char* remote_ip = get_map_element(remote_ips, test_domain);
				if(remote_ip == NULL && perform_force_update == 0)
				{
					if(test_domain != NULL)
					{
						remote_ip = lookup_domain_ip(test_domain);
						if(remote_ip != NULL)
						{
							set_map_element(remote_ips, test_domain, remote_ip);
						}
					}
				}
				//printf("remote ip = %s\n", remote_ip);
	
				//determine local ip, loading from saved list if ip was obtained less than check_interval seconds ago
				char *interface_name = service_config->ip_source == INTERFACE ? service_config->ip_interface : "internet";
				char *local_ip = (char*)get_map_element(local_ips, interface_name);
				int using_predefined_local_ip = local_ip == NULL ? 0 : 1;
				if(using_predefined_local_ip == 1)
				{
					time_t* update_time = (time_t*)get_map_element(local_ip_updates, interface_name);
					using_predefined_local_ip = (current_time - *update_time) < service_config->check_interval ? 1 : 0;
				}
				if(using_predefined_local_ip == 0)
				{
					local_ip = get_local_ip(service_config->ip_source, service_config->ip_source == INTERFACE ? (void*)service_config->ip_interface : (void*)service_config->ip_url);
					if(local_ip != NULL)
					{
						time_t* update_time;
						time_t *old_time = (time_t*)get_map_element(local_ips, interface_name);
						if(old_time != NULL)
						{
							update_time = old_time;
						}
						else
						{
							update_time = (time_t*)malloc(sizeof(time_t));
						}
						*update_time = current_time;
						set_map_element(local_ip_updates, interface_name, (void*)update_time);
						char* old_element = (char*)set_map_element(local_ips, interface_name, (void*)local_ip);
						if(old_element != NULL)
						{
							free(old_element);
						}
					}
				}
				//printf("local ip = %s\n", local_ip);

				//actually do update
				int update_status = do_single_update(service_config, service_providers, remote_ip, local_ip, perform_force_update, 0);
				//printf("update complete, result = %d\n", update_status);

				//if this update was in response to a request, send
				//result to message queue
				if(check_rq != NULL)
				{
					//printf("sending result of update to queue...");
					message_t resp_msg;
					resp_msg.msg_type = RESPONSE_MSG_TYPE;
					resp_msg.msg_line[0] = (char)update_status; //always a small number, should fit fine
					sprintf(resp_msg.msg_line+1, "%s", service_config->name);
					msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
					//printf("updated\n");
				}


				//schedule next update	
				if(update_status == UPDATE_SUCCESSFUL)
				{
					void* old_element = (char*)set_map_element(remote_ips, test_domain, (void*)strdup(local_ip));
					if(old_element != NULL)
					{
						free(old_element);
					}

					//update the last update time file
					char* filename = dynamic_strcat(2, UPDATE_INFO_DIR, service_config->name);
					FILE* update_file = fopen(filename, "w");
					free(filename);
					if(update_file != NULL)
					{
						fprintf(update_file, "%ld", (long)current_time);
						fclose(update_file);
					}

					time_t next_force_time = current_time + service_config->force_interval;
					time_t next_check_time = current_time + service_config->check_interval;
					next_update->next_time = next_force_time < next_check_time ? next_force_time : next_check_time;
					next_update->last_full_update = current_time;
				}
				else //treat unnecessary update exactly like failed update, except in case of failure make sure we wait a full check_interval before retry
				{
					if(remote_ip != NULL)
					{
						char* old_element = (char*)set_map_element(remote_ips, test_domain, (void*)remote_ip);
						if(old_element != NULL && old_element != remote_ip)
						{
							free(old_element);
						}
					}
					
	
					time_t next_force_time;
				       	if(update_status == UPDATE_NOT_NEEDED)
					{
						next_force_time = next_update->last_full_update + service_config->force_interval;
					}
					else //update_failed
					{
						next_force_time = current_time + service_config->force_interval;
					}
					time_t next_check_time = current_time + service_config->check_interval;
					next_update->next_time = next_force_time < next_check_time ? next_force_time : next_check_time;
				}

				//printf("waiting %d seconds before next update\n", (int)(next_update->next_time - current_time));

				push_priority_queue(update_queue, next_update->next_time-reference_time, next_update->service_name, next_update);
				
				check_rq = peek_priority_queue_node(request_queue);
				check_uq = peek_priority_queue_node(update_queue);
			}
		}

		

		//sleep for 400 milliseconds, or until signal is caught
		usleep(400*1000); 
		
		//handle request from non-daemon if there was one
		if(output_requested == 1)
		{
			//printf("output request detected\n");
			message_t next_req_message;
			next_req_message.msg_line[0] = '\0';
			int message_count = 0;
			while(msgrcv(mq, (void *)&next_req_message, MAX_MSG_LINE, REQUEST_MSG_TYPE, IPC_NOWAIT) > 0)
			{
				message_count++;
				int force_requested = next_req_message.msg_line[0] == 'f' ? 1 : 0;
				char *req_name = next_req_message.msg_line + 1;
				if(safe_strcmp(req_name, UPDATE_ALL_REQUEST_STR) == 0)
				{
					unsigned long num_configs;
					ddns_service_config** all_configs = (ddns_service_config**)get_map_values(service_configs, &num_configs);
				
					/* inform client how many updates we're scheduling */
					if(num_configs > 1)
					{
						message_t resp_msg;
						resp_msg.msg_type = RESPONSE_MSG_TYPE;
						resp_msg.msg_line[0] = UPDATE_OF_MULTIPLE_SERVICES_SCHEDULED;
						*((unsigned long*)(resp_msg.msg_line + 1)) = num_configs;
						msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
					}
					
					int config_index = 0;
					for(config_index = 0; config_index < num_configs; config_index++)
					{
						//printf("running update %d of %ld\n", config_index+1, num_configs);
						ddns_service_config *service_config = all_configs[config_index];
						if(service_config != NULL)
						{
							priority_queue_node *p = remove_priority_queue_node_with_id(update_queue, service_config->name);
							update_node* next_update = (update_node*)free_priority_queue_node(p);
							next_update->next_time = current_time;
	
							if(force_requested == 1)
							{
								next_update->last_full_update = 0;
							}
							push_priority_queue(request_queue, message_count, next_update->service_name, next_update);
						}
						else
						{
							message_t resp_msg;
							resp_msg.msg_type = RESPONSE_MSG_TYPE;
							resp_msg.msg_line[0] = UPDATE_FAILED;
							
							char* service_domain = get_map_element(service_config->variable_definitions, "domain");
							int name_len = strlen(service_config->name);
							int domain_len = strlen(service_domain);
							if(name_len + domain_len < MAX_MSG_LINE - 5)
							{
								sprintf(resp_msg.msg_line+1, "%s (%s)", service_config->name, service_domain);
							}
							else if(name_len < MAX_MSG_LINE -2)
							{
								sprintf(resp_msg.msg_line+1, "%s", service_config->name);
							}
							else
							{
								sprintf(resp_msg.msg_line+1, "next service");
							}
							
							msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
						}

					}
					
					free(all_configs);
				}
				else
				{
					ddns_service_config *service_config = get_map_element(service_configs, req_name);
					//printf("requested %s\n", req_name);
					if(service_config != NULL)
					{
						priority_queue_node *p = remove_priority_queue_node_with_id(update_queue, req_name);
						update_node* next_update = (update_node*)free_priority_queue_node(p);
						next_update->next_time = current_time;
	
						if(force_requested == 1)
						{
							next_update->last_full_update = 0;
						}
						push_priority_queue(request_queue, message_count, next_update->service_name, next_update);
					}
					else
					{
						message_t resp_msg;
						resp_msg.msg_type = RESPONSE_MSG_TYPE;
						resp_msg.msg_line[0] = UPDATE_FAILED;
						
						char* service_domain = get_map_element(service_config->variable_definitions, "domain");
						int name_len = strlen(service_config->name);
						int domain_len = strlen(service_domain);
						if(name_len + domain_len < MAX_MSG_LINE - 5)
						{
							sprintf(resp_msg.msg_line+1, "%s (%s)", service_config->name, service_domain);
						}
						else if(name_len < MAX_MSG_LINE -2)
						{
							sprintf(resp_msg.msg_line+1, "%s", service_config->name);
						}
						else
						{
							sprintf(resp_msg.msg_line+1, "next service");
						}
						msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
					}
				}			
			}
			output_requested = 0;
		
			//printf("done scheduling output request\n");
		}	
		
	}
	
	struct msqid_ds queue_data;
	msgctl(mq, IPC_RMID, &queue_data);


	//remove pid file
	unlink(PID_PATH);

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
			//perror(buf);
			return 0;
		}
		*cpOld = '/';
	}
	return 1;
}


int do_single_update(ddns_service_config *service_config, string_map *service_providers, char* remote_ip, char* local_ip, int force_update, int verbose)
{
	int update_status = UPDATE_FAILED;

	ddns_service_provider* def =(ddns_service_provider*)get_map_element(service_providers, service_config->service_provider);
	if(def != NULL) //should never be null since we check at program start, but double checking these things is often a good idea :-)
	{
		if(local_ip != NULL)
		{
			
			if(verbose > 0 ){ printf("local ip = %s\n", local_ip); }
			int do_update = 1;
			if(force_update == 0)
			{
				do_update = safe_strcmp(local_ip, remote_ip) == 0 ? 0 : 1;
			}
			if(do_update == 1)
			{
				if(verbose > 0 ){ printf("update needed or force requested, performing actual update\n"); }
				char* url_str = do_url_substitution(def, service_config, local_ip);
				http_response* page = get_url_str(url_str);
				
				if(page != NULL)
				{
					if(verbose > 0 ){ printf("page not null\n"); }
					if(verbose > 0 && page->header != NULL){ printf("page header:\n%s\n", page->header); }
					if(page->data != NULL)
					{
						if(verbose > 0  ) { printf("page data:\n%s\n\n", page->data); }
						if(def->success_regexp != NULL)
						{
							update_status = regexec(def->success_regexp, page->data, 0, NULL, 0) == 0 ? UPDATE_SUCCESSFUL : UPDATE_FAILED; 
						}
						else if(def->failure_regexp != NULL)
						{
							update_status = regexec(def->failure_regexp, page->data, 0, NULL, 0) == 0 ? UPDATE_FAILED : UPDATE_SUCCESSFUL;
						}
					}
					free_http_response(page);
				}
				free(url_str);
			}
			else
			{
				update_status = UPDATE_NOT_NEEDED;
			}
		}
	}
	return update_status;
}



char* do_url_substitution(ddns_service_provider* def, ddns_service_config* config, char* current_ip)
{
	char* url = strdup(def->url_template);
	char** variables = def->variables;
	int variable_index = 0;
	for(variable_index = 0; variables[variable_index] != NULL; variable_index++)
	{
		char* var_def = (char*)get_map_element(config->variable_definitions, variables[variable_index]);
				
		char* replace_pattern_tmp= dynamic_strcat(2, "[[", variables[variable_index]);
		char* replace_pattern = dynamic_strcat(2, replace_pattern_tmp, "]]");
		free(replace_pattern_tmp);
		to_uppercase(replace_pattern);
			
		char* new_url;
		if(var_def != NULL)
		{
			new_url = replace_str(url, replace_pattern, var_def);
		}
		else
		{
			new_url = replace_str(url, replace_pattern, "");
		}
		free(url);
		free(replace_pattern);
		url = new_url;
	}
	char* ip_url = replace_str(url, "[[IP]]", current_ip);
	free(url);
	url = ip_url;

	return url;
}

char *replace_str(char *s, char *old, char *new)
{
	char *ret;
	int i, count = 0;
	int newlen = strlen(new);
	int oldlen = strlen(old);

	char* dyn_s = strdup(s);
	s = dyn_s;
	for (i = 0; s[i] != '\0'; i++)
	{
		if (strstr(&s[i], old) == &s[i])
		{
			count++;
			i += oldlen - 1;
		}
	}
	ret = malloc(i + 1 + count * (newlen - oldlen));

	i = 0;
	while (*s)
	{
		if (strstr(s, old) == s)
		{
			strcpy(&ret[i], new);
			i += newlen;
			s += oldlen;
		}
		else
		{
			ret[i++] = *s++;
		}
	}
	ret[i] = '\0';
	free(dyn_s);

	return ret;
}

char* lookup_domain_ip(char* url_str)
{
	char* ip = NULL;

	url_data* url = parse_url(url_str);
	if(url !=  NULL);
	{
		struct hostent* host;
		host = gethostbyname(url->hostname);
		if(host != NULL)
		{
			ip = strdup((char*)inet_ntoa(*((struct in_addr *)host->h_addr)));
		}
		free_url(url);
	}
	return ip;
}

char* get_local_ip(int ip_source, void* check_parameter)
{
	char* ip = NULL;
	if(ip_source == INTERFACE)
	{
		ip = get_interface_ip((char*)check_parameter);
	}
	else
	{
		char** urls = (char**)check_parameter;
		
		//this should probably be loaded from a file -- implement this later
		char default_urls[][100] = {"http://checkip.dyndns.org", "http://checkip.org", "http://www.whatismyip.com/automation/n09230945.asp" "http://myip.dk/", "http://www.ipchicken.com/", "\0"};

		int url_index;
		if(urls != NULL)
		{
			for(url_index=0; urls[url_index] != NULL && ip == NULL; url_index++)
			{
				ip = get_ip_from_url(urls[url_index]);
			}
		}
		for(url_index=0; default_urls[url_index][0] != '\0' && ip == NULL; url_index++)
		{
			ip = get_ip_from_url(default_urls[url_index]);
		}
	}
	
	return ip;
}


char* get_ip_from_url(char* url)
{
	char* ip = NULL;
	http_response* page = get_url_str(url);
	if(page != NULL)
	{
		if(page->data != NULL)
		{
			int status;
			regex_t re;
			if (regcomp(&re, "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)", REG_EXTENDED) == 0)
			{
				regmatch_t m[5];
				status = regexec(&re, page->data, (size_t) 5, m, 0);
				if(status == 0)
				{
					int ip_length = m[0].rm_eo - m[0].rm_so;
					ip = (char*)malloc((1+ip_length)*sizeof(char));
					ip = memcpy(ip, page->data + m[0].rm_so, ip_length);
					ip[ip_length] = '\0';
				}
				regfree(&re);
			}
		}
		free_http_response(page);
	}
	return ip;
}


char* get_interface_ip(char* if_name)
{
	struct ifreq buffer;
	int s = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&buffer, 0x00, sizeof(buffer));
	strcpy(buffer.ifr_name, if_name);
	ioctl(s, SIOCGIFADDR, &buffer);
	close(s);

	struct sockaddr_in *addr = (struct sockaddr_in*)(&buffer.ifr_addr);	
	struct in_addr sin= (struct in_addr)addr->sin_addr;
	char *ip =  strdup((char*)inet_ntoa(sin));

	return ip;
}




string_map* load_service_configurations(char* filename, string_map* service_providers)
{
	char newline_terminator[3];
	newline_terminator[0] = '\n';
	newline_terminator[1] = '\r';


	
	//intialize service_confs to an empty string_map
	string_map* service_confs = initialize_map(1);
	
	//parse the config file
	FILE* service_configuration_file = fopen(filename, "r");	
	if(service_configuration_file != NULL)
	{
		dyn_read_t next;
		next = dynamic_read(service_configuration_file, newline_terminator, 2);
		char** variable = parse_variable_definition_line(next.str);
		while(next.terminator != EOF)
		{
			//cycle past space outside service def line
			while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
			{
				if(variable[0] != NULL)
				{
					free(variable[0]);
					free(variable[1]);
				}
				free(variable);
				next = dynamic_read(service_configuration_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
			}
			
			//we've found a service config section
			if(next.terminator != EOF)
			{
				ddns_service_config* service_conf = (ddns_service_config*)malloc(sizeof(ddns_service_config));
				service_conf->name = variable[1];
			
				//initialize values of service_conf to null values
				service_conf->service_provider = NULL;
				service_conf->check_interval = -1;
				service_conf->force_interval = -1;
				service_conf->ip_source = -1;
				service_conf->ip_url = NULL;
				service_conf->ip_interface = NULL;
				service_conf->variable_definitions = initialize_map(1); 	
				int service_enabled = 0;

				free(variable[0]);
				free(variable);
				
				next = dynamic_read(service_configuration_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
				{
					if(variable[0] != NULL)
					{
						int read = -1;
						char whitespace_separators[] = {'\t', ' '};
						if(safe_strcmp(variable[0], "enabled") == 0)
						{
							sscanf(variable[1], "%d", &service_enabled);
						}
						else if(safe_strcmp(variable[0], "service_provider") == 0)
						{
							service_conf->service_provider = variable[1];
						}
						else if(safe_strcmp(variable[0], "check_interval") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								service_conf->check_interval = read;
							}
						}
						else if(safe_strcmp(variable[0], "force_interval") == 0)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								service_conf->force_interval = read;
							}
						}
						else if(safe_strcmp(variable[0], "ip_source") == 0)
						{
							if(variable[1] != NULL)
							{
								char* dup_var = strdup(variable[1]);
								to_lowercase(dup_var);
								if(safe_strcmp(dup_var, "internet") == 0)
								{
									service_conf->ip_source = INTERNET;
								}
								if(safe_strcmp(dup_var, "interface") == 0)
								{
									service_conf->ip_source = INTERFACE;
								}
								free(dup_var);
							}
						}
						else if(safe_strcmp(variable[0], "ip_url") == 0)
						{
							service_conf->ip_url =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "ip_interface") == 0)
						{
							service_conf->ip_interface = variable[1];
						}
						else
						{
							if(variable[1] == NULL)
							{
								variable[1] = (char*)malloc(sizeof(char));
								variable[1][0] = '\0';
							}
							set_map_element(service_conf->variable_definitions, variable[0], variable[1]);
						}
						free(variable[0]);
					}
					free(variable);
					next = dynamic_read(service_configuration_file, newline_terminator, 2);
					variable = parse_variable_definition_line(next.str);
				}
				
				char* force_unit = remove_map_element(service_conf->variable_definitions, "force_unit");
				if(force_unit != NULL && service_conf->force_interval > 0)
				{
					int force_multiple = get_multiple_for_unit(force_unit);
					service_conf->force_interval = service_conf->force_interval* force_multiple;
					free(force_unit);
				}

				char* check_unit = remove_map_element(service_conf->variable_definitions, "check_unit");
				if(check_unit != NULL && service_conf->check_interval > 0)
				{
					int check_multiple = get_multiple_for_unit(check_unit);
					service_conf->check_interval = service_conf->check_interval* check_multiple;
					free(check_unit);
				}

				
				//test if service_conf has all necessary components
				void* service_provider = NULL;
				if(service_conf->service_provider != NULL )
				{
					service_provider = get_map_element(service_providers, service_conf->service_provider);
				}

				if(	service_enabled > 0 &&
					service_provider != NULL &&
					service_conf->check_interval > 0 &&
					service_conf->force_interval > 0 &&
					( service_conf->ip_source == INTERNET || (service_conf->ip_source == INTERFACE && service_conf->ip_interface != NULL))
				  )
				{
					set_map_element(service_confs, service_conf->name, (void*)service_conf);
				}
				else
				{
					free(service_conf->service_provider);
					free(service_conf->ip_url);
					free(service_conf->ip_interface);
					
					unsigned long num_keys;
					char** keys = get_map_keys(service_conf->variable_definitions, &num_keys);
					int key_index;
					for(key_index = 0; keys[key_index] != NULL; key_index++)
					{
						char* element = (char*)remove_map_element(service_conf->variable_definitions, keys[key_index]);
						free(element);
					}
					free(keys);
					free(service_conf->variable_definitions);
					free(service_conf);
				}
			}
		}
		fclose(service_configuration_file);
	}
	return service_confs;
}

int get_multiple_for_unit(char* unit)
{
	if(safe_strcmp(unit, "minutes") == 0)
	{
		return 60;
	}
	else if(safe_strcmp(unit, "hours"))
	{
		return 60*60;
	}
	else if(safe_strcmp(unit, "days"))
	{
		return 24*60*60;
	}
	else if(safe_strcmp(unit, "weeks"))
	{
		return 7*24*60*60;
	}
		
	return 1;
}


string_map* load_service_providers(char* filename)
{	
	string_map* service_providers = initialize_map(1);
	
	//parse the config file
	FILE *service_provider_file = fopen(filename, "r");	
	char newline_terminator[3];
	newline_terminator[0] = '\n';
	newline_terminator[1] = '\r';
	if(service_provider_file != NULL)
	{
		dyn_read_t next;
		next = dynamic_read(service_provider_file, newline_terminator, 2);
		char** variable = parse_variable_definition_line(next.str);
		while(next.terminator != EOF)
		{
			//cycle past space outside service def line
			while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
			{
				if(variable[0] != NULL)
				{
					free(variable[0]);
					free(variable[1]);
				}
				free(variable);
				next = dynamic_read(service_provider_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
			}
			
			//we've found a service definition
			if(next.terminator != EOF)
			{
				ddns_service_provider* service_provider = (ddns_service_provider*)malloc(sizeof(ddns_service_provider));
				service_provider->name = variable[1];
			
				//initialize values of service_provider to null values
				service_provider->url_template = NULL;
				service_provider->variables = NULL; 
				service_provider->success_regexp = NULL;
				service_provider->failure_regexp = NULL;


				free(variable[0]);
				free(variable);
				
				next = dynamic_read(service_provider_file, newline_terminator, 2);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
				{
					if(variable[0] != NULL)
					{
						if(safe_strcmp(variable[0], "url_template") == 0)
						{
							service_provider->url_template = variable[1];
						}
						else if(safe_strcmp(variable[0], "variables") == 0)
						{
							char whitespace_separators[] = {'\t', ' '};
							service_provider->variables =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "success_regexp") == 0 || safe_strcmp(variable[0], "failure_regexp") == 0 )
						{
							regex_t* regexp = (regex_t*)malloc(sizeof(regex_t));
							int valid = convert_to_regex(variable[1], regexp);
							if(valid == 1)
							{
								service_provider->success_regexp = safe_strcmp(variable[0], "success_regexp") == 0 ? regexp : service_provider->success_regexp;
								service_provider->failure_regexp = safe_strcmp(variable[0], "failure_regexp") == 0 ? regexp : service_provider->failure_regexp;
							}
							else
							{
								// in case of invalid expresion, convert_to_regex has already called reg_free
								// so we don't have to worry about calling reg_free here -- just free the initial malloc above
								free(regexp);
							}
						}
						else
						{
							free(variable[1]);
						}
						free(variable[0]);
					}
					free(variable);
					next = dynamic_read(service_provider_file, newline_terminator, 2);
					variable = parse_variable_definition_line(next.str);
				}

				
				//test if service_provider has all necessary components
				if(	service_provider->url_template != NULL &&
					service_provider->variables != NULL &&
					(service_provider->success_regexp != NULL || service_provider->failure_regexp != NULL)
				  )
				{
					set_map_element(service_providers, service_provider->name, (void*)service_provider);
				}
				else
				{
					free(service_provider->name);
					free(service_provider->url_template);
					free(service_provider->variables);
					free(service_provider->success_regexp);
					free(service_provider->failure_regexp);
					free(service_provider);
				}
			}
		}
		fclose(service_provider_file);
	}
	return service_providers;
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

// requires expression to be surrounded by '/' characters, and deals with escape
// characters '\/', '\r', '\n', and '\t' when escapes haven't been interpreted 
// (e.g. after recieving regex string from user)
//
// returns 1 on good regex, 0 on bad regex
int convert_to_regex(char* str, regex_t* p)
{
	char* trimmed = trim_flanking_whitespace(strdup(str));
	int trimmed_length = strlen(trimmed);
	
	int valid = 1;
	//regex must be defined by surrounding '/' characters
	if(trimmed[0] != '/' || trimmed[trimmed_length-1] != '/')
	{
		valid = 0;
		free(trimmed);
	}

	char* new = NULL;
	if(valid == 1)
	{
		char* internal = (char*)malloc(trimmed_length*sizeof(char));
		int internal_length = trimmed_length-2;	
		memcpy(internal, trimmed+1, internal_length);
		internal[internal_length] = '\0';
		free(trimmed);

		new = (char*)malloc(trimmed_length*sizeof(char));
		int new_index = 0;
		int internal_index = 0;
		char previous = '\0';
		while(internal[internal_index] != '\0' && valid == 1)
		{
			char next = internal[internal_index];
			if(next == '/' && previous != '\\')
			{
				valid = 0;
			}
			else if((next == 'n' || next == 'r' || next == 't' || next == '/') && previous == '\\')
			{
				char previous2 = '\0';
				if(internal_index >= 2)
				{
					previous2 = internal[internal_index-2];
				}

				new_index = previous2 == '\\' ? new_index : new_index-1;
				switch(next)
				{
					case 'n':
						new[new_index] = previous2 == '\\' ? next : '\n';
						break;
					case 'r':
						new[new_index] = previous2 == '\\' ? next : '\r';
						break;
					case 't':
						new[new_index] = previous2 == '\\' ? next : '\t';
						break;
					case '/':
						new[new_index] = previous2 == '\\' ? next : '/';
						break;
				}
				previous = '\0';
				internal_index++;
				new_index++;

			}
			else
			{
				new[new_index] = next;
				previous = next;
				internal_index++;
				new_index++;
			}
		}
		new[new_index] = '\0';
		if(previous == '\\')
		{
			valid = 0;
			free(new);
			new = NULL;
		}
	}
	if(valid == 1)
	{
		valid = regcomp(p,new,REG_EXTENDED) == 0 ? 1 : 0;
		if(valid == 0)
		{
			regfree(p);
		}
		free(new);
	}
	
	return valid;	
}
