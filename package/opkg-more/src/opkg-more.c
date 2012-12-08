/*  opkg-more --  Supplemental opkg functionality
 *                Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2012 by Eric Bishop <eric@gargoyle-router.com>
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
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "erics_tools.h"

typedef struct opkg_conf_struct
{
	char* lists_dir;
	list* dest_roots;
} opkg_conf;

opkg_conf* load_conf(char* conf_file_name);
void free_conf(opkg_conf* conf);
void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, char** load_variables);
int file_exists(const char* path);

int main(int argc, char** argv)
{
	//load conf_file as argument
	char* conf_file_name = strdup("/etc/opkg.conf");
	opkg_conf* conf = load_conf(conf_file_name);
	
	//main data variable
	string_map* package_data = initialize_string_map(1);
	

	// load list data
	// this tells us everything about packages except whether they are currently installed
	char* load_variables[] = { "Depends", "Installed-Size", "Status", NULL };
	load_package_data(conf->lists_dir, 1, package_data, load_variables);
	
	//load status data
	unsigned long num_dests;
	int dest_index =0;
	char** dests = (char**)get_list_values(conf->dest_roots, &num_dests);
	for(dest_index=0; dest_index < num_dests; dest_index++)
	{
		char* status_path = dynamic_strcat(3, dests[dest_index], "/", "status");
		if(file_exists(status_path))
		{
			load_package_data(status_path, 0, package_data, load_variables);
		}
		free(status_path);
	}
	free_null_terminated_string_array(dests);	



	





	
	printf("conf_file = %s\n", conf_file_name);
	printf("lists_dir = %s\n", conf->lists_dir);

	dests = (char**)get_list_values(conf->dest_roots, &num_dests);
	int i;
	printf("\n\ndests:\n");
	for(i=0; i < num_dests; i++)
	{
		printf("\t%s\n", dests[i]);
	}
	

	//cleanup
	free(conf_file_name);
	free(conf);
	
	return 0;
}


opkg_conf* load_conf(char* conf_file_name)
{
	opkg_conf* conf = (opkg_conf*)malloc(sizeof(opkg_conf));
	conf->dest_roots = initialize_list();
	conf->lists_dir = strdup("/usr/lib/opkg/lists");

	FILE* conf_file =  fopen(conf_file_name, "r");
	int read_data = 1;
	char next_line[1024];
	if(conf_file == NULL)
	{
		fprintf(stdout, "ERROR: couldn't open conf file \"%s\"\n", conf_file_name);
		exit(1);
	}
	while(read_data > 0)
	{
		next_line[0] = '\0';
		fgets(next_line, 1024, conf_file);
		if(next_line[0] != '\0')
		{
			unsigned long num_pieces=0;
			char whitespace[] = "\r\n\t "; //include newlines to strip them off end of line
			char** split_line = split_on_separators(next_line, whitespace, 4, -1, 0, &num_pieces);
			if(num_pieces >= 2)
			{
				if(strstr(split_line[0], "dest") == split_line[0])
				{
					push_list(conf->dest_roots, strdup(split_line[num_pieces-1]));
				}
				else if(strstr(split_line[0], "lists_dir") == split_line[0])
				{
					free(conf->lists_dir);
					conf->lists_dir = strdup(split_line[num_pieces-1]);
				}
			}
			free_null_terminated_string_array(split_line);
		}
		else
		{
			read_data = 0;
		}
	}
	fclose(conf_file);

	return conf;

}

void free_conf(opkg_conf* conf)
{
	unsigned long num_freed;
	free(conf->lists_dir);
	destroy_list(conf->dest_roots, DESTROY_MODE_FREE_VALUES, &num_freed);
	free(conf);
}


void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, char** load_variables)
{
	string_map* load_variable_map = initialize_string_map(0);
	int load_var_index=0;
	char* dummy = strdup("D");
	for(load_var_index=0; load_variables[load_var_index] != NULL; load_var_index++)
	{
		set_string_map_element(load_variable_map, load_variables[load_var_index], dummy);
	}

	list* file_list = initialize_list();
	if(source_is_dir)
	{
		DIR* dir = opendir(data_source);
		struct dirent *entry;
		if(dir == NULL)
		{
			fprintf(stderr, "ERROR: package list directory \"%s\" does not exist\n", data_source);
			exit(1);
		}
		while(entry = readdir(dir))
		{
			if(entry->d_type == DT_REG)
			{
				char* file_path = dynamic_strcat(3, data_source, "/", entry->d_name);
				push_list(file_list, file_path);
			}	
		}
		closedir(dir);
	}
	else
	{
		push_list(file_list, strdup(data_source));
	}
	while(file_list->length > 0)
	{
		char* file_path = (char*)shift_list(file_list);
		FILE* data_file = fopen(file_path, "r");
		if(data_file == NULL)
		{
			fprintf(stderr, "ERROR: opkg file \"%s\" does not exist\n", file_path);
			exit(1);
		}
		
		
		string_map* next_pkg_data = NULL;
		char next_line[16384];
		int read_data = 1;
		while(read_data > 0)
		{
			read_data = 0;
			next_line[0] = '\0';
			fgets(next_line, 16384, data_file);
			if(next_line[0] != '\0')
			{
				// unlike for config, don't use split_line for parsing line. 
				// split_line is simpler to use, but slower
				// (uses dynamic memory allocation)
				// We may need to parse a LOT of lines here, so be extra careful
				char *key = next_line;
				char *val = strchr(next_line, ':');
				char *end = val;
				if(val != NULL)
				{
					while( val[0] == ':' || val[0] == ' ' || val[0] == '\t')
					{
						val[0] = '\0';
						val++;
					}
					end = val;
					while(end[0] != '\0')
					{
						if(end[0] == '\r' || end[0] == '\n') 
						{ 
							end[0] = '\0';
						}
						else
						{
							end++;
						}
					}
					if(strcmp(key, "Package") == 0)
					{
						printf("package: %s\n", val);
						next_pkg_data = (string_map*)get_string_map_element(existing_package_data, val);
						if(next_pkg_data == NULL)
						{
							next_pkg_data = initialize_string_map(1);
							set_string_map_element(existing_package_data, val, next_pkg_data);
						}
					}
					else if( get_string_map_element(load_variable_map, key) != NULL)
					{
						void* old_val = set_string_map_element(next_pkg_data, key, strdup(val));
						if(old_val != NULL) { free(old_val); }
					}


				}
				read_data = 1;
			}
		}
		fclose(data_file);
		free(file_path);
	}






	unsigned long num_destroyed;
	destroy_list(file_list, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(load_variable_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	free(dummy);
}


int file_exists(const char* path)
{
	struct stat sbuf;
	return (stat(path,&sbuf) == 0 ? 1 : 0);
}


