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
#include <sys/statvfs.h>

#include <bbtargz.h>
#include <erics_tools.h>


#if __SIZEOF_POINTER__ == 8
	#define SCANFU64 "%lu"
#else
	#define SCANFU64 "%llu"
#endif


typedef struct opkg_conf_struct
{
	char* lists_dir;
	string_map* dest_roots;
	string_map* dest_names;
	string_map* dest_freespace;
	string_map* dest_totalspace;
} opkg_conf;


string_map* load_parameters(int argc, char** argv);
opkg_conf* load_conf(char* conf_file_name);
void free_conf(opkg_conf* conf);
int file_exists(const char* path);
void print_usage(void);
int convert_to_regex(char* str, regex_t* p);
int sort_string_cmp(const void *a, const void *b);


void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, char* dest_name);
int load_recursive_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes);

void print_output(string_map* package_data, char** sorted_matching_packages, opkg_conf* config, string_map* parameters);



int main(int argc, char** argv)
{

	string_map* parameters = load_parameters(argc, argv);


	//load conf_file
	opkg_conf* conf = load_conf(get_string_map_element(parameters, "config"));
	
	//main data variables
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
	char** sorted_matching_packages = get_string_map_keys(matching_packages, &num_matching_packages);
	int match_index=0;
	qsort(sorted_matching_packages, num_matching_packages, sizeof(char*), sort_string_cmp);
	int load_depends  = get_string_map_element(parameters, "required-depends") != NULL ? 1 : 0;
	int load_size     = get_string_map_element(parameters, "required-size")    != NULL ? 1 : 0;
	int load_will_fit = get_string_map_element(parameters, "will-fit")         != NULL ? 1 : 0;
	uint64_t free_bytes = 0;

	//if we want to calculate will_fit, need to determine free bytes on filesystem
	if(load_will_fit)
	{
		char* dest_name = get_string_map_element(parameters, "will-fit");
		char* dest_path = get_string_map_element(conf->dest_names, dest_name);
		if(dest_path != NULL)
		{
			struct statvfs fs_data;
			if( statvfs(dest_path, &fs_data) == 0 )
			{
				uint64_t block_size  = (uint64_t)fs_data.f_bsize;
				uint64_t blocks_free = (uint64_t)fs_data.f_bavail;
				free_bytes = block_size*blocks_free;
			}
		}
	}

	//recursively load depends/size/will_fit
	if(load_depends || load_size || load_will_fit)
	{
		for(match_index=0; sorted_matching_packages[match_index] != NULL ; match_index++)
		{
			load_recursive_variables(package_data, sorted_matching_packages[match_index], load_size, load_will_fit, free_bytes);
		}
	}

	//dump output
	print_output(package_data, sorted_matching_packages, conf, parameters);
	



	
	//cleanup
	free_null_terminated_string_array(sorted_matching_packages);
	free_null_terminated_string_array(dest_paths);	
	free_conf(conf);
	
	return 0;
}


