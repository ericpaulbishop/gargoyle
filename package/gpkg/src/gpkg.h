
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

#define LOAD_PARAMETER_DEFINED_PKG_VARIABLES 0  /* Parameter unused, load from parameter map */
#define LOAD_MINIMAL_PKG_VARIABLES           1  /* Status, Depends, Size only */
#define LOAD_ALL_PKG_VARIABLES               2  /* Everything */



#define CURRENT_VERSION_STRING "@@@CURRENT_V@@@"
#define LATEST_VERSION_STRING  "@@@LATEST_V@@@"
#define NOT_INSTALLED_STRING   "@@@NOTINST@@@"



/* conf defs/prototypes */
typedef struct opkg_conf_struct
{
	char* lists_dir;

	string_map* gzip_sources;
	string_map* plain_sources;

	string_map* dest_roots;
	string_map* dest_names;
	string_map* dest_freespace;
	string_map* dest_totalspace;
} opkg_conf;

opkg_conf* load_conf(const char* conf_file_name);
void free_conf(opkg_conf* conf);


/* package info load/save defs/prototypes */
uint64_t destination_bytes_free(opkg_conf* conf, char* dest_name);


string_map* get_package_current_or_latest(string_map* all_package_data, char* package_name, int* is_current, char** matching_version);
string_map* get_package_with_version(string_map* all_package_data, char* package_name, char* package_version);


void load_all_package_data(opkg_conf* conf, string_map* package_data, string_map* matching_packages, string_map* parameters, int load_all_packages, int load_variable_def, char* install_root);
void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, int load_all_packages, int load_variable_def, char* dest_name);
int load_recursive_package_data_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes);
void free_all_package_versions(string_map* all_versions);
void free_package_data(string_map* package_data);
void free_recursive_package_vars(string_map* package_data);
int something_depends_on(string_map* package_data, char* package_name);
void save_package_data_as_status_file(string_map* package_data, char* status_file_path);





/* Like system(3), but with error messages printed if the fork fails
   or if the child process dies due to an uncaught signal. Also, the
   return value is a bit simpler:

   -1 if there was any problem
   Otherwise, the 8-bit return value of the program ala WEXITSTATUS
   as defined in <sys/wait.h>.
*/
int xsystem(const char *argv[]);


#endif
