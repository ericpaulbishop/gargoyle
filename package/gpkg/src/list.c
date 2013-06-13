#include "gpkg.h"

void do_list(opkg_conf* conf, string_map* parameters, int format)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	unsigned long num_destroyed;

	char* only_dest = get_string_map_element(parameters, "only-destination");
	char* run_type = get_string_map_element(parameters, "run-type");
	int installed_only = strcmp(run_type, "list") != 0 || only_dest != NULL ? 1 : 0;


	int load_matching_only = get_string_map_element(parameters, "package-regex") != NULL ? 1 : 0;
	string_map* test_match_list = NULL;
	if( (test_match_list = get_string_map_element(parameters, "package-list")) != NULL)
	{
		if(test_match_list->num_elements > 0)
		{
			load_matching_only = 1;
		}
	}
	//printf("installed only = %d\n", installed_only);
	//printf("load matching only = %d\n", load_matching_only);

	int load_type = load_matching_only ? LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING : LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL;
	load_all_package_data(conf, package_data, matching_packages, parameters, load_type, NULL, 1, NULL );
	

	unsigned long num_all_packages;
	char** sorted_packages = get_string_map_keys( (load_matching_only ? matching_packages : package_data), &num_all_packages);
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	int package_index;
	qsort(sorted_packages, num_all_packages, sizeof(char*), sort_string_cmp);

	if(format == OUTPUT_JAVASCRIPT)
	{
		printf("pkg_list = [];\n");
	}
	else if(format == OUTPUT_JSON)
	{
		printf("{\n");
	}


	char description[4096];

	for(package_index=0;package_index < num_all_packages; package_index++)
	{
		int is_currently_installed;
		char* package_name = sorted_packages[package_index];
		if(strcmp(package_name, PROVIDES_STRING) != 0)
		{
			char* version = NULL;
			string_map* pkg_info = get_package_current_or_latest(package_data, package_name, &is_currently_installed, &version);
			if( (!installed_only) || is_currently_installed )
			{
				char* destination = get_string_map_element(pkg_info, "Install-Destination");
				if(only_dest == NULL || safe_strcmp(only_dest, destination) == 0)
				{
					escape_package_variable(get_string_map_element(pkg_info, "Description"), "Description", format, description, 4096);
					int print_description = (!installed_only) && description[0] != '\0'  ? 1 : 0;
					if(format == OUTPUT_JAVASCRIPT)
					{
						printf("pkg_list[\"%s\"] = [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? description : ""), "\"];\n");
					}
					else if(format == OUTPUT_JSON)
					{
						printf("\t\"%s\": [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? description : ""), "\"],\n");
					}
					else
					{
						printf("%s - %s%s%s\n", package_name, version, (print_description ? " - " : ""), (print_description ? description : ""));
					}
				}
			}
			free_if_not_null(version);
		
			if(format == OUTPUT_JSON)
			{
				printf("}\n");
			}
		}
	}
	free_null_terminated_string_array(sorted_packages);
	free_package_data(package_data);
	
}
