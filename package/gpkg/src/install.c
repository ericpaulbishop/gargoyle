

#include "gpkg.h"

int create_dir_and_test_writable(char* dir)
{
	int success;
	FILE* test_file = NULL;
	char* test_path = dynamic_strcat(2, dir, "/tmp.gpkg.write.test.tmp");
	mkdir_p(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	test_file = fopen(test_path, "w");
	success = test_file == NULL ? 0 : 1;
	if(test_file != NULL)
	{ 
		fclose(test_file);
       		rm_r(test_path);
	}
	free(test_path);

	return success;
}


void cp(char* src_path, char* dst_path)
{
	if(path_exists(src_path) != PATH_DOES_NOT_EXIST && path_exists(src_path) != PATH_IS_DIRECTORY )
	{
		//make necessary parent directories
		rm_r(dst_path);
		mkdir_p(dst_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		rm_r(dst_path);

		FILE* src = fopen(src_path, "rb");
		FILE* dst= fopen(dst_path, "wb");
		if(src != NULL && dst != NULL)
		{
			unsigned char buf[1024];
			int num_read = 1;
			while(num_read > 0)
			{
				num_read = fread(buf, 1, 1024, src);
				if(num_read > 0)
				{
					fwrite(buf, 1, num_read, dst);
				}
			}
		}
		if(src != NULL){ fclose(src); }
		if(dst != NULL){ fclose(dst); }
	}
}



//void do_install(opkg_conf* conf, char* pkg_name, char* install_root_name, char* link_root_name, char** version_criteria)
void do_install(opkg_conf* conf, string_map* pkgs, char* install_root_name, char* link_root_name, int is_upgrade, int overwrite_config, int overwrite_other_package_files, int force_reinstall, char* tmp_root)
{
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	string_map* pkgs_from_file = initialize_string_map(1);
	unsigned long num_destroyed;


	char* install_root_path = (char*)get_string_map_element(conf->dest_names, install_root_name);
	char* overlay_path = NULL; // no special treatment of overlay, can be reenabled  by setting this variable here if we ever need it


	char* test_dir  = dynamic_strcat(2, (overlay_path != NULL ? overlay_path : install_root_path), "/usr/lib/opkg/info");
	if(!create_dir_and_test_writable(test_dir))
	{
		fprintf(stderr, "ERROR: Specified install destination is not writable, exiting\n");
		exit(1);
	}
	free(test_dir);

	



	if(install_root_path == NULL)
	{
		printf("ERROR: No destination %s found, cannot install\n\n", install_root_name);
		exit(1);
	}
	
	char* tmp_dir = (char*)malloc(1024);
	if(create_tmp_dir(tmp_root == NULL ? "/tmp" : tmp_root, &tmp_dir) != 0)
	{
		fprintf(stderr, "ERROR: Could not create tmp dir, exiting\n");
		exit(1);
	}


	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL, install_root_name, 1, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);

		
	/* determine list of all packiages we are about to install, including dependencies */
	string_map* install_pkgs_map = initialize_string_map(1);
	char** install_pkg_list = NULL;	
	unsigned long install_pkg_list_len = 0;
	char* unsatisfied_dep_err = NULL;



	
	/* new string map var with all pkgs to install = pkgs, keys = version */
	unsigned long num_pkg_names;
	char** pkg_names = get_string_map_keys(pkgs, &num_pkg_names);
	int pkg_name_index;
	
	
	
	
	/* 
	 * Load data for any packages being installed via ipk and
	 * determine if any packages we are about to install 
	 * provide anything, and if so set package we are installing to preferred
	 */
	string_map* preferred_provides = initialize_string_map(1);
	for(pkg_name_index=0;pkg_name_index < num_pkg_names; pkg_name_index++)
	{
		char* pkg_name = pkg_names[pkg_name_index];
		char** version_criteria = get_string_map_element(pkgs, pkg_name);
		char* install_pkg_version = NULL;
		int install_pkg_is_current;

		
		/* deal with case where we're installing from file */
		if(path_exists(pkg_name))
		{
			//installing from file
			char* pkg_file = pkg_name;


			//extract control files
			int err = 0;
			char* tmp_control        = dynamic_strcat(2, tmp_dir, "/tmp_ctrl");
			char* tmp_control_prefix = dynamic_strcat(2, tmp_control, "/tmp.");
			char* tmp_control_name   = dynamic_strcat(2, tmp_control_prefix, "control");

			mkdir_p(tmp_control, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
			deb_extract(	pkg_file,
					stderr,
					extract_control_tar_gz | extract_all_to_fs| extract_preserve_date | extract_unconditional,
					tmp_control_prefix, 
					NULL, 
					&err);
			if(err != 0)
			{
				fprintf(stderr, "ERROR: %s is not a valid package file, cannot install\n", pkg_file);
				rm_r(tmp_dir);
				exit(1);
			}
			string_map* tmp_control_pkg_data = initialize_string_map(1);
			matching_packages = initialize_string_map(1);
		       	load_package_data(tmp_control_name, 0, tmp_control_pkg_data, matching_packages, NULL, LOAD_ALL_PKG_VARIABLES, NULL, NULL);
			unsigned long num_ctrl_names;
			char** ctrl_name_list = get_string_map_keys(tmp_control_pkg_data, &num_ctrl_names);
			destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
		

			err = 1; //set back to 0 when data successfully loaded
			if(num_ctrl_names > 0)
			{
				int ctrl_name_index;
				for(ctrl_name_index=0; ctrl_name_list[ctrl_name_index] != NULL; ctrl_name_index++)
				{
					if( strcmp(ctrl_name_list[ctrl_name_index], PROVIDES_STRING) != 0)
					{
						pkg_name = strdup(ctrl_name_list[ctrl_name_index]);
					}
				}



				char* version = NULL;
				int is_current;
				char* match_all_versions = "*";
				string_map* pkg_info = get_package_current_or_latest_matching(tmp_control_pkg_data, pkg_name, &match_all_versions, &is_current, &version);
				if(pkg_info != NULL)
				{
					err = 0;
					set_string_map_element(pkg_info, "Install-File-Location", strdup(pkg_file));
					set_string_map_element(pkg_info, "Version", version); //we need to save this, since we are going to set a special version to make sure data doesn't get over-written later, also no need to free version now

					char* special_version = dynamic_strcat(2, version, "@@_FILE_INSTALL_VERSION_@@");
					char** new_version_criteria = malloc(3*sizeof(char*));
					new_version_criteria[0] = strdup("=");
					new_version_criteria[1] = special_version;
					new_version_criteria[2] = NULL;
					version_criteria = new_version_criteria;
					
					string_map* all_current_versions = get_string_map_element(package_data, pkg_name);
					if(all_current_versions == NULL)
					{
						all_current_versions=initialize_string_map(1);
						set_string_map_element(package_data, pkg_name, all_current_versions);
					}
					set_string_map_element(all_current_versions, special_version, pkg_info);
					set_string_map_element(all_current_versions, LATEST_VERSION_STRING, special_version);
				
					free(pkg_names[pkg_name_index]);
					pkg_names[pkg_name_index] = strdup(pkg_name);

				
					set_string_map_element(pkgs, pkg_name, copy_null_terminated_string_array(new_version_criteria));	
					set_string_map_element(pkgs_from_file, pkg_name, strdup("D"));
				}
			}
			free_null_terminated_string_array(ctrl_name_list);
			if(err != 0)
			{
				fprintf(stderr, "ERROR: %s is not a valid package file, cannot install\n", pkg_file);
				rm_r(tmp_dir);
				exit(1);
			}

			free_if_not_null(tmp_control);
			free_if_not_null(tmp_control_prefix);
			free_if_not_null(tmp_control_name);
			rm_r(tmp_control);
		}

		/* determine if package provides anything, and set this package to preferred if so*/
		string_map* install_pkg_data = get_package_current_or_latest_matching_and_satisfiable(package_data, pkg_name, version_criteria, &install_pkg_is_current, &install_pkg_version);
		if(install_pkg_data != NULL)
		{
			char* provides_str = get_string_map_element(install_pkg_data, "Provides");
			if(provides_str != NULL)
			{
				if(strlen(provides_str) > 0)
				{
					unsigned long num_provides;
					char package_separators[] = {' ', ',', ':', ';', '\'', '\"', '\t', '\r', '\n'};
					char** provides_list = split_on_separators(provides_str, package_separators, 9, -1, 0, &num_provides);
					int provides_index;
					char* provides_unique_key = dynamic_strcat(3, pkg_name, "@", install_pkg_version);
					for(provides_index=0; provides_index < num_provides; provides_index++)
					{
						char* provides_name = strdup(provides_list[provides_index]);
						char* eq = strchr(provides_name, '=');
						if(eq != NULL) { *eq = '\0' ; }
						if(strlen(provides_name) > 0)
						{
							set_string_map_element(preferred_provides, provides_name, strdup(provides_unique_key));
						}
					}
				}
			}
		}
	
	}


	/* reload with new preferred_provides */
	free_recursive_package_vars(package_data);
	matching_packages = initialize_string_map(1);
	load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL, install_root_name, 1, preferred_provides );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	
	
	/* load data and do sanity checks for packages we are about to install */
	for(pkg_name_index=0;pkg_name_index < num_pkg_names; pkg_name_index++)
	{
		char* pkg_name = pkg_names[pkg_name_index];
		char** version_criteria = get_string_map_element(pkgs, pkg_name);
		char* install_pkg_version = NULL;
		int install_pkg_is_current;

		

		load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 
		string_map* install_pkg_data = get_package_current_or_latest_matching_and_satisfiable(package_data, pkg_name, version_criteria, &install_pkg_is_current, &install_pkg_version);
		if(install_pkg_data == NULL)
		{
			int have_current;
			char* current_version = NULL;
			string_map* cur_info = get_package_current_or_latest(package_data, pkg_name, &have_current, &current_version);
			if(have_current)
			{
				//should only get here if version_criteria[1] is not null -- the only reason get_package_current_or_latest_matching_and_satisfiable will return null, while package is installed is if there is a version mismatch
				char* cur_status = get_string_map_element(cur_info, "Status");
				if(strstr(cur_status, " hold ") != NULL)
				{
					unsatisfied_dep_err = dynamic_strcat(9, "ERROR: Package ", pkg_name, " (", version_criteria[0], " ", version_criteria[1], ") is installed,\n\t\tand has incompatible version ", current_version, " that is marked as 'hold'");
				}
			}
			else
			{
				//issue is with one of the dependencies, load temporary data to find it and be more specific with error
				install_pkg_data = get_package_current_or_latest_matching(package_data, pkg_name, version_criteria, &install_pkg_is_current, &install_pkg_version);
			}
			free_if_not_null(current_version);

		}
		char* install_status = install_pkg_data == NULL ? NULL : get_string_map_element(install_pkg_data, "Status");

	
		if(install_status != NULL)
		{
			char** old_el = set_string_map_element(install_pkgs_map, pkg_name, copy_null_terminated_string_array(version_criteria) );
			if(old_el != NULL){ free_null_terminated_string_array(old_el); }

			string_map* install_pkg_depend_map = get_string_map_element(install_pkg_data, "Required-Depends");
			if(install_pkg_depend_map != NULL)
			{
				unsigned long num_keys;
				char** load_detail_pkgs = get_string_map_keys(install_pkg_depend_map, &num_keys);
				int ldp_index;
				for(ldp_index=0;ldp_index < num_keys && unsatisfied_dep_err == NULL; ldp_index++)
				{
					char* dep_name = load_detail_pkgs[ldp_index];
					char** dep_def= get_string_map_element(install_pkg_depend_map, dep_name);
					if(get_string_map_element(install_pkgs_map, dep_name) != NULL)
					{
						/* 
						 * We really should check here whether old dependency def can be reconciled with the new one, and report an error if it can't 
						 * Right now we just use the heuristic that top-level (user specified, not dependency) package defs get preference, followed
						 * by first dependency encountered.
						 *
						 * Since right now versioning features aren't really being used very much other than kernel dependencies in Gargoyle/OpenWrt
						 * I'm just leaving this comment here as a reminder that this should be addressed at some point rather than messing with it now
						 *
						 */

						dep_def = get_string_map_element(install_pkgs_map, dep_name);
					}
					else
					{
						set_string_map_element(install_pkgs_map, dep_name, copy_null_terminated_string_array(dep_def));
					}
	
					//error checking, check that dependency definition exists
					char* latest_version = NULL;
					int latest_is_current = 0;
					string_map* dep_info = get_package_current_or_latest_matching_and_satisfiable(package_data, dep_name, dep_def, &latest_is_current, &latest_version);


					
					//check if we have a version installed different than what is required
					int have_current;
					char* current_version = NULL;
					string_map* cur_info = get_package_current_or_latest(package_data, dep_name, &have_current, &current_version);
					if(have_current && dep_info == NULL)
					{
						//should only get here if dep_def[1] is not null (version mismatch doesn't make sense if no version is specified)
						char* cur_status = get_string_map_element(cur_info, "Status");
						if(strstr(cur_status, " hold ") != NULL)
						{
							unsatisfied_dep_err = dynamic_strcat(11, "ERROR: Dependency ", dep_name, " (", dep_def[0], " ", dep_def[1], ") of package ", pkg_name, " is installed,\n\t\tbut has incompatible version ", current_version, " and is marked as 'hold'");
						}
						else
						{
							unsatisfied_dep_err = dynamic_strcat(10, "ERROR: Dependency ", dep_name, " (", dep_def[0], " ", dep_def[1], ") of package ", pkg_name, " is installed,\n\t\tbut has incompatible version ", current_version);
						}
					}
					free_if_not_null(current_version);
					free_if_not_null(latest_version);
	
					// check that dependency definition exists
					if(unsatisfied_dep_err == NULL && dep_info == NULL)
					{
						if(dep_def[1] != NULL)
						{
							unsatisfied_dep_err = dynamic_strcat(9, "ERROR: Dependency ", dep_name, " (", dep_def[0], " ", dep_def[1], ") of package ", pkg_name, " cannot be found, try updating your package lists");
						}
						else
						{
							unsatisfied_dep_err = dynamic_strcat(5, "ERROR: Dependency ", dep_name, " of package ", pkg_name, " cannot be found, try updating your package lists");
						}
					}
									
				}
				free_null_terminated_string_array(load_detail_pkgs);
			}
	
		}
		install_status = install_pkg_data == NULL ? NULL : get_string_map_element(install_pkg_data, "Status");


		/* error checking before we start install */
		if(install_pkg_data == NULL || install_status == NULL)
		{
			if(unsatisfied_dep_err != NULL)
			{
				fprintf(stderr, "%s\n", unsatisfied_dep_err);
			}
			else
			{
				fprintf(stderr, "ERROR: No package named %s found, try updating your package lists\n\n", pkg_name);
			}
			rm_r(tmp_dir);
			exit(1);
		}
		if(strstr(install_status, " installed") != NULL)
		{
			if(force_reinstall)
			{
				fprintf(stderr, "WARNING: Package %s is already installed, forcing removal and reinstallation\n\n", pkg_name);
				free_package_data(package_data);
				string_map* rm_pkg = initialize_string_map(1);
				set_string_map_element(rm_pkg, pkg_name, alloc_depend_def(NULL));
				do_remove(conf, rm_pkg, (overwrite_config ? 0 : 1), 0, 1, 0, tmp_root);
				
				//restart install
				return do_install(conf, pkgs, install_root_name, link_root_name, is_upgrade, overwrite_config, overwrite_other_package_files, force_reinstall, tmp_root);
				
			}
			else
			{
				fprintf(stderr, "WARNING: Package %s is already installed, ignoring\n", pkg_name);
				fprintf(stderr, "         Use --force-reinstall to force reinstallation\n\n");
				
				char** old_el = remove_string_map_element(install_pkgs_map, pkg_name);
				if(old_el != NULL){ free_null_terminated_string_array(old_el); };
				
				old_el = remove_string_map_element(pkgs_from_file, pkg_name);
				if(old_el != NULL){ free(old_el); };


			}

		}

		if(unsatisfied_dep_err != NULL)
		{
			fprintf(stderr, "%s\n", unsatisfied_dep_err);
			rm_r(tmp_dir);
			exit(1);
		}
	}


	/* load more detailed data on packages we are about to install */
	free_recursive_package_vars(package_data); /* note: whacks install_pkg_depend_map */	
	string_map* parameters = initialize_string_map(1);
	matching_packages = initialize_string_map(1);
	set_string_map_element(parameters, "package-list", install_pkgs_map);
	
	load_all_package_data(conf, package_data, matching_packages, parameters, LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING, install_root_name, 0, preferred_provides);
	
	unsigned long from_file_pkg_list_len;
	char** from_file_pkg_list = get_string_map_keys(pkgs_from_file, &from_file_pkg_list_len);
	int from_file_index;
	for(from_file_index=0; from_file_index < from_file_pkg_list_len; from_file_index++)
	{
		char* old = set_string_map_element(matching_packages, from_file_pkg_list[from_file_index], strdup("D"));
		free_if_not_null(old);
	}
	free_null_terminated_string_array(from_file_pkg_list);
	install_pkg_list = get_string_map_keys(matching_packages, &install_pkg_list_len);


	
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	destroy_string_map(parameters, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	

	char* all_pkg_list_str = join_strs(", ", install_pkg_list, install_pkg_list_len, 0, 0); 
	uint64_t combined_size = 0;
	int pkg_index;
	for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
	{
		char** match_criteria = get_string_map_element(install_pkgs_map, install_pkg_list[pkg_index]);
		string_map* pkg = get_package_current_or_latest_matching_and_satisfiable(package_data, install_pkg_list[pkg_index], match_criteria, NULL, NULL);
		char* next_size_str = get_string_map_element(pkg, "Installed-Size");
		uint64_t next_size = 0;
		if(sscanf(next_size_str,  SCANFU64, &next_size) > 0)
		{
			combined_size = combined_size + next_size; 
		}
	}

	uint64_t root_size = destination_bytes_free(conf, install_root_name);
	if(combined_size >= root_size )
	{
		fprintf(stderr, "ERROR: Not enough space in destination %s to install specified packages:\n\t%s\n\n", install_root_name, all_pkg_list_str);
		rm_r(tmp_dir);
		exit(1);
	}


	if(all_pkg_list_str != NULL)
	{
		printf("Preparing to install the following packages, which will require " SCANFU64 " bytes:\n\t%s\n\n", combined_size, all_pkg_list_str);
	}
	else
	{
		fprintf(stderr, "No packages to install.\n\n");
	}



	/* Set status of new required packages to half-installed, set user-installed on requested package, installed time on all */
	char* install_root_status_path = dynamic_strcat(2, install_root_path, "/usr/lib/opkg/status");
	string_map* install_root_status = initialize_string_map(1);
	matching_packages = initialize_string_map(1);
	if(path_exists(install_root_status_path))
	{
		load_package_data(install_root_status_path, 0, install_root_status, matching_packages, NULL, LOAD_ALL_PKG_VARIABLES, install_root_name, preferred_provides);
	}
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);



	time_t now = time(NULL);
	char install_time[20];
	sprintf(install_time, "%lu", now);
	for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
	{
		int is_installed;
		char* install_version = NULL;
		char** match_criteria = get_string_map_element(install_pkgs_map, install_pkg_list[pkg_index]);
		string_map* pkg = get_package_current_or_latest_matching_and_satisfiable(package_data, install_pkg_list[pkg_index], match_criteria, &is_installed, &install_version);


		if(is_installed == 0) /* should never be true, but check anyway */
		{
			char* old_status = remove_string_map_element(pkg, "Status");
			free(old_status);
			char* status_parts[3] = { "install", "ok", "half-installed" };
			status_parts[1] = get_string_map_element(pkgs, install_pkg_list[pkg_index]) != NULL ? "user" : status_parts[1];
			char* new_status = dynamic_strcat(5, status_parts[0], " ", status_parts[1], " ", status_parts[2]);
			set_string_map_element(pkg, "Status", new_status);

			set_string_map_element(pkg, "Installed-Time", strdup(install_time));
			set_string_map_element(pkg, "Install-Destination", strdup(install_root_name));
			if(link_root_name != NULL)
			{
				set_string_map_element(pkg, "Link-Destination", strdup(link_root_name));
			}

			add_package_data(install_root_status, &pkg, install_pkg_list[pkg_index], install_version, NULL); 
			/* Note: we just added pkg data structure from package_data to install_root_status, Be careful on cleanup! */
		}
	}
	save_package_data_as_status_file(install_root_status, install_root_status_path);




	

	string_map* install_called_pkgs = initialize_string_map(1);

	int err = 0;
	for(pkg_name_index=0;pkg_name_index < num_pkg_names; pkg_name_index++)
	{
		char* pkg_name = pkg_names[pkg_name_index];
		if(get_string_map_element(install_pkgs_map, pkg_name) != NULL && get_string_map_element(install_called_pkgs, pkg_name) == NULL)
		{
			int install_pkg_is_current;
			char* install_pkg_version = NULL;
			char** version_criteria = get_string_map_element(pkgs, pkg_name);
			get_package_current_or_latest_matching_and_satisfiable(package_data, pkg_name, version_criteria, &install_pkg_is_current, &install_pkg_version);
			err = recursively_install(pkg_name, install_pkg_version, install_root_name, link_root_name, overlay_path, is_upgrade, overwrite_config, overwrite_other_package_files, tmp_dir, conf, package_data, install_called_pkgs);
		
			free_if_not_null(install_pkg_version);
		}
	}
	

	if(err)
	{
		fprintf(stderr, "An error occurred during Installation, removing partially installed packages.\n");
		unsigned long num_install_called_pkgs;
		char** install_called_pkg_list = get_string_map_keys(install_called_pkgs, &num_install_called_pkgs);
		int pkg_index;
		for(pkg_index=0; pkg_index < num_install_called_pkgs; pkg_index++)
		{
			remove_individual_package(install_called_pkg_list[pkg_index], conf, package_data, tmp_dir, 0, 0);
		}
		free_null_terminated_string_array(install_called_pkg_list);
		//call remove function to do cleanup of partial install
		//DO NOT EXIT HERE, fixup status file below
	}
	//remove tmp dir -- need to do this whether or not there is an error
	rm_r(tmp_dir);
	free(tmp_dir);


	//set status of new packages to installed on success, and remove on failure
	for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
	{	
		/* no need to check version, should only be one installed version at a time... */
		string_map* pkg = get_package_current_or_latest(install_root_status, install_pkg_list[pkg_index], NULL, NULL); 
		if(pkg != NULL)
		{
			if(!err)
			{
				char* status = get_string_map_element(pkg, "Status");
				if(strstr(status, " half-installed") != NULL)
				{
					char* status_parts[3] = { "install", "ok", "installed" };
					status_parts[1] = get_string_map_element(pkgs, install_pkg_list[pkg_index]) != NULL ? "user" : status_parts[1];
					char* new_status = dynamic_strcat(5, status_parts[0], " ", status_parts[1], " ", status_parts[2]);
					char* old_status = set_string_map_element(pkg, "Status", new_status);
					free_if_not_null(old_status);
				}
			}
			else
			{
				string_map* all_pkg_versions = remove_string_map_element(install_root_status, install_pkg_list[pkg_index]);
				free_all_package_versions(all_pkg_versions);
			}
		}
	}
	save_package_data_as_status_file(install_root_status, install_root_status_path);
	if(!err)
	{
		if(all_pkg_list_str != NULL)
		{
			printf("Installation of packages successful.\n\n");
		}
	}
	else
	{
		printf("Finished removing partially installed packages.\n\n");
	}
}


int recursively_install(char* pkg_name, char* pkg_version, char* install_root_name, char* link_to_root, char* overlay_path, int is_upgrade, int overwrite_config, int overwrite_other_package_files, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs)
{
	int err=0;
	
	/* variables not allocated in this function, do not need to be freed */
	string_map* install_pkg_data    = get_package_with_version(package_data, pkg_name, pkg_version);
	char* src_file_path             = get_string_map_element(install_pkg_data, "Install-File-Location");
	char* src_id                    = get_string_map_element(install_pkg_data, "Source-ID");
	char* pkg_filename              = get_string_map_element(install_pkg_data, "Filename");
	string_map* pkg_dependencies    = get_string_map_element(install_pkg_data, "Required-Depends");
	char* install_root_path         = get_string_map_element(conf->dest_names, install_root_name);
	char* link_root_path            = link_to_root != NULL && safe_strcmp(link_to_root, install_root_name) != 0 ? get_string_map_element(conf->dest_names, link_to_root) : NULL;
	char* base_url = NULL;
	
	/* variables that may need to be freed */
	char* pkg_dest = NULL;
	string_map* files_to_link = NULL;
	char* info_dir            = NULL;
	char* control_name_prefix = NULL;
	char* list_file_name      = NULL;
	string_map* conf_files    = NULL;
	string_map* copied_conf_files = NULL;

	int install_root_len = strlen(install_root_path);
	char* fs_terminated_install_root = install_root_path[install_root_len-1] == '/' ? strdup(install_root_path) : dynamic_strcat(2, install_root_path, "/");
	
	int overlay_root_len;
	char* fs_terminated_overlay_root = NULL;
	if(overlay_path != NULL)
	{
		overlay_root_len = strlen(overlay_path);
		fs_terminated_overlay_root = overlay_path[overlay_root_len-1] == '/' ? strdup(overlay_path) : dynamic_strcat(2, overlay_path, "/");
	}
	else
	{
		fs_terminated_overlay_root = strdup(fs_terminated_install_root);
	}

	int link_root_len;
	char* fs_terminated_link_root = NULL;
	if(link_root_path != NULL)
	{
		link_root_len = strlen(link_root_path);
		fs_terminated_link_root = link_root_path[link_root_len-1] == '/' ? strdup(link_root_path) : dynamic_strcat(2, link_root_path, "/");
	}
	set_string_map_element(install_called_pkgs, pkg_name, strdup("D"));



	if(pkg_dependencies != NULL)
	{
		//recurse
		unsigned long num_deps;
		char** deps = get_string_map_keys(pkg_dependencies, &num_deps);
		int dep_index;
		for(dep_index=0; err == 0 && dep_index < num_deps ; dep_index++)
		{
			if(get_string_map_element(install_called_pkgs, deps[dep_index]) == NULL )
			{
				char** dep_def = get_string_map_element(pkg_dependencies, deps[dep_index]);
				int is_current;
				char* matching_version;
				string_map* dep_pkg = get_package_current_or_latest_matching_and_satisfiable(package_data, deps[dep_index], dep_def, &is_current, &matching_version);

				if(dep_pkg != NULL)
				{
					char* dep_status = get_string_map_element(dep_pkg, "Status");
					if(strstr(dep_status, " half-installed") != NULL)
					{
						err = recursively_install(deps[dep_index], matching_version, install_root_name, link_to_root, overlay_path, is_upgrade, overwrite_config, overwrite_other_package_files, tmp_dir, conf, package_data, install_called_pkgs);
						
					}
				}
				else
				{
					err = 1;
				}
			}
		}
	}
	
	if(install_root_path == NULL || ( src_id == NULL && install_pkg_data == NULL) || (pkg_filename == NULL && install_pkg_data == NULL) )
	{
		//sanity check
		err = 1;
	}
	if(err == 0)
	{
		printf("Preparing to install package %s...\n", pkg_name);
	}
	if(err == 0 && src_file_path == NULL)
	{
		//determine source url
		string_map* src_lists[2] = { conf->gzip_sources, conf->plain_sources };
		int src_list_index;
		for(src_list_index=0; src_list_index < 2 && base_url == NULL; src_list_index++)
		{
			base_url = (char*)get_string_map_element(src_lists[src_list_index], src_id);
		}
		err = base_url == NULL ? 1 : err;
		if(err == 1)
		{
			fprintf(stderr, "ERROR: Could determine download  URL for package %s\n", pkg_name);
		}
	}
	if(err == 0 && src_file_path == NULL)
	{
		
		//download package
		printf("\tDownloading...\n");
		char* src_url  = dynamic_strcat(3, base_url, "/", pkg_filename);
		pkg_dest = dynamic_strcat(3, tmp_dir, "/", pkg_name);
		FILE* package_file = fopen(pkg_dest, "w");
		if(package_file != NULL)
		{
			err = write_url_to_stream(src_url, "gpkg", NULL, package_file, NULL);
			fclose(package_file);
		}
		else
		{
			err = 1;
		}
		if(err == 1)
		{
			fprintf(stderr, "ERROR: Could not download package %s\n", pkg_name);
		}
		free(src_url);
	}
	if(err == 0 && src_file_path != NULL)
	{
		pkg_dest = strdup(src_file_path);
	}
	if(err == 0 && src_file_path == NULL)
	{
		//check md5sum
		char* md5sum = file_md5sum_alloc(pkg_dest);
		char* expected_md5sum = (char*)get_string_map_element(install_pkg_data, "MD5Sum");
		
		//printf("md5sum         = %s\n", md5sum);
		//printf("package md5sum = %s\n", (char*)get_string_map_element(install_pkg_data, "MD5Sum"));

		if(md5sum == NULL || expected_md5sum == NULL)
		{
			fprintf(stderr, "ERROR: Expected MD5Sum for %s not specified, cannot verify package\n", pkg_name);
			err = 1;
		}
		else if (safe_strcmp(md5sum, expected_md5sum) != 0)
		{
			fprintf(stderr, "ERROR: MD5Sum mismatch for %s package\n", pkg_name);
			fprintf(stderr, "       Expected:   %s\n", expected_md5sum);
			fprintf(stderr, "       Downloaded: %s\n\n", md5sum);
			err = 1;
		}
		else
		{
			printf("\tDownloaded %s successfully.\n\tInstalling %s...\n", pkg_name, pkg_name);
		}
		
		if(md5sum != NULL) { free(md5sum); }
		
	}
	if(err == 0)
	{
		// Extract list file contaiing list of files to install
		info_dir            = dynamic_strcat(2, fs_terminated_overlay_root, "usr/lib/opkg/info");
		control_name_prefix = dynamic_strcat(4, info_dir, "/", pkg_name, ".");
		list_file_name      = dynamic_strcat(4, info_dir, "/", pkg_name, ".list");
		
		mkdir_p(info_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
		FILE* list_file = fopen(list_file_name, "w");
		deb_extract(	pkg_dest,
				list_file,
				extract_quiet | extract_data_tar_gz | extract_list,
				NULL,
				NULL, 
				&err);
		fclose(list_file);
		if(err)
		{
			rm_r(list_file_name);
			fprintf(stderr, "ERROR: could not extract file list from packge %s.\n", pkg_name);
			fprintf(stderr, "       package file may be corrupt\n\n");
		}
	}
	if(err == 0)
	{
		//extract control files
		deb_extract(	pkg_dest,
				stderr,
				extract_control_tar_gz | extract_all_to_fs| extract_preserve_date | extract_unconditional,
				control_name_prefix, 
				NULL, 
				&err);
		if(err)
		{
			fprintf(stderr, "ERROR: could not extract control files from packge %s.\n", pkg_name);
			fprintf(stderr, "       package file may be corrupt\n\n");
		}

	}
	if(err == 0)
	{
		//check for file conflicts & correct list file to contain real root name in file paths
		unsigned long num_list_lines;
		char** list_file_lines = get_file_lines(list_file_name, &num_list_lines);


		char* conf_file_path = dynamic_strcat(4, info_dir, "/", pkg_name, ".conffiles");
		if(path_exists(conf_file_path))
		{
			unsigned long num_conf_lines;
			char** conf_file_lines =  get_file_lines(conf_file_path, &num_conf_lines);
			int conf_line_index;
			conf_files = initialize_string_map(1);
			for(conf_line_index=0; conf_line_index < num_conf_lines; conf_line_index++)
			{
				char* adjusted_conf_path = dynamic_strcat(2, fs_terminated_install_root, conf_file_lines[conf_line_index] + 1);
				set_string_map_element(conf_files, adjusted_conf_path, strdup("D"));
				free(adjusted_conf_path);
			}
			free_null_terminated_string_array(conf_file_lines);
		}
		free(conf_file_path);
	

		FILE* list_file = fopen(list_file_name, "w");
		int line_index;
		for(line_index=0; line_index < num_list_lines && (!err) ; line_index++)
		{
			int line_len = strlen( list_file_lines[line_index] );
			if(line_len > 2)
			{
				if(list_file_lines[line_index][0] == '.' && list_file_lines[line_index][1] == '/' && list_file_lines[line_index][line_len-1] != '/')
				{
					char* adjusted_file_path = dynamic_strcat(2, fs_terminated_install_root, list_file_lines[line_index] + 2);
					int is_conf_file = conf_files != NULL ? 
								(get_string_map_element(conf_files, adjusted_file_path) != NULL ? 1 : 0) : 
								0;
					if(strcmp(pkg_name, "opkg") == 0 && strcmp(list_file_lines[line_index], "./bin/opkg") == 0 && path_exists("/bin/opkg") == PATH_IS_SYMLINK)
					{
						//very special case: we're installing opkg, and here all the preliminary checks have already been passed
						//remove symlink placeholder to gpkg from /bin/opkg if it exists
						rm_r("/bin/opkg");
					}
					err = path_exists(adjusted_file_path) && is_conf_file == 0 && overwrite_other_package_files == 0 ? 1 : 0;
					if(err)
					{
						fprintf(stderr, "ERROR: file '%s'\n", adjusted_file_path);
						fprintf(stderr, "       from package %s already exists.\n\n", pkg_name);
					}
					else
					{
						fprintf(list_file, "%s\n", adjusted_file_path);
						if(link_root_path != NULL)
						{
							char* link_to_path = dynamic_strcat(2, fs_terminated_link_root, list_file_lines[line_index] + 2);
							files_to_link = files_to_link == NULL ? initialize_string_map(1) : files_to_link;
							set_string_map_element(files_to_link, adjusted_file_path, link_to_path);
							//don't free link_to_path, should be freed with files_to_link map
						}

						if(is_conf_file && path_exists(adjusted_file_path) && overwrite_config == 0)
						{
							char* tmp_conf_path = dynamic_strcat(2, tmp_dir, adjusted_file_path);
							
							cp(adjusted_file_path, tmp_conf_path);
							
							copied_conf_files = copied_conf_files == NULL ? initialize_string_map(1) : copied_conf_files;
							set_string_map_element(copied_conf_files, adjusted_file_path, tmp_conf_path);
						
							//don't free tmp_conf_path, should be freed with copied_conf_files map 
						}
					}
					free(adjusted_file_path);
	
				}
			}
		}
		fclose(list_file);
		if(list_file_lines != NULL) { free_null_terminated_string_array(list_file_lines); }
	}
	if(err == 0)
	{
		//run preinst
		err = run_script_if_exists(install_root_path, link_root_path, pkg_name, "preinst", (is_upgrade ? "upgrade" : "install") );
	}
	if(err == 0)
	{
		//extract package files
		deb_extract(	pkg_dest,
				stderr,
				extract_data_tar_gz | extract_all_to_fs| extract_preserve_date| extract_unconditional,
				fs_terminated_overlay_root, 
				NULL, 
				&err);
		if(err)
		{
			fprintf(stderr, "ERROR: could not extract application files from packge %s.\n", pkg_name);
			fprintf(stderr, "       package file may be corrupt\n\n");
		}

		//move any conf files back
		if(copied_conf_files != NULL)
		{
			unsigned long num_conf_paths;
			char** conf_paths = get_string_map_keys(copied_conf_files, &num_conf_paths);
			int conf_index;
			for(conf_index=0; conf_index < num_conf_paths; conf_index++)
			{
				char* tmp_conf_path = get_string_map_element(copied_conf_files, conf_paths[conf_index]);
				
				cp(tmp_conf_path, conf_paths[conf_index]);
			}
			destroy_string_map(copied_conf_files, DESTROY_MODE_FREE_VALUES, &num_conf_paths);
			if(conf_paths != NULL ) { free_null_terminated_string_array(conf_paths); }
			copied_conf_files = NULL;
		}
	}
	if(err == 0 && files_to_link != NULL)
	{
		unsigned long num_files;
		char** real_files = get_string_map_keys(files_to_link, &num_files);
		int file_index;
		if(num_files > 0)
		{
			char* link_file_name = dynamic_strcat(4, info_dir, "/", pkg_name, ".linked");
			FILE* link_file = fopen(link_file_name, "w");
			for(file_index=0; link_file != NULL && file_index < num_files; file_index++)
			{
				char* link_path = get_string_map_element(files_to_link, real_files[file_index]);
				if(!path_exists(link_path))
				{
					int sym_success;
					mkdir_p(link_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
					rm_r(link_path);
					sym_success = symlink(real_files[file_index], link_path);
					fprintf(link_file, "%s\n", link_path);
				}
			}
			if(link_file != NULL) { fclose(link_file); }
			free(link_file_name);
		}
		destroy_string_map(files_to_link, DESTROY_MODE_FREE_VALUES, &num_files);
		free_null_terminated_string_array(real_files);
		files_to_link = NULL;
	}
	if(err == 0)
	{
		//run postinst
		int warn = run_script_if_exists(install_root_path, link_root_path, pkg_name, "postinst", (is_upgrade ? "upgrade" : "install") );
		if(warn != 0)
		{
			fprintf(stderr, "Warning: postinstall script failed for package %s.\n", pkg_name);
		}
	}
	if(err == 0)
	{
		// remove downloaded package file in tmp dir & print success
		if(src_file_path == NULL) { rm_r(pkg_dest); }
		printf("\tSuccessfully installed %s.\n", pkg_name);
	}


	//cleanup
	unsigned long num_destroyed;
	free_if_not_null(pkg_dest);
	free_if_not_null(info_dir);
	free_if_not_null(control_name_prefix);
	free_if_not_null(list_file_name);
	free_if_not_null(fs_terminated_install_root);
	free_if_not_null(fs_terminated_overlay_root);
	free_if_not_null(fs_terminated_link_root);
	if(files_to_link != NULL)   { destroy_string_map(files_to_link,     DESTROY_MODE_FREE_VALUES, &num_destroyed); }
	if(conf_files != NULL)      { destroy_string_map(conf_files,        DESTROY_MODE_FREE_VALUES, &num_destroyed); }
	if(copied_conf_files!= NULL){ destroy_string_map(copied_conf_files, DESTROY_MODE_FREE_VALUES, &num_destroyed); }

	return err;
}



