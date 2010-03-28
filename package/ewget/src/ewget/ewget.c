#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ewget.h"

void print_usage_and_exit(char* pname);

int main(int argc, char** argv)
{
	int c;
	char* user_agent = NULL;
	char* out_file = strdup("-");
	while((c = getopt(argc, argv, "O:U:")) != -1)
	{	
		switch(c)
		{
			case 'O':
				free(out_file);
				out_file = strdup(optarg);
				break;
			case 'U':
				user_agent = strdup(optarg);
				break;
			default:
				print_usage_and_exit(argv[0]);
		}
	}
	if(optind == argc)
	{
		fprintf(stderr, "ERROR: No URL specified\n\n");
		print_usage_and_exit(argv[0]);
	}
	else
	{
		http_response* r = get_url(argv[optind], user_agent);
		if(r == NULL)
		{
			fprintf(stderr, "Could Not Fetch URL\n\n");
		}
		else if(r->data == NULL)
		{
			fprintf(stderr, "URL contains no data\n\n");
		}
		else
		{
			FILE* out = (strcmp(out_file, "-") == 0) ? stdout : fopen(out_file, "w");
			if(out == NULL)
			{
				fprintf(stderr, "Could not write to specified file.\n\n");
			}
			else
			{
				fprintf(out, "%s", r->data);
				printf("\n");
				fclose(out);
			}
		}
	}
	return 0;
}




void print_usage_and_exit(char* pname)
{
	fprintf(stderr, "USAGE: %s [OPTIONS] [URL]\n", pname);
	fprintf(stderr, "\t-U User Agent String\n");
	fprintf(stderr, "\t-O Output file, \'-\' for stdout, stdout is used if not specified\n");
	fprintf(stderr, "\n\n");				
	exit(1);
}