string_map* load_parameters(int argc, char** argv)
{
	//requires: --packages/-p OR -packages-matching/-m
	//requires one or more of: --will-fit/-f [dest], --required-depends/-d, --required-size/-s, --install-destination/-i, --user-installed/-u, --version/-v, --install-time/-t, --source/-o, description/-e
	//optional: --config/-c, --json/-j, --javascript/-a
	

	
	string_map* parameters = initialize_string_map(1);
	static struct option long_options[] = {
		{"help",                0, 0, 'h'},
		{"packages",            1, 0, 'p'},
		{"packages-matching",   1, 0, 'm'},
		{"will-fit",            1, 0, 'f'},
		{"required-depends",    0, 0, 'd'},
		{"required-size",       0, 0, 's'},
		{"install-destination", 0, 0, 'i'},
		{"user-installed",      0, 0, 'u'},
		{"version",             0, 0, 'v'},
		{"install-time",        0, 0, 't'},
		{"source",              0, 0, 'o'},
		{"description",         0, 0, 'e'},
		{"config",              1, 0, 'c'},
		{"json",                0, 0, 'j'},
		{"javascript",          0, 0, 'a'},
		{"human-readable",      0, 0, 'r'},
		{NULL, 0, NULL, 0}
	};
	int option_index;
	int c;
	while ((c = getopt_long(argc, argv, "hp:m:f:dsiuvtoec:jar", long_options, &option_index)) != -1)
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
				else
				{
					string_map* package_map = initialize_string_map(1);
					int p_index;
					for(p_index=0; p_index < num_pieces ; p_index++)
					{
						set_string_map_element(package_map, package_list[p_index], strdup("D"));
					}
					set_string_map_element(parameters, "packages", package_map);
					free_null_terminated_string_array(package_list);
				}
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
			case 'o':
				set_string_map_element(parameters, "source", strdup("D"));
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
			case 'r':
				//default
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
	if(get_string_map_element(parameters, "packages") != NULL && get_string_map_element(parameters, "packages-matching") != NULL)
	{
		fprintf(stderr, "ERROR: You can only specify one of --packages or --packages-matching\n");
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
		get_string_map_element(parameters, "source")              == NULL &&
		get_string_map_element(parameters, "description")         == NULL
		)
	{
		fprintf(stderr, "ERROR: You must specify at least one argument specifying what package information to display\n");
		exit(1);
	}		

	string_map* load_match_map = initialize_string_map(1);
	string_map* load_all_map = initialize_string_map(1);
	string_map* print_map = initialize_string_map(1);

	char* dummy = strdup("D");
	if(get_string_map_element(parameters, "will-fit") != NULL)
	{
		set_string_map_element(load_all_map, "Depends", dummy);
		set_string_map_element(load_all_map, "Installed-Size", dummy);
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(print_map, "Will-Fit", strdup("%s"));
	}
	if(get_string_map_element(parameters, "required-size") != NULL)
	{
		set_string_map_element(load_all_map, "Depends", dummy);
		set_string_map_element(load_all_map, "Installed-Size", dummy);
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(print_map, "Required-Size", strdup(SCANFU64));
	}
	if(get_string_map_element(parameters, "required-depends") != NULL)
	{
		set_string_map_element(load_all_map, "Depends", dummy);
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(print_map, "Required-Depends", strdup("M"));
	}
	if(get_string_map_element(parameters, "install-destination") != NULL)
	{
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(load_match_map, "Install-Destination", dummy);
		set_string_map_element(print_map, "Install-Destination", strdup("%s"));
	}
	if(get_string_map_element(parameters, "user-installed") != NULL)
	{
		set_string_map_element(load_all_map, "Status", dummy);
		set_string_map_element(load_all_map, "User-Installed", dummy);
		set_string_map_element(print_map, "User-Installed", strdup("%s"));
	}
	if(get_string_map_element(parameters, "version") != NULL)
	{
		set_string_map_element(load_match_map, "Version", dummy);
		set_string_map_element(print_map, "Version", strdup("%s"));
	}
	if(get_string_map_element(parameters, "time-installed") != NULL)
	{
		set_string_map_element(load_match_map, "Installed-Time", dummy);
		set_string_map_element(print_map, "Installed-Time", strdup("%s"));
	}
	if(get_string_map_element(parameters, "source") != NULL)
	{
		set_string_map_element(load_match_map, "Source", dummy);
		set_string_map_element(print_map, "Source", strdup("%s"));
	}
	if(get_string_map_element(parameters, "description") != NULL)
	{
		set_string_map_element(load_match_map, "Description", dummy);
		set_string_map_element(print_map, "Description", strdup("%s"));
	}
	unsigned long num_elements;
	char**  load_all_variables      = get_string_map_keys(load_all_map, &num_elements);
	char**  load_matching_variables = get_string_map_keys(load_match_map, &num_elements);
	set_string_map_element(parameters, "load_all_variables",      load_all_variables);
	set_string_map_element(parameters, "load_matching_variables", load_matching_variables);

	char** print_variables = get_string_map_keys(print_map, &num_elements);
	char** print_variable_formats = (char**)malloc((num_elements+1)*sizeof(char*));
	int print_variable_index = 0;
	for(print_variable_index=0; print_variable_index < num_elements; print_variable_index++)
	{
		print_variable_formats[print_variable_index] = strdup((char*)get_string_map_element(print_map, print_variables[print_variable_index]));
	}
	print_variable_formats[num_elements] = NULL;
	set_string_map_element(parameters, "print_variables",         print_variables);
	set_string_map_element(parameters, "print_variable_formats",  print_variable_formats);
	
	
	
	destroy_string_map(print_map,       DESTROY_MODE_FREE_VALUES,   &num_elements);
	destroy_string_map(load_all_map,    DESTROY_MODE_IGNORE_VALUES, &num_elements);
	destroy_string_map(load_match_map,  DESTROY_MODE_IGNORE_VALUES, &num_elements);
	free(dummy);
	
	return parameters;
}



