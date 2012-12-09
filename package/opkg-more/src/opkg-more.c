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
#include <getopt.h>
#include <regex.h>
#include <stdint.h>

#include "erics_tools.h"


#if __SIZEOF_POINTER__ == 8
	#define SSCANF64 "%lu"
#else
	#define SSCANF64 "%llu"
#endif


typedef struct opkg_conf_struct
{
	char* lists_dir;
	string_map* dest_roots;
} opkg_conf;

string_map* load_parameters(int argc, char** argv);
opkg_conf* load_conf(char* conf_file_name);
void free_conf(opkg_conf* conf);
int file_exists(const char* path);
void print_usage(void);
int convert_to_regex(char* str, regex_t* p);
int sort_string_cmp(const void *a, const void *b);



void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, char* dest_name);
int load_recursive_variables(string_map* package_data, char* package, int load_size, int load_will_fit);

int main(int argc, char** argv)
{

	string_map* parameters = load_parameters(argc, argv);


	//load conf_file
	opkg_conf* conf = load_conf(get_string_map_element(parameters, "config"));
	
	//main data variable
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	

	// load list data
	// this tells us everything about packages except whether they are currently installed
	load_package_data(conf->lists_dir, 1, package_data, matching_packages, parameters, NULL);
	
	//load status data
	unsigned long num_dests;
	int dest_index =0;
	char** dest_paths = (char**)get_string_map_keys(conf->dest_roots, &num_dests);
	for(dest_index=0; dest_index < num_dests; dest_index++)
	{
		char* status_path = dynamic_strcat(2, dest_paths[dest_index], "/usr/lib/opkg/status");
		if(file_exists(status_path))
		{
			load_package_data(status_path, 0, package_data, matching_packages, parameters, get_string_map_element(conf->dest_roots, dest_paths[dest_index]));
		}
		free(status_path);
	}
	
	
	//calculate total depends, total size, and will-fit if requested
	unsigned long num_matching_packages;
	char** match_array = get_string_map_keys(matching_packages, &num_matching_packages);
	int match_index=0;
	qsort(match_array, num_matching_packages, sizeof(char*), sort_string_cmp);
	int load_depends  = get_string_map_element(parameters, "required-depends") != NULL ? 1 : 0;
	int load_size     = get_string_map_element(parameters, "required-size")    != NULL ? 1 : 0;
	int load_will_fit = get_string_map_element(parameters, "will-fit")         != NULL ? 1 : 0;

	if(load_depends || load_size || load_will_fit)
	{
		for(match_index=0; match_array[match_index] != NULL ; match_index++)
		{
			load_recursive_variables(package_data, match_array[match_index], load_size, load_will_fit);
		}
	}


	
	//output
	int output_type = 0; //human readable
	output_type     = get_string_map_element(parameters, "json")       ? 1 : output_type;  //json
	output_type     = get_string_map_element(parameters, "javascript") ? 2 : output_type;  //javascript
	
	if(output_type == 0)
	{
		for(match_index=0; match_array[match_index] != NULL ; match_index++)
		{
			string_map* info = get_string_map_element(package_data, match_array[match_index]);
			uint64_t* required_size = (uint64_t*)get_string_map_element(info, "Required-Size");
			printf("Package: %s\nSize: " SSCANF64 "\n\n", match_array[match_index], *required_size);
		}
		
		
		
		free_null_terminated_string_array(match_array);
	}

	
	/*
	printf("lists_dir = %s\n", conf->lists_dir);
	
	int i;
	printf("\n\ndests:\n");
	for(i=0; i < num_dests; i++)
	{
		printf("\t%s\n", dest_paths[i]);
	}
	*/
	
	
	//cleanup
	free_null_terminated_string_array(dest_paths);	
	free(conf);
	
	return 0;
}

