
#ifndef __GPKG_H

#define __GPKG_H 1

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <unistd.h>
#include <getopt.h>
#include <regex.h>
#include <stdint.h>
#include <sys/statvfs.h>


#include <erics_tools.h>
#include <bbtargz.h>
#include <ewget.h>

#if __SIZEOF_POINTER__ == 8
	#define SCANFU64 "%lu"
#else
	#define SCANFU64 "%llu"
#endif




#define FILE_PATH_LEN 1024
#define DEFAULT_CONF_FILE_PATH "/etc/opkg.conf"



#define REMOVE_NO_ORPHANED_DEPENDENCIES           0
#define REMOVE_ORPHANED_DEPENDENCIES_IN_SAME_DEST 1
#define REMOVE_ALL_ORPHANED_DEPENDENCIES          2




#define LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_MATCHING  0  /* Parameter unused, load from parameter map */
#define LOAD_MINIMAL_PKG_VARIABLES_FOR_MATCHING            1  /* Status, Depends, Size only */
#define LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_MATCHING        2  /* Status, Depends, Size, Description only*/

#define LOAD_PARAMETER_DEFINED_PKG_VARIABLES_FOR_ALL       3  /* Parameter unused, load from parameter map */
#define LOAD_MINIMAL_PKG_VARIABLES_FOR_ALL                 4  /* Status, Depends, Size only */
#define LOAD_DESCRIPTIVE_PKG_VARIABLES_FOR_ALL             5  /* Status, Depends, Size, Description only*/

#define LOAD_MINIMAL_FOR_ALL_PKGS_DESCRIPTIVE_FOR_MATCHING 6
#define LOAD_MINIMAL_FOR_ALL_PKGS_PARAMETER_FOR_MATCHING   7
#define LOAD_MINIMAL_FOR_ALL_PKGS_ALL_FOR_MATCHING         8

#define LOAD_ALL_PKG_VARIABLES                             9  /* Everything */





#define PROVIDES_STRING                "@P"
#define PROVIDES_REAL_NAME_STRING      "@PRN"
#define PROVIDES_REAL_VERSION_STRING   "@PRV"
#define PROVIDES_VERSION_STRING        "@PV"
#define PROVIDES_IS_DEFAULT_STRING     "@PD"
#define PREFERRED_PROVIDES_STRING      "@PP"
#define CURRENT_PROVIDES_STRING        "@CP"


#define CURRENT_VERSION_STRING "@C"
#define LATEST_VERSION_STRING  "@L"
#define NOT_INSTALLED_STRING   "@N"




#define OUTPUT_HUMAN_READABLE 1
#define OUTPUT_JSON           2
#define OUTPUT_JAVASCRIPT     3



/* conf defs/prototypes */
typedef struct opkg_conf_struct
{
	char* lists_dir;

	string_map* gzip_sources;
	string_map* plain_sources;

	string_map* overlays;

	string_map* dest_roots;
	string_map* dest_names;
	string_map* dest_freespace;
	string_map* dest_totalspace;
} opkg_conf;

opkg_conf* load_conf(const char* conf_file_name);
void free_conf(opkg_conf* conf);

void sort_versions(char** version_arr, unsigned long version_arr_len);
int compare_versions(char* v1, char* v2);



/* package info load/save defs/prototypes */
uint64_t destination_bytes_free(opkg_conf* conf, char* dest_name);
string_map* get_package_latest(string_map* all_package_data, char* package_name, int* is_current, char** matching_version);
string_map* get_package_current_or_latest(string_map* all_package_data, char* package_name, int* is_current, char** matching_version);
string_map* get_package_latest_matching(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version);
string_map* get_package_current_or_latest_matching(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version);
string_map* get_package_current_or_latest_matching_and_satisfiable(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version);
string_map* get_package_latest_matching(string_map* all_package_data, char* package_name, char** matching, int* is_current, char** matching_version);
string_map* get_package_with_version(string_map* all_package_data, char* package_name, char* package_version);

void add_package_data(string_map* all_package_data, string_map** package, char* package_name, char* package_version, string_map* preferred_provides);
char** alloc_depend_def(char* def_version_str);

void load_all_package_data(opkg_conf* conf, string_map* package_data, string_map* matching_packages, string_map* parameters, int load_variable_def, char* install_root, int ignore_recursive_variables, string_map* preferred_provides);
void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, int load_variable_def, char* dest_name, string_map* preferred_provides);
int load_recursive_package_data_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes);


void free_all_package_versions(string_map* all_versions);
void free_package_data(string_map* package_data);
void free_recursive_package_vars(string_map* package_data);

int something_depends_on(string_map* package_data, char* package_name, char* but_not_same_package_with_version, char** pkg_that_depends_on_query);

void save_package_data_as_status_file(string_map* package_data, char* status_file_path);




/* xsystem.c 
 * Like system(3), but with error messages printed if the fork fails
   or if the child process dies due to an uncaught signal. Also, the
   return value is a bit simpler:

   -1 if there was any problem
   Otherwise, the 8-bit return value of the program ala WEXITSTATUS
   as defined in <sys/wait.h>.
*/
int xsystem(const char *argv[]);


/* update.c */
void update(opkg_conf* conf);

/* remove.c */
void do_remove(opkg_conf* conf, string_map* pkgs, int save_conf_files, int remove_orphaned_depends, int force, int warn_if_forced, char* tmp_root);
void remove_individual_package(char* pkg_name, opkg_conf* conf, string_map* package_data, char* tmp_dir, int save_conf_files, int is_orphaned_dependency);
int run_script_if_exists(char* install_root_path, char* install_link_path, char* pkg_name, char* script_type_postfix, char* action_arg);

/* install.c */
void cp(char* src, char* dst);
int create_dir_and_test_writable(char* dir);
void do_install(opkg_conf* conf, string_map* pkgs, char* install_root_name, char* link_root_name, int is_upgrade, int overwrite_config, int overwrite_other_package_files, int force_reinstall, char* tmp_root);
int recursively_install(char* pkg_name, char* pkg_version, char* install_root_name, char* link_to_root, char* overlay_path, int is_upgrade, int overwrite_config, int overwrite_other_package_files, char* tmp_dir, opkg_conf* conf, string_map* package_data, string_map* install_called_pkgs);

/* upgrade.c */
void do_upgrade(opkg_conf* conf, string_map* pkgs, int preserve_conf_files, char* install_root_name, char* link_root, char* tmp_root);


/* list.c */
void do_list(opkg_conf* conf, string_map* parameters, int format);

/* info.c */
void escape_package_variable(char* var_def, char* var_name, int format, char* out_buf, int buf_len);
void do_print_dest_info(opkg_conf* conf, int format);
void do_print_info(opkg_conf* conf, string_map* parameters, char* install_root, int format);

#endif
