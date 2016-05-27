

#include "gpkg.h"



void do_remove(opkg_conf* conf, string_map* pkgs, int save_conf_files, int remove_orphaned_depends, int force, int warn_if_forced, char* tmp_root)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	string_map* pkgs_to_maybe_remove  = initialize_string_map(1);
	string_map* pkg_status_paths      = initialize_string_map(1);
	string_map* path_to_status_data   = initialize_string_map(1);
	string_map* path_to_status_name   = initialize_string_map(1);
	string_map* path_to_root_name     = initialize_string_map(1);
	unsigned long num_destroyed;
	
	load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL, NULL, 1, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);

	
	
	unsigned long rm_pkg_list_length;
	char** rm_pkg_list = get_string_map_keys(pkgs, &rm_pkg_list_length);
	int rm_pkg_index;
	string_map* uninstalled_pkgs_to_ignore = initialize_string_map(1);
	for(rm_pkg_index=0;rm_pkg_index<rm_pkg_list_length;rm_pkg_index++)
	{
		char* pkg_name = rm_pkg_list[rm_pkg_index];
		load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0);


		int rm_pkg_is_installed = 0;
		string_map* rm_pkg_data = get_package_current_or_latest(package_data, pkg_name, &rm_pkg_is_installed, NULL);




		char* rm_status = NULL;
		char* rm_root_name = NULL;
		char* rm_root_path = NULL;
		char* package_depending_on_name = NULL;
		int other_package_depends_on = 0;

		if(rm_pkg_data != NULL)
		{
			rm_status = get_string_map_element(rm_pkg_data, "Status");
			rm_root_name = get_string_map_element(rm_pkg_data, "Install-Destination");
			rm_root_path = rm_root_name != NULL ? get_string_map_element(conf->dest_names, rm_root_name) : NULL;
			other_package_depends_on = something_depends_on(package_data, pkg_name, NULL, &package_depending_on_name);
		}

		
		/* error checking before we start install */
		if(rm_pkg_data == NULL || rm_status == NULL || rm_root_path == NULL)
		{
			fprintf(stderr, "ERROR: No package named %s found, cannot uninstall\n\n", pkg_name);
			exit(1);
		}
		if(!rm_pkg_is_installed)
		{
			fprintf(stderr, "WARNING: Package %s not installed, cannot uninstall\n\n", pkg_name);
			set_string_map_element(uninstalled_pkgs_to_ignore, pkg_name, strdup("D"));
		}
		else if(!create_dir_and_test_writable(rm_root_path))
		{
			fprintf(stderr, "ERROR: Package %s is installed to destination '%s' which is not writable, cannot uninstall\n", pkg_name, rm_root_name);
			exit(1);
		}
		else
		{
			if(strstr(rm_status, " hold ") != NULL)
			{
				fprintf(stderr, "ERROR: Package %s marked as 'hold', cannot uninstall\n\n", pkg_name);
				exit(1);
			}
			if(other_package_depends_on && !force)
			{
				fprintf(stderr, "ERROR: Installed package %s depends on %s, can't uninstall\n\n", package_depending_on_name, pkg_name);
				exit(1);
			}
			else if(other_package_depends_on && force && warn_if_forced)
			{
				fprintf(stderr, "WARNING: Forced remove specified, uninstalling %s even though %s depends on it\n\n", pkg_name, package_depending_on_name);
			}
				
			/* load data and status paths for packages we may need to de-install (depending on what orphaned dependencies are) */
			char* rm_status_path = dynamic_strcat(2, rm_root_path, "/usr/lib/opkg/status");
			set_string_map_element(pkg_status_paths, pkg_name, rm_status_path);
			set_string_map_element(path_to_status_data, rm_status_path, initialize_string_map(1));
			set_string_map_element(path_to_status_name, rm_status_path, strdup(rm_root_name));
	
	
			string_map* rm_deps = get_string_map_element(rm_pkg_data, "All-Depends");
			unsigned long num_rm_deps;
			char** rm_dep_list = get_string_map_keys(rm_deps, &num_rm_deps);
			int rm_dep_index;
			for(rm_dep_index=0; rm_dep_index < num_rm_deps; rm_dep_index++)
			{
				string_map* dep_data = get_package_current_or_latest(package_data, rm_dep_list[rm_dep_index], NULL, NULL);
		
				char* dep_status = get_string_map_element(dep_data, "Status");
				char* dep_root_name = get_string_map_element(dep_data, "Install-Destination");
				char* dep_root_path = get_string_map_element(conf->dest_names, dep_root_name);
				if(dep_status != NULL && dep_root_name != NULL)
				{
					if(strstr(dep_status, " ok ") != NULL)
					{
						if(remove_orphaned_depends != REMOVE_ORPHANED_DEPENDENCIES_IN_SAME_DEST || safe_strcmp(rm_root_name, dep_root_name) == 0 )
						{
							char* old_el = set_string_map_element(pkgs_to_maybe_remove, rm_dep_list[rm_dep_index], strdup("D"));
							free_if_not_null(old_el);
		
							char* status_path =  dynamic_strcat(2, dep_root_path, "/usr/lib/opkg/status");
							set_string_map_element(pkg_status_paths, rm_dep_list[rm_dep_index], status_path);
							if(get_string_map_element(path_to_status_data, status_path) == NULL)
							{
								set_string_map_element(path_to_status_data, status_path, initialize_string_map(1));
								set_string_map_element(path_to_status_name, status_path, strdup(dep_root_name));
							}
						}
					}
				}
			}
		}
	}

	/* load status path data for all relevant status files */
	unsigned long num_status_paths;
	char** status_paths = get_string_map_keys(path_to_status_data, &num_status_paths);
	int status_path_index;
	for(status_path_index=0; status_path_index < num_status_paths; status_path_index++)
	{
		if(path_exists(status_paths[status_path_index]))
		{
			matching_packages = initialize_string_map(1);
			string_map* status_data = get_string_map_element(path_to_status_data, status_paths[status_path_index]);
			char* status_name = get_string_map_element(path_to_status_name, status_paths[status_path_index]);

			load_package_data(status_paths[status_path_index], 0, status_data, matching_packages, NULL, LOAD_ALL_PKG_VARIABLES, status_name, NULL);
			destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);

		}
	}

	/* create tmp dir */
	char* tmp_dir = (char*)malloc(1024);
	if(create_tmp_dir(tmp_root == NULL ? "/tmp" : tmp_root, &tmp_dir) != 0)
	{
		fprintf(stderr, "ERROR: Could not create tmp dir, exiting\n");
		exit(1);
	}



	/* remove main packages
	 *
	 * Yes, we write status to disk before every round of removal because we want to mark half-installed packages in case
	 * program craps out for some unknown reason, even though this increases number of disk writes and can wear out flash 
	 */
	string_map* main_rm_status_paths= initialize_string_map(1);
	for(rm_pkg_index=0;rm_pkg_index<rm_pkg_list_length;rm_pkg_index++)
	{
		//set_string_map_element(pkg_status_paths, rm_dep_list[rm_dep_index], status_path);
		char* pkg_name = rm_pkg_list[rm_pkg_index];			
		if(get_string_map_element(uninstalled_pkgs_to_ignore, pkg_name) == NULL)
		{	
			char* rm_status_path = get_string_map_element(pkg_status_paths, pkg_name);
			string_map* rm_status_data = get_string_map_element(path_to_status_data, rm_status_path);
			string_map* rm_pkg_data = get_string_map_element(rm_status_data, pkg_name);

			char* old_status = set_string_map_element(rm_pkg_data, "Status", strdup("deinstall user half-installed"));
			free_if_not_null(old_status);
		
			char* old_path = set_string_map_element(main_rm_status_paths, rm_status_path, strdup("D"));
			free_if_not_null(old_path);
		}
	}


	unsigned long num_main_status_paths;
	char** main_rm_status_path_list = get_string_map_keys(main_rm_status_paths, &num_main_status_paths);
	int main_status_path_index;
	for(main_status_path_index=0;main_status_path_index < num_main_status_paths; main_status_path_index++)
	{
		char* rm_status_path = main_rm_status_path_list[main_status_path_index];
		string_map* rm_status_data = get_string_map_element(path_to_status_data, rm_status_path);
		save_package_data_as_status_file(rm_status_data, rm_status_path);
	}

	for(rm_pkg_index=0;rm_pkg_index<rm_pkg_list_length;rm_pkg_index++)
	{
		//set_string_map_element(pkg_status_paths, rm_dep_list[rm_dep_index], status_path);
		char* pkg_name = rm_pkg_list[rm_pkg_index];
		if(get_string_map_element(uninstalled_pkgs_to_ignore, pkg_name) == NULL)
		{
	
			char* rm_status_path = get_string_map_element(pkg_status_paths, pkg_name);
			string_map* rm_status_data = get_string_map_element(path_to_status_data, rm_status_path);
		
			remove_individual_package(pkg_name, conf, package_data, tmp_dir, save_conf_files, 0);
		
			string_map* already_removed = remove_string_map_element(rm_status_data, pkg_name);
			destroy_string_map(already_removed, DESTROY_MODE_FREE_VALUES, &num_destroyed); //fix this at some point, not freed properly
		}
	}
	for(main_status_path_index=0;main_status_path_index < num_main_status_paths; main_status_path_index++)
	{
		char* rm_status_path = main_rm_status_path_list[main_status_path_index];
		string_map* rm_status_data = get_string_map_element(path_to_status_data, rm_status_path);
		save_package_data_as_status_file(rm_status_data, rm_status_path);
	}



	/* remove orphaned dependencies if requested */
	unsigned long  orphaned_deps_found = remove_orphaned_depends == REMOVE_NO_ORPHANED_DEPENDENCIES ? 0 : 1;
	while(orphaned_deps_found > 0)
	{

		/* update package data to latest */
		matching_packages = initialize_string_map(1);
		free_package_data(package_data);
		package_data = initialize_string_map(1);
		
		load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL, NULL, 1, NULL );
		destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


		/* find list of orphaned dependencies */
		string_map* found_map = initialize_string_map(1);
		unsigned long num_to_test;
		char** test_list = get_string_map_keys(pkgs_to_maybe_remove, &num_to_test);
		int  test_index=0;
		for(test_index=0; test_index < num_to_test; test_index++)
		{
			if(get_string_map_element(package_data, test_list[test_index]) != NULL)
			{
				load_recursive_package_data_variables(package_data, test_list[test_index], 1, 0, 0); // load required-depends for packages of interest only
				int dep_is_installed;
				string_map* rm_dep_data = get_package_current_or_latest(package_data, test_list[test_index], &dep_is_installed, NULL);
				if(dep_is_installed)
				{
					if(!something_depends_on(package_data, test_list[test_index], NULL, NULL))
					{
						set_string_map_element(found_map, test_list[test_index], strdup("D"));
					}
				}
			}
		}
		free_null_terminated_string_array(test_list);
		orphaned_deps_found = found_map->num_elements;

		if(orphaned_deps_found > 0)
		{
			char** orphaned_depend_list = get_string_map_keys(found_map, &orphaned_deps_found);
			int orphaned_depend_index;
			string_map* changed_status_paths = initialize_string_map(1);

			//set statuses of packages we are removing in this round to half-installed
			for(orphaned_depend_index=0; orphaned_depend_index < orphaned_deps_found; orphaned_depend_index++)
			{
				char* status_path = get_string_map_element(pkg_status_paths, orphaned_depend_list[orphaned_depend_index]);
				string_map* status_data = get_string_map_element(path_to_status_data, status_path);
				string_map* dep_status_data = get_package_current_or_latest(status_data, orphaned_depend_list[orphaned_depend_index], NULL, NULL);
				char* old_status = set_string_map_element(dep_status_data, "Status",  strdup("deinstall ok half-installed"));
				free_if_not_null(old_status);
				if(get_string_map_element(changed_status_paths, status_path) == NULL)
				{
					set_string_map_element(changed_status_paths, status_path, strdup("D"));
				}
			}
			unsigned long num_changed_status_paths = 0;
			char** changed_status_path_list = get_string_map_keys(changed_status_paths, &num_changed_status_paths);
			int changed_status_index;
			destroy_string_map(changed_status_paths, DESTROY_MODE_FREE_VALUES, &num_changed_status_paths);

			// save each updated status file
			for(changed_status_index=0; changed_status_index < num_changed_status_paths; changed_status_index++)
			{
				char* status_path = changed_status_path_list[changed_status_index];
				string_map* status_data = get_string_map_element(path_to_status_data, status_path);
				save_package_data_as_status_file(status_data, status_path);
			}


			for(orphaned_depend_index=0; orphaned_depend_index < orphaned_deps_found; orphaned_depend_index++)
			{
				//remove the package
				remove_individual_package(orphaned_depend_list[orphaned_depend_index], conf, package_data, tmp_dir, save_conf_files, 1);
			}
			
			
			for(orphaned_depend_index=0; orphaned_depend_index < orphaned_deps_found; orphaned_depend_index++)
			{
				//remove from status data
				char* status_path = get_string_map_element(pkg_status_paths, orphaned_depend_list[orphaned_depend_index]);
				string_map* status_data = get_string_map_element(path_to_status_data, status_path);

				string_map* dep_already_removed = remove_string_map_element(status_data, orphaned_depend_list[orphaned_depend_index]);
				free_all_package_versions(dep_already_removed);


			}
			
			// save each updated status file
			for(changed_status_index=0; changed_status_index < num_changed_status_paths; changed_status_index++)
			{
				char* status_path = changed_status_path_list[changed_status_index];
				string_map* status_data = get_string_map_element(path_to_status_data, status_path);
				save_package_data_as_status_file(status_data, status_path);
			}
			free_null_terminated_string_array(changed_status_path_list);
			free_null_terminated_string_array(orphaned_depend_list);
		}
		destroy_string_map(found_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	}

	//cleanup
	free_package_data(package_data);
	rm_r(tmp_dir);


}