string_map* load_parameters(int argc, char** argv)
{
	//requires: --packages/-p OR -packages-matching/-m
	//requires one or more of: --will-fit/-f [dest], --required-depends/-d, --required-size/-s, --install-destination/-i, --user-installed/-u, --version/-v, --install-time/-t, description/-e
	//optional: --config/-c, --json/-j, --javascript/-a
	

	
	string_map* parameters = initialize_string_map(1);
	static struct option long_options[] = {
		{"packages",            1, 0, 'p'},
		{"packages-matching",   1, 0, 'm'},
		{"will-fit",            1, 0, 'f'},
		{"required-depends",    0, 0, 'd'},
		{"required-size",       0, 0, 's'},
		{"install-destination", 0, 0, 'i'},
		{"user-installed",      0, 0, 'u'},
		{"version",             0, 0, 'v'},
		{"time-installed",      0, 0, 't'},
		{"description",         0, 0, 'e'},
		{"config",              1, 0, 'c'},
		{"json",                0, 0, 'j'},
		{"javascript",          0, 0, 'a'},
		{NULL, 0, NULL, 0}
	};
	int option_index;
	int c;
	while ((c = getopt_long(argc, argv, "p:m:f:dsiuvtec:ja", long_options, &option_index)) != -1)
	{
		regex_t* packages_matching_regex = NULL;
		char* regex_str = NULL;
		int regex_len = 0;
		int valid_regex = 0;
		char** package_list = NULL;
		char package_separators[] = { ' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
		unsigned long num_pieces;
		switch(c)
		{
			case 'p':
				package_list = split_on_separators(optarg, package_separators, 9, -1, 0, &num_pieces);
				if(num_pieces == 0)
				{
					fprintf(stderr, "ERROR: list of packages to display appears to be empty");
					exit(1);
				}
				set_string_map_element(parameters, "packages", package_list);
				break;
			case 'm':
				packages_matching_regex = (regex_t*)malloc(sizeof(regex_t));
				regex_str = strdup(optarg);
				regex_len = strlen(regex_str);
				if(regex_str[0] != '/' || regex_str[regex_len-1] != '/')
				{
					free(regex_str);
					regex_str=dynamic_strcat(3, "/", optarg, "/");
				}
				if( convert_to_regex(regex_str, packages_matching_regex) )
				{
					free(regex_str);
					set_string_map_element(parameters, "packages-matching", packages_matching_regex);
				}
				else
				{
					fprintf(stderr, "ERROR: Invalid regular expression \"%s\"\n", optarg);
					exit(1);
				}

				break;
			case 'f':
				set_string_map_element(parameters, "will-fit", strdup(optarg));
				break;
			case 'd':
				set_string_map_element(parameters, "required-depends", strdup("D"));
				break;
			case 's':
				set_string_map_element(parameters, "required-size", strdup("D"));
				break;
			case 'i':
				set_string_map_element(parameters, "install-destination", strdup("D"));
				break;
			case 'u':
				set_string_map_element(parameters, "user-installed", strdup("D"));
				break;
			case 'v':
				set_string_map_element(parameters, "version", strdup("D"));
				break;
			case 't':
				set_string_map_element(parameters, "time-installed", strdup("D"));
				break;
			case 'e':
				set_string_map_element(parameters, "description", strdup("D"));
				break;
			case 'c':
				set_string_map_element(parameters, "config", strdup(optarg));
				break;
			case 'j':
				set_string_map_element(parameters, "json", strdup("D"));
				break;
			case 'a':
				set_string_map_element(parameters, "javascript", strdup("D"));
				break;
			default:
				print_usage();
				exit(0);
		}
	}

	if(get_string_map_element(parameters, "config") == NULL)
	{
		set_string_map_element(parameters, "config", strdup("/etc/opkg.conf"));
	}
	if(get_string_map_element(parameters, "packages") == NULL && get_string_map_element(parameters, "packages-matching") == NULL)
	{
		fprintf(stderr, "ERROR: You must specify either --packages or --packages-matching\n");
		exit(1);
	}
	if(
		get_string_map_element(parameters, "will-fit")            == NULL &&
		get_string_map_element(parameters, "required-depends")    == NULL &&
		get_string_map_element(parameters, "required-size")       == NULL &&
		get_string_map_element(parameters, "install-destination") == NULL &&
		get_string_map_element(parameters, "user-installed")      == NULL &&
		get_string_map_element(parameters, "version")             == NULL &&
		get_string_map_element(parameters, "time-installed")      == NULL &&
		get_string_map_element(parameters, "description")         == NULL
		)
	{
		fprintf(stderr, "ERROR: You must specify at least one argument specifying what package information to display\n");
		exit(1);
	}		

	string_map* load_match_map = initialize_string_map(1);
	string_map* load_all_map = initialize_string_map(1);
	char* dummy = strdup("D");
	if(get_string_map_element(parameters, "will-fit") != NULL || get_string_map_element(parameters, "required-size") != NULL)
	{
		set_string_map_element(load_all_map, "Depends", dummy);
		set_string_map_element(load_all_map, "Installed-Size", dummy);
		set_string_map_element(load_all_map, "Status", dummy);
	}
	if(get_string_map_element(parameters, "required-depends") != NULL)
	{
		set_string_map_element(load_all_map, "Depends", dummy);
		set_string_map_element(load_all_map, "Status", dummy);
	}
	if(get_string_map_element(parameters, "install-destination"))
	{
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(load_match_map, "Destination", dummy);

	}
	if(get_string_map_element(parameters, "user-installed") )
	{
		set_string_map_element(load_all_map, "Status", dummy);
	}
	if(get_string_map_element(parameters, "version"))
	{
		set_string_map_element(load_match_map, "Version", dummy);
	}
	if(get_string_map_element(parameters, "time-installed"))
	{
		set_string_map_element(load_match_map, "Installed-Time", dummy);
	}
	if(get_string_map_element(parameters, "description"))
	{
		set_string_map_element(load_match_map, "Description", dummy);
	}
	unsigned long num_elements;
	char**  load_all_variables      = get_string_map_keys(load_all_map, &num_elements);
	char**  load_matching_variables = get_string_map_keys(load_match_map, &num_elements);
	set_string_map_element(parameters, "load_all_variables",      load_all_variables);
	set_string_map_element(parameters, "load_matching_variables", load_matching_variables);

	destroy_string_map(load_all_map,   DESTROY_MODE_IGNORE_VALUES, &num_elements);
	destroy_string_map(load_match_map, DESTROY_MODE_IGNORE_VALUES, &num_elements);
	free(dummy);


	return parameters;

}



void print_usage(void)
{

}


opkg_conf* load_conf(char* conf_file_name)
{
	opkg_conf* conf = (opkg_conf*)malloc(sizeof(opkg_conf));
	conf->dest_roots = initialize_string_map(1);
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
					//key = file path, value = name
					set_string_map_element(conf->dest_roots, split_line[num_pieces-1], strdup(split_line[num_pieces-2]));
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
	destroy_string_map(conf->dest_roots, DESTROY_MODE_FREE_VALUES, &num_freed);
	free(conf);
}

void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, char* dest_name)
{
	regex_t* match_regex           = get_string_map_element(parameters, "packages-matching");
	string_map* matching_list      = get_string_map_element(parameters, "packages");
 	char** load_all_variables      = get_string_map_element(parameters, "load_all_variables");
	char** load_matching_variables = get_string_map_element(parameters, "load_matching_variables");
	

	string_map* load_variable_map = initialize_string_map(0);
	int load_var_index=0;
	char* all_dummy = strdup("A");
	char* matching_dummy = strdup("M");
	for(load_var_index=0; load_matching_variables[load_var_index] != NULL; load_var_index++)
	{
		set_string_map_element(load_variable_map, load_matching_variables[load_var_index], matching_dummy);
	}
	for(load_var_index=0; load_all_variables[load_var_index] != NULL; load_var_index++)
	{
		set_string_map_element(load_variable_map, load_all_variables[load_var_index], all_dummy);
	}
	int save_destination = get_string_map_element(load_variable_map, "Destination") != NULL ? 1 : 0 ;


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
		int next_package_matches = 0;
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
				char* var_type;
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
						next_pkg_data = (string_map*)get_string_map_element(existing_package_data, val);
						

						if(next_pkg_data == NULL)
						{
							next_pkg_data = initialize_string_map(1);
							set_string_map_element(existing_package_data, val, next_pkg_data);

						}
						if(match_regex != NULL)
						{
							next_package_matches = regexec(match_regex, val, 0, NULL, 0) == 0  ? 1 : 0;
						}
						else
						{
							next_package_matches = get_string_map_element(matching_list, val) != NULL ?  1 : 0;
						}
						if(next_package_matches)
						{
							set_string_map_element(matching_packages, val, strdup("D"));
							if(save_destination && dest_name != NULL)
							{
								set_string_map_element(next_pkg_data, "Destination", strdup(dest_name));
							}
						}
					}
					else if( (var_type = (char*)get_string_map_element(load_variable_map, key)) != NULL)
					{
						if(var_type[0] == 'A' || next_package_matches == 1)
						{
							void* old_val = set_string_map_element(next_pkg_data, key, strdup(val));
							if(old_val != NULL) { free(old_val); }
						}
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
	free(all_dummy);
	free(matching_dummy);
}