void print_usage(void)
{

	printf("USAGE:\n");
	printf("  opkg-more displays supplemental package information that\n");
	printf("  cannot be obtained from the opkg command.  In particular,\n");
	printf("  it can display the amount of space required to install a\n");
	printf("  package, *along with all needed dependencies*, and test\n");
	printf("  whether that amount of space exists for a given destination.\n");
	printf("\n");

	printf("  To select which packages to display, specify one of:\n");
	printf("    --packages [LIST_OF_PACKAGE_NAMES]\n");
	printf("    --packages-matching [REGULAR_EXPRESSION]\n");
	printf("\n");

	printf("  To select what information to display specify one or more of:\n");
	printf("    --will-fit, -w [DESTINATION] Whether there is enough space to\n");
	printf("                                 install this package to [DESTINATION]\n");
	printf("    --required-depends, -d       All uninstalled dependencies\n");
	printf("    --required-size, -s          Amount of space needed to install\n");
	printf("                                 this package and all dependencies\n");
	printf("    --install-destination, -i    Destination where the package is installed\n");
	printf("    --user-installed, -u         Whether package was installed by user\n");
	printf("    --version, -v                Package version\n");
	printf("    --install-time, -t           Unix epoch (UTC) when package was installed\n");
	printf("    --source, -o                 Source that provides this package\n");
	printf("    --description, -e            Description of package\n");
	printf("\n");

	printf("  Optional output formats:\n");
	printf("    --human-readable, -r         Human readable output format (default)\n");
	printf("    --json, -j                   JSON output format\n");
	printf("    --javascript, -a             Javascript variable output format\n");
	printf("\n");

	printf("  Other options:\n");
	printf("    --config, -c                 opkg config path, defaults to /etc/opkg.conf\n");
	printf("\n");


	printf("\n\n");	


}


