
#include "gpkg.h"

int include_variable(char* var, int package_matches, int include_package, int load_variable_def, string_map* load_variable_map);
int sort_version_cmp_internal(const void *a, const void *b);


static FILE* __save_pkg_status_stream = NULL;
void save_pkg_status_func(char* key, void* value);

void free_dep_map_func(char* key, void* value);
void free_pkg_func(char* key, void* value);
void free_recursive_pkg_vars_func(char* key, void* value);

static char*       __found_package_name = NULL;
static char*       __package_version_to_test_depends_on = NULL;
static int         __found_package_that_depends_on = 0;
static char*       __package_name_to_test_depends_on = NULL;
static string_map* __package_data_to_test_depends_on = NULL;
static char*       __but_not_same_package_with_version = NULL;
void something_depends_on_func(char* key, void* value);


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


list* get_directory_list(char* directory_path, int relative_path, unsigned char include_regular_files, unsigned char include_dirs, unsigned char include_links, unsigned char include_other)
{
	list* file_list = NULL;
	DIR* dir = opendir(directory_path);
	struct dirent *entry;
	if(dir != NULL)
	{
		file_list = initialize_list();
		while(entry = readdir(dir))
		{
			int include_entry = 0;
			include_entry = entry->d_type == DT_REG && include_regular_files                                                 ? 1 : include_entry;
			include_entry = entry->d_type == DT_DIR && include_dirs                                                          ? 1 : include_entry;
			include_entry = entry->d_type == DT_LNK && include_links                                                         ? 1 : include_entry;
			include_entry = (entry->d_type != DT_REG && entry->d_type != DT_DIR && entry->d_type != DT_LNK) && include_other ? 1 : include_entry;
			if(include_entry)
			{
				char* file_path = relative_path ? strdup(entry->d_name) : dynamic_strcat(3, directory_path, "/", entry->d_name);
				push_list(file_list, file_path);
			}
		}
		closedir(dir);
	}
	return file_list;
}

void load_all_package_data(opkg_conf* conf, string_map* package_data, string_map* matching_packages, string_map* parameters, int load_variable_def, char* install_root, int ignore_recursive_variables, string_map* preferred_provides)
{
	string_map* package_variables = parameters == NULL ? NULL : get_string_map_element(parameters, "package-variables");

	// load list data
	// this tells us everything about packages except whether they are currently installed
	load_package_data(conf->lists_dir, 1, package_data, matching_packages, parameters, load_variable_def, NULL, preferred_provides);

	//load control / status data
	unsigned long num_dests;
	int dest_index =0;
	char** dest_paths = (char**)get_string_map_keys(conf->dest_roots, &num_dests);
	for(dest_index=0; dest_index < num_dests; dest_index++)
	{
		char* info_dir_path = dynamic_strcat(2, dest_paths[dest_index], "/usr/lib/opkg/info");
		char* adjusted_info_dir_path = dynamic_replace(info_dir_path, "//", "/");

		char* status_path = dynamic_strcat(2, dest_paths[dest_index], "/usr/lib/opkg/status");
		char* adjusted_status_path = dynamic_replace(status_path, "//", "/");

		free(info_dir_path);
		free(status_path);
		info_dir_path = adjusted_info_dir_path;
		status_path   = adjusted_status_path;


		
		//load control data
		list* info_file_list = get_directory_list(info_dir_path, 1, 1, 0, 0, 0);
		if(info_file_list != NULL)
		{
			while(info_file_list->length > 0)
			{
				char* info_file_name = shift_list(info_file_list);
				if(strstr(info_file_name, ".control") != NULL)
				{
					char* control_path = dynamic_strcat(3, info_dir_path, "/", info_file_name);
					load_package_data(control_path, 0, package_data, matching_packages, parameters, load_variable_def, NULL, preferred_provides);
					free(control_path);
				}
				free(info_file_name);
			}
		}

		//load status data
		if(path_exists(status_path))
		{
			load_package_data(status_path, 0, package_data, matching_packages, parameters, load_variable_def, get_string_map_element(conf->dest_roots, dest_paths[dest_index]), preferred_provides);
		}
		free(info_dir_path);
		free(status_path);
	}
	free_null_terminated_string_array(dest_paths);	
	


	if(!ignore_recursive_variables)
	{
		//calculate total depends, total size, and will-fit if requested
		unsigned long num_pkgs_to_recurse_on;
		char** pkgs_to_recurse_on = get_string_map_keys( (load_variable_def == LOAD_ALL_PKG_VARIABLES || load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL ? package_data : matching_packages), &num_pkgs_to_recurse_on);


		int load_depends  = load_variable_def == LOAD_ALL_PKG_VARIABLES || load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING ? 1 : 0;
		int load_size     = load_variable_def == LOAD_ALL_PKG_VARIABLES || load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING ? 1 : 0;
		int load_will_fit = (load_variable_def == LOAD_ALL_PKG_VARIABLES || load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING) && install_root != NULL ? 1 : 0;
		if(package_variables != NULL && load_variable_def != LOAD_ALL_PKG_VARIABLES )
		{
			load_depends  = get_string_map_element(package_variables, "Required-Depends") != NULL  || get_string_map_element(package_variables, "All-Depends") != NULL ? 1 : 0;
			load_size     = get_string_map_element(package_variables, "Required-Size")    != NULL ? 1 : 0;
			load_will_fit = get_string_map_element(package_variables, "Will-Fit")         != NULL ? 1 : 0;
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
			int recurse_index;
			for(recurse_index=0; pkgs_to_recurse_on[recurse_index] != NULL ; recurse_index++)
			{
				load_recursive_package_data_variables(package_data, pkgs_to_recurse_on[recurse_index], load_size, load_will_fit, free_bytes);
			}
		}

		free_null_terminated_string_array(pkgs_to_recurse_on);
	}

}

/* returns package with exact version or NULL */
string_map* get_package_with_version(string_map* all_package_data, char* package_name, char* package_version)
{
	string_map *ret = NULL;
	string_map *all_versions = get_string_map_element(all_package_data, package_name);
	if(all_versions != NULL)
	{
		ret = get_string_map_element(all_versions, package_version);
	}
	return ret;
}

