#include "gpkg.h"

string_map* parse_parameters(int argc, char** argv);

void print_help(void);



int main(int argc, char** argv)
{
	parse_parameters(argc, argv);


	return(0);

	rm_r("/tmp/plugin_root");
	rm_r("/tmp/plugin_test");
	

	mkdir_p("/tmp/plugin_root", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	mkdir_p("/tmp/plugin_test", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	


	//install_to("tor_0.2.3.24-rc-2_x86.ipk", "tor", "/tmp/test1");

	
	opkg_conf *conf = load_conf(NULL);

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

	
	
	
	
	//update(conf);

	//do_install(conf, "kmod-mmc-over-gpio", "plugin_root", "plugin_test", NULL0, 0, 0, NULL);
	do_install(conf, "irssi", "plugin_root", "plugin_test", NULL, 0, 0, 0, NULL);

	do_remove(conf, "irssi", 0, 1, 0, 1);


	return(0);
}



string_map* parse_parameters(int argc, char** argv)
{
	printf("argc = %d\n", argc);
	string_map* parameters = initialize_string_map(1);
	if(argc == 1)
	{
		print_help();  //exits
	}
	char* main_var = argv[1];
	if(	strcmp(main_var, "list") != 0 && 
		strcmp(main_var, "list-installed") != 0 && 
		strcmp(main_var, "info") != 0 && 
		strcmp(main_var, "update") != 0 &&
		strcmp(main_var, "install") != 0 &&
	       	strcmp(main_var, "remove") != 0 &&
	       	strcmp(main_var, "upgrade") != 0
	  )
	{
		print_help();  //exits
	}
	set_string_map_element(parameters, MAIN_PARAM_NAME, main_var);








	return parameters;
}

void print_help(void)
{
	printf("help (edit text later)\n\n");
	exit(0);
}