opkg_conf* load_conf(char* conf_file_name)
{
	opkg_conf* conf = (opkg_conf*)malloc(sizeof(opkg_conf));
	conf->dest_roots      = initialize_string_map(1);
	conf->dest_names      = initialize_string_map(1);
	conf->dest_freespace  = initialize_string_map(1);
	conf->dest_totalspace = initialize_string_map(1);


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
					struct statvfs fs_data;
					uint64_t* free_bytes  = (uint64_t*)malloc(sizeof(uint64_t));
					uint64_t* total_bytes = (uint64_t*)malloc(sizeof(uint64_t));
					*free_bytes = 0;
					if( statvfs(split_line[num_pieces-1], &fs_data) == 0 )
					{
						uint64_t block_size   = (uint64_t)fs_data.f_frsize;
						uint64_t blocks_free  = (uint64_t)fs_data.f_bavail;
						uint64_t blocks_total = (uint64_t)fs_data.f_blocks;

						*free_bytes  = block_size*blocks_free;
						*total_bytes = block_size*blocks_total;
					}


					//key = file path, value = name
					set_string_map_element(conf->dest_roots, split_line[num_pieces-1], strdup(split_line[num_pieces-2]));

					//key = name, value = file path
					set_string_map_element(conf->dest_names, split_line[num_pieces-2], strdup(split_line[num_pieces-1]));

					//key = name, value = size (uint64_t)
					set_string_map_element(conf->dest_freespace,  split_line[num_pieces-2], free_bytes);
					set_string_map_element(conf->dest_totalspace, split_line[num_pieces-2], total_bytes);

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
	destroy_string_map(conf->dest_names,      DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_roots,      DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_freespace,  DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_totalspace, DESTROY_MODE_FREE_VALUES, &num_freed);
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
	int save_destination      = get_string_map_element(load_variable_map, "Install-Destination") != NULL ? 1 : 0 ;
	int save_user_installed   = get_string_map_element(load_variable_map, "User-Installed")      != NULL ? 1 : 0 ;


	list* file_list = initialize_list();
	if(source_is_dir)
	{
		DIR* dir = opendir(data_source);
		struct dirent *entry;
		if(dir == NULL)
		{
			fprintf(stderr, "WARNING: package list directory \"%s\" does not exist\n", data_source);
			return;
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
		FILE* data_file = NULL;
		FILE* raw_file = NULL;
		int gz_pid = -1;
		
		data_file = fopen(file_path, "r");
	       	if(data_file != NULL)
		{
			int byte1 = fgetc(data_file);
			int byte2 = fgetc(data_file);
			fclose(data_file);
			data_file = NULL;
			data_file = fopen(file_path, "r");
			if( data_file != NULL && byte1 == 0x1f && byte2 == 0x8b )
			{
				raw_file = data_file;
				data_file = gz_open(raw_file, &gz_pid);
			}
		}
		if(data_file == NULL)
		{
			if(raw_file != NULL)
			{
				fclose(raw_file);
			}
			fprintf(stderr, "WARNING: opkg file \"%s\" does not exist or can not be opened\n", file_path);
			return;
		}

		
				
		string_map* next_pkg_data = NULL;
		char next_line[16384];
		int read_data = 1;
		int next_package_matches = 0;
		char* last_variable = NULL;
		while(read_data > 0)
		{
			char* tmp_last_variable = last_variable;
			last_variable = NULL;
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
							if(save_destination)
							{
								set_string_map_element(next_pkg_data, "Install-Destination", (dest_name == NULL ? strdup("not_installed") : strdup(dest_name)  ));
							}
						}
					}
					else if( (var_type = (char*)get_string_map_element(load_variable_map, key)) != NULL)
					{
						if(var_type[0] == 'A' || next_package_matches == 1)
						{
							void* old_val = set_string_map_element(next_pkg_data, key, strdup(val));
							if(old_val != NULL) { free(old_val); }
							last_variable = strdup(key);
							
							if(save_user_installed)
							{
								if(strcmp(key, "Status") == 0)
								{
									char* is_user = strstr(val, " user ") != NULL ? strdup("true") : strdup("false");
									set_string_map_element(next_pkg_data, "User-Installed", is_user);

								}
							}
						}
					}
				}
				else if( (next_line[0] == ' ' || next_line[0] == '\t') && tmp_last_variable != NULL ) //no ':' separator in line
				{
					char* old = remove_string_map_element(next_pkg_data, tmp_last_variable);
					if(old != NULL)
					{
						int next_index;
						char* start = next_line;
						while(start[0] == ' ' || start[0] == '\t') { start++; };
						for(next_index=0; start[next_index] != '\r' && start[next_index] != '\n' && start[next_index] != '\0'; next_index++){};
						start[next_index] = '\0';
						char* new = dynamic_strcat(3, old, "\n", start);
						set_string_map_element(next_pkg_data, tmp_last_variable, new);
						last_variable = tmp_last_variable;
						tmp_last_variable = NULL;
					}


				}
				read_data = 1;
			}
			if(tmp_last_variable != NULL)
			{
				free(tmp_last_variable);
			}

		}
		if(last_variable != NULL)
		{
			free(last_variable);
		}

		fclose(data_file);
		if(raw_file != NULL)
		{
			fclose(raw_file);
			gz_close(gz_pid);
		}
		free(file_path);
	}


	unsigned long num_destroyed;
	destroy_list(file_list, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(load_variable_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	free(all_dummy);
	free(matching_dummy);
}

//returns 0 if already installed or package doesn't exist, 1 if we need to install it
int load_recursive_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes)
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
					sscanf(installed_size_str, SCANFU64, required_size);
				}
				
				if(deps != NULL)
				{
					unsigned long num_pieces;
					int dep_index;
					char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
					char** dep_list = split_on_separators(deps, package_separators, 9, -1, 0, &num_pieces);
					for(dep_index=0; dep_index < num_pieces; dep_index++)
					{
						if( load_recursive_variables(package_data, dep_list[dep_index], load_size, load_will_fit,free_bytes) )
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
			if(load_will_fit)
			{
				if( *required_size >= free_bytes )
				{
					set_string_map_element(package_info, "Will-Fit", strdup("false"));
				}
				else
				{
					set_string_map_element(package_info, "Will-Fit", strdup("true"));
				}
			}

			set_string_map_element(package_info, "Required-Depends", dep_map);		
		}
	}


	return ret;
}



