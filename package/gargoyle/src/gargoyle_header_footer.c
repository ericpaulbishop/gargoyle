/*
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>


#include "uci.h"
#include "priority_queue.h"
#include "string_util.h"

#define MINIMAL_HEADER 1
#define HEADER 2
#define FOOTER 3

void define_package_vars(char** package_vars_to_load);
void print_interface_vars(void);
void print_js_var(char* var, char* value);
char* get_interface_mac(char* if_name);
char* get_interface_ip(char* if_name);
char** load_interfaces_from_proc_file(char* filename);


int main(int argc, char **argv)
{
	int display_type = HEADER;
	int display_interface_vars = 0;	
	char* selected_page = "";
	char* selected_section = "";
	char* css_includes = "";
	char* js_includes = "";
	char* title = "Gargoyle Router Management Utility";
	char** package_variables_to_load = NULL;
	int c;
	
	while((c = getopt(argc, argv, "MmHhFfS:s:P:p:C:c:J:j:T:t:Ii")) != -1) //section, page, css includes, javascript includes, title, output interface variables
	{
		switch(c)
		{
			case 'M':
			case 'm':
				display_type = MINIMAL_HEADER;
				break;
			case 'H':
			case 'h':
				display_type = HEADER;
				break;
			case 'F':
			case 'f':
				display_type = FOOTER;
				break;
			case 'P':
			case 'p':
				selected_page=strdup(optarg);
				break;
			case 'S':
			case 's':
				selected_section=strdup(optarg);
				break;
			case 'C':
			case 'c':
				css_includes = strdup(optarg);
				break;
			case 'J':
			case 'j':
				js_includes = strdup(optarg);
				break;
			case 'T':
			case 't':
				title = strdup(optarg);
				break;
			case 'I':
			case 'i':
				display_interface_vars = 1;
				break;
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS] [UCI SECTIONS]\n", argv[0]);
				printf("\t-m Generate Minimal Header (for popup boxes used for editing table rows)");
				printf("\t-h Generate Standard Header");
				printf("\t-f Generate Footer");
				printf("\t-s [SECTION-ID] Section/Main Menu Id");
				printf("\t-p [PAGE-ID] Page Id");
				printf("\t-c [CSS FILES] List of additional css files necessary for page");
				printf("\t-j [JS FILES] List of additional javascript files necessary for page");
				printf("\t-t [TITLE] Title of page");
				printf("\t-i Include output of javascript variables that specify network interface ips and MAC addresses	");
				printf("\t-u print usage and exit\n");
				return 0;
		}
	}
	
	if(optind < argc)
	{
		int packages_length = argc-optind;
		package_variables_to_load = (char**)malloc((1+packages_length)*sizeof(char*));
		int arg_index;
		for(arg_index =optind; arg_index < argc; arg_index++)
		{
			package_variables_to_load[arg_index-optind] = strdup(argv[arg_index]);
		}
		package_variables_to_load[packages_length] = NULL;
	}
	
	
	
	struct uci_context *ctx;
	ctx = uci_alloc_context();

	
	struct uci_package *p = NULL;
	struct uci_element *e = NULL;
	
	
	if(display_type == HEADER || display_type == MINIMAL_HEADER)
	{
		printf("Content-type: text/html\n\n");

		if(uci_load(ctx, "gargoyle", &p) != UCI_OK)
		{
			printf("ERROR: no gargoyle package defined!\n");
			return 0;
		}

		char* theme_root = "themes";
		char* theme = "default";
		char* js_root = "js";
		char* web_root = "/www";
		char* bin_root = ".";
		if(uci_lookup(ctx, &e, p, "global", "theme_root") == UCI_OK)
		{
			theme_root=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "global", "theme") == UCI_OK)
		{
			theme=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "global", "js_root") == UCI_OK)
		{
			js_root=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "global", "web_root") == UCI_OK)
		{
			web_root=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "global", "bin_root") == UCI_OK)
		{
			bin_root=strdup(uci_to_option(e)->value);
		}

		char** all_css;
		char** all_js;
		char whitespace_separators[] = { '\t', ' ' };
		if(uci_lookup(ctx, &e, p, "global", "common_css") == UCI_OK && display_type == HEADER)
		{
			char* css_list = dynamic_strcat(3, uci_to_option(e)->value, " ", css_includes);
			all_css=split_on_separators(css_list, whitespace_separators, 2, -1, 0);
			free(css_list);
		}
		else
		{
			all_css=split_on_separators(css_includes, whitespace_separators, 2, -1, 0);
		}
		
		
		if(uci_lookup(ctx, &e, p, "global", "common_js") == UCI_OK)
		{
			char* js_list = dynamic_strcat(3, uci_to_option(e)->value, " ", js_includes);
			all_js=split_on_separators(js_list, whitespace_separators, 2, -1, 0);
			free(js_list);
		}
		else
		{
			all_js=split_on_separators(js_includes, whitespace_separators, 2, -1, 0);
		}
		
		
		if(uci_load(ctx, "system", &p) != UCI_OK)
		{
			printf("ERROR: no system package defined!\n");
			return 0;
		}
		char* hostname = "";
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			if(strcmp(section->type,"system")==0)
			{
				struct uci_element *e2;
				uci_foreach_element(&section->options, e2) 
				{
					if(strcmp(e2->name,"hostname")==0)
					{
						hostname = strdup(uci_to_option(e2)->value);
					}
				}
			}
		}
		


		printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
		printf("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
		printf("<head>\n");
		printf("\t<title>%s</title>\n", title);
		int css_index, js_index;
		for(css_index=0; all_css[css_index] != NULL; css_index++)
		{
			printf("\t<link rel=\"stylesheet\" href=\"/%s/%s/%s\" type=\"text/css\" />\n", theme_root, theme, all_css[css_index]);
		}
		for(js_index=0; all_js[js_index] != NULL; js_index++)
		{
			printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"/%s/%s\"></script>\n", js_root, all_js[js_index]);
		}
		printf("</head>\n");
		printf("<body>\n");

		if(display_type == HEADER)
		{
			printf("\t<div id=\"outer_logo\">\n");
			printf("\t\t<div id=\"inner_logo\">\n");
			printf("\t\t\t<div id=\"garg_title\">Gargoyle</div>\n");
			printf("\t\t\t<div id=\"garg_desc\">Router<br />Management<br />Utility</div>\n");
			printf("\t\t\t<div id=\"garg_host\">Device Name: %s</div>\n", hostname);
			printf("\t\t</div>\n");
			printf("\t</div>\n");

			printf("\t<div id=\"outer_header\"></div>\n");
			printf("\t<div id=\"outer_container\">\n");
			printf("\t\t<div id=\"main_external_container\">\n");
			printf("\t\t\t<div id=\"main_top\"></div>\n");
			printf("\t\t\t<div id=\"main_internal_container\">\n");
			printf("\n\n");
		}
		printf("<script>\n");
		printf("<!--\n");
		printf("\tvar gargoyleBinRoot = \"%s/%s\";\n", web_root, bin_root);
		if(display_interface_vars == 1)
		{
			print_interface_vars();
		}
		define_package_vars(package_variables_to_load);
		printf("\n\tvar testAjax = getRequestObj();\n");
		printf("\tif(!testAjax) { window.location = \"no_ajax.sh\"; }\n");
		printf("//-->\n");
		printf("</script>\n");
		printf("\n\n");
	
	}
	else if(display_type == FOOTER)
	{
		if(uci_load(ctx, "gargoyle", &p) != UCI_OK)
		{
			printf("ERROR: no gargoyle package defined!\n");
			return 0;
		}
	
		char* web_root = "/www";
		char* bin_root = ".";
		if(uci_lookup(ctx, &e, p, "global", "web_root") == UCI_OK)
		{
			web_root=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "global", "bin_root") == UCI_OK)
		{
			bin_root=strdup(uci_to_option(e)->value);
		}



		
		priority_queue sections = initialize_priority_queue();
		
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			int section_rank;
			if(sscanf(section->type, "%d", &section_rank) > 0)
			{
				priority_queue section_pages =  initialize_priority_queue();
					
				struct uci_element *e2;
				uci_foreach_element(&section->options, e2) 
				{
					int page_rank;
					if(sscanf(uci_to_option(e2)->value, "%d", &page_rank) > 0)
					{
						insert_priority_node(section_pages, strdup(e2->name), page_rank, NULL); 
					}
				}
				insert_priority_node(sections, strdup(section->e.name),  section_rank, section_pages);
			}
		}

		
		printf("\t\t\t</div>\n");
		printf("\t\t\t<div id=\"main_bottom\"></div>\n");
		printf("\t\t</div>\n");
		printf("\t\t<div id=\"nav_external_container\">\n");
		printf("\t\t\t<div id=\"nav_top\"></div>\n");
		printf("\t\t\t<div id=\"nav_internal_container1\">\n");
		printf("\t\t\t\t<div id=\"nav_internal_container2\">\n");
		printf("\t\t\t\t\t<div class=\"nav_internal_end1\"></div>\n");
		
		int first_section=1;
		int prev_section_selected=0;
		priority_queue_node *next_section;
		while( (next_section=pop_priority_queue(sections)) != NULL)
		{
			char* section_display = "";
			priority_queue section_pages;
		       	section_pages	= (priority_queue)next_section->data;
			if(uci_lookup(ctx, &e, p, "display", next_section->id) == UCI_OK)
			{
				section_display = strdup(uci_to_option(e)->value);
			}

			

			if(first_section == 0 && strcmp(selected_section, next_section->id) != 0)
			{
				if(prev_section_selected == 0)
				{
					printf("\t\t\t\t\t<div class=\"nav_unselected_divider\"></div>\n");
				}
				else
				{
					printf("\t\t\t\t\t<div class=\"nav_selected_divider2\"></div>\n");
				}
			}
			if(strcmp(selected_section, next_section->id) == 0)
			{
				if(first_section == 1)
				{
					printf("\t\t\t\t\t<div class=\"nav_selected_divider_end1\"></div>\n");
					printf("\t\t\t\t\t<div class=\"nav_selected_end1\">\n");
					printf("\t\t\t\t\t\t<div class=\"nav_selected_container_end1\">\n");
				}
				else
				{
					printf("\t\t\t\t\t<div class=\"nav_selected_divider1\"></div>\n");
					printf("\t\t\t\t\t<div class=\"nav_selected\">\n");
					printf("\t\t\t\t\t\t<div class=\"nav_selected_container\">\n");
				}



				if(get_first_in_priority_queue(section_pages) == NULL)
				{
					printf("\t\t\t\t\t\t\t<div class=\"selected_single_header\">%s</div>\n", section_display);
				}
				else
				{
					printf("\t\t\t\t\t\t\t<div class=\"selected_header\">%s</div>\n", section_display);
					printf("\t\t\t\t\t\t\t<div id=\"submenu_container\">\n");
					priority_queue_node *next_section_page;
					while( (next_section_page=pop_priority_queue(section_pages)) != NULL)
					{
						char* page_display="";
						char* page_script="";
						char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
						if(uci_lookup(ctx, &e, p, "display", lookup) == UCI_OK)
						{
							page_display = strdup(uci_to_option(e)->value);
						}
						if(uci_lookup(ctx, &e, p, "scripts", lookup) == UCI_OK)
						{
							page_script = strdup(uci_to_option(e)->value);
						}
						if(strcmp(next_section_page->id, selected_page)==0)
						{
							printf("\t\t\t\t\t\t\t\t<div class=\"submenu_selected\">%s</div>\n", page_display);
						}
						else
						{
							printf("\t\t\t\t\t\t\t\t<a href=\"/%s/%s\">%s</a>\n", bin_root, page_script, page_display);
						}
						free(lookup);
					}
					printf("\t\t\t\t\t\t\t</div>\n");
				}
				printf("\t\t\t\t\t\t</div>\n");
				printf("\t\t\t\t\t</div>\n");
				prev_section_selected=1;
			}
			else
			{
				if(first_section == 1)
				{
					printf("\t\t\t\t\t<div class=\"nav_unselected_divider_end1\"></div>\n");
					printf("\t\t\t\t\t<div class=\"nav_unselected_end1\">\n");
				}
				else
				{
					printf("\t\t\t\t\t<div class=\"nav_unselected\">\n");
				}
				
				priority_queue_node *next_section_page;
				char* next_section_script = "";
				if( (next_section_page=pop_priority_queue(section_pages)) != NULL)
				{
					char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
					if(uci_lookup(ctx, &e, p, "scripts", lookup) == UCI_OK)
					{
						next_section_script = strdup(uci_to_option(e)->value);
					}
					free(lookup);
				}
				else
				{
					if(uci_lookup(ctx, &e, p, "scripts", next_section->id) == UCI_OK)
					{
						next_section_script = strdup(uci_to_option(e)->value);
					}
				}
				printf("\t\t\t\t\t\t<a href=\"/%s/%s\">%s</a>\n", bin_root, next_section_script, section_display);
				printf("\t\t\t\t\t</div>\n");
				prev_section_selected=0;
			}
			first_section = 0;	
		}
		if(prev_section_selected ==1 )
		{
			printf("\t\t\t\t\t<div class=\"nav_selected_divider_end2\"></div>\n");
		}
		else
		{
			printf("\t\t\t\t\t<div class=\"nav_unselected_divider_end2\"></div>\n");
		}
	
		printf("\t\t\t\t\t<div class=\"nav_internal_end2\"></div>\n");
		printf("\t\t\t\t</div>\n");
		printf("\t\t\t</div>\n");
		printf("\t\t\t<div id=\"nav_bottom\"></div>\n");
		printf("\t\t</div>\n");
		printf("\t</div>\n");
		printf("\t<div id=\"outer_footer\"></div>\n");
		printf("</body>\n");
		printf("</html>\n");	
	}	
			
	uci_free_context(ctx);
}	

void define_package_vars(char** package_vars_to_load)
{
	int loaded = 0;
	if(package_vars_to_load != NULL)
	{
		int package_index;
		
		for(package_index=0; package_vars_to_load[package_index] != NULL; package_index++)
		{
			struct uci_context *ctx;
			ctx = uci_alloc_context();
			struct uci_package *p = NULL;
			struct uci_element *e = NULL;
			if(uci_load(ctx, package_vars_to_load[package_index], &p) == UCI_OK)
			{
				if(loaded == 0)
				{
					printf("\tvar uciOriginal = new UCIContainer();\n");
					loaded = 1;
				}
				uci_foreach_element( &p->sections, e)
				{
					struct uci_section *section = uci_to_section(e);
					
					printf("\tuciOriginal.set(\"%s\", \"%s\", \"\", \"%s\");\n",package_vars_to_load[package_index], section->e.name, section->type);
					
					struct uci_element *e2;
					uci_foreach_element(&section->options, e2) 	
					{
						printf("\tuciOriginal.set(\"%s\", \"%s\", \"%s\", \"%s\");\n",package_vars_to_load[package_index], section->e.name, e2->name, uci_to_option(e2)->value);
					}
				
				}
			}
		}
	}	
}

void print_interface_vars(void)
{
	// load interface names from /proc/net/dev
	// and wireless names from /proc/net/wireless
	//
	// this is wicked trashy (highly non-portable)
	// but 1) it works, 2) deals with the problem 
	// of managing switched interfaces and 3) deals with
	// the problem of determining wifi interfaces

	//load wireless interface name
	char* wireless_if = NULL;
	char** wireless_ifs = load_interfaces_from_proc_file("/proc/net/wireless");
	if(wireless_ifs[0] != NULL)
	{
		wireless_if = strdup(wireless_ifs[0]);
	}
	int wif_index = 0;
	for(wif_index = 0; wireless_ifs[wif_index] != NULL; wif_index++)
	{
		free(wireless_ifs[wif_index]);
	}
	free(wireless_ifs);


	// load lan & wan interface names
	// because we want DEFAULT interfaces, we can't load from config
	// by default lan interface is first eth* interface and wan is second
	// If exceptions to this are found, they should be hard-coded in here
	// to correct issues.
	char** interfaces = load_interfaces_from_proc_file("/proc/net/dev");
	
	int have_switched_eths=0;
	int interface_index; 
	for(interface_index=0; interfaces[interface_index] != NULL && have_switched_eths == 0; interface_index++)
	{
		have_switched_eths = strstr(interfaces[interface_index], "eth") != NULL && strstr(interfaces[interface_index], ".") != NULL ? 1 : 0;
	}

	
	char* default_lan_if = NULL;
	char* default_wan_if = NULL;
	for(interface_index=0; interfaces[interface_index] != NULL; interface_index++)
	{
		if(	strstr(interfaces[interface_index], "eth") != NULL && 
			(strstr(interfaces[interface_index], ".") != NULL || have_switched_eths == 0)
		  )
		{
			if(default_lan_if == NULL)
			{
				default_lan_if = strdup(interfaces[interface_index]);
			}
			else if(default_wan_if == NULL)
			{
				default_wan_if = strdup(interfaces[interface_index]);
			}
		}
		free(interfaces[interface_index]);
	}
	free(interfaces);

	//if we have switched eths but default_wan_if is not defined,
	//then we probably have just disabled it.  We wouldn't have an eth0.0
	//active if eth0.1 were not potentially available (it would just be eth0)
	if(have_switched_eths == 1 && default_wan_if == NULL)
	{
		default_wan_if = "eth0.1";
	}


	/*
	 * Load UCI interface variables
	*/
	char* uci_wan_mac = NULL;
	char* uci_wan_if = NULL;
	char* uci_lan_if = NULL;
	char* uci_lan_ip = NULL;
	char* uci_wireless = NULL;

	struct uci_context *ctx;
	ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	struct uci_element *e = NULL;
	if(uci_load(ctx, "network", &p) == UCI_OK)
	{
		if(uci_lookup(ctx, &e, p, "wan", "macaddr") == UCI_OK)
		{
			uci_wan_mac=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "wan", "ifname") == UCI_OK)
		{
			uci_wan_if=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "lan", "ifname") == UCI_OK)
		{
			uci_lan_if=strdup(uci_to_option(e)->value);
		}
		if(uci_lookup(ctx, &e, p, "lan", "ifaddr") == UCI_OK)
		{
			uci_lan_ip=strdup(uci_to_option(e)->value);
		}
	}
	if(uci_load(ctx, "wireless", &p) == UCI_OK)
	{
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			if(strstr(section->type, "wifi-device") != NULL)
			{
				uci_wireless=strdup(section->e.name);
			}
		}
	}

	char* current_lan_mac = get_interface_mac(uci_lan_if);
	char* current_lan_ip = uci_lan_ip != NULL ? uci_lan_ip : get_interface_ip("br-lan");
	char* current_wan_mac = get_interface_mac(uci_wan_if);
	char* current_wan_ip = get_interface_ip(uci_wan_if);
	char* current_wireless_mac = get_interface_mac(wireless_if);

	char* default_wan_mac = NULL;
	if(default_wan_if != NULL && uci_wan_mac == NULL)
	{
		default_wan_mac = get_interface_mac(default_wan_if);
		if(default_wan_mac == NULL)
		{
			default_wan_mac = strdup("00:11:22:33:44:55");
		}
	}
	else if(default_wan_if == NULL)
	{
		default_wan_mac = strdup("00:11:22:33:44:55");
	}
	else if(uci_wan_mac != NULL)
	{
		//determine lan_mac + 1
		char* inc_mac = strdup(current_lan_mac);
		int end;
		sscanf(inc_mac+15, "%X", &end);
		end = (end+1) % 0x100;
		sprintf(inc_mac+15, "%.2X", end);
		default_wan_mac = inc_mac;
	}


	/*
	printf("wireless_if=%s\n", wireless_if);
	printf("uci_wireless=%s\n", uci_wireless);
	printf("current_wireless_mac=%s\n\n", current_wireless_mac);


	printf("default_lan_if=%s\n", default_lan_if);
	printf("current_lan_if=%s\n", uci_lan_if);
	printf("current_lan_mac=%s\n", current_lan_mac);
	printf("current_lan_ip=%s\n\n", current_lan_ip);


	
	printf("default_wan_if=%s\n", default_wan_if);
	printf("current_wan_if=%s\n", uci_wan_if);
	printf("default_wan_mac=%s\n", default_wan_mac);
	printf("current_wan_mac=%s\n", current_wan_mac);
	printf("current_wan_ip=%s\n\n", current_wan_ip);
	*/


	print_js_var("wirelessIf", wireless_if);
	print_js_var("uciWireless", uci_wireless);
	print_js_var("currentWirelessMac", current_wireless_mac);


	print_js_var("defaultLanIf", default_lan_if);
	print_js_var("currentLanIf", uci_lan_if);
	print_js_var("currentLanMac", current_lan_mac);
	print_js_var("currentLanIp", current_lan_ip);


	print_js_var("defaultWanIf", default_wan_if);
	print_js_var("currentWanIf", uci_wan_if);
	print_js_var("defaultWanMac", default_wan_mac);
	print_js_var("currentWanMac", current_wan_mac);
	print_js_var("currentWanIp", current_wan_ip);

}



