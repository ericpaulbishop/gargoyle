#include "gpkg.h"

int install_to(const char* pkg_file, const char* pkg_name, const char* install_root);
void update(opkg_conf* conf);

void do_install(opkg_conf* conf, char* pkg_name, char* install_root);
int recursively_install(char* pkg_name, char* install_root, char* link_to_root, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs);



int main(void)
{
	rm_r("/tmp/plugin_root/");
	mkdir_p("/tmp/plugin_root", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	


	//install_to("tor_0.2.3.24-rc-2_x86.ipk", "tor", "/tmp/test1");

	
	opkg_conf *conf = load_conf(NULL);
	//update(conf);

	do_install(conf, "irssi", "plugin_root");

	


	return(0);
}


void update(opkg_conf* conf)
{
	mkdir_p(conf->lists_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );

	int is_gzip[2] = { 1, 0 };
	string_map* src_lists[2] = { conf->gzip_sources, conf->plain_sources };
	int src_list_index;
	for(src_list_index=0; src_list_index < 2 ; src_list_index++)
	{
		unsigned long num_keys;
		char** src_list = get_string_map_keys(src_lists[src_list_index], &num_keys);
		int src_index;
		for(src_index=0; src_index < num_keys; src_index++)
		{
			char* src_id = src_list[src_index];
			char* src_url = dynamic_strcat(2, (char*)get_string_map_element(src_lists[src_list_index], src_id), (is_gzip[src_list_index] ? "/Packages.gz" : "Packages"));
			char* package_file_path = dynamic_strcat(3,  conf->lists_dir, "/", src_id);
			char* package_tmp_file_path = dynamic_strcat(2, package_file_path, ".download.gpkg.tmp");
			
			
			
			FILE* package_tmp_file = fopen(package_tmp_file_path, "w");
			int read_err = 1;
			
			if(package_tmp_file != NULL)
			{
				printf("Downloading package list for %s source...\n", src_id);
				read_err = write_url_to_stream(src_url, "gpkg", NULL, package_tmp_file, NULL);
				fclose(package_tmp_file);
			}
			
			if(!read_err)
			{
				rm_r(package_file_path);
				rename(package_tmp_file_path, package_file_path);
				printf("Package list for %s downloaded successfully.\n\n", src_id);
			}
			else
			{
				rm_r(package_tmp_file_path);
				printf("WARNING: Could not retrieve package list for %s.\n\n", src_id);
			}
			
			free(src_url);
			free(package_tmp_file_path);
			free(package_file_path);
		}
		free_null_terminated_string_array(src_list);
	}
}




void do_install(opkg_conf* conf, char* pkg_name, char* install_root_name)
{

	string_map* package_data = initialize_string_map(1);
	string_map* matching_packages = initialize_string_map(1);
	unsigned long num_destroyed;

	char* install_root_path = (char*)get_string_map_element(conf->dest_names, install_root_name);
	if(install_root_path == NULL)
	{
		printf("ERROR: No destination %s found, cannot install\n\n", install_root_name);
		exit(1);
	}
	

	/* Determine all packages to install by first loading all package names, status & dependencies (and no other variables) */
	load_all_package_data(conf, package_data, matching_packages, NULL, 1, LOAD_MINIMAL_PKG_VARIABLES, install_root_name );
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	
	load_recursive_package_data_variables(package_data, pkg_name, 1, 0, 0); // load required-depends for package of interest only 
	

	string_map* install_pkg_data = get_string_map_element(package_data, pkg_name);
	char* install_status = get_string_map_element(install_pkg_data, "Status");
	char** install_pkg_list = NULL;	
	unsigned long install_pkg_list_len = 0;
	


	/* load detailed information for all packiages we are about to install */
	if(install_status != NULL)
	{
		string_map* load_detail_map = initialize_string_map(1);
		set_string_map_element(load_detail_map, pkg_name, strdup("D"));
		
		string_map* install_pkg_depend_map = get_string_map_element(install_pkg_data, "Required-Depends");
		if(install_pkg_depend_map != NULL)
		{
			unsigned long num_keys;
			char** load_detail_pkgs = get_string_map_keys(install_pkg_depend_map, &num_keys);
			int ldp_index;
			for(ldp_index=0;ldp_index < num_keys; ldp_index++)
			{
				set_string_map_element(load_detail_map, load_detail_pkgs[ldp_index], strdup("D"));
			}
		}
		free_recursive_pkg_vars(package_data); /* note: whacks install_pkg_depend_map */
		
	
		string_map* parameters = initialize_string_map(1);
		matching_packages = initialize_string_map(1);
		set_string_map_element(parameters, "packages", load_detail_map);
		load_all_package_data(conf, package_data, matching_packages, parameters, 0, LOAD_ALL_PKG_VARIABLES, install_root_name);
		install_pkg_list = get_string_map_keys(matching_packages, &install_pkg_list_len);
		
		int pkg_index;
		for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
		{
			printf("loaded %s\n", install_pkg_list[pkg_index]);
		}

		
		destroy_string_map(load_detail_map,   DESTROY_MODE_FREE_VALUES, &num_destroyed);
		destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);
		destroy_string_map(parameters, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);

	}
	install_status = get_string_map_element(install_pkg_data, "Status");
	char* will_fit = get_string_map_element(install_pkg_data, "Will-Fit");


	/* error checking before we start install */
	if(install_pkg_data == NULL || install_status == NULL)
	{
		printf("ERROR: No package named %s found, try updating your package lists\n\n", pkg_name);
		exit(1);
	}
	if(install_status == NULL || strstr(install_status, "installed ") != NULL)
	{
		printf("ERROR: Package %s is already installed\n\n", pkg_name);
		exit(1);
	}
	if(will_fit == NULL || strcmp(will_fit, "true") != 0)
	{
		printf("ERROR: Not enough space in destination %s to install package %s \n\n", install_root_name, pkg_name);
		exit(1);
	}


	/* Set status of new required packages to half-installed, set user-installed on requested package, installed time on all */
	char* install_root_status_path = dynamic_strcat(2, install_root_path, "/usr/lib/opkg/status");
	string_map* install_root_status = initialize_string_map(1);
	matching_packages = initialize_string_map(1);
	if(path_exists(install_root_status_path))
	{
		load_package_data(install_root_status_path, 0, install_root_status, matching_packages, NULL, 1, LOAD_ALL_PKG_VARIABLES, install_root_name);
	}
	destroy_string_map(matching_packages, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	int pkg_index;
	time_t now = time(NULL);
	char install_time[20];
	sprintf(install_time, "%lu", now);


	for(pkg_index=0; pkg_index < install_pkg_list_len; pkg_index++)
	{
		
		string_map* pkg = get_string_map_element(package_data, install_pkg_list[pkg_index]);
		if(get_string_map_element(pkg, "Installed-Time") == NULL)
		{
			printf("f\n");
			char* old_status = remove_string_map_element(pkg, "Status");
			free(old_status);
			char* status_parts[3] = { "install", "ok", "half-installed" };
			status_parts[1] = strcmp(pkg_name, install_pkg_list[pkg_index]) == 0 ? "user" : status_parts[1];
			char* new_status = dynamic_strcat(5, status_parts[0], " ", status_parts[1], " ", status_parts[2]);
			set_string_map_element(pkg, "Status", new_status);

			set_string_map_element(pkg, "Installed-Time", strdup(install_time));


			set_string_map_element(install_root_status, install_pkg_list[pkg_index], pkg);
		}
	}
	save_package_data_as_status_file(install_root_status, install_root_status_path);

	char* tmp_dir;
	if(!create_tmp_dir("/tmp", &tmp_dir))
	{
		printf("ERROR: Could not create tmp dir, exiting\n");
		exit(1);
	}
	
	
	

	
	

	

}

int recursively_install(char* pkg_name, char* install_root, char* link_to_root, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs)
{
	int err=0;
	
	string_map* pkg_data = get_string_map_element(package_data, pkg_name);
	char* src_id = get_string_map_element(package_data, "Source");
	char* base_url = NULL;


	string_map* src_lists[2] = { conf->gzip_sources, conf->plain_sources };
	int src_list_index;
	for(src_list_index=0; src_list_index < 2 && base_url == NULL; src_list_index++)
	{
		base_url = (char*)get_string_map_element(src_lists[src_list_index], src_id);
	}
	if(base_url == NULL)
	{
		err = 1;
	}

	char* pkg_dest = NULL;
	if(err == 0)
	{
		char* src_url  = dynamic_strcat(4, base_url, "/", pkg_name, ".ipk");
		pkg_dest = dynamic_strcat(4, tmp_dir, "/", pkg_name, ".ipk");
		FILE* package_file = fopen(pkg_dest, "w");
		if(package_file != NULL)
		{
			err = write_url_to_stream(src_url, "gpkg", NULL, package_file, NULL);
			fclose(package_file);
		}
		else
		{
			err = 1;
		}
		free(src_url);

		

		if(err == 1)
		{
			printf("ERROR: Could not download package %s\n", pkg_name);
		}

	}
	if(err == 0)
	{
		//check md5sum

	}
	if(err == 0)
	{

	}



	
	if(pkg_dest != NULL) {free(pkg_dest); };

	return err;
}












int install_to(const char* pkg_file, const char* pkg_name, const char* install_root)
{	
	int  err = 0;
	char dir_name[FILE_PATH_LEN];
	char control_name_prefix[FILE_PATH_LEN];
	char list_file_name[FILE_PATH_LEN];
	sprintf(dir_name, "%s/usr/lib/opkg/info", install_root);
	sprintf(control_name_prefix, "%s/%s.", dir_name, pkg_name);
	sprintf(list_file_name, "%s/%s.list", dir_name, pkg_name);
	mkdir_p(dir_name, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );


	//extract .list file & adjust file paths appropriately
	FILE* list_file = fopen(list_file_name, "w");
	deb_extract(	pkg_file,
			list_file,
			extract_quiet | extract_data_tar_gz | extract_list,
			NULL,
			NULL, 
			&err);
	fclose(list_file);
	
	if(err)
	{
		rm_r(list_file_name);
		fprintf(stderr, "ERROR: could not extract file list from packge %s.\n", pkg_file);
		fprintf(stderr, "       package file may be corrupt\n\n");
		return err;
	}

	list_file = fopen(list_file_name, "r");
	unsigned long list_file_length;
	char line_seps[] = {'\r', '\n'};
	char* list_file_data =  read_entire_file(list_file, FILE_PATH_LEN, &list_file_length);
	fclose(list_file);
	
	unsigned long num_list_lines;
	char** list_file_lines = split_on_separators(list_file_data, line_seps , 2, -1, 0, &num_list_lines);
	free(list_file_data);
	

	int install_root_len = strlen(install_root);
	char* fs_terminated_install_root = install_root[install_root_len-1] == '/' ? strdup(install_root) : dynamic_strcat(2, install_root, "/");
	list_file = fopen(list_file_name, "w");
	int line_index;
	for(line_index=0; line_index < num_list_lines && (!err) ; line_index++)
	{
		int line_len = strlen( list_file_lines[line_index] );
		if(line_len > 2)
		{
			if(list_file_lines[line_index][0] == '.' && list_file_lines[line_index][1] == '/' && list_file_lines[line_index][line_len-1] != '/')
			{
				char* adjusted_file_path = dynamic_strcat(2, install_root, fs_terminated_install_root, list_file_lines[line_index] + 2);
				fprintf(list_file, "%s\n", adjusted_file_path);
				err = path_exists(adjusted_file_path) ? 1 : 0;
				if(err)
				{
					fprintf(stderr, "ERROR: file '%s'\n", adjusted_file_path);
					fprintf(stderr, "       from package %s already exists.\n\n", pkg_name);
				}
				free(adjusted_file_path);

			}
		}
	}
	fclose(list_file);
	if(err)
	{
		rm_r(list_file_name);
		return err;
	}


	//extract control
	deb_extract(	pkg_file,
			stderr,
			extract_control_tar_gz | extract_all_to_fs| extract_preserve_date | extract_unconditional,
			control_name_prefix, 
			NULL, 
			&err);
	if(err)
	{
		rm_r(list_file_name);
		fprintf(stderr, "ERROR: could not extract control files from packge %s.\n", pkg_name);
		fprintf(stderr, "       package file may be corrupt\n\n");
		return err;
	}

	//extract package files
	
	deb_extract(	pkg_file,
			stderr,
			extract_data_tar_gz | extract_all_to_fs| extract_preserve_date| extract_unconditional,
			fs_terminated_install_root, 
			NULL, 
			&err);
	if(err)
	{
		rm_r(list_file_name);
		fprintf(stderr, "ERROR: could not extract application files from packge %s.\n", pkg_name);
		fprintf(stderr, "       package file may be corrupt\n\n");
		return err;
	}
	free(fs_terminated_install_root);

	
	
	
	return err;
}

