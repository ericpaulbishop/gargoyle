#include "gpkg.h"

void update(opkg_conf* conf);

void do_remove(opkg_conf* conf, char* pkg_name, int save_conf_files, int remove_orphaned_depends);
void remove_individual_package(char* pkg_name, opkg_conf* conf, string_map* package_data, char* tmp_dir, int save_conf_files, int is_orphaned_dependency);

void do_install(opkg_conf* conf, char* pkg_name, char* install_root_name, char* link_root_name);
int recursively_install(char* pkg_name, char* install_root, char* link_to_root, int is_upgrade, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs);

int run_script_if_exists(char* install_root_path, char* install_link_path, char* pkg_name, char* script_type_postfix, char* action_arg);


int main(void)
{
	rm_r("/tmp/plugin_root");
	rm_r("/tmp/plugin_test");
	

	mkdir_p("/tmp/plugin_root", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	mkdir_p("/tmp/plugin_test", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	


	//install_to("tor_0.2.3.24-rc-2_x86.ipk", "tor", "/tmp/test1");

	
	opkg_conf *conf = load_conf(NULL);

	/*
	printf("a\n");
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;
	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, NULL );
	printf("b\n");
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	string_map* libc_info = get_string_map_element(package_data, "libc");
	printf("c\n");
	string_map* libc_deps = get_string_map_element(libc_info, "All-Depends");
	if(libc_deps != NULL)
	{
		printf("d\n");
		char** libc_dep_list = get_string_map_keys(libc_deps, &num_destroyed);
		printf("e\n");
	
		int i;
		for(i=0; libc_dep_list[i] != NULL; i++)
		{
			printf("libc dep = %s\n", libc_dep_list[i]);
		}
	}
	else
	{
		printf("libc deps = NULL\n");
		char* deps = get_string_map_element(libc_info, "Depends");
		printf("dep str = %s\n", deps);

	}
	*/



	
	
	
	
	//update(conf);

	do_install(conf, "irssi", "plugin_root", "plugin_test");

	do_remove(conf, "irssi", 0, 1);


	return(0);
}

void update(opkg_conf* conf)
{
	mkdir_p(conf->lists_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );

	int is_gzip[2] = { 1, 0 };
	string_map* src_lists[2] = { conf->gzip_sources, conf->plain_sources };
	int src_list_index;
	for(src_list_index=0; src_list_index < 2 ; src_list_index++)
	{
		unsigned long num_keys;
		char** src_list = get_string_map_keys(src_lists[src_list_index], &num_keys);
		int src_index;
		for(src_index=0; src_index < num_keys; src_index++)
		{
			char* src_id = src_list[src_index];
			char* src_url = dynamic_strcat(2, (char*)get_string_map_element(src_lists[src_list_index], src_id), (is_gzip[src_list_index] ? "/Packages.gz" : "Packages"));
			char* package_file_path = dynamic_strcat(3,  conf->lists_dir, "/", src_id);
			char* package_tmp_file_path = dynamic_strcat(2, package_file_path, ".download.gpkg.tmp");
			
			
			
			FILE* package_tmp_file = fopen(package_tmp_file_path, "w");
			int read_err = 1;
			
			if(package_tmp_file != NULL)
			{
				printf("Downloading package list for %s source...\n", src_id);
				read_err = write_url_to_stream(src_url, "gpkg", NULL, package_tmp_file, NULL);
				fclose(package_tmp_file);
			}
			
			if(!read_err)
			{
				rm_r(package_file_path);
				rename(package_tmp_file_path, package_file_path);
				printf("Package list for %s downloaded successfully.\n\n", src_id);
			}
			else
			{
				rm_r(package_tmp_file_path);
				printf("WARNING: Could not retrieve package list for %s.\n\n", src_id);
			}
			
			free(src_url);
			free(package_tmp_file_path);
			free(package_file_path);
		}
		free_null_terminated_string_array(src_list);
	}
}


void do_remove(opkg_conf* conf, char* pkg_name, int save_conf_files, int remove_orphaned_depends)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	string_map* pkgs_to_maybe_remove  = initialize_string_map(1);
	string_map* pkg_status_paths      = initialize_string_map(1);
	string_map* path_to_status_data   = initialize_string_map(1);
	string_map* path_to_root_name     = initialize_string_map(1);
	unsigned long num_destroyed;

	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 

	string_map* rm_pkg_data = get_package_current_or_latest(package_data, pkg_name, NULL, NULL);

	char* rm_status = get_string_map_element(rm_pkg_data, "Status");
	char* rm_root_name = get_string_map_element(rm_pkg_data, "Install-Destination");
	char* rm_root_path = rm_root_name != NULL ? get_string_map_element(conf->dest_names, rm_root_name) : NULL;


	/* error checking before we start install */
	if(rm_pkg_data == NULL || rm_status == NULL)
	{
		fprintf(stderr, "ERROR: No package named %s found, cannot uninstall\n\n", pkg_name);
		exit(1);
	}
	if(rm_status == NULL || rm_root_path == NULL ||  strstr(rm_status, " installed") == NULL )
	{
		fprintf(stderr, "ERROR: Package %s not installed, cannot uninstall\n\n", pkg_name);
		exit(1);
	}

	/* load data and status paths for packages we may need to de-install (depending on what orphaned dependencies are) */
	char* rm_status_path = dynamic_strcat(2, rm_root_path, "/usr/lib/opkg/status");
	set_string_map_element(pkg_status_paths, pkg_name, rm_status_path);
	set_string_map_element(path_to_status_data, rm_status_path, initialize_string_map(1));


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
				set_string_map_element(pkgs_to_maybe_remove, rm_dep_list[rm_dep_index], strdup("D"));
				char* status_path =  dynamic_strcat(2, dep_root_path, "/usr/lib/opkg/status");
				set_string_map_element(pkg_status_paths, rm_dep_list[rm_dep_index], status_path);
				if(get_string_map_element(path_to_status_data, status_path) == NULL)
				{
					set_string_map_element(path_to_status_data, status_path, initialize_string_map(1));
				}
			}
		}
	}

	/* load status path data for all relevant status files */
	unsigned long num_status_paths;
	char** status_paths = get_string_map_keys(path_to_status_data, &num_status_paths);
	int status_path_index;
	string_map* rm_status_data = NULL;
	for(status_path_index=0; status_path_index < num_status_paths; status_path_index++)
	{
		if(path_exists(status_paths[status_path_index]))
		{
			matching_packages = initialize_string_map(1);
			string_map* status_data = get_string_map_element(path_to_status_data, status_paths[status_path_index]);
			load_package_data(status_paths[status_path_index], 0, status_data, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, "dummy-dest-name");
			destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			if(strcmp(status_paths[status_path_index], rm_status_path) == 0)
			{
				rm_status_data = status_data;
				rm_pkg_data = get_string_map_element(status_data, pkg_name);
			}
		}
	}

	/* create tmp dir */
	char* tmp_dir = (char*)malloc(1024);
	if(create_tmp_dir("/tmp", &tmp_dir) != 0)
	{
		fprintf(stderr, "ERROR: Could not create tmp dir, exiting\n");
		exit(1);
	}



	/* remove main package 
	 *
	 * Yes, we write status to disk before every round of removal because we want to mark half-installed packages in case
	 * program craps out for some unknown reason, even though this increases number of disk writes and can wear out flash 
	 */
	char* old_status = set_string_map_element(rm_pkg_data, "Status", strdup("deinstall user half-installed"));
	free_if_not_null(old_status);
	save_package_data_as_status_file(rm_status_data, rm_status_path);
	
	remove_individual_package(pkg_name, conf, package_data, tmp_dir, save_conf_files, 0);
	
	string_map* already_removed = remove_string_map_element(rm_status_data, pkg_name);
	destroy_string_map(already_removed, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	save_package_data_as_status_file(rm_status_data, rm_status_path);


	/* remove orphaned dependencies if requested */
	unsigned long  orphaned_deps_found = remove_orphaned_depends;
	while(orphaned_deps_found > 0)
	{

		/* update package data to latest */
		matching_packages = initialize_string_map(1);
		free_package_data(package_data);
		package_data = initialize_string_map(1);
		
		//load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, NULL );
		load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, NULL );
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
				string_map* rm_dep_data = get_package_current_or_latest(package_data, test_list[test_index], NULL, NULL);
				char* install_root = get_string_map_element(rm_dep_data, "Install-Destination");
				if(install_root != NULL &&  strcmp(install_root, "not_installed") != 0 )
				{
					if(!something_depends_on(package_data, test_list[test_index]))
					{
						set_string_map_element(found_map, test_list[test_index], strdup("D"));
					}
				}
			}
		}
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
		unsigned long num_list_lines;
		char** conf_file_lines =  get_file_lines(conf_file_name, &num_list_lines);
		int conf_line_index;
		for(conf_line_index=0; conf_line_index < num_list_lines; conf_line_index++)
		{
			if(path_exists(conf_file_lines[conf_line_index]))
			{
				char* tmp_conf_path = dynamic_strcat(2, tmp_dir, conf_file_lines[conf_line_index] );
				mkdir_p(tmp_conf_path,  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
				rm_r(tmp_conf_path);
				rename(conf_file_lines[conf_line_index], tmp_conf_path);
				set_string_map_element(copied_conf_files, conf_file_lines[conf_line_index], tmp_conf_path);

			}
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
			rm_r(conf_paths[conf_index]);
			rename(tmp_conf_path, conf_paths[conf_index]);
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




void do_install(opkg_conf* conf, char* pkg_name, char* install_root_name, char* link_root_name)
{

	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;

	char* install_root_path = (char*)get_string_map_element(conf->dest_names, install_root_name);
	if(install_root_path == NULL)
	{
		printf("ERROR: No destination %s found, cannot install\n\n", install_root_name);
		exit(1);
	}
	

	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, install_root_name );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	
	load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 
	

	string_map* install_pkg_data = get_package_current_or_latest(package_data, pkg_name, NULL, NULL);

	char* install_status = get_string_map_element(install_pkg_data, "Status");
	char** install_pkg_list = NULL;	
	unsigned long install_pkg_list_len = 0;
	


	/* load detailed information for all packiages we are about to install */
	if(install_status != NULL)
	{
		string_map* load_detail_map = initialize_string_map(1);
		set_string_map_element(load_detail_map, pkg_name, strdup("D"));
		
		string_map* install_pkg_depend_map = get_string_map_element(install_pkg_data, "Required-Depends");
		if(install_pkg_depend_map != NULL)
		{
			unsigned long num_keys;
			char** load_detail_pkgs = get_string_map_keys(install_pkg_depend_map, &num_keys);
			int ldp_index;
			for(ldp_index=0;ldp_index < num_keys; ldp_index++)
			{
				set_string_map_element(load_detail_map, load_detail_pkgs[ldp_index], strdup("D"));
			}
		}
		free_recursive_package_vars(package_data); /* note: whacks install_pkg_depend_map */
		
	
		string_map* parameters = initialize_string_map(1);
		matching_packages = initialize_string_map(1);
		set_string_map_element(parameters, "packages", load_detail_map);
		load_all_package_data(conf, package_data, matching_packages, parameters, 0, LOAD_ALL_PKG_VARIABLES, install_root_name);
		install_pkg_list = get_string_map_keys(matching_packages, &install_pkg_list_len);
		
		destroy_string_map(load_detail_map,   DESTROY_MODE_FREE_VALUES, &num_destroyed);
		destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
		destroy_string_map(parameters, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);

	}
	install_status = get_string_map_element(install_pkg_data, "Status");
	char* will_fit = get_string_map_element(install_pkg_data, "Will-Fit");


	/* error checking before we start install */
	if(install_pkg_data == NULL || install_status == NULL)
	{
		fprintf(stderr, "ERROR: No package named %s found, try updating your package lists\n\n", pkg_name);
		exit(1);
	}
	if(install_status == NULL || strstr(install_status, " installed") != NULL)
	{
		fprintf(stderr, "ERROR: Package %s is already installed\n\n", pkg_name);
		exit(1);
	}
	if(will_fit == NULL || strcmp(will_fit, "true") != 0)
	{
		fprintf(stderr, "ERROR: Not enough space in destination %s to install package %s \n\n", install_root_name, pkg_name);
		exit(1);
	}



	/* Set status of new required packages to half-installed, set user-installed on requested package, installed time on all */
	char* install_root_status_path = dynamic_strcat(2, install_root_path, "/usr/lib/opkg/status");
	string_map* install_root_status = initialize_string_map(1);
	matching_packages = initialize_string_map(1);
	if(path_exists(install_root_status_path))
	{
		load_package_data(install_root_status_path, 0, install_root_status, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, install_root_name);
	}
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	int pkg_index;
	time_t now = time(NULL);
	char install_time[20];
	sprintf(install_time, "%lu", now);


	for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
	{
		int is_installed;
		char* install_version = NULL;
		string_map* pkg = get_package_current_or_latest(package_data, install_pkg_list[pkg_index], &is_installed, &install_version);

		if(is_installed == 0)
		{
			char* old_status = remove_string_map_element(pkg, "Status");
			free(old_status);
			char* status_parts[3] = { "install", "ok", "half-installed" };
			status_parts[1] = strcmp(pkg_name, install_pkg_list[pkg_index]) == 0 ? "user" : status_parts[1];
			char* new_status = dynamic_strcat(5, status_parts[0], " ", status_parts[1], " ", status_parts[2]);
			set_string_map_element(pkg, "Status", new_status);

			set_string_map_element(pkg, "Installed-Time", strdup(install_time));
			set_string_map_element(pkg, "Install-Destination", strdup(install_root_name));
			if(link_root_name != NULL)
			{
				set_string_map_element(pkg, "Link-Destination", strdup(link_root_name));
			}

			add_package_data(install_root_status, &pkg, install_pkg_list[pkg_index], install_version); 
			/* Note: we just added pkg data structure from package_data to install_root_status, Be careful on cleanup! */
		}
	}
	save_package_data_as_status_file(install_root_status, install_root_status_path);

	char* tmp_dir = (char*)malloc(1024);
	if(create_tmp_dir("/tmp", &tmp_dir) != 0)
	{
		fprintf(stderr, "ERROR: Could not create tmp dir, exiting\n");
		exit(1);
	}
	

	string_map* install_called_pkgs = initialize_string_map(1);
	int err = recursively_install(pkg_name, install_root_name, link_root_name, 0, tmp_dir, conf, package_data, install_called_pkgs);
	
	

	if(err)
	{
		fprintf(stderr, "An error occurred during Installation, removing partially installed packages.\n");
		unsigned long num_install_called_pkgs;
		char** install_called_pkg_list = get_string_map_keys(install_called_pkgs, &num_install_called_pkgs);
		int pkg_index;
		for(pkg_index=0; pkg_index < num_install_called_pkgs; pkg_index++)
		{
			remove_individual_package(pkg_name, conf, package_data, tmp_dir, 0, 0);
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
		string_map* pkg = get_package_current_or_latest(install_root_status, install_pkg_list[pkg_index], NULL, NULL);
		if(pkg != NULL)
		{
			if(!err)
			{
				char* status = get_string_map_element(pkg, "Status");
				if(strstr(status, " half-installed") != NULL)
				{
					char* status_parts[3] = { "install", "ok", "installed" };
					status_parts[1] = strcmp(pkg_name, install_pkg_list[pkg_index]) == 0 ? "user" : status_parts[1];
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
		printf("Installation of %s package successful.\n\n", pkg_name);
	}
	else
	{
		printf("Finished removing partially installed packages.\n\n", pkg_name);
	}

	
}


int recursively_install(char* pkg_name, char* install_root_name, char* link_to_root, int is_upgrade, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs)
{
	int err=0;
	
	/* variables not allocated in this function, do not need to be freed */
	string_map* install_pkg_data    = get_package_current_or_latest(package_data, pkg_name, NULL, NULL);
	char* src_id                    = get_string_map_element(install_pkg_data, "Source-ID");
	char* pkg_filename              = get_string_map_element(install_pkg_data, "Filename");
	string_map* pkg_dependencies    = get_string_map_element(install_pkg_data, "Required-Depends");
	char* install_root_path         = get_string_map_element(conf->dest_names, install_root_name);
	char* link_root_path            = link_to_root != NULL ? get_string_map_element(conf->dest_names, link_to_root) : NULL;
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
		for(dep_index=0; err == 0 && dep_index < num_deps && get_string_map_element(install_called_pkgs, deps[dep_index]) == NULL ; dep_index++)
		{
			string_map* dep_pkg = get_package_current_or_latest(package_data, deps[dep_index], NULL, NULL);

			if(dep_pkg != NULL)
			{
				char* dep_status = get_string_map_element(dep_pkg, "Status");
				if(strstr(dep_status, " half-installed") != NULL)
				{
					err = recursively_install(deps[dep_index], install_root_name, link_to_root, is_upgrade, tmp_dir, conf, package_data, install_called_pkgs);
					
				}
			}
			else
			{
				err = 1;
			}
		}
	}

	if(err == 0 && src_id == NULL || pkg_filename == NULL || install_root_path == NULL)
	{
		//sanity check
		err = 1;
	}
	if(err == 0)
	{
		printf("Preparing to install package %s...\n", pkg_name);
	}
	if(err == 0)
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

	if(err == 0)
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
	if(err == 0)
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
		info_dir            = dynamic_strcat(2, install_root_path, "/usr/lib/opkg/info");
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
			char** conf_file_lines =  get_file_lines(conf_file_path, &num_list_lines);
			int conf_line_index;
			conf_files = initialize_string_map(1);
			for(conf_line_index=0; conf_line_index < num_conf_lines; conf_line_index++)
			{
				set_string_map_element(conf_files, conf_file_lines[conf_line_index], strdup("D"));
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
					err = path_exists(adjusted_file_path) && is_conf_file == 0 ? 1 : 0;
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

						if(is_conf_file && path_exists(adjusted_file_path))
						{
							char* tmp_conf_path = dynamic_strcat(2, tmp_dir, adjusted_file_path);
							mkdir_p(tmp_conf_path,  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
							rm_r(tmp_conf_path);
							rename(adjusted_file_path, tmp_conf_path);
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
				fs_terminated_install_root, 
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
				rm_r(conf_paths[conf_index]);
				rename(tmp_conf_path, conf_paths[conf_index]);
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
					mkdir_p(link_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
					rm_r(link_path);
					symlink(real_files[file_index], link_path);
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
		err = run_script_if_exists(install_root_path, link_root_path, pkg_name, "postinst", (is_upgrade ? "upgrade" : "install") );
	}
	if(err == 0)
	{
		// remove downloaded package file in tmp dir & print success
		rm_r(pkg_dest);
		printf("\tSuccessfully installed %s.\n", pkg_name);
	}


	//cleanup
	unsigned long num_destroyed;
	free_if_not_null(pkg_dest);
	free_if_not_null(info_dir);
	free_if_not_null(control_name_prefix);
	free_if_not_null(list_file_name);
	free_if_not_null(fs_terminated_install_root);
	free_if_not_null(fs_terminated_link_root);
	if(files_to_link != NULL)   { destroy_string_map(files_to_link,     DESTROY_MODE_FREE_VALUES, &num_destroyed); }
	if(conf_files != NULL)      { destroy_string_map(conf_files,        DESTROY_MODE_FREE_VALUES, &num_destroyed); }
	if(copied_conf_files!= NULL){ destroy_string_map(copied_conf_files, DESTROY_MODE_FREE_VALUES, &num_destroyed); }




	return err;
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