void print_js_var(char* var, char* value)
{
	if(value != NULL)
	{
		printf("\tvar %s = \"%s\";\n", var, value);
	}
	else
	{
		printf("\tvar %s = \"\";\n", var);
	}
}

char* get_interface_mac(char* if_name)
{
	char* mac = NULL;
	if(if_name != NULL)
	{
		struct ifreq buffer;
		int s = socket(PF_INET, SOCK_DGRAM, 0);
		memset(&buffer, 0x00, sizeof(buffer));
		strcpy(buffer.ifr_name, if_name);
	
		if(ioctl(s, SIOCGIFHWADDR, &buffer) != -1)
		{
			mac = (char*)malloc(19*sizeof(char));
			int mac_index=0;
			for(mac_index = 0; mac_index < 6; mac_index++ )
			{
				sprintf(mac+(3*mac_index), "%.2X:", (unsigned char)buffer.ifr_hwaddr.sa_data[mac_index]);
			}
			mac[17] = '\0';
		}	
		close(s);
	}
	return mac;
}


char* get_interface_ip(char* if_name)
{
	char* ip = NULL;
	if(if_name != NULL)
	{
		struct ifreq buffer;
		int s = socket(PF_INET, SOCK_DGRAM, 0);
		memset(&buffer, 0x00, sizeof(buffer));
		strcpy(buffer.ifr_name, if_name);
	
		if(ioctl(s, SIOCGIFADDR, &buffer) != -1)
		{
			struct sockaddr_in *addr = (struct sockaddr_in*)(&buffer.ifr_addr);
			struct in_addr sin= (struct in_addr)addr->sin_addr;
			ip =  strdup((char*)inet_ntoa(sin));
		}
		close(s);
	}
	return ip;
}

