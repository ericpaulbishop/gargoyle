#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <erics_tools.h>
#include <libbbtargz.h>
#include <ewget.h>

#define FILE_PATH_LEN 1024


int mkdir_p(const char* path, mode_t mode);
void rm_r(const char* path);
int path_exists(const char* path);
int create_tmp_dir(const char* tmp_root, char** tmp_dir);
int install_to(const char* pkg_file, const char* pkg_name, const char* install_root);

int main(void)
{
	srand(time(NULL));


	rm_r("/tmp/test1/");
	mkdir_p("/tmp/test1/test2/test 3/test4/test5", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	rm_r("/tmp/test1/test2");


	install_to("tor_0.2.3.24-rc-2_x86.ipk", "tor", "/tmp/test1");

	return(0);
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

int create_tmp_dir(const char* tmp_root, char** tmp_dir)
{
	sprintf((*tmp_dir), "%s/tmp_%d\n", tmp_root, rand());
	return (mkdir_p(*tmp_dir,  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH  ));
}

//returns:
// 0 if path doesn't exist
// 1 if path is regular file
// 2 if path is directory
// 3 if path is symbolic link
// 4 if path exists and is something else
int path_exists(const char* path)
{
	struct stat fs;
	int exists = stat(path,&fs) >= 0 ? 4 : 0;
	if(exists > 0)
	{
		exists = S_ISREG(fs.st_mode) ? 1 : exists;
		exists = S_ISDIR(fs.st_mode) ? 2 : exists;
		exists = S_ISLNK(fs.st_mode) ? 3 : exists;
	}
	return exists;
}

int mkdir_p(const char* path, mode_t mode)
{
	int err=0;
	struct stat fs;

	char* dup_path = strdup(path);
	char* sep = strchr(dup_path, '/');
	sep = (sep == dup_path) ? strchr(sep+1, '/') : sep;
	while(sep != NULL && err == 0)
	{
		sep[0] = '\0';
		if(stat(dup_path,&fs) >= 0)
		{
			if(!S_ISDIR(fs.st_mode))
			{
				err = 1;
			}
		}
		else
		{
			mkdir(dup_path, mode);
		}
		err =1;
		if(stat(dup_path,&fs) >= 0)
		{
			err = S_ISDIR(fs.st_mode) ? 0 : 1;
		}

		sep[0] = '/';
		sep = strchr(sep+1, '/');
	}
	if(err == 0)
	{
		if(stat(dup_path,&fs) >= 0)
		{
			if(!S_ISDIR(fs.st_mode))
			{
				err = 1;
			}
		}
		else
		{
			mkdir(dup_path, mode);
		}
		err =1;
		if(stat(dup_path,&fs) >= 0)
		{
			err = S_ISDIR(fs.st_mode) ? 0 : 1;
		}

	}
	free(dup_path);
	return err;
}

void rm_r(const char* path)
{
	struct stat fs;
	if(stat(path,&fs) >= 0)
	{ 	
		if(S_ISDIR (fs.st_mode))
		{
			/* remove directory recursively */
			struct dirent **entries;
			int num_entry_paths;
			int entry_path_index;
			num_entry_paths = scandir(path,  &entries, 0, alphasort);
			for(entry_path_index=0; entry_path_index < num_entry_paths; entry_path_index++)
			{
				struct dirent *dentry = entries[entry_path_index];
				if(strcmp(dentry->d_name, "..") != 0 && strcmp(dentry->d_name, ".") != 0) 
				{
					char* entry_path = (char*)malloc(strlen(path) + strlen(dentry->d_name) + 2);
					sprintf(entry_path,"%s/%s", path, dentry->d_name);
					
					/* recurse */
					rm_r(entry_path);
					
					free(entry_path);
				}
			}
			rmdir(path);
		}
		else 
		{
			/* remove regular file, no need to recurse */
			remove(path);
		}
	}
}




