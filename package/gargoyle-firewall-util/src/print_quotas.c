/*  backup_quotas --	Used to backup quota data from iptables rules that use the "bandwidth" module
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009 by Eric Bishop <eric@gargoyle-router.com>
 *
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <erics_tools.h>
#include <uci.h>
#include <ipt_bwctl.h>

list* get_all_sections_of_type(struct uci_context *ctx, char* package, char* section_type);
void  backup_quota(char* quota_id, char* quota_backup_dir);
char* get_uci_option(struct uci_context* ctx,char* package_name, char* section_name, char* option_name);
char* get_option_value_string(struct uci_option* uopt);


int main(void)
{
	struct uci_context *ctx = uci_alloc_context();
	list* quota_sections = get_all_sections_of_type(ctx, "firewall", "quota");
	system("mkdir -p /usr/data/quotas");
	unlock_bandwidth_semaphore_on_exit();

	/* for each ip have uint64_t[6], */
	string_map *ip_to_bandwidth = initialize_string_map(1);	
	while(quota_sections->length > 0)
	{
		char* next_quota = shift_list(quota_sections);
		

		/* base id for quota is the ip associated with it*/
		char *ip = get_uci_option(ctx, "firewall", next_quota, "ip");
			
		if(ip == NULL)
		{
			ip = strdup("ALL");
		}
		else if(strcmp(ip, "") == 0)
		{
			ip = strdup("ALL");
		}

			
		char* types[] = { "ingress_limit", "egress_limit", "combined_limit" };
		char* postfixes[] = { "_ingress", "_egress", "_combined" };
		
		
		int type_index;
		for(type_index=0; type_index < 3; type_index++)
		{
			char* defined = get_uci_option(ctx, "firewall", next_quota, types[type_index]);
			if(defined != NULL)
			{
				char* type_id = dynamic_strcat(2, ip, postfixes[type_index]);
				ip_bw* ip_buf;
				unsigned long num_ips = 0;
				int query_succeeded = get_all_bandwidth_usage_for_rule_id(type_id, &num_ips, &ip_buf, 5000);
				if(query_succeeded && num_ips > 0)
				{
					unsigned long ip_index = 0;
					for(ip_index = 0; ip_index < num_ips; ip_index++)
					{
						ip_bw next = ip_buf[ip_index];
						char* next_ip = NULL;
						if(next.ip == 0)
						{
							next_ip = strdup(ip);
						}
						else
						{
							struct in_addr addr;
							addr.s_addr = next.ip;
							next_ip = strdup(inet_ntoa(addr));
						}
						uint64_t *bw_list = get_string_map_element(ip_to_bandwidth,next_ip);
						if(bw_list == NULL)
						{
							bw_list = (uint64_t*)malloc(sizeof(uint64_t)*6);
							bw_list[0] = 0;
							bw_list[1] = 0;
							bw_list[2] = 0;
							bw_list[3] = 0;
							bw_list[4] = 0;
							bw_list[5] = 0;
							set_string_map_element(ip_to_bandwidth, next_ip, bw_list);
						}
						bw_list[type_index] = 1;
						bw_list[type_index+3] = next.bw;
					}
				}
				free(type_id);
				free(defined);

			}
		}
		free(ip);
		free(next_quota);
	}
	

	unsigned long num_ips;
	char** ip_list = (char**)get_string_map_keys(ip_to_bandwidth, &num_ips);
	printf("var quota_ip_list = [ ");
	int print_comma = 0;
	unsigned long ip_index;
	for(ip_index=0; ip_index < num_ips; ip_index++)
	{
		if(print_comma)
		{
			printf(", \"%s\"", ip_list[ip_index]);
		}
		else
		{
			print_comma = 1;
			printf("\"%s\"", ip_list[ip_index]);
		}
	}
	printf(" ];\n");
	printf("var quota_defs = new Array();\n");
	for(ip_index=0; ip_index < num_ips; ip_index++)
	{
		char* next_ip = ip_list[ip_index];
		uint64_t* def = get_string_map_element(ip_to_bandwidth, next_ip);
		if(def != NULL)
		{
			print_comma = 0;
			printf("quota_defs[ \"%s\" ] = [ ", next_ip);
			int type_index;
			for(type_index=0; type_index < 3; type_index++)
			{
				if(print_comma)
				{
					printf(", ");
				}
				else
				{
					print_comma = 1;
				}
				if(!def[type_index])
				{
					printf("-1");
				}
				else
				{
					printf("%lld", (long long int)def[type_index+3]);
				}
			}
			printf(" ];\n");
		}
	}


	unsigned long num;
	destroy_list(quota_sections, DESTROY_MODE_FREE_VALUES, &num);
	uci_free_context(ctx);

	return 0;
}

list* get_all_sections_of_type(struct uci_context *ctx, char* package, char* section_type)
{

	struct uci_package *p = NULL;
	struct uci_element *e = NULL;

	list* sections_of_type = initialize_list();
	if(uci_load(ctx, package, &p) == UCI_OK)
	{
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			if(safe_strcmp(section->type, section_type) == 0)
			{
				push_list(sections_of_type, strdup(section->e.name));
			}
		}
	}
	return sections_of_type;
}


char* get_uci_option(struct uci_context* ctx, char* package_name, char* section_name, char* option_name)
{
	char* option_value = NULL;
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
			struct uci_element *e = (struct uci_element*)ptr.o;
			option_value = get_option_value_string(uci_to_option(e));
		}
	}
	free(lookup_str);

	return option_value;
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