char** load_interfaces_from_proc_file(char* filename)
{
	char** file_lines = NULL;
	int num_interfaces = 0;	
	
	FILE *interface_file = fopen(filename, "r");	
	char newline_terminator[]= {'\n', '\r'};
	if(interface_file != NULL)
	{
		char* file_data = read_entire_file(interface_file, 100);
		file_lines = split_on_separators(file_data, newline_terminator, 2, -1, 0);
		fclose(interface_file);
		free(file_data);
		
		int line_index=0;
		for(line_index=0; file_lines[line_index] != NULL; line_index++)
		{
			num_interfaces = num_interfaces + (strstr(file_lines[line_index], ":") != NULL ? 1 : 0);
		}

	}

		
	char** interfaces = (char**)malloc((num_interfaces+1)*sizeof(char*));
	interfaces[0] = NULL;
	
	if(file_lines != NULL)
	{
		int interface_index=0;
		int line_index=0;
		for(line_index=0; file_lines[line_index] != NULL; line_index++)
		{
			char* interface_end = strstr(file_lines[line_index], ":");
			if(interface_end != NULL)
			{
				int new_length= (int)(interface_end - file_lines[line_index]);
				char* interface = (char*)malloc((1+new_length)*sizeof(char));
				memcpy(interface, file_lines[line_index], new_length);
				interface[new_length] = '\0';
				trim_flanking_whitespace(interface);
				interfaces[interface_index] = interface;
				interface_index++;
				interfaces[interface_index] = NULL;
			}
			free(file_lines[line_index]);
		}
		free(file_lines);

	}

	return interfaces;
}
