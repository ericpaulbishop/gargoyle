
#include "gpkg.h"

static FILE* __save_pkg_status_stream = NULL;
void save_pkg_status_func(char* key, void* value);
void free_pkg_func(char* key, void* value);
void free_recursive_pkg_vars_func(char* key, void* value);


uint64_t destination_bytes_free(opkg_conf* conf, char* dest_name)
{
	char* dest_path = get_string_map_element(conf->dest_names, dest_name);
	uint64_t free_bytes = 0;
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
	return free_bytes;
}

void load_all_package_data(opkg_conf* conf, string_map* package_data, string_map* matching_packages, string_map* parameters, int load_all_packages, int load_variable_def, char* install_root)
{
	// load list data
	// this tells us everything about packages except whether they are currently installed
	load_package_data(conf->lists_dir, 1, package_data, matching_packages, parameters, load_all_packages, load_variable_def, NULL);
	
	//load status data
	unsigned long num_dests;
	int dest_index =0;
	char** dest_paths = (char**)get_string_map_keys(conf->dest_roots, &num_dests);
	for(dest_index=0; dest_index < num_dests; dest_index++)
	{
		char* status_path = dynamic_strcat(2, dest_paths[dest_index], "/usr/lib/opkg/status");
		if(path_exists(status_path))
		{
			load_package_data(status_path, 0, package_data, matching_packages, parameters, load_all_packages, load_variable_def, get_string_map_element(conf->dest_roots, dest_paths[dest_index]));
		}
		free(status_path);
	}
	
	
	//calculate total depends, total size, and will-fit if requested
	unsigned long num_matching_packages;
	char** sorted_matching_packages = get_string_map_keys(matching_packages, &num_matching_packages);
	int match_index=0;
	do_istr_sort(sorted_matching_packages, num_matching_packages);
	int load_depends  = load_variable_def == LOAD_ALL_PKG_VARIABLES ? 1 : 0;
	int load_size     = load_variable_def == LOAD_ALL_PKG_VARIABLES ? 1 : 0;
	int load_will_fit = load_variable_def == LOAD_ALL_PKG_VARIABLES && install_root != NULL ? 1 : 0;
	if(parameters != NULL && load_variable_def != LOAD_ALL_PKG_VARIABLES )
	{
		load_depends  = get_string_map_element(parameters, "required-depends") != NULL ? 1 : 0;
		load_size     = get_string_map_element(parameters, "required-size")    != NULL ? 1 : 0;
		load_will_fit = get_string_map_element(parameters, "will-fit")         != NULL ? 1 : 0;
	}
	uint64_t free_bytes = 0;

	//if we want to calculate will_fit, need to determine free bytes on filesystem
	if(install_root != NULL)
	{
		free_bytes = destination_bytes_free(conf, install_root);
	}

	//recursively load depends/size/will_fit
	if(load_depends || load_size || load_will_fit)
	{
		for(match_index=0; sorted_matching_packages[match_index] != NULL ; match_index++)
		{
			load_recursive_package_data_variables(package_data, sorted_matching_packages[match_index], load_size, load_will_fit, free_bytes);
		}
	}
}