/* if installed returns current, otherwise returns latest version, and if not found NULL */
/* if is_current is not NULL, *is_current will be set to 1 if returned package is current, otherwise 0 */
/* if matching_version is not NULL, *matching version will be set to matching version, otherwise NULL */
string_map* internal_get_package_current_or_latest(string_map* all_package_data, char* package_name, int prefer_latest_to_current, int* is_current, char** matching_version)
{
	string_map *ret = NULL;
	if(is_current != NULL)       { *is_current = 0;          }
	if(matching_version != NULL) { *matching_version = NULL; }

	string_map *all_versions = get_string_map_element(all_package_data, package_name);
	if(all_versions != NULL)
	{
		char* current = get_string_map_element(all_versions, CURRENT_VERSION_STRING);
		char* latest = get_string_map_element(all_versions, LATEST_VERSION_STRING);
		if(current != NULL && prefer_latest_to_current == 0)
		{
			if(is_current != NULL)       { *is_current = 1; }
			if(matching_version != NULL) { *matching_version = strdup(current); }
			ret = get_string_map_element(all_versions, current);
		}
		else if (latest != NULL )
		{
			if(is_current != NULL) { *is_current = safe_strcmp(current, latest) == 0 ? 1 : 0; } /* safe_strcmp returns -1 or 1 cleanly if one arg is NULL */
			if(matching_version != NULL) { *matching_version = strdup(latest); }
			ret = get_string_map_element(all_versions, latest);
		}
	}
	if(ret == NULL)
	{
		string_map *all_provides = get_string_map_element(all_package_data, PROVIDES_STRING);
		if(all_provides != NULL)
		{
			string_map* all_provides_for_name = get_string_map_element(all_provides, package_name);
			if(all_provides_for_name != NULL)
			{
				char* pkg_key = get_string_map_element(all_provides_for_name, CURRENT_PROVIDES_STRING);
				pkg_key = pkg_key == NULL ? get_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING) : pkg_key;
				if(pkg_key != NULL)
				{
					string_map* provides_pkg = get_string_map_element(all_provides_for_name, pkg_key);
					char* real_name = get_string_map_element(provides_pkg, PROVIDES_REAL_NAME_STRING);
					ret = internal_get_package_current_or_latest(all_package_data, real_name, prefer_latest_to_current, is_current, matching_version);
				}
			}
		}
	}
	


	return ret;
}
string_map* get_package_current_or_latest(string_map* all_package_data, char* package_name, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest(all_package_data, package_name, 0,  is_current, matching_version);
}
string_map* get_package_latest(string_map* all_package_data, char* package_name, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest(all_package_data, package_name, 1,  is_current, matching_version);
}




string_map* internal_get_package_current_or_latest_matching(string_map* all_package_data, char* package_name, int prefer_latest_to_current, char** matching, int* is_current, char** matching_version, int must_be_satisfiable)
{
	string_map *ret = NULL;
	if(is_current != NULL)       { *is_current = 0;          }
	if(matching_version != NULL) { *matching_version = NULL; }


	int all_versions_valid = 0;
	if(matching[0][0] == '*' || 
		( strcmp(matching[0], ">>") != 0 && strcmp(matching[0], ">")  != 0 &&  
		  strcmp(matching[0], "<<") != 0 && strcmp(matching[0], "<")  != 0 && 
		  strcmp(matching[0], "<=") != 0 && strcmp(matching[0], ">=") != 0 &&         
		  strcmp(matching[0], "=") != 0  && strcmp(matching[0], "==") != 0 )
		)
	{
		all_versions_valid = 1;
	}
	if(all_versions_valid && (!must_be_satisfiable))
	{
		ret = internal_get_package_current_or_latest(all_package_data, package_name, prefer_latest_to_current, is_current, matching_version);
	}
	else
	{
		string_map *all_versions = get_string_map_element(all_package_data, package_name);
		if(all_versions != NULL)
		{
			char* current_version = get_string_map_element(all_versions, CURRENT_VERSION_STRING);
			unsigned long num_versions;
			char** version_list = get_string_map_keys(all_versions, &num_versions);
			int version_index;
			char* found_version = NULL;
			for(version_index=0;version_index < num_versions;version_index++)
			{
				char* test_version = version_list[version_index];
				if(strcmp(test_version, CURRENT_VERSION_STRING) != 0 && strcmp(test_version, LATEST_VERSION_STRING) != 0)
				{
					int valid = all_versions_valid;
					int cmp = 0;
					if(valid == 0)
					{
						cmp = compare_versions(test_version, matching[1]);
						valid = (cmp == 0 && (strcmp(matching[0], "=") == 0  || strcmp(matching[0], "==") == 0 || strcmp(matching[0], "<=") == 0 || strcmp(matching[0], ">=") == 0)) ? 1 : valid;
						valid = (cmp <  0 && (strcmp(matching[0], "<") == 0  || strcmp(matching[0], "<<") == 0 || strcmp(matching[0], "<=") == 0)) ? 1 : valid;
						valid = (cmp >  0 && (strcmp(matching[0], ">") == 0  || strcmp(matching[0], ">>") == 0 || strcmp(matching[0], ">=") == 0)) ? 1 : valid;
					}

					if(valid && must_be_satisfiable)
					{
						string_map* pkg_version_info = get_string_map_element(all_versions, test_version);
						valid = valid && safe_strcmp((char*)get_string_map_element(pkg_version_info,"Depends-Satisfiable"), "Y") == 0;
					}
					if(found_version != NULL && valid)
					{
						if(safe_strcmp(test_version, current_version) == 0 && prefer_latest_to_current == 0)
						{
							found_version = test_version;
						}
						else
						{
							cmp = compare_versions(test_version, found_version);
							found_version = cmp > 0 && (prefer_latest_to_current || safe_strcmp(found_version, current_version) != 0) ? test_version : found_version;
						}
					}
					else if(found_version == NULL && valid)
					{
						found_version = test_version;
					}
				}
			}
			if(found_version != NULL)
			{
				if(is_current != NULL) { *is_current = safe_strcmp(current_version, found_version) == 0 ? 1 : 0; }
				if(matching_version != NULL) { *matching_version = strdup(found_version); }
				ret = get_string_map_element(all_versions, found_version);
			}
			free_null_terminated_string_array(version_list); // note, this whacks found_version
	
		}
	
		if(ret == NULL)
		{
			string_map *all_provides = get_string_map_element(all_package_data, PROVIDES_STRING);
			if(all_provides != NULL)
			{
				string_map* all_provides_for_name = get_string_map_element(all_provides, package_name);
				if(all_provides_for_name != NULL)
				{
					char* pkg_key = get_string_map_element(all_provides_for_name, CURRENT_PROVIDES_STRING);
					pkg_key = pkg_key == NULL ? get_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING) : pkg_key;
					if(pkg_key != NULL)
					{
						string_map* provides_pkg = get_string_map_element(all_provides_for_name, pkg_key);
						char* real_name = get_string_map_element(provides_pkg, PROVIDES_REAL_NAME_STRING);
						char* match_all_str = strdup("*");
						ret = internal_get_package_current_or_latest_matching(all_package_data, real_name, prefer_latest_to_current, &match_all_str, is_current, matching_version, must_be_satisfiable);
						free(match_all_str);
					}
				}
			}
		}
	}


	return ret;

}
string_map* get_package_current_or_latest_matching(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest_matching(all_package_data, package_name, 0, matching, is_current, matching_version, 0);
}
string_map* get_package_latest_matching(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest_matching(all_package_data, package_name, 1, matching, is_current, matching_version, 0);
}
string_map* get_package_current_or_latest_matching_and_satisfiable(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest_matching(all_package_data, package_name, 0, matching, is_current, matching_version, 1);
}
string_map* get_package_latest_matching_and_satisfiable(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version)
{
	return internal_get_package_current_or_latest_matching(all_package_data, package_name, 1, matching, is_current, matching_version, 1);
}