void print_output(string_map* package_data, char** sorted_matching_packages, opkg_conf* conf, string_map* parameters)
{
	//output
	int output_type = 0; //human readable
	output_type     = get_string_map_element(parameters, "json")       ? 1 : output_type;  //json
	output_type     = get_string_map_element(parameters, "javascript") ? 2 : output_type;  //javascript
	if(output_type == 1)
	{
		printf("{\n");
		printf("\t\"Destinations\": {\n");

	}
	else if(output_type == 2)
	{
		printf("var opkg_info = [];\n");
		printf("var opkg_matching_packages = [];\n");
		printf("var opkg_dests = [];\n");
	}

	unsigned long num_dests;
	unsigned long dest_index;
	char** dests = get_string_map_keys(conf->dest_freespace, &num_dests);
	for(dest_index=0 ; dest_index < num_dests; dest_index++)
	{
		char* dest_name          = dests[dest_index];
		char* dest_root          = (char*)get_string_map_element(conf->dest_names, dest_name);
		uint64_t* dest_freespace  = (uint64_t*)get_string_map_element(conf->dest_freespace, dest_name);
		uint64_t* dest_totalspace = (uint64_t*)get_string_map_element(conf->dest_totalspace, dest_name);
		if(output_type == 1)
		{
			if(dest_index >0){ printf(",\n"); }
			printf("\t\t\"%s\": {\n", dest_name);
			printf("\t\t\t\"Root\": \"%s\",\n", dest_root);
			printf("\t\t\t\"Bytes-Free\": "SCANFU64",\n", *dest_freespace);
			printf("\t\t\t\"Bytes-Total\": "SCANFU64"\n", *dest_totalspace);
			printf("\t\t}");
		}
		else if(output_type == 2)
		{
			printf("opkg_dests['%s'] = [];\n", dest_name);
			printf("opkg_dests['%s']['Root'] = '%s';\n", dest_name, dest_root);
			printf("opkg_dests['%s']['Bytes-Free']  = "SCANFU64"\n", dest_name, *dest_freespace);
			printf("opkg_dests['%s']['Bytes-Total'] = "SCANFU64"\n", dest_name, *dest_totalspace);
		}
		else
		{
			printf("Destination: %s", dest_name);
			printf("Destination-Root: %s\n", dest_root);
			printf("Destination-Bytes-Free: "SCANFU64"\n", *dest_freespace);
			printf("Destination-Bytes-Total: "SCANFU64"\n", *dest_totalspace);
			printf("\n");
		}
	}
	if(output_type == 1)
	{
		printf("\n\t},\n");
		printf("\t\"Packages\" : {\n");
	}


	char** print_variables        = (char**)get_string_map_element(parameters, "print_variables");
	char** print_variable_formats = (char**)get_string_map_element(parameters, "print_variable_formats");
	int match_index;
	for(match_index=0; sorted_matching_packages[match_index] != NULL ; match_index++)
	{
		string_map* info = get_string_map_element(package_data, sorted_matching_packages[match_index]);
		int print_index;
		int printed_index =0;
		
		if(output_type == 1)
		{
			if(match_index >0){ printf(",\n"); }
			printf("\t\t\"%s\": {\n", sorted_matching_packages[match_index]);
		}
		else if(output_type == 2)
		{
			printf("opkg_info[\"%s\"] = [];\n", sorted_matching_packages[match_index]);
			printf("opkg_matching_packages.push(\"%s\");\n", sorted_matching_packages[match_index]);
		}
		else
		{
			printf("Package: %s\n", sorted_matching_packages[match_index]);
		}
		for(print_index=0; print_variables[print_index] != NULL ; print_index++)
		{
			char* var_name = print_variables[print_index];
			void* var = get_string_map_element(info, var_name);
			if(var != NULL && print_variable_formats[print_index][0] == '%')
			{
				if(print_variable_formats[print_index][1] == 's')
				{
					char* esc_val_1 = dynamic_replace((char*)var, "\\", "\\\\");
					char* esc_val_2 = dynamic_replace(esc_val_1, "\"", "\\\"");
					char* esc_val_3 = dynamic_replace(esc_val_2, "\n", "\\n");
					if(output_type == 1)
					{
						if(printed_index >0){ printf(",\n"); }
						printf("\t\t\t\"%s\": \"%s\"", var_name, esc_val_3);
					}
					else if(output_type == 2)
					{
						printf("opkg_info[\"%s\"][\"%s\"] = \"%s\";\n", sorted_matching_packages[match_index], var_name, esc_val_3 );
					}
					else
					{
						char spacer[30];
						int varlen =strlen(var_name);
						int vari;
						spacer[0] = '\n';
						spacer[1] = '\t';
						for(vari=2; vari < varlen+4; vari++)
						{
							spacer[vari] = ' ';
						}
						spacer[vari] = '\0';
						free(esc_val_1);
						esc_val_1 = dynamic_replace((char*)var, "\n", spacer);
						printf("\t%s: %s\n", var_name, esc_val_1);
					}
					free(esc_val_1);
					free(esc_val_2);
					free(esc_val_3);
				}
				else if(print_variable_formats[print_index][1] == 'l')
				{
					if(output_type == 1)
					{
						if(printed_index >0){ printf(",\n"); }
						printf("\t\t\t\"%s\": " SCANFU64 , var_name, *((uint64_t*)var));
					}
					else if (output_type == 2)
					{
						printf("opkg_info[\"%s\"][\"%s\"] = "SCANFU64";\n", sorted_matching_packages[match_index], var_name, *((uint64_t*)var)  );

					}
					else
					{
						printf("\t%s: " SCANFU64 "\n", var_name, *((uint64_t*)var));
					}
				}
				printed_index++;
			}
			else if(var != NULL && print_variable_formats[print_index][0] == 'M')
			{
				unsigned long num_deps;
				unsigned long dep_index;
				char** dep_list = get_string_map_keys((string_map*)var, &num_deps);
				
				if(output_type == 1)
				{
					if(printed_index >0){ printf(",\n"); }
					printf("\t\t\t\"%s\": [\n", var_name);
				}
				else if(output_type ==2)
				{
					const char* end = num_deps > 0 ? "\n" : "];\n";
					printf("opkg_info[\"%s\"][\"%s\"] = [%s", sorted_matching_packages[match_index], var_name, end );
				}
				else
				{
					printf("\t%s:\n", var_name);
				}

				for(dep_index=0; dep_index < num_deps; dep_index++)
				{
					if(output_type == 1)
					{
						if(dep_index >0){ printf(",\n"); }
						printf("\t\t\t\t\"%s\"", dep_list[dep_index]);
					}
					else if(output_type == 2)
					{
						if(dep_index >0){ printf(",\n"); }
						printf("\t\"%s\"", dep_list[dep_index]);
					}
					else
					{
						printf("\t\t%s\n", dep_list[dep_index]);
					}
				}
				
				if(output_type == 1)
				{
					printf("\n\t\t\t]");
				}
				else if(output_type == 2)
				{
					if( num_deps > 0)
					{
						printf("\n\t];\n");
					}
				}
				
				printed_index++;
			}
		}
		if(output_type == 1)
		{
			printf("\n\t\t}");
		}
		else
		{
			printf("\n");
		}
	}
	if(output_type == 1)
	{
		printf("\n\t}\n");
		printf("}\n");
	}
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
