


#include "gpkg.h"


void do_upgrade(opkg_conf* conf, char* pkg_name, char** new_version_criteria)
{
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;

	
	char* new_pkg_version = NULL;
	int new_pkg_is_current;
	new_version_criteria = new_version_criteria == NULL ? alloc_depend_def(NULL) : new_version_criteria;
	string_map* upgrade_pkg_data = get_package_latest_matching(package_data, pkg_name, new_version_criteria, &new_pkg_is_current, &new_pkg_version);


	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	//load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, install_root_name );
	//destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	
	//load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 

	
}


