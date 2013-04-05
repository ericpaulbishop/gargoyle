#include "gpkg.h"

string_map* parse_parameters(int argc, char** argv);

void print_usage(void);



int main(int argc, char** argv)
{
	
	string_map* parameters = parse_parameters(argc, argv);

	opkg_conf *conf = load_conf(NULL);

	char* run_type                   = get_string_map_element(parameters, "run-type");
	int force_overwrite_other_files  = get_string_map_element(parameters, "force-overwrite")         != NULL ? 1 : 0;
	int force_overwrite_configs      = get_string_map_element(parameters, "force-overwrite-configs") != NULL ? 1 : 0;
	int force_depends                = get_string_map_element(parameters, "force-depends")           != NULL ? 1 : 0;
	char* install_root               = get_string_map_element(parameters, "install-destination");
	install_root                     = install_root == NULL ? strdup("root") : install_root;
	char* link_root                  = get_string_map_element(parameters, "link-destination");
	char* tmp_root                   = get_string_map_element(parameters, "tmp_dir");
	tmp_root                         = tmp_root == NULL ? strdup("/tmp") : tmp_root;
	string_map* pkgs                 = get_string_map_element(parameters, "package-list");
	char* format_str                 = get_string_map_element(parameters, "output-format");
	int format                       = OUTPUT_HUMAN_READABLE;
	if(format_str != NULL)
	{
		format = strcmp(format_str, "json") == 0 ? OUTPUT_JSON : format;
		format = strcmp(format_str, "js") == 0 || strcmp(format_str, "javascript") == 0 ? OUTPUT_JAVASCRIPT : format;
	}

	if(strcmp(run_type, "install") == 0)
	{
		do_install(conf, pkgs, install_root, link_root, 0, force_overwrite_configs, force_overwrite_other_files, tmp_root);
	}
	else if(strcmp(run_type, "remove") == 0)
	{
		do_remove(conf, pkgs, !force_overwrite_configs, 0, force_depends, 1);
	}
	else if(strcmp(run_type, "upgrade") == 0)
	{
		do_upgrade(conf, pkgs, !force_overwrite_configs, install_root, link_root);
	}
	else if(strcmp(run_type, "update") == 0)
	{
		update(conf);
	}
	else if((strcmp(run_type, "list") == 0) || strcmp(run_type, "list-installed") == 0 || strcmp(run_type, "list_installed") == 0)
	{
		do_list(conf, parameters, format);
	}
	else if(strcmp(run_type, "dest-info") == 0 || strcmp(run_type, "dest_info") == 0)
	{
		do_print_dest_info(conf, format);
	}
	else if(strcmp(run_type, "info") == 0)
	{
		do_print_info(conf, parameters, format);
	}

	return(0);

	/*
	opkg_conf *test_conf = load_conf(NULL);

	rm_r("/tmp/plugin_root");
	rm_r("/tmp/plugin_test");
	

	mkdir_p("/tmp/plugin_root", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	mkdir_p("/tmp/plugin_test", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	


	//install_to("tor_0.2.3.24-rc-2_x86.ipk", "tor", "/tmp/test1");
	string_map* pkgs = initialize_string_map(1);
	set_string_map_element(pkgs, "irssi",  alloc_depend_def(NULL));
	//set_string_map_element(pkgs, "zlib",  alloc_depend_def(NULL));
	set_string_map_element(pkgs, "/home/eric/gargoyle/package/gpkg/src/zlib_1.2.7-1_ar71xx.ipk",  alloc_depend_def(NULL));
	
	//update(conf);

	//do_install(test_conf, "kmod-mmc-over-gpio", "plugin_root", "plugin_test", NULL0, 0, 0, NULL);
	do_install(test_conf, pkgs, "plugin_root", "plugin_test", 0, 0, 0, NULL);

	//do_remove(test_conf, pkgs, 0, 1, 0, 1);

	*/
	

	/*
	printf("a\n");
	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;
	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, NULL );
	printf("b\n");
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);



	int is_installed;
	char* matching[2] = { "*", NULL };
	string_map* pkg_info = get_package_current_or_latest_matching(package_data, "irssi", matching, &is_installed, NULL);
	
	printf("irssi is installed = %d\n", is_installed);

	
	printf("c\n");
	string_map* pkg_deps = get_string_map_element(pkg_info, "Required-Depends");
	if(pkg_deps != NULL)
	{
		printf("d\n");
		char** pkg_dep_list = get_string_map_keys(pkg_deps, &num_destroyed);
		printf("e\n");
	
		int i;
		for(i=0; pkg_dep_list[i] != NULL; i++)
		{
			printf("irssi dep = %s\n", pkg_dep_list[i]);
		}
	}
	else
	{
		printf("irssi deps = NULL\n");
		char* deps = get_string_map_element(pkg_info, "Depends");
		printf("dep str = %s\n", deps);

	}
	
	return 0;
	*/

	
	


	return(0);
}