void add_package_data(string_map* all_package_data, string_map** package, char* package_name, char* package_version, string_map* preferred_provides)
{
	string_map* all_versions = get_string_map_element(all_package_data, package_name);
	string_map* existing = NULL;
	int set_all_versions = all_versions == NULL ? 1 : 0;

	if(all_versions != NULL)
	{
		existing = get_string_map_element(all_versions, package_version);
	}
	else
	{
		all_versions = initialize_string_map(1);
	}


	/* if status is set to not-installed remove destination, as that implies it's installed */
	char* new_status = get_string_map_element(*package, "Status");
	char* new_dest   = get_string_map_element(*package, "Install-Destination");
	if(new_status != NULL && new_dest != NULL)
	{
		if(strstr(new_status, "not-installed") != NULL)
		{
			free_if_not_null( set_string_map_element(*package, "Install-Destination", strdup(NOT_INSTALLED_STRING)) );
		}
	}


	if(existing != NULL)
	{
		unsigned long num_keys;
		char** new_keys = get_string_map_keys(*package, &num_keys);
		int ki;
		for(ki=0; ki < num_keys; ki++)
		{
			char* new_element = remove_string_map_element(*package, new_keys[ki]);
			int ignore = strcmp(new_keys[ki], "Install-Destination") == 0 && strcmp(new_element, NOT_INSTALLED_STRING) == 0 ? 1 : 0;
			char* to_free = ignore ? new_element : set_string_map_element(existing, new_keys[ki],  new_element);
			free_if_not_null(to_free);
		}
		destroy_string_map(*package, DESTROY_MODE_FREE_VALUES, &num_keys);
		free_null_terminated_string_array(new_keys);
		*package = existing;
	}
	else
	{
		set_string_map_element(all_versions, package_version, *package);
	}
	

	/** set latest & current **/
	char* latest = NULL;
	char* current = NULL;
	unsigned long num_versions;
	char** all_version_names = get_string_map_keys(all_versions, &num_versions);
	int vi;
	for(vi=0;vi<num_versions; vi++)
	{
		if( strcmp(all_version_names[vi], CURRENT_VERSION_STRING) != 0 && strcmp(all_version_names[vi], LATEST_VERSION_STRING) != 0)
		{
			latest = latest == NULL ? all_version_names[vi] : latest;
			latest = compare_versions(all_version_names[vi], latest) > 0 ? all_version_names[vi] : latest;
	
			string_map* vpkg = get_string_map_element(all_versions, all_version_names[vi]);
			char* install_root = get_string_map_element(vpkg, "Install-Destination");
			if(install_root != NULL)
			{
				if(strcmp(install_root, NOT_INSTALLED_STRING) != 0)
				{		
					current = all_version_names[vi];
					
				}
			}
		}
	}



	char* old = set_string_map_element(all_versions, LATEST_VERSION_STRING, strdup(latest));
	free_if_not_null(old);
	if(current != NULL)
	{
		old = set_string_map_element(all_versions, CURRENT_VERSION_STRING, strdup(current));
		free_if_not_null(old);
	}
	free_null_terminated_string_array(all_version_names);





	if(set_all_versions)
	{
		set_string_map_element(all_package_data, package_name, all_versions);
	}



	//Handle 'Provides' option -- this is a huge pain in the ass...
	string_map* all_provides = get_string_map_element(all_package_data, PROVIDES_STRING);
	if(all_provides == NULL)
	{
		all_provides = initialize_string_map(1);
		set_string_map_element(all_package_data, PROVIDES_STRING, all_provides);

	}
	char* provides_str = get_string_map_element(*package, "Provides");
	if(provides_str != NULL)
	{
		if(strlen(provides_str) > 0)
		{
			unsigned long num_provides;
			char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
			char** provides_list = split_on_separators(provides_str, package_separators, 9, -1, 0, &num_provides);
			int provides_index;
			char* provides_unique_key = dynamic_strcat(3, package_name, "@", package_version);
	
			for(provides_index=0; provides_index < num_provides; provides_index++)
			{
				char* provides_name = strdup(provides_list[provides_index]);
				char* provides_version = NULL;
				if( strchr(provides_name, '=') != NULL)
				{
					char* tmp = provides_name;
					char* eq = strchr(tmp, '=');
					eq[0] = '\0';
					eq++;
					provides_version = strdup(eq);
					provides_name = strdup(provides_name);
					free(tmp);
				}
				else if( provides_index+1 < num_provides && provides_list[provides_index+1][0] == '=')
				{
					provides_index++;
					provides_version = strdup( (provides_list[provides_index] + 1) );
				}
				else
				{
					provides_version = strdup(package_version);
				}
	
				
				string_map* all_provides_for_name = get_string_map_element(all_provides, provides_name);
				if(all_provides_for_name == NULL)
				{
					all_provides_for_name = initialize_string_map(1);
					set_string_map_element(all_provides, provides_name, all_provides_for_name);
				}
				
				string_map* provides_map = get_string_map_element(all_provides_for_name, provides_unique_key);
				if(provides_map ==  NULL)
				{
					provides_map = initialize_string_map(1);
					set_string_map_element(provides_map, PROVIDES_REAL_NAME_STRING, strdup(package_name));
					set_string_map_element(provides_map, PROVIDES_REAL_VERSION_STRING, strdup(package_version));
					set_string_map_element(provides_map, PROVIDES_VERSION_STRING, strdup(provides_version));
					set_string_map_element(all_provides_for_name, provides_unique_key, provides_map);
				}
			
				/*
				 * My initial thought was to set the preferred package to use 
				 * when something has a provides alias as a dependency, and none
				 * of the packages that provide that thing are currently installed
				 * as the package that is smallest.
				 *
				 * However, opkg seems to use whichever is last alphabetically, and package 
				 * naming tends to assume this by having -custom, -full, and -mini versions 
				 * of the package.  The -custom ends up being the smallest since unless 
				 * compiled with specified parameters there are NO options compiled in and
				 * it's basically useless.  The -mini is the one that makes the most sense with
				 * reasonable minimal options and is the one that most people will want.  
				 *
				 * So... I'm just going to set the last alphabetically as the preferred package, 
				 * for compatibility with opkg if nothing else.
				 *
				 */
				unsigned char in_preferred_provides = 0;
				if(preferred_provides != NULL)
				{
					char* preferred_key = get_string_map_element(preferred_provides, provides_name);
					if( preferred_key != NULL )
					{
						free_if_not_null( set_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING, strdup(preferred_key)) );
						in_preferred_provides = 1;
					}
				}
				if(!in_preferred_provides)
				{
					char* preferred_key = get_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING);
					unsigned char is_preferred = preferred_key == NULL ? 1 : 0;
					if(preferred_key != NULL)
					{
						is_preferred = strcmp(provides_unique_key,preferred_key) > 0 ? 1 : 0;
					}	
					if(is_preferred)
					{
						free_if_not_null( set_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING, strdup(provides_unique_key)) );
					}
				}		
				
				char* cur_key = get_string_map_element(all_provides_for_name, CURRENT_PROVIDES_STRING);
				if(cur_key == NULL && current != NULL)
				{
					set_string_map_element(all_provides_for_name, CURRENT_PROVIDES_STRING, strdup(provides_unique_key));
				}

				free(provides_name);
				free(provides_version);
	
			}
			free_null_terminated_string_array(provides_list);
			free(provides_unique_key);

		}
	}




}


