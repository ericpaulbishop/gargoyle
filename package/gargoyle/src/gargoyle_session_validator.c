#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdarg.h>
#include <time.h>

#include <erics_tools.h>
#define malloc safe_malloc
#define strdup safe_strdup

#include "sha256.h"

#define DEFAULT_SESSION_TIMEOUT_MINUTES 15

extern char* crypt( const char* key, const char* setting );

char* get_root_hash(void);
char* get_cookie_time(time_t t);

int main (int argc, char **argv)
{
	char *password = NULL;
	char *cookie_hash = NULL;
	char *cookie_exp = NULL;
	char *user_agent = NULL;
	char *src_ip = NULL;
	char *redirect = NULL;
	int timeout_minutes = DEFAULT_SESSION_TIMEOUT_MINUTES;
	unsigned long browser_time = 0;
	int unconditionally_generate = 0;

	int next_opt;
	int read;
	while((next_opt = getopt(argc, argv, "p:P:c:C:e:E:a:A:i:I:r:R:t:T:b:B:gG")) != -1)
	{
		switch(next_opt)
		{
			case 'p':
			case 'P':
				password = strdup(optarg);
				break;
			case 'c':
			case 'C':
				cookie_hash = strdup(optarg);
				break;
			case 'e':
			case 'E':
				cookie_exp = strdup(optarg);
				break;
			case 'a':
			case 'A':
				user_agent = strdup(optarg);
				break;
			case 'i':
			case 'I':
				src_ip = strdup(optarg);
				break;
			case 'r':
			case 'R':
				redirect = strdup(optarg);
				break;
			case 't':
			case 'T':
				read = sscanf(optarg, "%d", &timeout_minutes);
				if(read > 0)
				{
					timeout_minutes = timeout_minutes > 0 ? timeout_minutes : DEFAULT_SESSION_TIMEOUT_MINUTES;
				}
				else
				{
					timeout_minutes = DEFAULT_SESSION_TIMEOUT_MINUTES;
				}
				break;
			case 'b':
			case 'B':
				read = sscanf(optarg, "%ld", &browser_time);
				browser_time = read > 0 ? browser_time : 0;

				break;
			case 'g':
			case 'G':
				unconditionally_generate = 1;
				break;
		}
	}


	int valid = 0;
	char* root_hash = get_root_hash();
	if(root_hash != NULL)
	{
		time_t now;
		time(&now);
		int expired = 0;

		if(password != NULL)
		{
			valid = strcmp( crypt(password, root_hash), root_hash) == 0 ? 1 : 0;
		}
		else if(cookie_hash != NULL && cookie_exp != NULL && user_agent != NULL && src_ip != NULL)
		{
			// first check that session hasn't expired
			time_t exp_time;
			int read = sscanf(cookie_exp, "%ld", &exp_time);
			if(read > 0)
			{
				expired = 1;
				if(exp_time > now && (exp_time - (timeout_minutes*60) - 2) <= now)
				{
					expired = 0;
				}
			}

			//now check if hash is valid
			char *combined = dynamic_strcat(4, root_hash, cookie_exp, user_agent, src_ip);
			char* hashed = get_sha256_hash_hex_str(combined);
			if(strcmp(hashed, cookie_hash) == 0)
			{
				if(expired == 0 && read > 0)
				{
					valid = 1;
				}
			}
			else
			{
				//if hash doesn't match, problem isn't expiration
				expired = 0;
			}
			free(hashed);
			free(combined);
		}
		if(unconditionally_generate == 1)
		{
			valid = 1;
		}
		if(valid == 1 && src_ip != NULL && user_agent != NULL)
		{
			//set new cookie with new timout
			char* new_hash;
			char* combined;
			char new_exp[100] = "";
			time_t new_exp_t = now+(timeout_minutes*60);
			sprintf(new_exp, "%ld", new_exp_t);
			char* cookie_exp;
		       	if( browser_time > 0 && ((browser_time - now) < (-5*60) || (browser_time - now) > (5*60)) )
			{
				//set cookie expiration based on browser time if server time & browser time disagree by more than 5 minutes, and browser time is defined
				time_t cookie_exp_t = browser_time+(timeout_minutes*60);
				cookie_exp = get_cookie_time(cookie_exp_t);

			}
			else
			{
				//set cookie based on server time
				cookie_exp = get_cookie_time(new_exp_t);
			}
			combined = dynamic_strcat(4, root_hash, new_exp, user_agent, src_ip);
			new_hash = get_sha256_hash_hex_str(combined);
			
			//if we don't know browser time, don't set cookie expiration (in the browser -- server timeout still implemented), otherwise set it
			if(browser_time == 0)
			{
				printf("echo \"Set-Cookie:hash=%s; Path=/;\"; echo \"Set-Cookie:exp=%s; Path=/;\"; ", new_hash, new_exp);
			}
			else
			{
				printf("echo \"Set-Cookie:hash=%s; Expires=%s; Path=/;\"; echo \"Set-Cookie:exp=%s; Expires=%s; Path=/;\"; ", new_hash, cookie_exp, new_exp, cookie_exp);
			}

			free(new_hash);
			free(combined);
			free(cookie_exp);
		}
		else
		{
			if(redirect != NULL)
			{
				//do redirect to login page
				char exp_str[20] = "";
				if(expired == 1)
				{
					sprintf(exp_str, "?expired=1");
				}
				printf("echo \"HTTP/1.1 301 Moved Permanently\" ;echo \"Location: %s%s\" ;", redirect, exp_str);
			}
			else
			{
				// if we're checking validity of password, don't do redirect, just report invalid
				// so this code can be embedded in a plain text response
				printf("echo \"invalid\" ;");
			}
		}
		free(root_hash);
	}
	if(valid == 0)
	{
		//if not valid make sure we die after checking so remainder of page is not displayed
		printf("exit");
	}
	printf("\n");

	return 0;
}

char* get_root_hash_from_file(const char* file)
{
	int found = 0;
	FILE *pw = fopen(file, "r");
	char* root_hash = NULL;

	if(pw != NULL)
	{
		char line[512];
		char* test = fgets(line, 511, pw);
		while(test != NULL && !found)
		{
			if(strlen(test) > 5)
			{
				test[4] = '\0';
				if(strcmp(test, "root") == 0)
				{
					char* hash_end;
					found = 1;
					test = test + 5;
					hash_end = strchr(test, ':');
					*hash_end = '\0';
					root_hash = strdup(test);
				}
			}
			test = fgets(line, 511, pw);
		}
		fclose(pw);
	}
	return root_hash;


}

char* get_root_hash(void)
{
	char* root_hash = get_root_hash_from_file("/etc/shadow");
	if(root_hash == NULL)
	{
		root_hash = get_root_hash_from_file("/etc/passwd");
	}
	return root_hash;
}

char* get_cookie_time(time_t t)
{
	struct tm* utc = gmtime(&t);

	char *wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	char utc_str[200];

	sprintf(utc_str, "%s, %d %s %d %02d:%02d:%02d UTC",
		wdays[utc->tm_wday], utc->tm_mday, months[utc->tm_mon],
		(utc->tm_year + 1900), utc->tm_hour, utc->tm_min, utc->tm_sec);

	return strdup(utc_str);
}