//returns 0 if already installed or package doesn't exist, 1 if we need to install it
int load_recursive_variables(string_map* package_data, char* package, int load_size, int load_will_fit)
{
	string_map* package_info = get_string_map_element(package_data, package);
	int ret = 1;
	if(package_info == NULL)
	{
		ret = 0;
	}
	else
	{
		char* package_status     = get_string_map_element(package_info, "Status");
		string_map* dep_map      = get_string_map_element(package_info, "Required-Depends");
		uint64_t* required_size  = get_string_map_element(package_info, "Required-Size");
		load_size = load_will_fit || load_size;
		if(dep_map == NULL)
		{
			if(load_size)
			{
				required_size = (uint64_t*)malloc(sizeof(uint64_t));
				*required_size = 0;
			}
			dep_map = initialize_map(1);
	
			if(strstr(package_status, "not-installed") == NULL)
			{
				ret=0;
			}
			else
			{
				char* deps = get_string_map_element(package_info, "Depends");
				if(load_size)
				{
					char* installed_size_str = get_string_map_element(package_info, "Installed-Size");
					sscanf(installed_size_str, SSCANF64, required_size);
				}
				
				if(deps != NULL)
				{
					unsigned long num_pieces;
					int dep_index;
					char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
					char** dep_list = split_on_separators(deps, package_separators, 9, -1, 0, &num_pieces);
					for(dep_index=0; dep_index < num_pieces; dep_index++)
					{
						if( load_recursive_variables(package_data, dep_list[dep_index], load_size, load_will_fit) )
						{
							set_string_map_element(dep_map, dep_list[dep_index], strdup("D"));
							
							string_map* dep_info = get_string_map_element(package_data, dep_list[dep_index]);
							string_map* dep_dep_map = get_string_map_element(dep_info, "Required-Depends");
							if(dep_dep_map != NULL)
							{
								unsigned long num_dep_deps;
								unsigned long dep_dep_index;
								char** dep_dep_list = get_string_map_keys(dep_dep_map, &num_dep_deps);
								for(dep_dep_index=0; dep_dep_index < num_dep_deps; dep_dep_index++)
								{
									set_string_map_element(dep_map, dep_dep_list[dep_dep_index], strdup("D"));
								}
								free_null_terminated_string_array(dep_dep_list);
							}
							
							if(load_size)
							{
								uint64_t* dep_size = (uint64_t*)get_string_map_element(dep_info, "Required-Size");
								*required_size = (*required_size) + (*dep_size);
							}
							
						}
					}
				}
				
			}
			
			if(load_size)
			{
				set_string_map_element(package_info, "Required-Size", required_size);
			}
			set_string_map_element(package_info, "Required-Depends", dep_map);		
		}
	}


	return ret;
}