void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, int load_all_packages, int load_variable_def, char* dest_name)
{
	regex_t* match_regex           = parameters != NULL ? get_string_map_element(parameters, "packages-matching") : NULL;
	string_map* matching_list      = parameters != NULL ? get_string_map_element(parameters, "packages") : NULL;
 	char** load_all_variables      = parameters != NULL ? get_string_map_element(parameters, "load_all_variables") : NULL;
	char** load_matching_variables = parameters != NULL ? get_string_map_element(parameters, "load_matching_variables") : NULL;
	

	string_map* load_variable_map = initialize_string_map(0);
	int load_var_index=0;
	char* all_dummy = strdup("A");
	char* matching_dummy = strdup("M");

	if(parameters == NULL)
	{
		load_all_packages = 1;
		load_variable_def = load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES ? LOAD_MINIMAL_PKG_VARIABLES : load_variable_def;
	}
	if(load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES)
	{
		if(load_matching_variables != NULL)
		{
			for(load_var_index=0; load_matching_variables[load_var_index] != NULL; load_var_index++)
			{
				set_string_map_element(load_variable_map, load_matching_variables[load_var_index], matching_dummy);
			}
		}
		if(load_all_variables != NULL)
		{
			for(load_var_index=0; load_all_variables[load_var_index] != NULL; load_var_index++)
			{
				set_string_map_element(load_variable_map, load_all_variables[load_var_index], all_dummy);
			}
		}
	}
	else if(load_variable_def == LOAD_MINIMAL_PKG_VARIABLES)
	{
		set_string_map_element(load_variable_map, "Status",  all_dummy);
		set_string_map_element(load_variable_map, "Depends", all_dummy);
		set_string_map_element(load_variable_map, "Installed-Size",    all_dummy);
	}
	else
	{
		load_variable_def = LOAD_ALL_PKG_VARIABLES;
		/* no need to set entries in load_variable_map */
	}

	int save_destination      = get_string_map_element(load_variable_map, "Install-Destination") != NULL ? 1 : 0 ;
	int save_user_installed   = get_string_map_element(load_variable_map, "User-Installed")      != NULL ? 1 : 0 ;
	int save_src_id           = get_string_map_element(load_variable_map, "Source-ID")           != NULL ? 1 : 0 ;


	list* file_list = initialize_list();
	char* pkg_src_id = NULL;
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
		if(source_is_dir)
		{
			pkg_src_id = strrchr(file_path, '/');
			pkg_src_id = pkg_src_id == NULL ? file_path : pkg_src_id+1;
		}
		else
		{
			pkg_src_id = NULL;
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

						next_package_matches = load_all_packages;
						if(!next_package_matches)
						{
							if(match_regex != NULL)
							{
								next_package_matches = regexec(match_regex, val, 0, NULL, 0) == 0  ? 1 : 0;
							}
							else
							{
								next_package_matches = get_string_map_element(matching_list, val) != NULL ?  1 : 0;
							}
						}
						if(next_package_matches)
						{
							set_string_map_element(matching_packages, val, strdup("D"));
							if(save_destination || load_variable_def == LOAD_ALL_PKG_VARIABLES )
							{
								set_string_map_element(next_pkg_data, "Install-Destination", (dest_name == NULL ? strdup("not_installed") : strdup(dest_name)  ));
							}
							if(pkg_src_id != NULL && (save_src_id || load_variable_def == LOAD_ALL_PKG_VARIABLES))
							{
								set_string_map_element(next_pkg_data, "Source-ID", strdup(pkg_src_id));
							}
						}
					}
					else if(load_variable_def == LOAD_ALL_PKG_VARIABLES || (var_type = (char*)get_string_map_element(load_variable_map, key)) != NULL)
					{
						if(load_variable_def == LOAD_ALL_PKG_VARIABLES || var_type[0] == 'A' || next_package_matches == 1)
						{
							void* old_val = set_string_map_element(next_pkg_data, key, strdup(val));
							if(old_val != NULL) { free(old_val); }
							last_variable = strdup(key);
							
							if(load_variable_def == LOAD_ALL_PKG_VARIABLES  || save_user_installed)
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
int load_recursive_package_data_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes)
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
					if(installed_size_str != NULL)
					{
						sscanf(installed_size_str, SCANFU64, required_size);
					}
				}
				
				if(deps != NULL)
				{
					unsigned long num_pieces;
					int dep_index;
					char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
					char** dep_list = split_on_separators(deps, package_separators, 9, -1, 0, &num_pieces);
				
					for(dep_index=0; dep_index < num_pieces; dep_index++)
					{
						if( load_recursive_package_data_variables(package_data, dep_list[dep_index], load_size, load_will_fit,free_bytes) )
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

void save_pkg_status_func(char* key, void* value)
{
	string_map* pkg_map = (string_map*)value;
	char* pkg_vars[] = { "Version", "Depends", "Provides", "Status", "Architecture", "Installed-Time", NULL };
	int var_index;
	fprintf(__save_pkg_status_stream, "Package: %s\n", key);
	for(var_index=0; pkg_vars[var_index] != NULL; var_index++)
	{
		char* var_def = get_string_map_element(pkg_map, pkg_vars[var_index]);
		fprintf(__save_pkg_status_stream, "%s: %s\n", pkg_vars[var_index], (var_def == NULL ? "" : var_def));
	}
	fprintf(__save_pkg_status_stream, "\n");
}


void save_package_data_as_status_file(string_map* package_data, char* status_file_path)
{
	if(!path_exists(status_file_path))
	{
		/* make sure parent dir of path exists */
		mkdir_p(status_file_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH  );
		rm_r(status_file_path);
	}

	__save_pkg_status_stream = fopen(status_file_path, "w");
	if(__save_pkg_status_stream != NULL)
	{
		apply_to_every_string_map_value(package_data, save_pkg_status_func);
		fclose(__save_pkg_status_stream);
	}
	__save_pkg_status_stream = NULL;
}


void free_pkg_func(char* key, void* value)
{
	string_map* pkg_map = (string_map*)value;
	unsigned long num_destroyed;

	/* deal with dependency map, which is not simple string */
	string_map* dep_map = remove_string_map_element(pkg_map, "Required-Depends");
	destroy_string_map(dep_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);

	/* deal with version crap when it gets implemented */
	
	/* free the main package map, only strings left */
	destroy_string_map(pkg_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);

}

void free_package_data(string_map* package_data)
{
	unsigned long num_destroyed;
	apply_to_every_string_map_value(package_data, free_pkg_func);
	
	/* destroy top level map, values already freed so call with DESTROY_MODE_IGNORE_VALUES */
	destroy_string_map(package_data, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);

}

void free_recursive_pkg_vars_func(char* key, void* value)
{
	string_map* pkg_map = (string_map*)value;
	unsigned long num_destroyed;


	string_map* dep_map = remove_string_map_element(pkg_map, "Required-Depends");
	char* required_size = remove_string_map_element(pkg_map, "Required-Size");
	char* will_fit      = remove_string_map_element(pkg_map, "Will-Fit");
	
	if(dep_map != NULL)       { destroy_string_map(dep_map, DESTROY_MODE_FREE_VALUES, &num_destroyed); }
	if(required_size != NULL) { free(required_size); }
	if(will_fit != NULL)      { free(will_fit); }
	
}
void free_recursive_pkg_vars(string_map* package_data)
{
	apply_to_every_string_map_value(package_data, free_recursive_pkg_vars_func);
}


