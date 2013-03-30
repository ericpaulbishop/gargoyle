#include "gpkg.h"

void do_list(opkg_conf* conf, string_map* parameters, int format)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	unsigned long num_destroyed;

	
	char* run_type = get_string_map_element(parameters, "run-type");
	int installed_only = strcmp(run_type, "list") != 0 ? 1 : 0;

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
	load_all_package_data(conf, package_data, matching_packages, parameters, load_type, NULL );
	

	unsigned long num_all_packages;
	char** sorted_packages = get_string_map_keys( (load_matching_only ? matching_packages : package_data), &num_all_packages);
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


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
			char* adj_description = description == NULL ? NULL : strdup(description);
			if(description != NULL && (format == OUTPUT_JSON || format == OUTPUT_JAVASCRIPT))
			{
				char* esc_val_1;
				char* esc_val_2;
				free(adj_description);
				esc_val_1         = dynamic_replace(description, "\\", "\\\\");
				esc_val_2         = dynamic_replace(esc_val_1, "\"", "\\\"");
				adj_description   = dynamic_replace(esc_val_2, "\n", "\\n");
				free(esc_val_1);
				free(esc_val_2);
			}
			int print_description = (!installed_only) && description != NULL ? 1 : 0;
			if(format == OUTPUT_HUMAN_READABLE)
			{
				printf("%s - %s%s%s\n", package_name, version, (print_description ? " - " : ""), (print_description ? adj_description : ""));
			}
			else if(format == OUTPUT_JAVASCRIPT)
			{
				printf("pkg_info[\"%s\"] = [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? adj_description : ""), "\"];\n");
			}
			else if(format == OUTPUT_JSON)
			{
				printf("\t\"%s\": [\"%s%s%s%s", package_name, version, (print_description ? "\",\"" : ""),  (print_description ? adj_description : ""), "\"],\n");
			}
			free_if_not_null(adj_description);
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