int file_exists(const char* path)
{
	struct stat sbuf;
	return (stat(path,&sbuf) == 0 ? 1 : 0);
}

int convert_to_regex(char* str, regex_t* p)
{
	char* trimmed = trim_flanking_whitespace(strdup(str));
	int trimmed_length = strlen(trimmed);
	
	int valid = 1;
	//regex must be defined by surrounding '/' characters
	if(trimmed[0] != '/' || trimmed[trimmed_length-1] != '/')
	{
		valid = 0;
		free(trimmed);
	}

	char* new = NULL;
	if(valid == 1)
	{
		char* internal = (char*)malloc(trimmed_length*sizeof(char));
		int internal_length = trimmed_length-2;	
		memcpy(internal, trimmed+1, internal_length);
		internal[internal_length] = '\0';
		free(trimmed);

		new = (char*)malloc(trimmed_length*sizeof(char));
		int new_index = 0;
		int internal_index = 0;
		char previous = '\0';
		while(internal[internal_index] != '\0' && valid == 1)
		{
			char next = internal[internal_index];
			if(next == '/' && previous != '\\')
			{
				valid = 0;
			}
			else if((next == 'n' || next == 'r' || next == 't' || next == '/') && previous == '\\')
			{
				char previous2 = '\0';
				if(internal_index >= 2)
				{
					previous2 = internal[internal_index-2];
				}

				new_index = previous2 == '\\' ? new_index : new_index-1;
				switch(next)
				{
					case 'n':
						new[new_index] = previous2 == '\\' ? next : '\n';
						break;
					case 'r':
						new[new_index] = previous2 == '\\' ? next : '\r';
						break;
					case 't':
						new[new_index] = previous2 == '\\' ? next : '\t';
						break;
					case '/':
						new[new_index] = previous2 == '\\' ? next : '/';
						break;
				}
				previous = '\0';
				internal_index++;
				new_index++;

			}
			else
			{
				new[new_index] = next;
				previous = next;
				internal_index++;
				new_index++;
			}
		}
		new[new_index] = '\0';
		if(previous == '\\')
		{
			valid = 0;
			free(new);
			new = NULL;
		}
		free(internal);
	}
	if(valid == 1)
	{
		valid = regcomp(p,new,REG_EXTENDED) == 0 ? 1 : 0;
		if(valid == 0)
		{
			regfree(p);
		}
		free(new);
	}
	
	return valid;	
}

/* comparison function for qsort */ 
int sort_string_cmp(const void *a, const void *b) 
{ 
    const char **a_ptr = (const char **)a;
    const char **b_ptr = (const char **)b;
    return strcmp(*a_ptr, *b_ptr);
}