void remove_individual_package(char* pkg_name, opkg_conf* conf, string_map* package_data, char* tmp_dir, int save_conf_files, int is_orphaned_dependency)
{
	string_map* install_pkg_data    = get_package_current_or_latest(package_data, pkg_name, NULL, NULL);

	char* install_root_name         = get_string_map_element(install_pkg_data, "Install-Destination");
	char* install_root_path         = get_string_map_element(conf->dest_names, install_root_name);
	char* link_root_name            = get_string_map_element(install_pkg_data, "Link-Destination");
	char* link_root_path            = link_root_name != NULL ? get_string_map_element(conf->dest_names, install_root_name) : NULL;
	char* control_postfix_list[]    = { "control", "list",  "linked", "conffiles", "prerm", "postrm", "preinst", "postinst", NULL };

	char* info_dir                = dynamic_strcat(2, install_root_path, "/usr/lib/opkg/info");
	char* list_file_name          = dynamic_strcat(4, info_dir, "/", pkg_name, ".list");
	char* link_file_name          = dynamic_strcat(4, info_dir, "/", pkg_name, ".linked");
	char* conf_file_name          = dynamic_strcat(4, info_dir, "/", pkg_name, ".conffiles");
	string_map* copied_conf_files = initialize_string_map(1);
	
	int install_root_len = strlen(install_root_path);
	char* fs_terminated_install_root = install_root_path[install_root_len-1] == '/' ? strdup(install_root_path) : dynamic_strcat(2, install_root_path, "/");


	if(is_orphaned_dependency)
	{
		printf("Removing orphaned dependency %s...\n", pkg_name);
	}
	else
	{
		printf("Removing package %s...\n", pkg_name);
	}

	//run prerm
	run_script_if_exists(install_root_path, link_root_path, pkg_name, "prerm", "remove" );

	//copy conf files to tmp dir
	if(path_exists(conf_file_name) && save_conf_files)
	{
		unsigned long num_conf_lines;
		char** conf_file_lines =  get_file_lines(conf_file_name, &num_conf_lines);
		int conf_line_index;
		for(conf_line_index=0; conf_line_index < num_conf_lines; conf_line_index++)
		{
			char* adjusted_conf_path = dynamic_strcat(2, fs_terminated_install_root, conf_file_lines[conf_line_index] + 1);
			if(adjusted_conf_path)
			{
				char* tmp_conf_path = dynamic_strcat(2, tmp_dir, conf_file_lines[conf_line_index] );
				cp(adjusted_conf_path, tmp_conf_path);
				set_string_map_element(copied_conf_files, adjusted_conf_path, tmp_conf_path);
			}
			free(adjusted_conf_path);
		}
		free_null_terminated_string_array(conf_file_lines);
	}	
	
	
	//unlink if .linked file exists
	if(path_exists(link_file_name))
	{
		unsigned long num_list_lines;
		char** link_file_lines =  get_file_lines(link_file_name, &num_list_lines);
		int link_line_index;
		for(link_line_index=0; link_line_index < num_list_lines; link_line_index++)
		{
			if(path_exists(link_file_lines[link_line_index]) == PATH_IS_SYMLINK)
			{
				rm_r(link_file_lines[link_line_index]);
			}
		}
		free_null_terminated_string_array(link_file_lines);
	}
	
	//remove all (non-directory) files
	if(path_exists(list_file_name))
	{
		unsigned long num_list_lines;
		char** list_file_lines =  get_file_lines(list_file_name, &num_list_lines);
		int list_line_index;
		for(list_line_index=0; list_line_index < num_list_lines; list_line_index++)
		{
			int path_type = path_exists(list_file_lines[list_line_index]);
			if(path_type != PATH_DOES_NOT_EXIST && path_type != PATH_IS_DIRECTORY)
			{
				rm_r(list_file_lines[list_line_index]);
			}
		}
		free_null_terminated_string_array(list_file_lines);
	}

	//call postrm
	run_script_if_exists(install_root_path, link_root_path, pkg_name, "postrm", "remove" );

	
	//remove control files (.control, .list, .linked, .conffiles, .prerm, .postrm, .preinst, .postinst )
	int control_file_index;
	for(control_file_index=0; control_postfix_list[control_file_index] != NULL; control_file_index++)
	{
		char* control_file_name = dynamic_strcat(5, info_dir, "/", pkg_name, ".", control_postfix_list[control_file_index]);
		if(path_exists(control_file_name))
		{
			rm_r(control_file_name);
		}
		free(control_file_name);
	}


	//copy conf files back
	if(copied_conf_files->num_elements > 0)
	{
		unsigned long num_conf_paths;
		char** conf_paths = get_string_map_keys(copied_conf_files, &num_conf_paths);
		int conf_index;
		for(conf_index=0; conf_index < num_conf_paths; conf_index++)
		{
			char* tmp_conf_path = get_string_map_element(copied_conf_files, conf_paths[conf_index]);
			cp(tmp_conf_path, conf_paths[conf_index]);
		}
		if(conf_paths != NULL ) { free_null_terminated_string_array(conf_paths); }

	}
	
	//cleanup & return,  NOTE: Status file is NOT updated by this function
	unsigned long num_destroyed;
	free_if_not_null(info_dir);
	free_if_not_null(list_file_name);
	free_if_not_null(link_file_name);
	free_if_not_null(conf_file_name);
	destroy_string_map(copied_conf_files, DESTROY_MODE_FREE_VALUES, &num_destroyed); 

	printf("Finished removing %s.\n\n", pkg_name);

}



/* returns err
 * if script doesn't exist still returns err=0
 * err=1 only if error running script
 */
int run_script_if_exists(char* install_root_path, char* link_root_path, char* pkg_name, char* script_type_postfix, char* action_arg)
{
	int err = 0;
	char* script_path = dynamic_strcat(5, install_root_path, "/usr/lib/opkg/info/", pkg_name, ".", script_type_postfix);
	if(path_exists(script_path))
	{
		setenv("PKG_ROOT", install_root_path, 1);
		if(link_root_path == NULL)
		{
			unsetenv("PKG_LINK_ROOT");
		}
		else
		{
			setenv("PKG_LINK_ROOT", link_root_path, 1);
		}
		char* cmd = dynamic_strcat(5, script_path, " ", action_arg, " ", pkg_name);
		const char* argv[] = {"sh", "-c", cmd, NULL};
		err = xsystem(argv);
		free(cmd);
	}
	free(script_path);
	return err;
}



