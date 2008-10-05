#include "erics_tools.h"

int main(void)
{
	FILE* f = fopen("tmp", "r");	
	char terminators[] = "\n\r";

	dyn_read_t next;
	next.terminator = '\n';
	while(next.terminator != EOF)
	{
       		next =  dynamic_read(f, terminators, 2);
		printf("read \"%s\"\n", next.str);
		free(next.str);
	}
	return 0;
}
