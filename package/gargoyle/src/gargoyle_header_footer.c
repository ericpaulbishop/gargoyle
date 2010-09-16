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


#include <erics_tools.h>
#include <uci.h>
#define malloc safe_malloc
#define strdup safe_strdup

#define MINIMAL_HEADER 1
#define HEADER 2
#define FOOTER 3

void define_package_vars(char** package_vars_to_load);
void print_interface_vars(void);
void print_hostname_map(void);
void print_js_var(char* var, char* value);
char* get_interface_mac(char* if_name);
char* get_interface_ip(char* if_name);
char* get_interface_netmask(char* if_name);
char** load_interfaces_from_proc_file(char* filename);
string_map* get_hostnames(void);
char* get_option_value_string(struct uci_option* uopt);
int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name);




int main(int argc, char **argv)
{
	int display_type = HEADER;
	int display_interface_vars = 0;
	int display_hostname_map = 0;
	char* selected_page = "";
	char* selected_section = "";
	char* css_includes = "";
	char* js_includes = "";
	char* title = "Gargoyle Router Management Utility";
	char** package_variables_to_load = NULL;
	int c;
	
	while((c = getopt(argc, argv, "MmHhFfS:s:P:p:C:c:J:j:T:t:IiNnUu")) != -1) //section, page, css includes, javascript includes, title, output interface variables, hostnames, usage
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
			case 'N':
			case 'n':
				display_hostname_map = 1;
				break;
			case 'U':
			case 'u':
			default:
				printf("USAGE: %s [OPTIONS] [UCI SECTIONS]\n", argv[0]);
				printf("\t-m Generate Minimal Header (for popup boxes used for editing table rows)\n");
				printf("\t-h Generate Standard Header\n");
				printf("\t-f Generate Footer\n");
				printf("\t-s [SECTION-ID] Section/Main Menu Id\n");
				printf("\t-p [PAGE-ID] Page Id\n");
				printf("\t-c [CSS FILES] List of additional css files necessary for page\n");
				printf("\t-j [JS FILES] List of additional javascript files necessary for page\n");
				printf("\t-t [TITLE] Title of page\n");
				printf("\t-i Include output of javascript variables that specify network interface ips and MAC addresses\n");
				printf("\t-n Include output of javascript variables specifying association of ips with hostnames\n");
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
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "theme_root") == UCI_OK)
		{
			theme_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "theme") == UCI_OK)
		{
			theme=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "js_root") == UCI_OK)
		{
			js_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "web_root") == UCI_OK)
		{
			web_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "bin_root") == UCI_OK)
		{
			bin_root=get_option_value_string(uci_to_option(e));
		}

		char** all_css;
		char** all_js;
		char whitespace_separators[] = { '\t', ' ' };
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "common_css") == UCI_OK && display_type == HEADER)
		{
			char* css_list = dynamic_strcat(3, get_option_value_string(uci_to_option(e)), " ", css_includes);
			unsigned long num_pieces;
			all_css=split_on_separators(css_list, whitespace_separators, 2, -1, 0, &num_pieces);
			free(css_list);
		}
		else
		{
			unsigned long num_pieces;
			all_css=split_on_separators(css_includes, whitespace_separators, 2, -1, 0, &num_pieces);
		}
		
		
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "common_js") == UCI_OK)
		{
			unsigned long num_pieces;
			char* js_list = dynamic_strcat(3, get_option_value_string(uci_to_option(e)), " ", js_includes);
			all_js=split_on_separators(js_list, whitespace_separators, 2, -1, 0, &num_pieces);
			free(js_list);
		}
		else
		{
			unsigned long num_pieces;
			all_js=split_on_separators(js_includes, whitespace_separators, 2, -1, 0, &num_pieces);
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
						hostname = get_option_value_string(uci_to_option(e2));
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
			printf("\t<div id=\"darken\" ><iframe id=\"d_iframe\" class=\"select_free\"></iframe></div>\n");
			printf("\t<div id=\"wait_msg\">\n");
			printf("\t\t<div id=\"wait_txt\">\n");
			printf("\t\t\tPlease Wait While Settings Are Applied\n");
			printf("\t\t</div>\n");
			printf("\t\t<div id=\"wait_icon\">\n");
			printf("\t\t\t<img src=\"/%s/%s/images/wait_icon.gif\" />\n", theme_root, theme);
			printf("\t\t</div>\n");
			printf("\t\t<iframe id=\"m_iframe\" class=\"select_free\"></iframe>\n");
			printf("\t</div>\n");


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
		if(display_hostname_map == 1)
		{
			print_hostname_map();
		}
		define_package_vars(package_variables_to_load);
		printf("\n\tsetBrowserTimeCookie();\n");
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
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "web_root") == UCI_OK)
		{
			web_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "bin_root") == UCI_OK)
		{
			bin_root=get_option_value_string(uci_to_option(e));
		}



		
		priority_queue* sections = initialize_priority_queue();
		
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			int section_rank;
			if(sscanf(section->type, "%d", &section_rank) > 0)
			{
				priority_queue* section_pages =  initialize_priority_queue();
					
				struct uci_element *e2;
				uci_foreach_element(&section->options, e2) 
				{
					int page_rank;
					if(sscanf(get_option_value_string(uci_to_option(e2)), "%d", &page_rank) > 0)
					{
						push_priority_queue(section_pages, page_rank, strdup(e2->name), NULL); 
					}
				}
				push_priority_queue(sections, section_rank, strdup(section->e.name),  section_pages);
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
		while( (next_section=shift_priority_queue_node(sections)) != NULL)
		{
			char* section_display = "";
			priority_queue* section_pages;
		       	section_pages	= (priority_queue*)next_section->value;
			if(get_uci_option(ctx, &e, p, "gargoyle", "display", next_section->id) == UCI_OK)
			{
				section_display = get_option_value_string(uci_to_option(e));
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



				if(peek_priority_queue_node(section_pages) == NULL)
				{
					printf("\t\t\t\t\t\t\t<div class=\"selected_single_header\">%s</div>\n", section_display);
				}
				else
				{
					printf("\t\t\t\t\t\t\t<div class=\"selected_header\">%s</div>\n", section_display);
					printf("\t\t\t\t\t\t\t<div id=\"submenu_container\">\n");
					priority_queue_node *next_section_page;
					while( (next_section_page=shift_priority_queue_node(section_pages)) != NULL)
					{
						char* page_display="";
						char* page_script="";
						char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
						if(get_uci_option(ctx, &e, p, "gargoyle", "display", lookup) == UCI_OK)
						{
							page_display = get_option_value_string(uci_to_option(e));
						}
						if(get_uci_option(ctx, &e, p, "gargoyle", "scripts", lookup) == UCI_OK)
						{
							page_script = get_option_value_string(uci_to_option(e));
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
				if( (next_section_page=shift_priority_queue_node(section_pages)) != NULL)
				{
					char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
					if(get_uci_option(ctx, &e, p, "gargoyle", "scripts", lookup) == UCI_OK)
					{
						next_section_script = get_option_value_string(uci_to_option(e));
					}
					free(lookup);
				}
				else
				{
					if(get_uci_option(ctx, &e, p, "gargoyle", "scripts", next_section->id) == UCI_OK)
					{
						next_section_script = get_option_value_string(uci_to_option(e));
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
					
					printf("\tuciOriginal.set('%s', '%s', '', \"%s\");\n",package_vars_to_load[package_index], section->e.name, section->type);
					
					struct uci_element *e2;
					uci_foreach_element(&section->options, e2) 	
					{
						printf("\tuciOriginal.set('%s', '%s', '%s', \"%s\");\n",package_vars_to_load[package_index], section->e.name, e2->name, get_option_value_string(uci_to_option(e2)));
					}
				
				}
			}
		}
	}	
}

void print_interface_vars(void)
{
	// load default interface names from /proc/net/dev
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



	
	char* default_lan_if  = NULL;
	char* default_wan_if  = NULL;

	// load lan & wan interface names
	// because we want DEFAULT interfaces, we can't load from config
	// by default lan interface is first eth* interface and wan is second
	// If exceptions to this are found, they should be hard-coded in here
	// to correct issues.
	char** interfaces = load_interfaces_from_proc_file("/proc/net/dev");
	
	int have_switched_eths=0;
	int interface_index; 
	unsigned long num_destroyed;
	for(interface_index=0; interfaces[interface_index] != NULL && have_switched_eths == 0; interface_index++)
	{
		have_switched_eths = strstr(interfaces[interface_index], "eth") != NULL && strstr(interfaces[interface_index], ".") != NULL ? 1 : 0;
	}

	list* eths = initialize_list();	
	for(interface_index=0; interfaces[interface_index] != NULL; interface_index++)
	{
		if(	strstr(interfaces[interface_index], "eth") != NULL && 
			(strstr(interfaces[interface_index], ".") != NULL || have_switched_eths == 0)
		  )
		{
			push_list(eths, strdup(interfaces[interface_index]));
		}
		free(interfaces[interface_index]);
	}
	free(interfaces);
	destroy_list(eths, DESTROY_MODE_FREE_VALUES, &num_destroyed);


	if(eths->length > 1)
	{
		default_wan_if = (char*)pop_list(eths);
	}
	while(eths->length > 0)
	{
		if(default_lan_if == NULL)
		{
			default_lan_if = (char*)shift_list(eths);
		}
		else
		{
			char* tmp = default_lan_if;
			default_lan_if = dynamic_strcat(3, tmp, " ", (char*)shift_list(eths));
			free(tmp);
		}
	}

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
	char* uci_wan_mac     = NULL;
	char* uci_wan_if      = NULL;
	char* uci_wan_dev     = NULL;
	char* uci_wan_gateway = NULL;
	char* uci_lan_if      = NULL;
	char* uci_lan_ip      = NULL;
	char* uci_lan_mask    = NULL;
	char* uci_wireless    = NULL;

	struct uci_context *ctx;
	ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	struct uci_element *e = NULL;
	if(uci_load(ctx, "network", &p) == UCI_OK)
	{
		if(get_uci_option(ctx, &e, p, "network", "wan", "macaddr") == UCI_OK)
		{
			uci_wan_mac=get_option_value_string(uci_to_option(e));
		}
	}


	struct uci_context *state_ctx = uci_alloc_context();
	uci_add_history_path(state_ctx, state_ctx->savedir);
       	uci_set_savedir(state_ctx, "/var/state"); 
	if(uci_load(state_ctx, "network", &p) == UCI_OK)
	{
		char* switch_dev = NULL;
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "device") == UCI_OK)
		{
			uci_wan_dev=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "ifname") == UCI_OK)
		{
			uci_wan_if=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "gateway") == UCI_OK)
		{
			uci_wan_gateway=get_option_value_string(uci_to_option(e));
		}

		if(get_uci_option(state_ctx, &e, p, "network", "lan", "ifname") == UCI_OK)
		{
			uci_lan_if=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "ipaddr") == UCI_OK)
		{
			uci_lan_ip=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "netmask") == UCI_OK)
		{
			uci_lan_mask=get_option_value_string(uci_to_option(e));
		}
		
		//if default wan if is set to a switch device, then we have it backwards, and the wan should be the lan
		//this is a problem for, e.g. routerstation pro where eth1 is the switch
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			if(strstr(section->type, "switch") != NULL)
			{
				struct uci_element *e2;
				uci_foreach_element(&section->options, e2) 
				{
					if(strcmp(e2->name,"device")==0)
					{
						switch_dev = get_option_value_string(uci_to_option(e2));
					}
				}
			}
		}
		if(switch_dev != NULL)
		{
			if(strcmp(switch_dev, default_wan_if) == 0)
			{
				//wan is set to a switch, so swap default lan & wan if
				char* tmp = default_wan_if;
				default_wan_if = default_lan_if;
				default_lan_if = tmp;
			}
			free(switch_dev);
		}
		
	}
	if(uci_load(state_ctx, "wireless", &p) == UCI_OK)
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



	//if multiple interfaces in bridge, uci_lan_if will contain multiple ifs, 
	//which get_interface_mac wouldn't be able to handle
	//thus we need to lookup br-lan instead of uci_lan_if
	char* current_lan_mac = get_interface_mac("br-lan");
	char* current_lan_ip = uci_lan_ip != NULL ? uci_lan_ip : get_interface_ip("br-lan");
	char* current_lan_mask = uci_lan_mask != NULL ? uci_lan_mask : get_interface_netmask("br-lan");

	char* current_wan_mac = get_interface_mac(uci_wan_dev);
	char* current_wan_ip = get_interface_ip(uci_wan_if);
	char* current_wan_mask = get_interface_netmask(uci_wan_if);
	char* current_wireless_mac = get_interface_mac(wireless_if);
	if(current_wireless_mac == NULL)
	{
		current_wireless_mac = get_interface_mac(uci_wireless);
	}

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





	print_js_var("wirelessIf", wireless_if);
	print_js_var("uciWireless", uci_wireless);
	print_js_var("currentWirelessMac", current_wireless_mac);


	print_js_var("defaultLanIf", default_lan_if);
	print_js_var("currentLanIf", uci_lan_if);
	print_js_var("currentLanMac", current_lan_mac);
	print_js_var("currentLanIp", current_lan_ip);
	print_js_var("currentLanMask", current_lan_mask);


	print_js_var("defaultWanIf", default_wan_if);
	print_js_var("defaultWanMac", default_wan_mac);
	print_js_var("currentWanIf", uci_wan_if);
	print_js_var("currentWanMac", current_wan_mac);
	print_js_var("currentWanIp", current_wan_ip);
	print_js_var("currentWanMask", current_wan_mask);
	print_js_var("currentWanGateway", uci_wan_gateway);

	uci_free_context(ctx);
	uci_free_context(state_ctx);
}

