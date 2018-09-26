/* gipquery -	A small tool for fetching external IP address and GeoLocation
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *			Code thoughtfully borrowed from Eric Bishop's ddns_updater
 *
 *
 * Copyright © 2018 by Michael Gray <support@lantisproject.com>
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

#define MAX_LOOKUP_URL_LENGTH	65
char default_ip_lookup_url[][MAX_LOOKUP_URL_LENGTH] = {
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

char default_geoip_lookup_url[][MAX_LOOKUP_URL_LENGTH] = {
							"http://geoplugin.net/json.gp?ip=[[IP]]",
							"http://geoip.nekudo.com/api/[[IP]]",
							"http://ip-api.com/json/[[IP]]",
//							"https://tools.keycdn.com/geo.json?host=[[IP]]",
							"http://api.geoiplookup.net/?query=[[IP]]",
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
						
char* get_local_ip(void);
char* get_ip_from_url(char* url);

char* get_geo_from_ip(char* ip);
char* get_geoip_from_url(char* url);

char* get_random_user_agent(void);
char* get_random_iplookup_url(void);
char* get_random_geoiplookup_url(void);

char* do_url_ip_substitution(char* url, char* ip);
char *http_req_escape(char *unescaped);
char *replace_str(char *s, char *old, char *new);

int main(int argc, char** argv)
{
	int get_internet_ip = 0;
	int get_geoip_location = 0;
	char* ipv4_address = NULL;
	char* return_format = strdup("HR");

	int c;
	while((c = getopt(argc, argv, "IiGgUuA:a:R:r:")) != -1)
	{
		switch(c)
		{
			case 'I':
			case 'i':
				get_internet_ip = 1;
				break;
			case 'G':
			case 'g':
				get_geoip_location = 1;
				break;
			case 'A':
			case 'a':
				free(ipv4_address);
				ipv4_address = strdup(optarg);
				break;
			case 'R':
			case 'r':
				free(return_format);
				return_format = strdup(optarg);
				break;
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS]\n", argv[0]);
				printf("\t-i lookup IP address\n");
				printf("\t-g [-a IPv4 ADDRESS] lookup GeoIP location -- defaults to fetching new\n");
				printf("\t-a [IPv4 ADDRESS] IP to lookup for GeoIP location -- optional\n");
				printf("\t-r [RETURN FORMAT] format of output -- optional -- JS (Javascript), HR (Human Readable)\n");
				printf("\t-u print usage and exit\n");
				return 0;
		}
	}
	
	char* ip = NULL;
	char* country_code = NULL;
	
	if((get_internet_ip == 1 ) || (get_geoip_location == 1 && ipv4_address == NULL))
	{
		ip = get_local_ip();
	}
	
	if((get_geoip_location == 1) && ((ip != NULL) || (ipv4_address != NULL)))
	{
		if(ipv4_address != NULL)
		{
			free_if_not_null(ip);
			ip = strdup(ipv4_address);
		}
		country_code = get_geo_from_ip(ip);
	}
	
	if(ip != NULL && country_code != NULL)
	{
		if((strcmp(ip, "NO-CONN") == 0) || (strcmp(country_code, "NO-CONN") == 0))
		{
			free_if_not_null(ip);
			free_if_not_null(country_code);
			ip = NULL;
			country_code = NULL;
		}
	}
	
	if(strcmp(return_format, "HR") == 0)
	{
		printf("ip: %s\n", ip);
		printf("country_code: %s\n",(get_geoip_location ? country_code : "Not requested"));
	}
	else if(strcmp(return_format, "JS") == 0)
	{
		printf("var geo_ipaddress = \"%s\";\n", (ip != NULL ? ip : ""));
		printf("var geo_countrycode = \"%s\";\n", (country_code != NULL ? country_code : ""));
	}
	
	free_if_not_null(ip);
	free_if_not_null(country_code);

	return 0;
}

char* get_geo_from_ip(char* ip)
{
	char* country_code = NULL;
	char* next_url;
	char* test_url;
	int attempt = 0;
	next_url = get_random_geoiplookup_url();
	while((country_code == NULL) && (attempt < 4))
	{
		test_url = do_url_ip_substitution(next_url,ip);
		country_code = get_geoip_from_url(test_url);
		if(country_code == NULL) { next_url = get_random_geoiplookup_url(); }
		attempt = attempt + 1;
	}

	return country_code;
}

char* get_local_ip(void)
{
	char* ip = NULL;
	char* next_url;
	int attempt = 0;
	next_url = get_random_iplookup_url();
	while((ip == NULL) && (attempt < 5))
	{
		ip = get_ip_from_url(next_url);
		if(ip == NULL) { next_url = get_random_iplookup_url(); }
		attempt = attempt + 1;
	}

	return ip;
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

char* get_random_iplookup_url(void)
{
	int num_iplookup_urls=0;
	for(num_iplookup_urls=0; default_ip_lookup_url[num_iplookup_urls][0] != '\0'; num_iplookup_urls++);
	if(!srand_called)
	{
		srand( time(NULL) );
		srand_called=1;
	}
	int ip_num = rand() % num_iplookup_urls;
	return default_ip_lookup_url[ip_num];
}

char* get_random_geoiplookup_url(void)
{
	int num_geoiplookup_urls=0;
	for(num_geoiplookup_urls=0; default_geoip_lookup_url[num_geoiplookup_urls][0] != '\0'; num_geoiplookup_urls++);
	if(!srand_called)
	{
		srand( time(NULL) );
		srand_called=1;
	}
	int geoip_num = rand() % num_geoiplookup_urls;
	return default_geoip_lookup_url[geoip_num];
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
	else
	{
		ip = strdup("NO-CONN");
	}
	return ip;
}

char* get_geoip_from_url(char* url)
{
	char* country_code = NULL;
	http_response* page = get_url(url, get_random_user_agent());
	if(page != NULL)
	{
		if(page->data != NULL)
		{
			int status;
			regex_t re;
			if (regcomp(&re, "(country[cC]ode|country.*\"[cC]ode)(\":\"|>)([A-Z]{2})(\"|<)", REG_EXTENDED) == 0)
			{
				regmatch_t m[5];
				status = regexec(&re, page->data, (size_t) 5, m, 0);
				if(status == 0)
				{
					int country_code_length = m[3].rm_eo - m[3].rm_so;
					country_code = (char*)malloc((1+country_code_length)*sizeof(char));
					country_code = memcpy(country_code, page->data + m[3].rm_so, country_code_length);
					country_code[country_code_length] = '\0';
				}
				regfree(&re);
			}
		}
		free_http_response(page);
	}
	else
	{
		country_code = strdup("NO-CONN");
	}
	return country_code;
}

char* do_url_ip_substitution(char* url, char* ip)
{
	char* replaced = strdup(url);
	char* replace_pattern = dynamic_strcat(3, "[[", "IP", "]]");
	char* new_replaced;
	if(ip != NULL)
	{
		new_replaced = dynamic_replace(replaced, replace_pattern, ip);
	}
	else
	{
		new_replaced = dynamic_replace(replaced, replace_pattern, "");
	}
	free(replace_pattern);
	free(replaced);
	replaced = new_replaced;

	return replaced;
}
