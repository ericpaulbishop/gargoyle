#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		printf("error, must specify input and output files\n");
	}
	else if(argc == 2)
	{
		printf("error, must specify output id\n");
	}
	else
	{
		FILE* out = NULL;
		FILE* in = NULL;
		unsigned long file_size = 0;
		char out_name[4096];
		sprintf(out_name, "%s.c", argv[2]);

		in = fopen(argv[1], "rb");
		out = fopen(out_name, "w");
		
		if(in != NULL && out != NULL)
		{
			int next = fgetc(in);
			
			fprintf(out, "unsigned char _binary_%s_data[]={", argv[2]);
			while(next != EOF)
			{
				if(file_size == 0)
				{
					fprintf(out,"%d", next);
				}
				else
				{
					fprintf(out,",%d", next);
				}
				file_size++;
				next = fgetc(in);
			}
			fprintf(out, "};\n");
			fprintf(out, "unsigned long _binary_%s_size=%ld;\n", argv[2], file_size);
		}
		if (in != NULL) fclose(in);
		if (out != NULL) fclose(out);

	}
	return 0;
}
