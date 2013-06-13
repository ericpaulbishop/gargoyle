#include "gpkg.h"


void escape_package_variable(char* var_def, char* var_name, int format, char* out_buf, int buf_len)
{
	if(out_buf != NULL && buf_len > 0)
	{
		out_buf[0] = '\0';
	}
	if(var_def != NULL && out_buf != NULL && buf_len > 0)
	{
		int index;
		int out_index=0;
		for(index=0; var_def[index] != '\0' && index < (buf_len-1); index++)
		{
			if(format == OUTPUT_JSON || format == OUTPUT_JAVASCRIPT)
			{
				if(var_def[index] == '\\' || var_def[index] == '\"' || var_def[index] == '\n' || var_def[index] == '\r' )
				{
					char ch = var_def[index];
				       	ch = (ch == '\r' || ch == '\n') ? 'n' : ch;
					
					
					out_buf[out_index] = '\\';
					out_index++;
					out_buf[out_index] = ch;
				}
				else
				{
					out_buf[out_index] = var_def[index];
				}
			}
			else
			{
				if(var_def[index] == '\n' || var_def[index] == '\r' )
				{
					int varlen =strlen(var_name);
					int vari;
					out_buf[out_index] = '\n';
					for(vari=1; vari < varlen+3; vari++)
					{
						out_index++;
						out_buf[out_index] = ' ';
					}
				}
				else
				{
					out_buf[out_index] = var_def[index];
				}
			}
			out_index++;
		}
		out_buf[out_index] = '\0';
	}
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





void do_print_info(opkg_conf* conf, string_map* parameters, char* install_root, int format)
{
	string_map* package_data          = initialize_string_map(1);
	string_map* matching_packages     = initialize_string_map(1);
	unsigned long num_destroyed;

	
	char* only_dest = get_string_map_element(parameters, "only-destination");
	int load_matching_only = get_string_map_element(parameters, "package-regex") != NULL ? 1 : 0;
	string_map* test_match_list = NULL;
	if( (test_match_list = get_string_map_element(parameters, "package-list")) != NULL)
	{
		if(test_match_list->num_elements > 0)
		{
			load_matching_only = 1;
		}
	}
	//printf("load matching only = %d\n", load_matching_only);

	int have_package_vars = 0;
	string_map* package_variables = get_string_map_element(parameters, "package-variables");
	if(package_variables != NULL)
	{
		have_package_vars = package_variables->num_elements > 0 ? 1 : 0;
	}
	//printf("have package vars = %d\n", have_package_vars);

	int load_type  = LOAD_ALL_PKG_VARIABLES;
	load_type      = load_matching_only && (!have_package_vars) ? LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING       : load_type;
	load_type      = (!load_matching_only) && have_package_vars ? LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL     : load_type;
	load_type      = load_matching_only && have_package_vars    ? LOAD_MINIMAL_FOR_ALL_PKGS_PARAMETER_FOR_MATCHING : load_type;


	//only recurse if user specifically requested a recursively defined variable -- determining
	//these variables for a large number of packages tends to eat up memory
	int load_recursive = 0;
	if(have_package_vars)
	{
		if(	get_string_map_element(package_variables, "All-Depends") != NULL || 
			get_string_map_element(package_variables, "Required-Depends") != NULL || 
			get_string_map_element(package_variables, "Required-Size") != NULL ||
		       	get_string_map_element(package_variables, "Will-Fit") != NULL
			)
		{
			load_recursive = 1;
		}
	}




	load_all_package_data(conf, package_data, matching_packages, parameters, load_type, install_root, !load_recursive, NULL);
	
	unsigned long num_packages;
	char** sorted_packages = get_string_map_keys( (load_matching_only ? matching_packages : package_data), &num_packages);
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);

	int package_index;
	qsort(sorted_packages, num_packages, sizeof(char*), sort_string_cmp);

	if(format == OUTPUT_JAVASCRIPT)
	{
		printf("pkg_info = [];\n");
	}
	else if(format == OUTPUT_JSON)
	{
		printf("{\n");
	}

	char escape_var_buf[4096];
	char escape_ver_buf[4096];
	int printed_a_package=0;
	for(package_index=0;package_index < num_packages; package_index++)
	{
		char* package_name = sorted_packages[package_index];
		if(strcmp(package_name, PROVIDES_STRING) != 0)
		{
	
			string_map* all_versions = get_string_map_element(package_data, package_name);
	
			char* current_version = remove_string_map_element(all_versions, CURRENT_VERSION_STRING);
			char* latest_version  = remove_string_map_element(all_versions, LATEST_VERSION_STRING);
			unsigned long num_versions;
			char** versions = get_string_map_keys(all_versions, &num_versions);
			if(num_versions > 1)
			{
				sort_versions(versions, num_versions);
			}
	
			int printed_package_start = 0;
			if(num_versions > 0)
			{
	
	
				int version_index;
				for(version_index=0; version_index < num_versions; version_index++)
				{
					string_map* pkg_info = get_string_map_element(all_versions, versions[version_index]);
					char* destination = get_string_map_element(pkg_info, "Install-Destination");
					if(only_dest == NULL || safe_strcmp(only_dest, destination) == 0)
					{
						if(printed_package_start == 0)
						{
							if(format == OUTPUT_JAVASCRIPT)
							{
								printf("pkg_info[\"%s\"] = [];\n", package_name);
							}
							else if(format == OUTPUT_JSON)
							{
								if(package_index >0 && printed_a_package == 1){ printf(",\n"); }
								printf("\t\"%s\": {\n", package_name);
							}
							printed_package_start = 1;
							printed_a_package=1;
						}					
						escape_package_variable(versions[version_index], "Version", format, escape_ver_buf, 4096);
		
						if(format == OUTPUT_JAVASCRIPT)
						{
							printf("pkg_info[\"%s\"][\"%s\"] = [];\n", package_name, escape_ver_buf);
						}
						else if(format == OUTPUT_JSON)
						{
							if(version_index >0){ printf(",\n"); }
							printf("\t\t\"%s\": {\n", escape_ver_buf);
						}
						else
						{
							printf("Package: %s\n", package_name);
							printf("Version: %s\n", escape_ver_buf);
						}
		
						unsigned long num_vars_to_print;
						char** vars_to_print = get_string_map_keys(package_variables != NULL ? package_variables : pkg_info, &num_vars_to_print);
						int var_index;
						for(var_index=0; var_index < num_vars_to_print; var_index++)
						{
							char* var_name = vars_to_print[var_index];
							void* var_def = get_string_map_element(pkg_info, var_name);
							if(var_def != NULL)
							{
								if(strcmp(var_name, "Required-Size") == 0)
								{
									if(format == OUTPUT_JAVASCRIPT)
									{
										printf("pkg_info[\"%s\"][\"%s\"][\"%s\"] = "SCANFU64";\n", package_name, escape_ver_buf, var_name, *((uint64_t*)var_def) );
									}
									else if(format == OUTPUT_JSON)
									{
										if(var_index >0){ printf(",\n"); }
										printf("\t\t\t\"%s\": "SCANFU64"", var_name, *((uint64_t*)var_def) ) ;
									}
									else
									{
										printf("%s: "SCANFU64"\n", var_name, *((uint64_t*)var_def) ) ;
									}
								}
								else if(strcmp(var_name, "Required-Depends") == 0 || strcmp(var_name, "All-Depends") == 0)
								{
									unsigned long num_deps;
									unsigned long dep_index;
									char** dep_list = get_string_map_keys((string_map*)var_def, &num_deps);
									if(format == OUTPUT_JAVASCRIPT)
									{
										const char* end = num_deps > 0 ? "\n" : "];\n";
										printf("pkg_info[\"%s\"][\"%s\"][\"%s\"] = [%s", package_name, escape_ver_buf, var_name, end );
									}
									else if(format == OUTPUT_JSON)
									{
										if(var_index >0){ printf(",\n"); }
										printf("\t\t\t\"%s\": [\n", var_name);
									}
									else
									{
										printf("%s: ", var_name);
									}
		
		
									for(dep_index=0; dep_index < num_deps; dep_index++)
									{
										char* dep_name = dep_list[dep_index];
										char** dep_def = get_string_map_element((string_map*)var_def, dep_name);
										char* dep_def_str;
										if(dep_def[0][0] != '*')
										{
											dep_def_str = dynamic_strcat(5, " (", dep_def[0], " ", dep_def[1], ")");
										}
										else
										{
											dep_def_str = strdup("");
										}
										if(format == OUTPUT_JAVASCRIPT)
										{
											if(dep_index >0){ printf(",\n"); }
											printf("\t\"%s%s\"", dep_name, dep_def_str);
										}
										else if(format == OUTPUT_JSON)
										{
											if(dep_index >0){ printf(",\n"); }
											printf("\t\t\t\t\"%s%s\"", dep_name, dep_def_str);
										}
										else
										{
											if(dep_index >0){ printf(", "); }
											printf("%s%s", dep_name, dep_def_str);
										}
										free(dep_def_str);
									}
									if(format == OUTPUT_JAVASCRIPT)
									{
										if( num_deps > 0)
										{
											printf("\n\t];\n");
										}
									}
									else if(format == OUTPUT_JSON)
									{
										printf("\n\t\t\t]");
									}
									else
									{
										printf("\n");
									}
									free_null_terminated_string_array(dep_list);
								}
								else
								{
									escape_package_variable((char*)var_def, var_name, format, escape_var_buf, 4096);
	
									if(strcmp(var_name, "Install-Destination") == 0 && strcmp((char*)var_def, NOT_INSTALLED_STRING) == 0)
									{
										sprintf(escape_var_buf, "Not Installed");
									}
									if(format == OUTPUT_JAVASCRIPT)
									{
										printf("pkg_info[\"%s\"][\"%s\"][\"%s\"] = \"%s\";\n", package_name, escape_ver_buf, var_name, escape_var_buf );
									}
									else if(format == OUTPUT_JSON)
									{
										if(var_index >0){ printf(",\n"); }
										printf("\t\t\t\"%s\": \"%s\"", var_name, escape_var_buf);
									}
									else
									{
										printf("%s: %s\n", var_name, escape_var_buf);
									}
		
								}
							}
						}
						//end of one version of one package
						if(format == OUTPUT_JSON)
						{
							printf("\n\t\t}");
						}
						else //javascript or human readable
						{
							printf("\n");
						}
						free_null_terminated_string_array(vars_to_print);
					}
				}
	
				if(format == OUTPUT_JSON && printed_package_start == 1)
				{
					printf("\n\t}");
				}
			}
	
			free_if_not_null(current_version);
			free_if_not_null(latest_version);
			free_null_terminated_string_array(versions);

		}

	}
	if(format == OUTPUT_JSON)
	{
		printf("\n}\n");
	}

	free_null_terminated_string_array(sorted_packages);
	free_package_data(package_data);


}

