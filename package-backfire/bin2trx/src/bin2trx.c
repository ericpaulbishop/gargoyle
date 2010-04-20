/*  bin2trx --	A simple program to strip headers to convert .bin files to .trx files
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009 by Eric Bishop <eric@gargoyle-router.com>
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


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("You must specify exactly one argument: the *.bin file to convert to *.trx\n");
	}
	else
	{
		char* fileName = argv[1];
		FILE* adjFile = fopen(fileName, "rb+");
		if(adjFile != NULL)
		{
			fpos_t start_pos;
			unsigned char hdr_test[5];
			int offset;
			int read_test;

			int val = fgetpos(adjFile, &start_pos);

			read_test = fread(hdr_test, 1, 4, adjFile);
			read_test = read_test == 4 ? 4 : 0;
			hdr_test[4] = '\0';
			offset = 0;
			while(read_test > 0 && strcmp(hdr_test, "HDR0") != 0)
			{
				unsigned char next;
				read_test = fread(&next, 1, 1, adjFile);
				if(read_test > 0)
				{
					hdr_test[0] = hdr_test[1];
					hdr_test[1] = hdr_test[2];
					hdr_test[2] = hdr_test[3];
					hdr_test[3] = next;
				}
				offset++;
			}
		
			
			if(read_test > 0 && offset > 0)
			{
				unsigned long new_length = 0;
				unsigned char buffer[4096];
				int buffer_length = 4096;
				unsigned long adjIndex =0;

				fseek(adjFile, offset, SEEK_SET);

				//shift bytes in blocks of 4096
				while(buffer_length == 4096)
				{
					buffer_length = fread(buffer, 1, 4096, adjFile);
					new_length = new_length + buffer_length;
					fseek(adjFile, -1*(buffer_length+offset), SEEK_CUR);
					fwrite(buffer, 1, buffer_length, adjFile);
					if(buffer_length == 4096)
					{
						fseek(adjFile, offset, SEEK_CUR);
					}
				}
				fclose(adjFile);
				truncate(fileName, new_length);
			}
			else
			{	
				fclose(adjFile);
				if(offset > 0)
				{
					fprintf(stderr, "ERROR: Invalid bin/trx file, no header found\n");
				}
			}
		}
	}
	return 0;
}
