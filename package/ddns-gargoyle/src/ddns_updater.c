/*  ddns_updater -	A small tool for periodically performing dynamic DNS updates
 *  			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Eric Bishop 
 * 			http://www.gargoyle-router.com
 * 		  
 *
 *  Copyright © 2008-2011 by Eric Bishop <eric@gargoyle-router.com>
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
#include "ewget.h"
#define malloc safe_malloc
#define strdup safe_strdup

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <syslog.h>
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


#define MAX_LOOKUP_URL_LENGTH	65

int daemon_pid_file;

char default_ip_lookup_url_data[][MAX_LOOKUP_URL_LENGTH] = {
							"http://checkmyip.com",
							"http://www.ipchicken.com",
							"http://www.tracemyip.org",
							"http://checkip.dyndns.org",
							"http://checkip.org", 
							"http://www.ip-address.org",
							"http://my-ip-address.com",
							"http://www.selfseo.com/what_is_my_ip.php",
							"http://aruljohn.com",
							"http://www.lawrencegoetz.com/programs/ipinfo/",
							"http://myipinfo.net",
							"http://www.ip-1.com/",
							"http://www.myipnumber.com",
							"http://www.dslreports.com/whois",
							"\0"
							};

#define MAX_USER_AGENT_LENGTH	125
char user_agents[][MAX_USER_AGENT_LENGTH] = {
						"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0)",        //IE 8, Windows
						"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.2; .NET CLR 1.1.4322)",  //IE 7, Windows
						"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0",         //IE 9, Windows
						"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_4) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.46 Safari/536.5", //Chrome, Mac
						"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_4) AppleWebKit/536.25 (KHTML, like Gecko) Version/6.0 Safari/536.25",       //Safari 6, Mac
						"\0"
						};





char** default_ip_lookup_urls = NULL;


typedef struct
{
	char* name;
	char* url_template;	  	// contains vars that should be replaced, eg http://[USER]:[PASS]@dynservice.com
	char** required_variables;	 	
	char** optional_variables;
	char** optional_variable_defaults;
	char** meta_variables;
	string_map* meta_variable_defs;
	string_map* meta_variables_to_cache;
	string_map* unescaped_variables;
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
	string_map* cached_meta_variables;

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
int get_multiple_for_unit(char* unit);

char* get_local_ip(int ip_source, void* check_parameter);
char* get_next_url_and_rotate(char **urls);
void  initialize_default_ip_lookup_urls(void);
void  free_default_ip_lookup_urls(void);
char* get_ip_from_url(char* url);
char* get_interface_ip(char* if_name);

char* do_url_substitution(ddns_service_provider* def, ddns_service_config* config, char* current_ip);
char* do_line_substitution(char* line, string_map* variables, string_map* escaped_variables);
char *http_req_escape(char *unescaped);
char *replace_str(char *s, char *old, char *new);


int do_single_update(ddns_service_config *service_config, string_map *service_providers, char* remote_ip, char* local_ip, int force_update, int verbose);
char* lookup_domain_ip(char* url_str);

void run_request(string_map *service_configs, string_map *service_providers, char **service_names, int force_update, char display_format, int verbose);
void run_request_through_daemon(char **service_names, int force_update, char display_format);
void run_daemon(string_map *service_configs, string_map *service_providers, int force_update, char run_in_background);

void daemonize(char run_in_background);
void signal_handler(int sig);
int create_path(const char *name, int mode);

void free_service_configs(string_map* service_configs);
void free_service_providers(string_map* service_providers);
void free_service_config(ddns_service_config* config);
void free_service_provider(ddns_service_provider* provider);
void free_update_nodes(update_node** update_nodes, unsigned long num_nodes);


int srand_called = 0;
char* get_random_user_agent(void);


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
	
	daemon_pid_file = -1;
	char* config_file = strdup("/etc/ddns_updater.conf");
	char* service_provider_file = strdup("/etc/ddns_providers.conf");
	char is_daemon = 0;
	char force_update = 0;
	char verbose = 0;
	char display_format = 'h';
	char** service_names = (char**)malloc(sizeof(char*));
	service_names[0] = NULL;
	char run_in_background = 1;

	int c;
	while((c = getopt(argc, argv, "c:C:P:p:DdFfHhMmVvUuGg")) != -1)
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
			case 'G':
			case 'g':
				run_in_background = 0;
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
				
				free(config_file);
				free(service_provider_file);
				int sn_index=0;
				for(sn_index=0; service_names[sn_index] != NULL ; sn_index++)
				{
					free(service_names[sn_index]);
				}
				free(service_names);
				
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
				run_daemon(service_configs, service_providers, force_update, run_in_background);
			}
			else
			{
				run_request(service_configs, service_providers, service_names, force_update, display_format, verbose);
				// handle each service requested or all if none specified
			}
		}
		free_service_configs(service_configs);
		free_service_providers(service_providers);
		free_default_ip_lookup_urls();
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

	free(config_file);
	free(service_provider_file);
	int sn_index=0;
	for(sn_index=0; service_names[sn_index] != NULL ; sn_index++)
	{
		free(service_names[sn_index]);
	}
	free(service_names);

	return 0;
}

void free_service_configs(string_map* service_configs)
{
	unsigned long num_configs;
	unsigned long config_index = 0;
	ddns_service_config** sconfigs = (ddns_service_config**)destroy_string_map(service_configs, DESTROY_MODE_RETURN_VALUES, &num_configs);
	for(config_index = 0; config_index < num_configs; config_index++)
	{
		free_service_config(sconfigs[config_index]);
	}
	free(sconfigs);
}
void free_service_config(ddns_service_config* config)
{
	unsigned long num_destroyed;
	destroy_string_map(config->variable_definitions, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(config->cached_meta_variables, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	free_null_terminated_string_array(config->ip_url);
	free(config->service_provider);
	free(config->ip_interface);
	free(config->name);
	free(config);
}

void free_service_providers(string_map* service_providers)
{
	unsigned long num_providers;
	unsigned long provider_index;
	ddns_service_provider** sproviders = (ddns_service_provider**)destroy_string_map(service_providers, DESTROY_MODE_RETURN_VALUES, &num_providers);
	for(provider_index = 0; provider_index < num_providers; provider_index++)
	{
		free_service_provider(sproviders[provider_index]);
	}
	free(sproviders);
}
void free_service_provider(ddns_service_provider* provider)
{
	unsigned long num_destroyed;
	free(provider->name);
	free(provider->url_template);
	free_null_terminated_string_array(provider->required_variables);
	free_null_terminated_string_array(provider->optional_variables);
	free_null_terminated_string_array(provider->optional_variable_defaults);
	free_null_terminated_string_array(provider->meta_variables);
	destroy_string_map(provider->meta_variable_defs, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(provider->meta_variables_to_cache, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(provider->unescaped_variables, DESTROY_MODE_FREE_VALUES, &num_destroyed);

	if(provider->success_regexp != NULL)
	{
		regfree(provider->success_regexp);
		free(provider->success_regexp);
	}
	if(provider->failure_regexp != NULL)
	{
		regfree(provider->failure_regexp);
		free(provider->success_regexp);
	}
	free(provider);
}
void free_update_nodes(update_node** update_nodes, unsigned long num_nodes)
{
	unsigned long node_index = 0;
	for(node_index=0; node_index < num_nodes; node_index++)
	{
		update_node* u = update_nodes[node_index];
		free(u->service_name);
		free(u);
	}
	free(update_nodes);
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

	if(all_found == 1)
	{
		free_null_terminated_string_array(service_names);
	}
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
		unsigned long read_length;
		char newline_terminator[3] = { '\r', '\n' };
		dyn_read_t pid_read = dynamic_read(pid_file, newline_terminator, 2, &read_length);
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
		memset(req_msg.msg_line, '\0', MAX_MSG_LINE); 
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
			memset(req_msg.msg_line, '\0', MAX_MSG_LINE); 
			req_msg.msg_line[0] = force_update == 1 ? 'f' : ' ';
			sprintf(req_msg.msg_line + 1, "%s", service_names[name_index]);
			msgsnd(mq, (void *)&req_msg, MAX_MSG_LINE, 0);
		}
		messages_expected = name_index;
	}



	// signal daemon that we've sent requests
	kill((pid_t)daemon_pid, SIGUSR1);


	string_map* result_map = initialize_map(1);

	message_t resp_msg;
	unsigned long messages_read = 0;
	while(messages_read < messages_expected) //blocking read
	{
		int q_read_status = msgrcv(mq, (void *)&resp_msg, MAX_MSG_LINE, RESPONSE_MSG_TYPE, 0);
		if(q_read_status > 0)
		{
			int update_status = (int)resp_msg.msg_line[0];
			messages_expected = messages_expected + ( update_status == UPDATE_OF_MULTIPLE_SERVICES_SCHEDULED ? *((unsigned long*)(resp_msg.msg_line + 1)) : 0);
			//printf("mesages_expected = %ld\n", messages_expected);

			char *service_name = (char*)resp_msg.msg_line +1;
			if(display_format == 'h')
			{
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						printf("\nupdate of %s successful\n", service_name);
						break;
					case UPDATE_NOT_NEEDED:
						printf("\nupdate of %s not needed\n", service_name);
						break;
					case UPDATE_FAILED:
						printf("\nupdate of %s failed\n", service_name);
						break;
				}
			}
			else
			{
				int* next_status = (int*)malloc(sizeof(int));
				switch(update_status)
				{
					case UPDATE_SUCCESSFUL:
						*next_status = 2;
						break;
					case UPDATE_NOT_NEEDED:
						*next_status = 1;
						break;
					case UPDATE_FAILED:
					default:
						*next_status = 0;
						break;
				}
				if(!all_found)
				{
					set_map_element(result_map, service_name, next_status);
				}
				else 
				{
					printf("%d ", *next_status);
					free(next_status);
				}
			}
			messages_read++;
		}
	}

	if(display_format != 'h' && !all_found)
	{
		for(name_index = 0; service_names[name_index] != NULL; name_index++)
		{
			char* service_name = service_names[name_index];
			int* result = (int*)get_map_element(result_map, service_name);
			if(result != NULL)
			{
				printf("%d ", *result);
			}
			else
			{
				printf("0 ");
			}
		}

	}

	printf("\n");
	
	unsigned long num_destroyed;
	destroy_string_map(result_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
}


void run_daemon(string_map *service_configs, string_map* service_providers, int force_update, char run_in_background)
{


	daemonize(run_in_background); //call with 0 for forground of 1 for background

	//enable logging to syslog
	openlog("ddns_gargoyle", LOG_NDELAY|LOG_PID, LOG_DAEMON );


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
			unsigned long read_length;
			char newline_terminator[] = { '\n', '\r' };
			dyn_read_t next = dynamic_read(update_file, newline_terminator, 2, &read_length);
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
		next_update->service_name = strdup(service_config->name);
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
			
				char* test_domain = get_map_element(service_config->variable_definitions, "domain");
				if(perform_force_update)
				{
					syslog(LOG_INFO, "Forcing update:");
				}
				else
				{
					syslog(LOG_INFO, "Checking whether update needed:");
				}
				syslog(LOG_INFO, "\tservice provider=%s", service_config->service_provider);
				if(test_domain != NULL)
				{
					syslog(LOG_INFO, "\tdomain=%s", test_domain);
				}

				//determine remote ip
				if(test_domain == NULL) 
				{
					/* 
					 * if domain not defined, e.g. we're updating OpenDNS and 
					 * not typical dynamic dns service use service name as 
					 * unique identifier for storing remote_ip
					 */ 
					test_domain = service_config->name;
				}
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
	
				//determine local ip, loading from saved list if ip was obtained less than 3 seconds ago (in case of multiple simultaneous updates)
				char *interface_name = service_config->ip_source == INTERFACE ? service_config->ip_interface : "internet";
				char *local_ip = (char*)get_map_element(local_ips, interface_name);
				int using_predefined_local_ip = (local_ip == NULL) ? 0 : 1;
				if(using_predefined_local_ip == 1)
				{
					time_t* update_time = (time_t*)get_map_element(local_ip_updates, interface_name);
					if(update_time != NULL)
					{
						using_predefined_local_ip = (current_time - *update_time) < 3 ? 1 : 0;
					}
					else
					{
						using_predefined_local_ip = 0;
					}
				}
				if(using_predefined_local_ip == 0)
				{
					local_ip = get_local_ip(service_config->ip_source, service_config->ip_source == INTERFACE ? (void*)service_config->ip_interface : (void*)service_config->ip_url);
					if(local_ip != NULL)
					{
						time_t *update_time = (time_t*)get_map_element(local_ip_updates, interface_name);
						if(update_time != NULL)
						{
							*update_time = current_time;
						}
						else
						{
							update_time = (time_t*)malloc(sizeof(time_t));
							*update_time = current_time;
							set_map_element(local_ip_updates, interface_name, (void*)update_time);
						}
						char* old_element = (char*)set_map_element(local_ips, interface_name, (void*)local_ip);
						if(old_element != NULL )
						{
							free(old_element);
						}
					}
				}
				if(local_ip != NULL) { syslog(LOG_INFO, "\tlocal IP  = %s", local_ip);       }
				if(local_ip == NULL) { syslog(LOG_INFO, "\tlocal IP cannot be determined");  }
				
				if(remote_ip != NULL){ syslog(LOG_INFO, "\tremote IP = %s\n", remote_ip);     }
				if(remote_ip == NULL){ syslog(LOG_INFO, "\tremote IP cannot be determined");  }



				//actually do update
				int update_status = UPDATE_FAILED;
				if(local_ip != NULL)
				{
					update_status = do_single_update(service_config, service_providers, remote_ip, local_ip, perform_force_update, 0);
				}
				if(update_status == UPDATE_FAILED)     {  syslog(LOG_INFO, "\tUpdate failed\n\n"); }
				if(update_status == UPDATE_NOT_NEEDED) {  syslog(LOG_INFO, "\tUpdate not needed, IPs match\n\n"); }
				if(update_status == UPDATE_SUCCESSFUL) {  syslog(LOG_INFO, "\tUpdate successful\n\n"); }


				//if this update was in response to a request, send
				//result to message queue
				if(check_rq != NULL)
				{
					message_t resp_msg;
					resp_msg.msg_type = RESPONSE_MSG_TYPE;
					memset(resp_msg.msg_line, '\0', MAX_MSG_LINE); 
					resp_msg.msg_line[0] = (char)update_status; //always a small number, should fit fine
					sprintf(resp_msg.msg_line+1, "%s", service_config->name);
					msgsnd(mq, (void *)&resp_msg, MAX_MSG_LINE, 0);
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
						memset(resp_msg.msg_line, '\0', MAX_MSG_LINE); 
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
							if(next_update == NULL)
							{
								//BAD!!!
								exit(1);
							}
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
							memset(resp_msg.msg_line, '\0', MAX_MSG_LINE); 
							resp_msg.msg_line[0] = UPDATE_FAILED;
							sprintf(resp_msg.msg_line+1, "%s", req_name);
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
						//printf("service config %s not null\n", req_name);
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
						//printf("service config %s is null\n", req_name);
						message_t resp_msg;
						resp_msg.msg_type = RESPONSE_MSG_TYPE;
						memset(resp_msg.msg_line, '\0', MAX_MSG_LINE); 
						resp_msg.msg_line[0] = UPDATE_FAILED;
						sprintf(resp_msg.msg_line+1, "%s", req_name);
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

	//close system log
	closelog();

	//remove pid file
	if(daemon_pid_file >= 0)
	{
		// unlocked variable unused, but we get compilation warnings if we don't store return value
		int unlocked = -1;
		unlocked = lockf(daemon_pid_file,F_ULOCK,0);
		unlocked++; //dummy, prevents compilation warnings

		close(daemon_pid_file);
	}
	unlink(PID_PATH);


	//cleanup used memory (makes finding TRUE memory leaks with valgrind a lot easier)
	unsigned long num_destroyed;
	free_null_terminated_string_array(service_names);
	destroy_string_map(local_ip_updates, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(local_ips, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(remote_ips, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	unsigned long num_r;
	unsigned long num_u;
	update_node** rvalues = (update_node**)destroy_priority_queue(request_queue, DESTROY_MODE_RETURN_VALUES, &num_r);
	update_node** uvalues = (update_node**)destroy_priority_queue(update_queue, DESTROY_MODE_RETURN_VALUES, &num_u);
	free_update_nodes(rvalues, num_r);
	free_update_nodes(uvalues, num_u);	


}

void daemonize(char run_in_background) //background variable is useful for debugging, causes program to run in foreground if 0
{

	if(run_in_background != 0)
	{
		//fork and end parent process
		FILE* reopened;
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
		//
		// reopened file handle doesn't do anything, 
		// but we get compilation warnings if we don't store/use return values
		reopened = freopen( "/dev/null", "r", stdin);
		reopened = freopen( "/dev/null", "w", stdout);
		reopened = freopen( "/dev/null", "w", stderr);
		if(reopened){ i++; } //dummy, prevents compilation warnings
	}


	// record pid to lockfile
	daemon_pid_file = open(PID_PATH,O_RDWR|O_CREAT,0644);
	if(daemon_pid_file<0) // exit if we can't open file
	{
		exit(1);
	}
	if(lockf(daemon_pid_file,F_TLOCK,0)<0) // try to lock file, exit if we can't
	{
		exit(1);
	}
	char pid_str[25];
	sprintf(pid_str,"%d\n",getpid());
	if( write(daemon_pid_file,pid_str,strlen(pid_str)) < strlen(pid_str))
	{
		exit(1);
	}


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
				if(verbose > 0) { printf("fetching: \"%s\"\n", url_str); }
				http_response* page = get_url(url_str, NULL);
				
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



char* do_url_substitution(ddns_service_provider* provider, ddns_service_config* config, char* current_ip)
{
	
	string_map*  all_variables = initialize_string_map(1);
	string_map*  escaped_variables = initialize_string_map(0);

	// load required variables 
	int var_index=0;
	for(var_index=0; (provider->required_variables)[var_index] != NULL; var_index++)
	{
		char* var_name = (provider->required_variables)[var_index];
		char* var_def = (char*)get_map_element(config->variable_definitions, var_name);
		if(var_def != NULL)
		{
			set_string_map_element(all_variables, var_name, strdup(var_def) );
		}
		if( get_string_map_element(provider->unescaped_variables, var_name) == NULL)
		{
			set_string_map_element(escaped_variables, var_name, strdup("d"));
		}

	}
	
	//load optional variables
	if(provider->optional_variables != NULL)
	{
		for(var_index=0; (provider->optional_variables)[var_index] != NULL; var_index++)
		{
			char* var_name = (provider->optional_variables)[var_index];
			char* var_def = (char*)get_string_map_element(config->variable_definitions, var_name);
			if(var_def != NULL)
			{
				set_string_map_element(all_variables, var_name, strdup(var_def) );
			}
			else
			{
				var_def = (provider->optional_variable_defaults)[var_index];
				if(var_def != NULL)
				{
					set_string_map_element(all_variables, var_name, strdup(var_def) );
				}
			}
			if( get_string_map_element(provider->unescaped_variables, var_name) == NULL)
			{
				set_string_map_element(escaped_variables, var_name, strdup("d"));
			}
		}
	}

	//set ip variable
	set_string_map_element(all_variables, "IP", strdup(current_ip));


	//compute meta-variables
	if(provider->meta_variables != NULL)
	{
		for(var_index=0; (provider->meta_variables)[var_index] != NULL; var_index++)
		{
			char* var_name = (provider->meta_variables)[var_index];
			char* meta_var = (char*)get_string_map_element(config->cached_meta_variables, var_name);
			char* meta_def = (char*)get_string_map_element(provider->meta_variable_defs, var_name);
			if(meta_var == NULL)
			{
				if(meta_def != NULL)
				{
					unsigned long num_lines;
					char* meta_sub = do_line_substitution(meta_def, all_variables, NULL);
					char** output = get_shell_command_output_lines(meta_sub, &num_lines);
					if(num_lines > 0)
					{
						meta_var = strdup(output[0]);
					}
					free_null_terminated_string_array(output);
					free(meta_sub);
				}
			}
			else
			{
				/* 
				 * need to dynamically allocate meta_var 
				 * (everything in all_variables gets freed after update) 
				 */
				char *tmp = strdup(meta_var);
				meta_var = tmp;
			}
			if(meta_var != NULL)
			{
				set_string_map_element(all_variables, var_name, meta_var);
				
				// cache only specified meta-variables not already cached
				if(	get_string_map_element(provider->meta_variables_to_cache, var_name ) != NULL && 
					get_string_map_element(config->cached_meta_variables, var_name) == NULL
					)
				{
					//since meta_var gets freed.. we need yet ANOTHER copy to cache
					set_string_map_element(config->cached_meta_variables, var_name, strdup(meta_var));
				}
			}
			else
			{
				set_string_map_element(all_variables, var_name, strdup(""));
			}
			if( get_string_map_element(provider->unescaped_variables, var_name) == NULL)
			{
				set_string_map_element(escaped_variables, var_name, strdup("d"));
			}
		}
	}

	
	//do substitution for url
	char* url = do_line_substitution(provider->url_template, all_variables, escaped_variables);
	

	//free computed variables and return
	unsigned long num_destroyed;
	destroy_string_map(all_variables, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(escaped_variables, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	
	return url;
}

char* do_line_substitution(char* line, string_map* variables, string_map* escaped_variables)
{
	unsigned long num_keys;
	char** variable_names = (char**)get_string_map_keys(variables, &num_keys);
	int var_index = 0;
	char* replaced = strdup(line);
	for(var_index=0; variable_names[var_index] != NULL; var_index++)
	{
		char* var = variable_names[var_index];
		char* replace_pattern = dynamic_strcat(3, "[[", var, "]]");
		to_uppercase(replace_pattern);
		char* var_def = (char*)get_string_map_element(variables, var);
		char* new_replaced;
		if(var_def != NULL)
		{
			unsigned char do_escape = 0;
			if(escaped_variables != NULL)
			{
				do_escape = get_string_map_element(escaped_variables, variable_names[var_index]) == NULL ? 0 : 1;
			}

			if(do_escape == 1)
			{
				char* escaped_def = http_req_escape(var_def);
				new_replaced = dynamic_replace(replaced, replace_pattern, escaped_def);
				free(escaped_def);
			}
			else
			{
				new_replaced = dynamic_replace(replaced, replace_pattern, var_def);
			}
		}
		else
		{
			new_replaced = dynamic_replace(replaced, replace_pattern, "");
		}
		free(replace_pattern);
		free(replaced);
		replaced = new_replaced;
	}
	free_null_terminated_string_array(variable_names);

	return replaced;
}



char *http_req_escape(char *unescaped)
{
	//be sure to do '%' first, to avoid much craziness
	char escape_chars[] = { '%', '#', '\t', ' ', '<', '>', '{', '}', '|', '\\', '^', '~', '[', ']', '`', ';', '/', '?', ':', '@', '=', '&', '$', '\0' };
	char* escaped = strdup(unescaped);
	int ec_index=0;

	for(ec_index=0; escape_chars[ec_index] != '\0'; ec_index++)
	{
		char old[2];
		char new[4];
		char* new_escaped;
		sprintf(old, "%c", escape_chars[ec_index]);
		sprintf(new, "%%%.2x", escape_chars[ec_index]);
		new_escaped = dynamic_replace(escaped, old, new);
		free(escaped);
		escaped = new_escaped;
	}

	return escaped;
}



char* lookup_domain_ip(char* url_str)
{
	char* ip = NULL;

	url_request* url = parse_url(url_str, NULL);
	if(url !=  NULL)
	{
		struct hostent* host;
		host = gethostbyname(url->hostname);
		if(host != NULL)
		{
			ip = strdup((char*)inet_ntoa(*((struct in_addr *)host->h_addr)));
		}
		free_url_request(url);
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

		char* next_url;
		char  first_url[MAX_LOOKUP_URL_LENGTH];
		int   is_first_lookup;
		initialize_default_ip_lookup_urls();


		if(urls != NULL)
		{
			is_first_lookup=1;
			next_url = get_next_url_and_rotate(urls);
			strcpy(first_url, next_url);
			while(ip == NULL && (strcmp(first_url, next_url) != 0 || is_first_lookup == 1))
			{
				ip = get_ip_from_url(next_url);
				syslog(LOG_INFO, "\t\t%s local ip from url: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
				if(ip == NULL) { next_url = get_next_url_and_rotate(urls); }
				is_first_lookup = 0;
			}
		}
		if(ip == NULL)
		{
			is_first_lookup=1;
			next_url = get_next_url_and_rotate(default_ip_lookup_urls);
			strcpy(first_url, next_url);
			while(ip == NULL && (strcmp(first_url, next_url) != 0 || is_first_lookup == 1))
			{
				ip = get_ip_from_url(next_url);
				syslog(LOG_INFO, "\t\t%s local ip from url: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
				if(ip == NULL) { next_url = get_next_url_and_rotate(default_ip_lookup_urls); }
				is_first_lookup = 0;
			}
		}

	}

	return ip;
}

void  initialize_default_ip_lookup_urls(void)
{
	if(default_ip_lookup_urls == NULL)
	{
		int num_urls;
		int url_index;
		for(num_urls=0; default_ip_lookup_url_data[num_urls][0] != '\0'; num_urls++){}
		default_ip_lookup_urls = (char**)malloc( (num_urls+2)*sizeof(char*) );
		for(url_index=0; url_index < num_urls+1; url_index++)
		{
			default_ip_lookup_urls[url_index] = (char*)malloc( MAX_LOOKUP_URL_LENGTH );
			strcpy(default_ip_lookup_urls[url_index], default_ip_lookup_url_data[url_index]);
		}
		default_ip_lookup_urls[url_index] = NULL;
	}
}

void free_default_ip_lookup_urls(void)
{
	if(default_ip_lookup_urls != NULL)
	{
		free_null_terminated_string_array(default_ip_lookup_urls);
	}
}



char* get_next_url_and_rotate(char **urls)
{
	char next[MAX_LOOKUP_URL_LENGTH];
	int url_index;


	strcpy(next, urls[0]);
	for(url_index=0; urls[url_index+1][0] != '\0' ; url_index++)
	{
		strcpy(urls[url_index], urls[url_index+1]);
	}
	strcpy(urls[url_index], next);
	

	return urls[url_index];
}

char* get_random_user_agent(void)
{
	int num_user_agents=0;
	for(num_user_agents=0; user_agents[num_user_agents][0] != '\0'; num_user_agents++);
	if(!srand_called)
	{
		srand( time(NULL) );
		srand_called=1;
	}
	int ua_num = rand() % num_user_agents;
	return user_agents[ua_num];
}


char* get_ip_from_url(char* url)
{
	char* ip = NULL;
	http_response* page = get_url(url, get_random_user_agent() );
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
		unsigned long read_length;
		next = dynamic_read(service_configuration_file, newline_terminator, 2, &read_length);
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
				next = dynamic_read(service_configuration_file, newline_terminator, 2, &read_length);
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
				service_conf->cached_meta_variables = initialize_string_map(0);
				int service_enabled = 0;

				free(variable[0]);
				free(variable);
				
				next = dynamic_read(service_configuration_file, newline_terminator, 2, &read_length);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
				{
					if(variable[0] != NULL)
					{
						int read = -1;
						char whitespace_separators[] = {'\t', ' '};
						if(safe_strcmp(variable[0], "enabled") == 0 && variable[1] != NULL)
						{
							sscanf(variable[1], "%d", &service_enabled);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "service_provider") == 0 && variable[1] != NULL)
						{
							service_conf->service_provider = variable[1];
						}
						else if(safe_strcmp(variable[0], "check_interval") == 0 && variable[1] != NULL)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								service_conf->check_interval = read;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "force_interval") == 0 && variable[1] != NULL)
						{
							if(sscanf(variable[1], "%d", &read) > 0)
							{
								service_conf->force_interval = read;
							}
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "ip_source") == 0 && variable[1] != NULL)
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
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "ip_url") == 0  && variable[1] != NULL)
						{
							unsigned long num_pieces;
							service_conf->ip_url =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "ip_interface") == 0  && variable[1] != NULL)
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
					next = dynamic_read(service_configuration_file, newline_terminator, 2, &read_length);
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
					free_service_config(service_conf);
				}
			}
		}
		if(variable[0] != NULL)
		{
			free(variable[0]);
		}
		if(variable[1] != NULL)
		{
			free(variable[1]);
		}
		free(variable);
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
	else if(safe_strcmp(unit, "hours") == 0)
	{
		return 60*60;
	}
	else if(safe_strcmp(unit, "days") == 0)
	{
		return 24*60*60;
	}
	else if(safe_strcmp(unit, "weeks") == 0)
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
		unsigned long read_length;
		next = dynamic_read(service_provider_file, newline_terminator, 2, &read_length);
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
				next = dynamic_read(service_provider_file, newline_terminator, 2, &read_length);
				variable = parse_variable_definition_line(next.str);
			}
			
			//we've found a service definition
			if(next.terminator != EOF)
			{
				ddns_service_provider* service_provider = (ddns_service_provider*)malloc(sizeof(ddns_service_provider));
				service_provider->name = variable[1];
			
				//initialize values of service_provider to null values
				service_provider->url_template = NULL;
				service_provider->success_regexp = NULL;
				service_provider->failure_regexp = NULL;
				service_provider->required_variables = NULL;
				service_provider->optional_variables = NULL;
				service_provider->optional_variable_defaults = NULL;
				service_provider->meta_variables = NULL;
				service_provider->meta_variable_defs = initialize_string_map(0);
				service_provider->meta_variables_to_cache = initialize_string_map(0);
				service_provider->unescaped_variables = initialize_string_map(0);

				free(variable[0]);
				free(variable);
				
				next = dynamic_read(service_provider_file, newline_terminator, 2, &read_length);
				variable = parse_variable_definition_line(next.str);
				while(next.terminator != EOF && safe_strcmp(variable[0], "service") != 0)
				{
					if(variable[0] != NULL)
					{
						if(safe_strcmp(variable[0], "url_template") == 0 && variable[1] != NULL)
						{
							service_provider->url_template = variable[1];
						}
						else if(safe_strcmp(variable[0], "required_variables") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							service_provider->required_variables =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "optional_variables") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							service_provider->optional_variables =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "optional_variable_defaults") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							service_provider->optional_variable_defaults =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "meta_variables") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							service_provider->meta_variables =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "meta_variables_to_cache") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							char** cache_list =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							int cache_index;
							for(cache_index=0; cache_list[cache_index] != NULL; cache_index++)
							{
								char* dummy = strdup("1");
								set_string_map_element(service_provider->meta_variables_to_cache, cache_list[cache_index], dummy);
							}
							free_null_terminated_string_array(cache_list);
							free(variable[1]);
						}
						else if(safe_strcmp(variable[0], "unescaped_variables") == 0 && variable[1] != NULL)
						{
							unsigned long num_pieces;
							char whitespace_separators[] = {'\t', ' '};
							char** unescaped_list =  split_on_separators(variable[1], whitespace_separators, 2, -1, 0, &num_pieces);
							int unescaped_index;
							for(unescaped_index=0; unescaped_list[unescaped_index] != NULL; unescaped_index++)
							{
								char* dummy = strdup("1");
								set_string_map_element(service_provider->unescaped_variables, unescaped_list[unescaped_index], dummy);
							}
							free_null_terminated_string_array(unescaped_list);
							free(variable[1]);
						}
						else if( (safe_strcmp(variable[0], "success_regexp") == 0 || safe_strcmp(variable[0], "failure_regexp") == 0 ) && variable[1] != NULL)
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
							free(variable[1]);
						}
						else if(variable[1] != NULL)
						{
							set_string_map_element(service_provider->meta_variable_defs, variable[0], variable[1]);
						}
						free(variable[0]);
					}
					free(variable);
					next = dynamic_read(service_provider_file, newline_terminator, 2, &read_length);
					variable = parse_variable_definition_line(next.str);
				}

				
				//test if service_provider has all necessary components
				if(	service_provider->url_template != NULL &&
					service_provider->required_variables != NULL &&
					(service_provider->success_regexp != NULL || service_provider->failure_regexp != NULL)
				  )
				{
					set_map_element(service_providers, service_provider->name, (void*)service_provider);
				}
				else
				{
					if( service_provider->url_template == NULL)
					{
						fprintf(stderr, "WARNING: couldn't load service provider %s (no url template)\n", service_provider->name);
					}
					else if( service_provider->required_variables == NULL)
					{
						fprintf(stderr, "WARNING: couldn't load service provider %s (no required variables)\n", service_provider->name);
					}
					else if( service_provider->success_regexp == NULL && service_provider->failure_regexp == NULL)
					{
						fprintf(stderr, "WARNING: couldn't load service provider %s (no regular expression)\n", service_provider->name);
					}
					else
					{
						fprintf(stderr, "WARNING: couldn't load service provider %s\n", service_provider->name);
					}
					free_service_provider(service_provider);
				}
			}
		}
		if(variable[0] != NULL)
		{
			free(variable[0]);
		}
		if(variable[1] != NULL)
		{
			free(variable[1]);
		}
		free(variable);

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
			unsigned long num_pieces;
			char** split_line = split_on_separators(trimmed_line, whitespace_separators, 2, 2, 1, &num_pieces);
			if(split_line[0] != NULL)
			{
				variable_definition[0] = strdup(split_line[0]);
				if(split_line[1] != NULL)
				{
					variable_definition[1] = strdup(split_line[1]);
				}
			}
			free_null_terminated_string_array(split_line);
		}
		free(trimmed_line);
		free(line);  // we always call with dynamically allocated memory, and for convenience we free it here, so we don't have to do it elsewhere
	}
	return variable_definition;
}


