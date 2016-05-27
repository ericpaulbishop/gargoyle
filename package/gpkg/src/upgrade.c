


#include "gpkg.h"


void do_upgrade(opkg_conf* conf, string_map* pkgs, int preserve_conf_files, char* install_root_name, char* link_root, char* tmp_root)
{

	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;

	char* install_root_path = (char*)get_string_map_element(conf->dest_names, install_root_name);
	char* overlay_path = (char*)get_string_map_element(conf->overlays, install_root_name);


	char* test_dir  = dynamic_strcat(2, (overlay_path != NULL ? overlay_path : install_root_path), "/usr/lib/opkg/info");
	if(!create_dir_and_test_writable(test_dir))
	{
		fprintf(stderr, "ERROR: Specified upgrade destination is not writable, exiting\n");
		exit(1);
	}
	free(test_dir);

	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL, NULL, 1, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	string_map* all_new_pkgs_map = initialize_string_map(1);
	uint64_t combined_old_size = 0;

	unsigned long num_pkgs_to_upgrade;
	char** upgrade_pkg_list = get_string_map_keys(pkgs, &num_pkgs_to_upgrade);
	int upgrade_pkg_index;
	for(upgrade_pkg_index=0; upgrade_pkg_index < num_pkgs_to_upgrade; upgrade_pkg_index++)
	{
		char* pkg_name = upgrade_pkg_list[upgrade_pkg_index];
		load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0);


		char** new_version_criteria = get_string_map_element(pkgs, pkg_name);
		char* old_pkg_version = NULL;
		char* new_pkg_version = NULL;
		int new_pkg_is_current;
		int old_pkg_is_current;
		string_map* old_pkg_data = get_package_current_or_latest(package_data, pkg_name, &old_pkg_is_current, &old_pkg_version);
		string_map* new_pkg_data = get_package_latest_matching(package_data, pkg_name, new_version_criteria, &new_pkg_is_current, &new_pkg_version);
		char* old_status = old_pkg_data != NULL ? get_string_map_element(old_pkg_data, "Status") : NULL;
		char* old_install_root = get_string_map_element(old_pkg_data, "Install-Destination");

	
		if(old_pkg_is_current == 0 || install_root_name == NULL)
		{
			fprintf(stderr, "ERROR: package %s is not currently installed, cannot upgrade. Try installing it instead.\n", pkg_name);
			exit(1);
		}
		if(strcmp(old_pkg_version, new_pkg_version) == 0)
		{
			fprintf(stderr, "ERROR: package %s is already the latest version (%s)\n", pkg_name, old_pkg_version);
			exit(1);
		}
		//if old_pkg_data were null, we would have exited above, so we should have old_status
		if(strstr(old_status, " hold ") != NULL)
		{
			fprintf(stderr, "ERROR: Package %s is marked as 'hold', cannot upgrade\n", pkg_name);
			exit(1);
		}

		char *pkg_that_requires_old_version = NULL;
		if(something_depends_on(package_data, pkg_name, new_pkg_version, &pkg_that_requires_old_version))
		{
			fprintf(stderr, "ERROR: Package %s is not compatible with the latest version of %s (%s), cannot upgrade\n", pkg_that_requires_old_version, pkg_name, new_pkg_version);
			exit(1);
		}


		uint64_t next_old_size = 0;
		char* old_size_str = get_string_map_element(old_pkg_data, "Installed-Size");
		if(old_size_str != NULL)
		{
			sscanf(old_size_str, SCANFU64, &next_old_size);
		}
		if(strcmp(old_install_root, install_root_name) == 0)
		{
			combined_old_size = combined_old_size + next_old_size;
		}



		char** old_def = set_string_map_element(all_new_pkgs_map, pkg_name, copy_null_terminated_string_array(new_version_criteria));
		if(old_def != NULL) { free_null_terminated_string_array(old_def); }
		string_map* req_deps = get_string_map_element(new_pkg_data, "Required-Depends");
		if(req_deps != NULL)
		{
			unsigned long num_req_deps;
			char** req_dep_names = get_string_map_keys(req_deps, &num_req_deps);
			int req_dep_index;
			for(req_dep_index=0;req_dep_index < num_req_deps; req_dep_index++)
			{
				char* req_dep_name = req_dep_names[req_dep_index];
				if(get_string_map_element(all_new_pkgs_map, req_dep_name) == NULL)
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

					char** req_dep_def = get_string_map_element(req_deps, req_dep_name);
					set_string_map_element(all_new_pkgs_map, req_dep_name, copy_null_terminated_string_array(req_dep_def));
				}
			}
		}
	}
	
	//check whether we have room
	uint64_t combined_new_size = 0;
	unsigned long num_new_pkgs = 0;
	char** all_new_pkgs = get_string_map_keys(all_new_pkgs_map, &num_new_pkgs);
	int new_pkg_index;
	for(new_pkg_index=0; new_pkg_index < num_new_pkgs; new_pkg_index++)
	{
		char* new_pkg_name = all_new_pkgs[new_pkg_index];
		char** new_pkg_def = get_string_map_element(all_new_pkgs_map, new_pkg_name);
		string_map* new_pkg = get_package_latest_matching(package_data, new_pkg_name, new_pkg_def, NULL, NULL);
		if(new_pkg != NULL)
		{
			char* new_size_str = get_string_map_element(new_pkg, "Installed-Size");
			if(new_size_str != NULL)
			{
				uint64_t next_new_size = 0;
				sscanf(new_size_str, SCANFU64, &next_new_size);
				combined_new_size = combined_new_size + next_new_size;

			}

		}
	}
	uint64_t additional_space_required = combined_old_size > combined_new_size ? 0 : combined_new_size - combined_old_size;
	uint64_t space_available = destination_bytes_free(conf, install_root_name);
	if(space_available <= additional_space_required)
	{
		fprintf(stderr, "ERROR: Not enough space (have " SCANFU64 " bytes, need " SCANFU64 " bytes) to upgrade specified packages to latest versions, cannot upgrade\n", space_available, additional_space_required );
		exit(1);
	}
	
	free_package_data(package_data);
	
	do_remove(conf, pkgs, preserve_conf_files, 0, 1, 0, tmp_root);
	do_install(conf, pkgs, install_root_name, link_root, 1, (!preserve_conf_files), 0, 0, tmp_root);

}


