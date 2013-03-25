#include "gpkg.h"

string_map* parse_parameters(int argc, char** argv);

void print_usage(void);



int main(int argc, char** argv)
{
	
	string_map* parameters = parse_parameters(argc, argv);

	opkg_conf *conf = load_conf(NULL);

	char* run_type                   = get_string_map_element(parameters, "run_type");
	int force_overwrite_other_files  = get_string_map_element(parameters, "force-overwrite")         != NULL ? 1 : 0;
	int force_overwrite_configs      = get_string_map_element(parameters, "force-overwrite-configs") != NULL ? 1 : 0;
	int force_depends                = get_string_map_element(parameters, "force-depends")           != NULL ? 1 : 0;
	char* install_root               = get_string_map_element(parameters, "install-destination");
	install_root                     = install_root == NULL ? strdup("root") : install_root;
	char* link_root                  = get_string_map_element(parameters, "link-destination");
	char* tmp_root                   = get_string_map_element(parameters, "tmp_dir");
	tmp_root                         = tmp_root == NULL ? strdup("/tmp") : tmp_root;
	string_map* pkgs                 = get_string_map_element(parameters, "package_list");
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
	else if(strcmp(run_type, "list") == 0)
	{
		do_list(conf, 0, format);
	}
	else if(strcmp(run_type, "list-installed") == 0)
	{
		do_list(conf, 1, format);
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
		strcmp(run_type, "info") != 0 && 
		strcmp(run_type, "update") != 0 &&
		strcmp(run_type, "install") != 0 &&
	       	strcmp(run_type, "remove") != 0 &&
	       	strcmp(run_type, "upgrade") != 0
	  )
	{
		print_usage();  //exits
	}
	set_string_map_element(parameters, "run_type", run_type);


	static struct option long_options[] = {
		{"force-depends",           0, 0, 'n'},
		{"force_depends",           0, 0, 'n'},
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
		{"output-format",           1, 0, 'p'},
		{"output_format",           1, 0, 'p'},
		{"help",                    0, 0, 'h'},
		{NULL, 0, NULL, 0}
	};

	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "hevm", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'n':
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
			case 'p':
				set_string_map_element(parameters, "output-format", strdup(optarg));
				break;
			case 'h':
			default:
				print_usage();
				break;
		}
	}


	option_index = option_index  < 2 ? 2 : option_index;
	
	string_map* pkg_list = initialize_string_map(1);
	set_string_map_element(parameters, "package_list", pkg_list);
	for(option_index; option_index < argc; option_index++)
	{
		if(argv[option_index][0] != '-')
		{
			set_string_map_element(pkg_list, argv[option_index], alloc_depend_def(NULL));
		}
	}

	return parameters;
}

void print_usage(void)
{
	printf("help (edit text later)\n\n");
	exit(0);
}
