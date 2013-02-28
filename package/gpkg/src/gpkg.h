
#ifndef __GPKG_H

#define __GPKG_H 1

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <unistd.h>
#include <getopt.h>
#include <regex.h>
#include <stdint.h>
#include <sys/statvfs.h>


#include <erics_tools.h>
#include <libbbtargz.h>
#include <ewget.h>



#if __SIZEOF_POINTER__ == 8
	#define SCANFU64 "%lu"
#else
	#define SCANFU64 "%llu"
#endif


#define FILE_PATH_LEN 1024
#define DEFAULT_CONF_FILE_PATH "/etc/opkg.conf"


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
void load_all_package_data(opkg_conf* conf, string_map* existing_package_data, string_map* matching_packages, string_map* parameters);
void load_package_data(char* data_source, int source_is_dir, string_map* existing_package_data, string_map* matching_packages, string_map* parameters, char* dest_name);
int load_recursive_package_data_variables(string_map* package_data, char* package, int load_size, int load_will_fit, uint64_t free_bytes);

void save_package_data_as_status_file(string_map* package_data, char* status_file_path);


#endif