/* returns 1 if v1 > v2, 0 if the same -1 if v2 > v1 */
int compare_versions(char* v1, char* v2)
{
	char version_separators[] = { '.', '-', '_', ' ', '\t', '(', ')', '{', '}', '[', ']', '+', '=', ';', '?', ',', '|', '/', '\\', '*', '&', '@', '#', '!', '$', '%', '~', '<', '>' };
	int ret = strcmp(v1,v2);
	if(ret != 0)
	{
		unsigned long v1_parts;
		unsigned long v2_parts;
		char** v1_split = split_on_separators(v1, version_separators, 29, -1, 0, &v1_parts);
		char** v2_split = split_on_separators(v2, version_separators, 29, -1, 0, &v2_parts);

		int mismatch_found = 0;
		int part_index;
		for(part_index=0; part_index < v1_parts && part_index < v2_parts && (!mismatch_found); part_index++)
		{
			unsigned long partnum1;
			unsigned long partnum2;
			int isnum1 = sscanf(v1_split[part_index], "%lu", &partnum1);
			int isnum2 = sscanf(v2_split[part_index], "%lu", &partnum2);


			if(isnum1 == 1 && isnum2 == 1)
			{
				ret = partnum1 >  partnum2 ? 1  : ret;
				ret = partnum1 == partnum2 ? 0  : ret;
				ret = partnum1 <  partnum2 ? -1 : ret;
			}
			else
			{
				ret = strcmp(v1_split[part_index], v2_split[part_index]);
			}
			mismatch_found = ret != 0 ? 1 : 0;
		}
		free_null_terminated_string_array(v1_split);
		free_null_terminated_string_array(v2_split);

		if(ret == 0 && v1_parts < v2_parts)
		{
			ret = -1;
		}
		if(ret == 0 && v2_parts > v1_parts)
		{
			ret = 1;
		}
	}
	return ret;
}



/*  comparison functions for qsort */ 
int sort_version_cmp_internal(const void *a, const void *b)
{ 
    const char **a_ptr = (const char **)a;
    const char **b_ptr = (const char **)b;
    return compare_versions((char*)*a_ptr, (char*)*b_ptr);
}

/* wrappers for qsort call */
void sort_versions(char** version_arr, unsigned long version_arr_len)
{
	qsort(version_arr, version_arr_len, sizeof(char*), sort_version_cmp_internal);
}


int include_variable(char* var, int package_matches, int include_package, int load_variable_def, string_map* load_variable_map)
{
	int ret = 0;
	ret = include_package && load_variable_def == LOAD_ALL_PKG_VARIABLES ? 1 : ret;
	ret = package_matches && load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING ? 1 : ret;
	if(ret == 0 && include_package)
	{
		char* var_load_type = get_string_map_element(load_variable_map, var);
		if(var_load_type != NULL)
		{
			ret = strcmp(var_load_type, "A") == 0 ? 1 : ret;
			ret = strcmp(var_load_type, "M") == 0 && package_matches ? 1 : ret;
		}
	}
	return ret;

}





