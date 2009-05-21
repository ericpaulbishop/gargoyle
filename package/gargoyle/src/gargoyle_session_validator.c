#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdarg.h>
#include <time.h>

#include <erics_tools.h>

#include "sha256.h"

#define SESSION_TIMEOUT_MINUTES 10

extern char* crypt( const char* key, const char* setting );

char* get_root_hash(void);

int main (int argc, char **argv)
{
	char *password = NULL;
	char *cookie_hash = NULL;
	char *cookie_exp = NULL;
	char *user_agent = NULL;
	char *src_ip = NULL;
	char *redirect = NULL;

	int next_opt;
	while((next_opt = getopt(argc, argv, "p:P:c:C:e:E:a:A:i:I:")) != -1)
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
				if(exp_time > now && exp_time - (SESSION_TIMEOUT_MINUTES*60) < now)
				{
					expired = 0;
				}
			}

			//now check if hash is valid
			char *combined = dynamic_strcat(4, root_hash, cookie_exp, user_agent, src_ip);
			char* hashed = get_sha256_hash_hex_str(combined);
			if(strcmp(hashed, cookie_hash) == 0)
			{
				if(expired == 0)
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
		if(valid == 1 && src_ip != NULL && user_agent != NULL)
		{
			//set new cookie with new timout
			char* new_hash;
			char *combined;
			char new_exp[100] = "";
			sprintf(new_exp, "%ld", (now+(SESSION_TIMEOUT_MINUTES*60)));
			combined = dynamic_strcat(4, root_hash, new_exp, user_agent, src_ip);
			new_hash = get_sha256_hash_hex_str(combined);
			printf("echo \"Set-Cookie:hash=%s;\"; echo \"Set-Cookie:exp=%s;\"; ", new_hash, new_exp);
			free(new_hash);
			free(combined);
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

char* get_root_hash(void)
{
	int found = 0;
	FILE *pw = fopen("/etc/passwd", "r");
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
