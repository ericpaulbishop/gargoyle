#include "gpkg.h"

int main(void)
{
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
	string_map* libc_info = get_package_current_or_latest_matching(package_data, "libc", matching, &is_installed, NULL);
	
	printf("libc is installed = %d\n", is_installed);

	
	printf("c\n");
	string_map* libc_deps = get_string_map_element(libc_info, "All-Depends");
	if(libc_deps != NULL)
	{
		printf("d\n");
		char** libc_dep_list = get_string_map_keys(libc_deps, &num_destroyed);
		printf("e\n");
	
		int i;
		for(i=0; libc_dep_list[i] != NULL; i++)
		{
			printf("libc dep = %s\n", libc_dep_list[i]);
		}
	}
	else
	{
		printf("libc deps = NULL\n");
		char* deps = get_string_map_element(libc_info, "Depends");
		printf("dep str = %s\n", deps);

	}
	
	return 0;
	*/


	
	
	
	
	//update(conf);

	do_install(conf, "irssi", "plugin_root", "plugin_test");

	do_remove(conf, "irssi", 0, 1, 0);


	return(0);
}



