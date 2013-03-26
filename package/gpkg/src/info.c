#include "gpkg.h"

void do_print_dest_info(opkg_conf* conf, int format)
{
	//output
	if(format == OUTPUT_JSON)
	{
		printf("{\n");

	}
	else if(format == OUTPUT_JAVASCRIPT)
	{
		printf("var pkg_dests = [];\n");
	}

	unsigned long num_dests;
	unsigned long dest_index;
	char** dests = get_string_map_keys(conf->dest_freespace, &num_dests);
	for(dest_index=0 ; dest_index < num_dests; dest_index++)
	{
		char* dest_name          = dests[dest_index];
		char* dest_root          = (char*)get_string_map_element(conf->dest_names, dest_name);
		uint64_t* dest_freespace  = (uint64_t*)get_string_map_element(conf->dest_freespace, dest_name);
		uint64_t* dest_totalspace = (uint64_t*)get_string_map_element(conf->dest_totalspace, dest_name);
		if(format == OUTPUT_JSON)
		{
			if(dest_index >0){ printf(",\n"); }
			printf("\t\"%s\": {\n", dest_name);
			printf("\t\t\"Root\": \"%s\",\n", dest_root);
			printf("\t\t\"Bytes-Free\": "SCANFU64",\n", *dest_freespace);
			printf("\t\t\"Bytes-Total\": "SCANFU64"\n", *dest_totalspace);
			printf("\t}");
		}
		else if(format == OUTPUT_JAVASCRIPT)
		{
			printf("pkg_dests['%s'] = [];\n", dest_name);
			printf("pkg_dests['%s']['Root'] = '%s';\n", dest_name, dest_root);
			printf("pkg_dests['%s']['Bytes-Free']  = "SCANFU64"\n", dest_name, *dest_freespace);
			printf("pkg_dests['%s']['Bytes-Total'] = "SCANFU64"\n", dest_name, *dest_totalspace);
		}
		else
		{
			printf("Destination: %s", dest_name);
			printf("Destination-Root: %s\n", dest_root);
			printf("Destination-Bytes-Free: "SCANFU64"\n", *dest_freespace);
			printf("Destination-Bytes-Total: "SCANFU64"\n", *dest_totalspace);
			printf("\n");
		}
	}
	if(format == OUTPUT_JSON)
	{
		printf("}\n");

	}


}

