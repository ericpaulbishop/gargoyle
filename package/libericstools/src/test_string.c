#include "erics_tools.h"

int main(void)
{
	FILE* f = fopen("tmp", "r");	
	char terminators[] = "\n\r";
	
	char* file_data = read_entire_file(f, 100);
	printf("%s\n", file_data);
	fclose(f);

	f = fopen("tmp", "r");	
	dyn_read_t next;
	next.terminator = '\n';
	while(next.terminator != EOF)
	{
       		next =  dynamic_read(f, terminators, 2);
		printf("read \"%s\"\n", next.str);
		free(next.str);
	}
	fclose(f);

	return 0;
}
