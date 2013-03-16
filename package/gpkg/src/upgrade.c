


#include "gpkg.h"


void do_upgrade(opkg_conf* conf, char* pkg_name, char** new_version_criteria)
{
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;

	

	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	//load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 


	char* old_pkg_version = NULL;
	char* new_pkg_version = NULL;
	int new_pkg_is_current;
	int old_pkg_is_current;
	new_version_criteria = new_version_criteria == NULL ? alloc_depend_def(NULL) : new_version_criteria;
	string_map* old_pkg_data = get_package_current_or_latest(package_data, pkg_name, &old_pkg_is_current, &old_pkg_version);
	string_map* new_pkg_data = get_package_latest_matching(package_data, pkg_name, new_version_criteria, &new_pkg_is_current, &new_pkg_version);
	char* old_status = old_pkg_data != NULL ? get_string_map_element(old_pkg_data, "Status") : NULL;
	
	if(old_pkg_is_current == 0)
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


	
	
	
	
}