void print_hostname_map(void)
{
	string_map* ip_to_hostname = get_hostnames();
	printf("\tvar ipToHostname = [];\n");
	if(ip_to_hostname->num_elements == 0)
	{
		printf("\tvar ipsWithHostname = [];\n");
	}
	else
	{
		unsigned long num_ips;
		char** ip_list = get_string_map_keys(ip_to_hostname, &num_ips);
		char* ips_with_hostname_str = join_strs("\",\"", ip_list, -1, 0, 0);
		int ip_index;
		
		printf("\tvar ipsWithHostname = [ \"%s\" ];\n", ips_with_hostname_str);
		for(ip_index=0; ip_index < num_ips; ip_index++)
		{
			char* ip = ip_list[ip_index];
			char* name = remove_string_map_element(ip_to_hostname, ip);
			printf("\tipToHostname[ \"%s\" ] = \"%s\";\n", ip, name);
			free(name);
		}
		free_null_terminated_string_array(ip_list);
	}
	unsigned long num_destroyed;
	destroy_string_map(ip_to_hostname, DESTROY_MODE_FREE_VALUES, &num_destroyed);
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

char* get_interface_netmask(char* if_name)
{
	char* mask = NULL;
	if(if_name != NULL)
	{
		struct ifreq buffer;
		int s = socket(PF_INET, SOCK_DGRAM, 0);
		memset(&buffer, 0x00, sizeof(buffer));
		strcpy(buffer.ifr_name, if_name);
	
		if(ioctl(s, SIOCGIFNETMASK, &buffer) != -1)
		{
			struct sockaddr_in *addr = (struct sockaddr_in*)(&buffer.ifr_addr);
			struct in_addr sin= (struct in_addr)addr->sin_addr;
			mask =  strdup((char*)inet_ntoa(sin));
		}
		close(s);
	}
	return mask;

}

char** load_interfaces_from_proc_file(char* filename)
{
	char** file_lines = NULL;
	int num_interfaces = 0;	
	
	FILE *interface_file = fopen(filename, "r");	
	char newline_terminator[]= {'\n', '\r'};
	if(interface_file != NULL)
	{
		unsigned long read_length;
		char* file_data = read_entire_file(interface_file, 100, &read_length);
		unsigned long num_pieces;
		file_lines = split_on_separators(file_data, newline_terminator, 2, -1, 0, &num_pieces);
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

string_map* get_hostnames(void)
{
	string_map* ip_to_hostname = initialize_string_map(1);

	char* hostname_files[] = { "/tmp/dhcp.leases", "/etc/hosts", NULL };
	int ip_indices[] = { 2, 0 };
	int name_indices[] = { 3, 1 };
	int file_index;
	for(file_index = 0; hostname_files[file_index] != NULL; file_index++)
	{
		int ip_index = ip_indices[file_index];
		int name_index = name_indices[file_index];
		int min_line_pieces = ip_index > name_index ? ip_index+1 : name_index+1;

		FILE* name_file = fopen(hostname_files[file_index], "r");
		if(name_file != NULL)
		{
			char *line = NULL;
			unsigned long read_len;
			int sep = '\n';
			do
			{
				sep = dyn_read_line(name_file, &line, &read_len);
				unsigned long num_line_pieces;
				char* whitespace_seps = "\t ";
				char** line_pieces = split_on_separators(line, whitespace_seps, 2, -1, 0, &num_line_pieces);
				free(line);
				if(num_line_pieces >= min_line_pieces)
				{
					char *name = strdup(line_pieces[ name_index ]);
					trim_flanking_whitespace(name);
					if(name[0] != '*' && name[0] != '\0')
					{
						char* ip  = line_pieces[ ip_index ];
						set_string_map_element(ip_to_hostname, ip, name);
					}
					else
					{
						free(name);
					}
				}
				free_null_terminated_string_array(line_pieces);
			}while(sep != EOF);
			fclose(name_file);
		}
	}

	//make sure local ips get set to hostname
	FILE* hostname_file = fopen("/proc/sys/kernel/hostname", "r");
	if(hostname_file != NULL)
	{
		char *line = NULL;
		unsigned long read_len;
		dyn_read_line(hostname_file, &line, &read_len);
		trim_flanking_whitespace(line);
		if(strlen(line) > 0)
		{
			//load lan ip & wan ip from uci
			struct uci_context *state_ctx = uci_alloc_context();
			struct uci_package *p = NULL;
			struct uci_element *e = NULL;
			uci_add_history_path(state_ctx, state_ctx->savedir);
		       	uci_set_savedir(state_ctx, "/var/state");
			if(uci_load(state_ctx, "network", &p) == UCI_OK)
			{
				if(get_uci_option(state_ctx, &e, p, "network", "lan", "ipaddr") == UCI_OK)
				{
					char* uci_lan_ip=get_option_value_string(uci_to_option(e));
					set_string_map_element(ip_to_hostname, uci_lan_ip, strdup(line));
					free(uci_lan_ip);
				}
				if(get_uci_option(state_ctx, &e, p, "network", "wan", "ipaddr") == UCI_OK)
				{
					char* uci_wan_ip=get_option_value_string(uci_to_option(e));
					set_string_map_element(ip_to_hostname, uci_wan_ip, strdup(line));
					free(uci_wan_ip);
				}
			}
			uci_free_context(state_ctx);
			
			//127.0.0.1 is always local, so add it unconditionally
			set_string_map_element(ip_to_hostname, "127.0.0.1", strdup(line));

		}
		free(line);
		fclose(hostname_file);
	}


	return ip_to_hostname;



}



#ifdef UCI_OLD_VERSION //valid for uci version 0.3.3 for kamikaze 7.09
char* get_option_value_string(struct uci_option* uopt)
{
	char* opt_str = strdup(uopt->value);
	
	/* escape backslash characters & quote characters so javascript can parse variables properly */
	char* tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\\", "\\\\");
	free(tmp);
	tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\"", "\\\"");
	free(tmp);

	return opt_str;
}
int get_uci_option(struct uci_context* ctx,  struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name)
{
	return uci_lookup(ctx, e, p, section_name, option_name) ;
}

#else //valid for newer uci versions in trunk (tested with 0.6.2)

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

	/* escape backslash characters & quote characters so javascript can parse variables properly */
	char* tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\\", "\\\\");
	free(tmp);
	tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\"", "\\\"");
	free(tmp);


	return opt_str;
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

#endif
