/* ddns_updater -	A small tool for periodically performing dynamic DNS updates
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Eric Bishop
 * 			http://www.gargoyle-router.com
 *
 *
 * Copyright © 2008-2013 by Eric Bishop <eric@gargoyle-router.com>
 *
 * This file is free software: you may copy, redistribute and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
	char* url_template;		// contains vars that should be replaced, eg http://[USER]:[PASS]@dynservice.com
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
	int check_interval;			// seconds
	int force_interval;			// seconds
	int ip_source;				// INTERFACE or CHECK_URL
	char** ip_url;				// url or (whitespace separated) list of urls if ip_source is CHECK_URL
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
int convert_to_regex(char* str, regex_t* p);
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

int main(int argc, char** argv)
{
	printf("test 1\n");

	initialize_default_ip_lookup_urls();

	int num_lookup_urls;
	for(num_lookup_urls=0; default_ip_lookup_urls[num_lookup_urls][0] != '\0'; num_lookup_urls++);
	printf("num lookup urls = %d\n", num_lookup_urls);

	int i;
	for(i=0; i < num_lookup_urls; i++)
	{
		char* ip = get_local_ip(INTERNET, NULL);
		printf("ip = %s\n", ip);
	}
	return 0;

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
				//syslog(LOG_INFO, "\t\t%s local ip from url: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
				printf("\t\t%s local IP from URL: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
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
				//syslog(LOG_INFO, "\t\t%s local ip from url: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
				printf("\t\t%s local IP from URL: %s\n",  (ip == NULL ? "Could not determine" : "Successfully retrieved"),  next_url);
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

int srand_called = 0;
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
	http_response* page = get_url(url, get_random_user_agent());
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