string_map* parse_parameters(int argc, char** argv)
{
	string_map* parameters = initialize_string_map(1);
	if(argc == 1)
	{
		print_usage();  //exits
	}
	char* run_type = argv[1];
	if(	strcmp(run_type, "list") != 0 && 
		strcmp(run_type, "list-installed") != 0 && 
		strcmp(run_type, "list_installed") != 0 && 
		strcmp(run_type, "dest-info") != 0 && 
		strcmp(run_type, "dest_info") != 0 && 
		strcmp(run_type, "info") != 0 && 
		strcmp(run_type, "update") != 0 &&
		strcmp(run_type, "install") != 0 &&
	       	strcmp(run_type, "remove") != 0 &&
	       	strcmp(run_type, "upgrade") != 0
	  )
	{
		print_usage();  //exits
	}
	set_string_map_element(parameters, "run-type", run_type);


	static struct option long_options[] = {
		{"force-depends",           0, 0, 'f'},
		{"force_depends",           0, 0, 'f'},
		{"force-overwrite",         0, 0, 'w'},
		{"force_overwrite",         0, 0, 'w'},
		{"force-maintainer",        0, 0, 'm'},
		{"force_maintainer",        0, 0, 'm'},
		{"force-overwrite-configs", 0, 0, 'm'},
		{"force_overwrite-configs", 0, 0, 'm'},
		{"dest",                    1, 0, 'd'},
		{"link-dest",               1, 0, 'l'},
		{"link_dest",               1, 0, 'l'},
		{"tmp-dir",                 1, 0, 't'},
		{"tmp_dir",                 1, 0, 't'},
		{"output-format",           1, 0, 'o'},
		{"output_format",           1, 0, 'o'},
		{"matching-regex",          0, 0, 'r'},
		{"matching_regex",          0, 0, 'r'},
		{"regex",                   0, 0, 'r'},
		{"package-variables",       1, 0, 'v'},
		{"package_variables",       1, 0, 'v'},
		{"help",                    0, 0, 'h'},
		{NULL, 0, NULL, 0}
	};


	int expect_regex =0;

	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "fwmd:l:t:o:rv:h", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'f':
				set_string_map_element(parameters, "force-depends", strdup("D"));
				break;
			case 'w':
				set_string_map_element(parameters, "force-overwrite", strdup("D"));
				break;
			case 'm':
				set_string_map_element(parameters, "force-overwrite-configs", strdup("D"));
				break;
			case 'd':
				set_string_map_element(parameters, "install-destination", strdup(optarg));
				break;
			case 'l':
				set_string_map_element(parameters, "link-destination", strdup(optarg));
				break;
			case 't':
				set_string_map_element(parameters, "tmp-dir", strdup(optarg));
				break;
			case 'o':
				set_string_map_element(parameters, "output-format", strdup(optarg));
				break;
			case 'v':
				if(strlen(optarg) >0)
				{

					unsigned long num_pieces=0;
					char seps[] = {',', ';', ':', ' ', '\r', '\n', '\t' }; //include newlines to strip them off end of line
					char** split_line = split_on_separators(optarg, seps, 7, -1, 0, &num_pieces);
					if(num_pieces > 0)
					{
						string_map* package_variables = initialize_string_map(1); 
						int piece_index;
						for(piece_index=0;piece_index < num_pieces; piece_index++)
						{
							if(strlen(split_line[piece_index]) > 0)
							{
								set_string_map_element(package_variables, split_line[piece_index], strdup("D"));
							}
						}
						if(package_variables->num_elements > 0)
						{
							set_string_map_element(parameters, "package-variables", package_variables);
						}
					}
					free_null_terminated_string_array(split_line);
				}
				break;

			case 'r': 
				//--matching-regex, or just --regex
				expect_regex = 1;
				break;
			case 'h':
			default:
				print_usage(); //exits
				break;
		}
	}

	if( expect_regex == 1 && strcmp(run_type, "info") != 0 && strcmp(run_type, "list") != 0 && strcmp(run_type, "list-installed") != 0 && strcmp(run_type, "list_installed") )
	{
		fprintf(stderr, "ERROR: package list can only specified with regular expression when using 'list', 'list-installed' or 'info' commands\n\n");
		exit(1);
	}


	
	string_map* pkg_list = initialize_string_map(1);
	set_string_map_element(parameters, "package-list", pkg_list);
	for(option_index=1; option_index < argc; option_index++)
	{
		int option_str_len = strlen(argv[option_index]);
		if(argv[option_index][0] == '-' && option_str_len > 1)
		{
			int has_arg=0;
			int test_index;
			const char* name;
			const char* test_name = argv[option_index] + 1;
			int is_long_opt=0;
			if(argv[option_index][1] == '-')
			{
				test_name = argv[option_index] + 2;
				is_long_opt = 1;
			}
			for(test_index=0; (name = (long_options[test_index]).name) != NULL ; test_index++)
			{
				if( (is_long_opt == 1 && strcmp(test_name, name) == 0) || (is_long_opt == 0 && test_name[0] == (long_options[test_index]).val) )
				{
					has_arg = (long_options[test_index]).has_arg;
				}
			}
			if(has_arg)
			{
				option_index++;
			}
		}
		else if(strcmp(argv[option_index], run_type) != 0)
		{
			if(expect_regex)
			{
				regex_t* packages_matching_regex = (regex_t*)malloc(sizeof(regex_t));
				char* regex_str = strdup(argv[option_index]);
				int regex_len = strlen(regex_str);
				if(get_string_map_element(parameters, "package-regex") != NULL)
				{
					fprintf(stderr, "ERROR: Only one regular expression can be specified at a time.\n\n");
					exit(1); 

				}
				if(regex_str[0] != '/' || regex_str[regex_len-1] != '/')
				{
					free(regex_str);
					regex_str=dynamic_strcat(3, "/", argv[option_index], "/");
				}
				if( convert_to_regex(regex_str, packages_matching_regex) )
				{
					set_string_map_element(parameters, "package-regex", packages_matching_regex);
				}
				else
				{
					fprintf(stderr, "ERROR: Invalid regular expression \"%s\"\n\n", regex_str);
					exit(1); 
				}
				free(regex_str);
			}
			else
			{
				set_string_map_element(pkg_list, argv[option_index], alloc_depend_def(NULL));
			}
		}
	}
	if(	strcmp(run_type, "install") == 0 ||
	       	strcmp(run_type, "remove")  == 0 ||
	       	strcmp(run_type, "upgrade") == 0
		)
	{
		if(pkg_list->num_elements == 0)
		{
			printf("ERROR: No packages specified to %s.\n\n", run_type);
			exit(1);
		}
	}


	return parameters;
}

void print_usage(void)
{
	printf("help (edit text later)\n\n");
	exit(0);
}
