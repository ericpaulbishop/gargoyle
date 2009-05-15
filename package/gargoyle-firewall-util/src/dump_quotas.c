#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#include <erics_tools.h>
#include <uci.h>

char** get_section_list(struct uci_context* ctx, char* package, char* list_section, char* quota_type);
void dump_quota_chain(struct uci_context* ctx, char* table, char* chain, char* package, char** type_sections, char* type_option, time_t now);

int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name);
char* get_option_value_string(struct uci_option* uopt);


int main()
{
	struct uci_context *ctx = uci_alloc_context();
	char** ingress_list  = get_section_list(ctx, "firewall", "quota_order", "ingress_quota_sections");
	char** egress_list   = get_section_list(ctx, "firewall", "quota_order", "egress_quota_sections");
	char** combined_list = get_section_list(ctx, "firewall", "quota_order", "combined_quota_sections");
	
	time_t now;
	time(&now);

	dump_quota_chain(ctx, "mangle", "ingress_quotas",  "firewall", ingress_list,  "ingress_used", now);
	dump_quota_chain(ctx, "mangle", "egress_quotas",   "firewall", egress_list,   "egress_used", now);
	dump_quota_chain(ctx, "mangle", "combined_quotas", "firewall", combined_list, "combined_used", now);


	struct uci_ptr ptr;
	if (uci_lookup_ptr(ctx, &ptr, "firewall", true) == UCI_OK)
	{
		uci_commit(ctx, &ptr.p, false);
	}
	uci_free_context(ctx);

	return 0;
}

char** get_section_list(struct uci_context *ctx, char* package, char* list_section, char* quota_type)
{
	char** section_list = NULL;

	struct uci_package *p = NULL;
	struct uci_element *e = NULL;
	if(get_uci_option(ctx, &e, p, package, list_section, quota_type) == UCI_OK)
	{
		char* list_str = get_option_value_string(uci_to_option(e));
		char spacers[] = {' ', '\t'};
		section_list = split_on_separators(list_str, spacers, 2, -1, 0);
		free(list_str);
	}
	
	return section_list;
}

void dump_quota_chain(struct uci_context *ctx, char* table, char* chain, char* package, char** type_sections, char* type_option, time_t now)
{
	char line[1024];
	char* output_command;
	FILE *output;
	list* quota_vals = initialize_list();
	char search_str[] = "--current_bandwidth ";
	int search_length = strlen(search_str);

	output_command = dynamic_strcat(4, "iptables -t ", table, " -L ", chain);

	output = popen(output_command, "r");
	while (fgets(line, 1024, output) != NULL)
	{
		char* cur_str = strstr(line, search_str);
		if(cur_str != NULL)
		{
			int num_read;
			u_int64_t *next_len = (u_int64_t*)malloc(sizeof(u_int64_t));
			cur_str = cur_str + search_length;
			num_read = sscanf(cur_str, "%lld", next_len);
			if(num_read > 0)
			{
				push_list(quota_vals, next_len);
			}
			else
			{
				free(next_len);
			}
		}
	}
	pclose(output);
	free(output_command);
	
	
	struct uci_ptr ptr;
	if(quota_vals != NULL && type_sections != NULL)
	{
		int section_index;
		for(section_index=0; type_sections[section_index] != NULL && quota_vals->length > 0; section_index++)
		{
			char used_id[1024];
			char time_id[1024];
			u_int64_t *next_len = (u_int64_t*)shift_list(quota_vals);
	       		sprintf(used_id, "firewall.%s.%s=%lld", type_sections[section_index], type_option, *next_len);
			sprintf(time_id, "firewall.%s.last_backup_time=%ld", type_sections[section_index], now);
			if (uci_lookup_ptr(ctx, &ptr, used_id, true) == UCI_OK){ uci_set(ctx, &ptr); }
			if (uci_lookup_ptr(ctx, &ptr, time_id, true) == UCI_OK){ uci_set(ctx, &ptr); }

			// printf("%d) %s %s = %lld bytes\n", section_index, type_sections[section_index], type_option, *next_len);
		}
	}	
}



int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name)
{
	struct uci_ptr ptr;
	char* lookup_str = dynamic_strcat(5, package_name, ".", section_name, ".", option_name);
	int ret_value = uci_lookup_ptr(ctx, &ptr, lookup_str, 1);
	if(ret_value == UCI_OK)
	{
		if( !(ptr.flags & UCI_LOOKUP_COMPLETE))
		{
			ret_value = UCI_ERR_NOTFOUND;
		}
		else
		{
			*e = (struct uci_element*)ptr.o;
		}
	}
	free(lookup_str);

	return ret_value;
}



// this function dynamically allocates memory for
// the option string, but since this program exits
// almost immediately (after printing variable info)
// the massive memory leak we're opening up shouldn't
// cause any problems.  This is your reminder/warning
// that this might be an issue if you use this code to
// do anything fancy.
char* get_option_value_string(struct uci_option* uopt)
{
	char* opt_str = NULL;
	if(uopt->type == UCI_TYPE_STRING)
	{
		opt_str = strdup(uopt->v.string);
	}
	if(uopt->type == UCI_TYPE_LIST)
	{
		struct uci_element* e;
		uci_foreach_element(&uopt->v.list, e)
		{
			if(opt_str == NULL)
			{
				opt_str = strdup(e->name);
			}
			else
			{
				char* tmp;
				tmp = dynamic_strcat(3, opt_str, " ", e->name);
				free(opt_str);
				opt_str = tmp;
			}
		}
	}

	return opt_str;
}


