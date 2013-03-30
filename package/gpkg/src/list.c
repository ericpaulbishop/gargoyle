#include "gpkg.h"

void do_list(opkg_conf* conf, int installed_only, int format)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	unsigned long num_destroyed;


	load_all_package_data(conf, package_data, matching_packages, NULL, LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL, NULL );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	

	unsigned long num_all_packages;
	char** sorted_packages = get_string_map_keys(package_data, &num_all_packages);
	int package_index;
	qsort(sorted_packages, num_all_packages, sizeof(char*), sort_string_cmp);

	if(format == OUTPUT_JAVASCRIPT)
	{
		printf("pkg_info = [];\n");
	}
	else if(format == OUTPUT_JSON)
	{
		printf("{\n");
	}

	for(package_index=0;package_index < num_all_packages; package_index++)
	{
		int is_currently_installed;
		char* package_name = sorted_packages[package_index];
		char* version = NULL;
		string_map* pkg_info = get_package_current_or_latest(package_data, package_name, &is_currently_installed, &version);
		if( (!installed_only) || is_currently_installed )
		{
			char* description = get_string_map_element(pkg_info, "Description");
			int print_description = (!installed_only) && description != NULL ? 1 : 0;
			if(format == OUTPUT_HUMAN_READABLE)
			{
				printf("%s - %s%s%s\n", package_name, version, (print_description ? " - " : ""), (print_description ? description : ""));
			}
			else if(format == OUTPUT_JAVASCRIPT)
			{
				printf("pkg_info[\"%s\"] = [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? description : ""), "\"];\n");
			}
			else if(format == OUTPUT_JSON)
			{
				printf("\t\"%s\": [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? description : ""), "\"],\n");
			}
		}
		free_if_not_null(version);
	}
	if(format == OUTPUT_JSON)
	{
		printf("}\n");
	}

	free_null_terminated_string_array(sorted_packages);
	free_package_data(package_data);
	
}
