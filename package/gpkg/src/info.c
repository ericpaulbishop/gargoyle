#include "gpkg.h"


char* escape_package_variable(char* def, int format)
{
	char* escaped = NULL;
	if(def != NULL && (format == OUTPUT_JSON || format == OUTPUT_JAVASCRIPT))
	{
		char* esc_val_1 = dynamic_replace(def, "\\", "\\\\");
		char* esc_val_2 = dynamic_replace(esc_val_1, "\"", "\\\"");
		char* esc_val_3 = dynamic_replace(esc_val_2, "\n", "\\n");
		escaped         = dynamic_replace(esc_val_3, "\r", "\\n");
		free(esc_val_1);
		free(esc_val_2);
		free(esc_val_3);
	}
	else if(def != NULL)
	{
		escaped = strdup(def);
	}
	return escaped;
}

void do_print_dest_info(opkg_conf* conf, int format)
{
	//output
	if(format == OUTPUT_JSON)
	{
		printf("{\n");

	}
	else if(format == OUTPUT_JAVASCRIPT)
	{
		printf("var pkg_dests = [];\n");
	}

	unsigned long num_dests;
	unsigned long dest_index;
	char** dests = get_string_map_keys(conf->dest_freespace, &num_dests);
	for(dest_index=0 ; dest_index < num_dests; dest_index++)
	{
		char* dest_name          = dests[dest_index];
		char* dest_root          = (char*)get_string_map_element(conf->dest_names, dest_name);
		uint64_t* dest_freespace  = (uint64_t*)get_string_map_element(conf->dest_freespace, dest_name);
		uint64_t* dest_totalspace = (uint64_t*)get_string_map_element(conf->dest_totalspace, dest_name);
		if(format == OUTPUT_JSON)
		{
			if(dest_index >0){ printf(",\n"); }
			printf("\t\"%s\": {\n", dest_name);
			printf("\t\t\"Root\": \"%s\",\n", dest_root);
			printf("\t\t\"Bytes-Free\": "SCANFU64",\n", *dest_freespace);
			printf("\t\t\"Bytes-Total\": "SCANFU64"\n", *dest_totalspace);
			printf("\t}");
		}
		else if(format == OUTPUT_JAVASCRIPT)
		{
			printf("pkg_dests['%s'] = [];\n", dest_name);
			printf("pkg_dests['%s']['Root'] = '%s';\n", dest_name, dest_root);
			printf("pkg_dests['%s']['Bytes-Free']  = "SCANFU64"\n", dest_name, *dest_freespace);
			printf("pkg_dests['%s']['Bytes-Total'] = "SCANFU64"\n", dest_name, *dest_totalspace);
		}
		else
		{
			printf("Destination: %s", dest_name);
			printf("Destination-Root: %s\n", dest_root);
			printf("Destination-Bytes-Free: "SCANFU64"\n", *dest_freespace);
			printf("Destination-Bytes-Total: "SCANFU64"\n", *dest_totalspace);
			printf("\n");
		}
	}
	if(format == OUTPUT_JSON)
	{
		printf("\n}\n");
	}
}





void do_print_info(opkg_conf* conf, string_map* parameters, int format)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	unsigned long num_destroyed;

	
	int load_matching_only = get_string_map_element(parameters, "package-regex") != NULL ? 1 : 0;
	string_map* test_match_list = NULL;
	if( (test_match_list = get_string_map_element(parameters, "package-list")) != NULL)
	{
		if(test_match_list->num_elements > 0)
		{
			load_matching_only = 1;
		}
	}

	int have_package_vars = 0;
	string_map* package_variables = get_string_map_element(parameters, "package-variables");
	if(package_variables != NULL)
	{
		have_package_vars = package_variables->num_elements > 0 ? 1 : 0;
	}

	int load_type  = LOAD_ALL_PKG_VARIABLES;
	load_type      = load_matching_only && (!have_package_vars) ? LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING       : load_type;
	load_type      = (!load_matching_only) && have_package_vars ? LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL     : load_type;
	load_type      = load_matching_only && have_package_vars    ? LOAD_MINIMAL_FOR_ALL_PKGS_PARAMETER_FOR_MATCHING : load_type;

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
		char* package_name = sorted_packages[package_index];

	}
	if(format == OUTPUT_JSON)
	{
		printf("}\n");
	}

	free_null_terminated_string_array(sorted_packages);
	free_package_data(package_data);


}

