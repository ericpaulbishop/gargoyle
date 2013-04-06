
#include "gpkg.h"

opkg_conf* load_conf(const char* conf_file_name)
{
	char* dupe_conf_file_name = conf_file_name == NULL ? strdup(DEFAULT_CONF_FILE_PATH) : strdup(conf_file_name);

	opkg_conf* conf = (opkg_conf*)malloc(sizeof(opkg_conf));

	conf->gzip_sources    = initialize_string_map(1);
	conf->plain_sources   = initialize_string_map(1);

	conf->overlays        = initialize_string_map(1);

	conf->dest_roots      = initialize_string_map(1);
	conf->dest_names      = initialize_string_map(1);
	conf->dest_freespace  = initialize_string_map(1);
	conf->dest_totalspace = initialize_string_map(1);


	conf->lists_dir = strdup("/usr/lib/opkg/lists");

	FILE* conf_file =  fopen(dupe_conf_file_name, "r");
	int read_data = 1;
	char next_line[1024];
	char* overlay_root = NULL;
	if(conf_file == NULL)
	{
		fprintf(stdout, "ERROR: couldn't open conf file \"%s\"\n", conf_file_name);
		exit(1);
	}
	while(read_data > 0)
	{
		char* tmp;
		next_line[0] = '\0';
		tmp = fgets(next_line, 1024, conf_file);
		if(next_line[0] != '\0')
		{
			unsigned long num_pieces=0;
			char whitespace[] = "\r\n\t "; //include newlines to strip them off end of line
			char** split_line = split_on_separators(next_line, whitespace, 4, -1, 0, &num_pieces);
			if(num_pieces >= 2)
			{
				if(strstr(split_line[0], "dest") == split_line[0])
				{
					struct statvfs fs_data;
					uint64_t* free_bytes  = (uint64_t*)malloc(sizeof(uint64_t));
					uint64_t* total_bytes = (uint64_t*)malloc(sizeof(uint64_t));
					*free_bytes = 0;
					if( statvfs(split_line[num_pieces-1], &fs_data) == 0 )
					{
						uint64_t block_size   = (uint64_t)fs_data.f_frsize;
						uint64_t blocks_free  = (uint64_t)fs_data.f_bavail;
						uint64_t blocks_total = (uint64_t)fs_data.f_blocks;

						*free_bytes  = block_size*blocks_free;
						*total_bytes = block_size*blocks_total;
					}


					//key = file path, value = name
					set_string_map_element(conf->dest_roots, split_line[num_pieces-1], strdup(split_line[num_pieces-2]));

					//key = name, value = file path
					set_string_map_element(conf->dest_names, split_line[num_pieces-2], strdup(split_line[num_pieces-1]));

					//key = name, value = size (uint64_t)
					set_string_map_element(conf->dest_freespace,  split_line[num_pieces-2], free_bytes);
					set_string_map_element(conf->dest_totalspace, split_line[num_pieces-2], total_bytes);

				}
				else if(strcmp(split_line[0], "lists_dir") == 0 && num_pieces > 2)
				{
					free(conf->lists_dir);
					conf->lists_dir = strdup(split_line[num_pieces-1]);
				}
				else if(strcmp(split_line[0], "src/gz") == 0)
				{
					set_string_map_element(conf->gzip_sources, split_line[num_pieces-2], strdup(split_line[num_pieces-1]));
				}
				else if(strcmp(split_line[0], "src") == 0 && num_pieces > 2)
				{
					set_string_map_element(conf->plain_sources, split_line[num_pieces-2], strdup(split_line[num_pieces-1]));
				}
				else if( strcmp(split_line[0], "option") == 0 && strstr(split_line[1], "overlay_") != NULL && num_pieces > 2)
				{
					char* overlay_name = strstr(split_line[1], "overlay_") + strlen("overlay_");
					if(path_exists(split_line[num_pieces-1]))
					{
						set_string_map_element(conf->overlays, overlay_name, strdup(split_line[num_pieces-1]));
					}
				}
			}
			free_null_terminated_string_array(split_line);
		}
		else
		{
			read_data = 0;
		}
	}
	fclose(conf_file);


	free(dupe_conf_file_name);

	return conf;

}

void free_conf(opkg_conf* conf)
{
	unsigned long num_freed;
	free(conf->lists_dir);

	destroy_string_map(conf->gzip_sources,    DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->plain_sources,   DESTROY_MODE_FREE_VALUES, &num_freed);

	destroy_string_map(conf->overlays,   DESTROY_MODE_FREE_VALUES, &num_freed);
	
	destroy_string_map(conf->dest_names,      DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_roots,      DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_freespace,  DESTROY_MODE_FREE_VALUES, &num_freed);
	destroy_string_map(conf->dest_totalspace, DESTROY_MODE_FREE_VALUES, &num_freed);

	free(conf);
}