void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, int load_variable_def, char* dest_name, string_map* preferred_provides)
{
	regex_t* match_regex           = parameters != NULL ? get_string_map_element(parameters, "package-regex") : NULL;
	string_map* matching_list      = parameters != NULL ? get_string_map_element(parameters, "package-list") : NULL;
	string_map* package_variables = parameters == NULL ? NULL : get_string_map_element(parameters, "package-variables");


	if(load_variable_def < LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING || load_variable_def >  LOAD_ALL_PKG_VARIABLES )
	{
		//if invalid load_variable_def set, load everything
		load_variable_def =  LOAD_ALL_PKG_VARIABLES;
	}
	
	if(package_variables == NULL && load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING || load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL)
	{
		return;
	}
	if(match_regex == NULL && matching_list == NULL && ( 
		load_variable_def == LOAD_MINIMAL_PKG_VARIABLES_FOR_MATCHING || 
		load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING || 
		load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING)
	  	)
	{
		return;
	}



	string_map* load_variable_map = initialize_string_map(0);
	int load_var_index=0;
	char* all_dummy = strdup("A");
	char* matching_dummy = strdup("M");


	int include_all_packages;
	include_all_packages = 	load_variable_def != LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING && 
				load_variable_def != LOAD_MINIMAL_PKG_VARIABLES_FOR_MATCHING &&
				load_variable_def != LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING
				? 1 : 0;
	
	if(	load_variable_def == LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL || 
		load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL || 
		load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_DESCRIPTIVE_FOR_MATCHING ||
		load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_PARAMETER_FOR_MATCHING ||
		load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING 
			)
	{
		set_string_map_element(load_variable_map, "Status",              all_dummy);
		set_string_map_element(load_variable_map, "Depends",             all_dummy);
		set_string_map_element(load_variable_map, "Provides",            all_dummy);
		set_string_map_element(load_variable_map, "Installed-Size",      all_dummy);
		set_string_map_element(load_variable_map, "Install-Destination", all_dummy);
		set_string_map_element(load_variable_map, "Link-Destination",    all_dummy);
	}
	if( load_variable_def == LOAD_MINIMAL_PKG_VARIABLES_FOR_MATCHING || load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING )
	{
		set_string_map_element(load_variable_map, "Status",              matching_dummy);
		set_string_map_element(load_variable_map, "Depends",             matching_dummy);
		set_string_map_element(load_variable_map, "Provides",            matching_dummy);
		set_string_map_element(load_variable_map, "Installed-Size",      matching_dummy);
		set_string_map_element(load_variable_map, "Install-Destination", matching_dummy);
		set_string_map_element(load_variable_map, "Link-Destination",    matching_dummy);
	}
	if(load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_DESCRIPTIVE_FOR_MATCHING || load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL || load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING)
	{
		set_string_map_element(load_variable_map, "Description", (load_variable_def == LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL ? all_dummy : matching_dummy));
	}
	if(load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING || load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL || load_variable_def == LOAD_MINIMAL_FOR_ALL_PKGS_PARAMETER_FOR_MATCHING )
	{
		unsigned long num_vars;
		char** vars = get_string_map_keys(package_variables, &num_vars);
		int var_index;
		for(var_index=0; var_index < num_vars; var_index++)
		{
			if(get_string_map_element(load_variable_map, vars[var_index]) == NULL)
			{
				set_string_map_element(load_variable_map, vars[var_index], (load_variable_def == LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL ? all_dummy : matching_dummy));
			}
		}
		free_null_terminated_string_array(vars);
	}



	list* file_list = NULL;
	char* pkg_src_id = NULL;
	if(source_is_dir)
	{
		file_list = get_directory_list(data_source, 0, 1, 0, 0, 0);
		file_list = file_list == NULL ? initialize_list() : file_list;
	}
	else
	{
		file_list = initialize_list();
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
		char* pkg_name= NULL;
		char* pkg_version=NULL;

		char next_line[16384];
		int read_data = 1;
		int next_package_matches = 0;
		int include_next_package = 0;
		char* last_variable = NULL;
		int loaded_at_least_one_variable = 0;
		while(read_data > 0)
		{
			char* tmp_last_variable = last_variable;
			char* f;
			last_variable = NULL;
			read_data = 0;
			next_line[0] = '\0';
			f = fgets(next_line, 16384, data_file);
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
						if(pkg_name != NULL && pkg_version != NULL && loaded_at_least_one_variable)
						{
							add_package_data(existing_package_data, &next_pkg_data, pkg_name, pkg_version, preferred_provides);
							next_pkg_data = NULL;
						}
						unsigned long num_destroyed;
						free_if_not_null(pkg_name);
						free_if_not_null(pkg_version);
						if(next_pkg_data != NULL) { destroy_string_map(next_pkg_data, DESTROY_MODE_FREE_VALUES, &num_destroyed); }
						next_pkg_data = NULL;
						loaded_at_least_one_variable = 0;

						pkg_name = strdup(val);
						next_pkg_data = initialize_string_map(1);

						next_package_matches = 0;
						if(match_regex != NULL)
						{
							next_package_matches = regexec(match_regex, val, 0, NULL, 0) == 0  ? 1 : 0;
						}
						else if(matching_list != NULL)
						{
							next_package_matches = get_string_map_element(matching_list, val) != NULL ?  1 : 0;
						}
						include_next_package = next_package_matches || include_all_packages ;

						if(include_next_package)
						{
							if(next_package_matches && matching_packages != NULL)
							{
								set_string_map_element(matching_packages, val, strdup("D"));
							}
							if(include_variable("Install-Destination", next_package_matches, include_next_package, load_variable_def, load_variable_map))
							{
								set_string_map_element(next_pkg_data, "Install-Destination", (dest_name == NULL ? strdup(NOT_INSTALLED_STRING) : strdup(dest_name)  ));
							}
							if(pkg_src_id != NULL && include_variable("Source-ID", next_package_matches, include_next_package, load_variable_def, load_variable_map))
							{
								set_string_map_element(next_pkg_data, "Source-ID", strdup(pkg_src_id));
							}
						}
					}
					else if(strcmp(key, "Version") == 0)
					{
						pkg_version = strdup(val);
					}
					else if(include_variable(key, next_package_matches, include_next_package, load_variable_def, load_variable_map))
					{
						loaded_at_least_one_variable = 1;
						void* old_val = set_string_map_element(next_pkg_data, key, strdup(val));
						free_if_not_null(old_val);
						last_variable = strdup(key);
						
						if(strcmp(key, "Status") == 0 && include_variable("User-Installed", next_package_matches, include_next_package, load_variable_def, load_variable_map))
						{
							char* is_user = strstr(val, " user ") != NULL ? strdup("true") : strdup("false");
							set_string_map_element(next_pkg_data, "User-Installed", is_user);
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
						free(old);
					}

				}
				read_data = 1;
			}
			if(tmp_last_variable != NULL)
			{
				free(tmp_last_variable);
			}

		}
		if(pkg_name != NULL && pkg_version != NULL && loaded_at_least_one_variable)
		{
			add_package_data(existing_package_data, &next_pkg_data, pkg_name, pkg_version, preferred_provides);
			next_pkg_data = NULL;
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

char** alloc_depend_def(char* def_version_str)
{
	char** dep_def = def_version_str == NULL ? (char**)malloc(2*sizeof(char*)) : (char**)malloc(3*sizeof(char*));
	dep_def[0] = NULL;
	dep_def[1] = NULL;
	if(def_version_str != NULL )
	{
		char* adj = strdup(def_version_str + 1);
		if(strchr(adj, ')') != NULL)
		{
			*(strchr(adj, ')')) = '\0';
			char* start = adj;
			char* end = adj;
			while(*end == '>' || *end == '<' || *end == '=') 
			{
				end++;
			}
			char old = *end;
			*end = '\0';
			if(strlen(start) > 0)
			{
				dep_def[0] = strdup(start);
				*end = old;
				while(*end == ' ' || *end == '\t')
				{
					end++;
				}
				start = end;
				while(*end != '\0' && *end != ' ' && *end != '\t')
				{
					end++;
				}
				*end = '\0';
				if(strlen(start) > 0)
				{
					dep_def[1] = strdup(start);
					dep_def[2] = NULL;
				}
				else
				{
					free(dep_def[0]);
					dep_def[0] = NULL;
				}
			}
		}
		free(adj);
	}
	if(dep_def[0] == NULL)
	{
		dep_def[0] = strdup("*");
	}
	return dep_def;
}




//returns 0 if already installed or package doesn't exist, 1 if we need to install it
int load_recursive_package_data_variables(string_map* package_data, char* package_name, int load_size, int load_will_fit, uint64_t free_bytes)
{
	char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
	int some_version_is_installed = 0;
	char* installed_version = NULL;
	string_map* package_info = get_package_current_or_latest(package_data, package_name, &some_version_is_installed, &installed_version);	
	int ret = some_version_is_installed ? 0 : 1;
	
	
	string_map* all_versions = get_string_map_element(package_data, package_name);
	if(all_versions != NULL)
	{
		unsigned long num_versions;
		char** all_version_list = get_string_map_keys(all_versions, &num_versions); 
		int version_index;

		for(version_index=0; version_index < num_versions; version_index++)
		{
			char* package_version = all_version_list[version_index];
			if(strcmp(package_version, LATEST_VERSION_STRING) != 0 && strcmp(package_version, CURRENT_VERSION_STRING) != 0)
			{
				int package_is_installed = some_version_is_installed && (strcmp(installed_version, package_version) == 0);
				string_map* package_info = get_string_map_element(all_versions, package_version);
				char* package_status     = get_string_map_element(package_info, "Status");
				string_map* req_dep_map  = get_string_map_element(package_info, "Required-Depends");
				string_map* all_dep_map  = get_string_map_element(package_info, "All-Depends");
				uint64_t* required_size  = get_string_map_element(package_info, "Required-Size");
				load_size = load_will_fit || load_size;
				
				
				if(req_dep_map == NULL) // indicates it hasn't already been loaded, test prevents infinite recursion
				{
					int depends_satisfiable = 1; 
					if(package_status != NULL)
					{
						if(some_version_is_installed && (!package_is_installed) && (strstr(package_status, " hold ") != NULL))
						{
							int strc = strcmp(installed_version, package_version);
							depends_satisfiable = 0;
						}
					}

					if(load_size)
					{
						required_size = (uint64_t*)malloc(sizeof(uint64_t));
						*required_size = 0;
					}

					
					


					req_dep_map = initialize_map(1);
					all_dep_map = initialize_map(1);


					char* deps = get_string_map_element(package_info, "Depends");
					char** dep_list = NULL;
					unsigned long num_deps = 0;
					if(deps != NULL)
					{
						dep_list = split_on_separators(deps, package_separators, 9, -1, 0, &num_deps);
					}

					if( (!package_is_installed) && load_size )
					{
						char* installed_size_str = get_string_map_element(package_info, "Installed-Size");
						if(installed_size_str != NULL)
						{
							sscanf(installed_size_str, SCANFU64, required_size);
						}
					}

					int dep_index;
					for(dep_index=0; dep_index < num_deps; dep_index++)
					{
						char* dep_name = dep_list[dep_index];
						char** dep_def = NULL;
						int dep_is_installed;
						if(get_string_map_element(package_data, dep_name) == NULL)
						{
							string_map* all_provides = get_string_map_element(package_data, PROVIDES_STRING);
							if(all_provides != NULL)
							{
								string_map* all_provides_for_name = get_string_map_element(all_provides, dep_name);
								if(all_provides_for_name != NULL)
								{
									char* pkg_key = get_string_map_element(all_provides_for_name, CURRENT_PROVIDES_STRING);
									pkg_key = pkg_key == NULL ? get_string_map_element(all_provides_for_name, PREFERRED_PROVIDES_STRING) : pkg_key;
									if(pkg_key != NULL)
									{
										string_map* provides_pkg = get_string_map_element(all_provides_for_name, pkg_key);
										char* real_def_name = get_string_map_element(provides_pkg, PROVIDES_REAL_NAME_STRING) ;
										dep_name = strdup(real_def_name);
									}
								}
							}
						}
						
						//recurse
						load_recursive_package_data_variables(package_data, dep_name, load_size, load_will_fit, free_bytes);
						if( dep_list[dep_index+1] != NULL )
						{
							if(dep_list[dep_index+1][0] == '(' )
							{
								int last_part_inc=1;
								for(last_part_inc=1;  dep_list[dep_index+last_part_inc] != NULL  && dep_list[dep_index+last_part_inc][ strlen(dep_list[dep_index+last_part_inc])-1] != ')'; last_part_inc++) ;
								if(dep_list[dep_index+last_part_inc] != NULL && dep_list[dep_index+last_part_inc][ strlen(dep_list[dep_index+last_part_inc])-1] == ')' )
								{
									char* def_str = strdup("");
									int last_part_inc_index;
									for(last_part_inc_index=1; last_part_inc_index <= last_part_inc; last_part_inc_index++)
									{
										char* tmp_def_str=dynamic_strcat(2, def_str, dep_list[dep_index+last_part_inc_index]);
										free(def_str);
										def_str = tmp_def_str;									
									}

									dep_def = alloc_depend_def(def_str);
									free(def_str);
									dep_index= dep_index+(last_part_inc);
								}
							}
						}
						dep_def = dep_def == NULL ? alloc_depend_def(NULL) : dep_def;
						
						string_map* dep_info = get_package_current_or_latest_matching_and_satisfiable(package_data, dep_name, dep_def, &dep_is_installed, NULL);
						if(dep_info == NULL)
						{
							dep_info = get_package_current_or_latest_matching(package_data, dep_name, dep_def, &dep_is_installed, NULL);
						}

						set_string_map_element(all_dep_map, dep_name, dep_def);
						if(!dep_is_installed)
						{
							set_string_map_element(req_dep_map, dep_name, copy_null_terminated_string_array(dep_def));
						}

						if(dep_info != NULL)
						{
							char* add_map_names[3]  = { "All-Depends", "Required-Depends", NULL };
							string_map* add_maps[3] = { all_dep_map, req_dep_map, NULL };
							add_map_names[1] = dep_is_installed ? NULL :  add_map_names[1] ;
							
							
								
							int add_map_index;
							for(add_map_index=0; add_map_names[add_map_index] != NULL; add_map_index++)
							{
								string_map* add_map = add_maps[add_map_index];
								string_map* dep_dep_map = get_string_map_element(dep_info, add_map_names[add_map_index]);
								

								if(dep_dep_map != NULL)
								{

									unsigned long num_dep_deps;
									unsigned long dep_dep_index;
									char** dep_dep_list = get_string_map_keys(dep_dep_map, &num_dep_deps);
									for(dep_dep_index=0; dep_dep_index < num_dep_deps; dep_dep_index++)
									{
										char** dep_dep_def = (char**)get_string_map_element(dep_dep_map, dep_dep_list[dep_dep_index]);
										char** old = set_string_map_element(add_map, dep_dep_list[dep_dep_index], copy_null_terminated_string_array(dep_dep_def));
										if(old != NULL){ free_null_terminated_string_array(old); }

									}
									free_null_terminated_string_array(dep_dep_list);
								}
							}


							if( (!dep_is_installed) && load_size)
							{
								uint64_t* dep_size = (uint64_t*)get_string_map_element(dep_info, "Required-Size");
								*required_size = (*required_size) + (dep_size == NULL ? 0 : *dep_size); // should never be null, but let's be careful
							}


							if(!dep_is_installed)
							{
								char* dep_is_satisfiable_str = get_string_map_element(dep_info, "Depends-Satisfiable");
								if(dep_is_satisfiable_str == NULL)
								{
									depends_satisfiable = 0;
								}
								else
								{
									depends_satisfiable = depends_satisfiable && (strcmp(dep_is_satisfiable_str,"Y") == 0);
								}
							}
						}
						else
						{
							depends_satisfiable = dep_is_installed ? 1 : 0;
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
					set_string_map_element(package_info, "Required-Depends", req_dep_map);
					set_string_map_element(package_info, "All-Depends",      all_dep_map);
					if(depends_satisfiable)
					{
						set_string_map_element(package_info, "Depends-Satisfiable", strdup("Y"));
					}
					else
					{
						set_string_map_element(package_info, "Depends-Satisfiable", strdup("N"));
					}
				
					if(dep_list != NULL)
					{
						free_null_terminated_string_array(dep_list);
					}
				}

			}

		}
		free_null_terminated_string_array(all_version_list);
	}
	free_if_not_null(installed_version);
	return ret;
}

void save_pkg_status_func(char* key, void* value)
{
	string_map* all_versions = (string_map*)value;
	char* current = get_string_map_element(all_versions, CURRENT_VERSION_STRING);
	string_map* pkg = current != NULL ? get_string_map_element(all_versions, current) : NULL;
	if(pkg != NULL)
	{
		char* pkg_vars[] = { "Depends", "Provides", "Status", "Architecture", "Installed-Time", "Link-Destination", NULL };
		int var_index;
		if(get_string_map_element(pkg, "Version") != NULL)
		{
			current = (char*)get_string_map_element(pkg, "Version");
		}
		fprintf(__save_pkg_status_stream, "Package: %s\n", key);
		fprintf(__save_pkg_status_stream, "Version: %s\n", current);
		for(var_index=0; pkg_vars[var_index] != NULL; var_index++)
		{
			char* var_def = (char*)get_string_map_element(pkg, pkg_vars[var_index]);
			fprintf(__save_pkg_status_stream, "%s: %s\n", pkg_vars[var_index], (var_def == NULL ? "" : var_def));
		}
		fprintf(__save_pkg_status_stream, "\n");
	}
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

void free_dep_map_func(char* key, void* value)
{
	free_null_terminated_string_array(value);
}

void free_package_data_pkg_func(char* key, void* value)
{
	string_map* pkg_map = (string_map*)value;
	unsigned long num_destroyed;

	/* deal with dependency maps, which are not simple strings */
	string_map* req_dep_map = remove_string_map_element(pkg_map, "Required-Depends");
	string_map* all_dep_map = remove_string_map_element(pkg_map, "All-Depends");
	if(req_dep_map != NULL)
	{
		apply_to_every_string_map_value(req_dep_map, free_dep_map_func);
		destroy_string_map(req_dep_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	}
	if(all_dep_map != NULL)
	{
		apply_to_every_string_map_value(all_dep_map, free_dep_map_func);
		destroy_string_map(all_dep_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	}

	/* deal with version crap when it gets implemented */
	
	/* free the main package map, only strings left */
	destroy_string_map(pkg_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
}

void free_all_package_versions(string_map* all_versions)
{
	unsigned long num_destroyed;
	char* latest  =  remove_string_map_element(all_versions, LATEST_VERSION_STRING);
	char* current =  remove_string_map_element(all_versions, CURRENT_VERSION_STRING);
	free_if_not_null(latest);
	free_if_not_null(current);

	apply_to_every_string_map_value(all_versions, free_package_data_pkg_func);
	destroy_string_map(all_versions, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
}

void free_package_data_version_func(char* key, void* value)
{
	free_all_package_versions((string_map*)value);
}

void free_package_data(string_map* package_data)
{
	unsigned long num_destroyed;
	apply_to_every_string_map_value(package_data, free_package_data_version_func);
	
	/* destroy top level map, values already freed so call with DESTROY_MODE_IGNORE_VALUES */
	destroy_string_map(package_data, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
}




void free_recursive_package_vars_pkg_func(char* key, void* value)
{
	if(strcmp(key, LATEST_VERSION_STRING) != 0 && strcmp(key, CURRENT_VERSION_STRING) != 0)
	{
		string_map* pkg_map = (string_map*)value;
		unsigned long num_destroyed;

		string_map* req_dep_map = remove_string_map_element(pkg_map, "Required-Depends");
		string_map* all_dep_map = remove_string_map_element(pkg_map, "All-Depends");
		if(req_dep_map != NULL)
		{
			apply_to_every_string_map_value(req_dep_map, free_dep_map_func);
			destroy_string_map(req_dep_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
		}
		if(all_dep_map != NULL)
		{
			apply_to_every_string_map_value(all_dep_map, free_dep_map_func);
			destroy_string_map(all_dep_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
		}


		char* required_size = remove_string_map_element(pkg_map, "Required-Size");
		char* will_fit      = remove_string_map_element(pkg_map, "Will-Fit");
		free_if_not_null(required_size);
		free_if_not_null(will_fit);	
	}
	
}
void free_recursive_package_vars_version_func(char* key, void* value)
{
	apply_to_every_string_map_value((string_map*)value, free_recursive_package_vars_pkg_func);
}
void free_recursive_package_vars(string_map* package_data)
{
	apply_to_every_string_map_value(package_data, free_recursive_package_vars_version_func);
}



void something_depends_on_func(char* key, void* value)
{
	if(__found_package_that_depends_on == 0)
	{
		string_map* all_versions = (string_map*)value;
		char* current = get_string_map_element(all_versions, CURRENT_VERSION_STRING);
		string_map* pkg = current != NULL ? get_string_map_element(all_versions, current) : NULL;
		char* depend_str = pkg != NULL ? (char*)get_string_map_element(pkg, "Depends") : NULL ;
		
		if(current != NULL && pkg != NULL && depend_str != NULL)
		{
			string_map* dep_map = get_string_map_element(pkg, "All-Depends");
			int need_to_free_dep_map = 0;
			if(dep_map == NULL)
			{
				need_to_free_dep_map = 1;
				dep_map = initialize_string_map(1);
				char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
				unsigned long num_deps;
				char** dep_list = split_on_separators(depend_str, package_separators, 9, -1, 0, &num_deps);
				int dep_index;
				for(dep_index=0; dep_index < num_deps; dep_index++)
				{
	
				
					char* dep_name = dep_list[dep_index];
					char** dep_def = NULL;
					int dep_is_installed;
					if( dep_list[dep_index+1] != NULL )
					{
						if(dep_list[dep_index+1][0] == '(' )
						{
							int last_part_inc=1;
							for(last_part_inc=1;  dep_list[dep_index+last_part_inc] != NULL  && dep_list[dep_index+last_part_inc][ strlen(dep_list[dep_index+last_part_inc])-1] != ')'; last_part_inc++) ;
							if(dep_list[dep_index+last_part_inc] != NULL && dep_list[dep_index+last_part_inc][ strlen(dep_list[dep_index+last_part_inc])-1] == ')' )
							{
								char* def_str = strdup("");
								int last_part_inc_index;
								for(last_part_inc_index=1; last_part_inc_index <= last_part_inc; last_part_inc_index++)
								{
									char* tmp_def_str=dynamic_strcat(2, def_str, dep_list[dep_index+last_part_inc_index]);
									free(def_str);
									def_str = tmp_def_str;									
								}
	
								dep_def = alloc_depend_def(def_str);
								free(def_str);
								dep_index= dep_index+(last_part_inc);
							}
						}
					}
					dep_def = dep_def == NULL ? alloc_depend_def(NULL) : dep_def;
					set_string_map_element(dep_map, dep_name, dep_def);
				}
			}



			char* install_root = get_string_map_element(pkg, "Install-Destination");
			if(dep_map != NULL && install_root != NULL && strcmp(install_root, NOT_INSTALLED_STRING) != 0) 
			{
				char** matching = get_string_map_element(dep_map, __package_name_to_test_depends_on);
				if(__but_not_same_package_with_version == NULL && matching != NULL)
				{
					__found_package_that_depends_on = 1;
				}
				else if(matching != NULL)
				{
					if( 	strcmp(matching[0], ">>") == 0 && strcmp(matching[0], ">")  == 0 &&  
		  				strcmp(matching[0], "<<") == 0 && strcmp(matching[0], "<")  == 0 && 
		  				strcmp(matching[0], "<=") == 0 && strcmp(matching[0], ">=") == 0 &&         
		  				strcmp(matching[0], "=") == 0  && strcmp(matching[0], "==") == 0 )
					{
						int cmp = compare_versions(__but_not_same_package_with_version, matching[1]);

						int valid = 0;
						valid = (cmp == 0 && (strcmp(matching[0], "=") == 0  || strcmp(matching[0], "==") == 0 || strcmp(matching[0], "<=") == 0 || strcmp(matching[0], ">=") == 0)) ? 1 : valid;
						valid = (cmp <  0 && (strcmp(matching[0], "<") == 0  || strcmp(matching[0], "<<") == 0 || strcmp(matching[0], "<=") == 0)) ? 1 : valid;
						valid = (cmp >  0 && (strcmp(matching[0], ">") == 0  || strcmp(matching[0], ">>") == 0 || strcmp(matching[0], ">=") == 0)) ? 1 : valid;
						if(valid == 0)
						{
							__found_package_that_depends_on = 1;
						}

					}
				}
				
				if(__found_package_that_depends_on)
				{
					//test for mutual dependency, return 0 in that case
					string_map* test_pkg_dep_map = get_string_map_element(__package_data_to_test_depends_on, "All-Depends");
					if(test_pkg_dep_map != NULL)
					{
						__found_package_that_depends_on = get_string_map_element(test_pkg_dep_map, key) != NULL ? 0 : __found_package_that_depends_on;
					}
				}
				

				if(__found_package_that_depends_on )
				{
					__found_package_name = key ; // don't dynamically allocate, we do strdup on match in calling function
					//printf("found that %s depends on %s, installed in %s\n", key, __package_name_to_test_depends_on, install_root);
				}
			}

			if(need_to_free_dep_map)
			{
				unsigned long num_destroyed;
				apply_to_every_string_map_value(dep_map, free_dep_map_func);
				destroy_string_map(dep_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
			}
		}
	}
}
int something_depends_on(string_map* package_data, char* package_name, char* but_not_same_package_with_version, char** pkg_that_depends_on_query)
{
	int ret;
	__package_name_to_test_depends_on = package_name;
	__package_data_to_test_depends_on = get_package_current_or_latest(package_data, package_name, NULL, NULL);
	__found_package_name = NULL;
	__but_not_same_package_with_version = but_not_same_package_with_version;
	ret = __found_package_that_depends_on = 0;

	if(__package_data_to_test_depends_on != NULL)
	{
		apply_to_every_string_map_value(package_data, something_depends_on_func);
		if(__found_package_name != NULL && pkg_that_depends_on_query != NULL)
		{
			*pkg_that_depends_on_query  = strdup(__found_package_name);
		}
	}
	ret = __found_package_that_depends_on;
	__package_name_to_test_depends_on = NULL;
	__package_data_to_test_depends_on = NULL;
	__found_package_that_depends_on = 0;
	__found_package_name = NULL;
	__but_not_same_package_with_version;
	return ret;
}


