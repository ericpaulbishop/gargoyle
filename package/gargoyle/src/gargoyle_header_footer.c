/*
 *  Copyright © 2008-2013 by Eric Bishop <eric@gargoyle-router.com>
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

#define DEFAULT_IF_FILE "/etc/gargoyle_default_ifs"

void define_package_vars(char** package_vars_to_load);
void print_interface_vars(void);
void print_hostname_map(void);
void print_js_var(char* var, char* value);
void print_js_list_var(char* var, list** value);
char* get_interface_mac(char* if_name);
char* get_interface_ip(char* if_name);
char* get_interface_netmask(char* if_name);
char* get_interface_gateway(char* if_name);
int load_saved_default_interfaces( char** default_lan_if, char** default_wan_if, char** default_wan_mac);
void save_default_interfaces(char* default_lan_if, char* default_wan_if, char* default_wan_mac);
char** load_interfaces_from_proc_file(char* filename);
string_map* get_hostnames(void);
char* get_option_value_string(struct uci_option* uopt);
int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name);
char** ParseGHF_TranslationStrings(char* web_root, char* active_lang, char* fallback_lang);




int main(int argc, char **argv)
{
	int display_type = HEADER;
	int display_interface_vars = 0;
	int display_hostname_map = 0;
	char* selected_page = "";
	char* selected_section = "";
	char* css_includes = "";
	char* js_includes = "";
	char* langstr_js_includes = "";
	char* title = "Gargoyle Router Management Utility";
	char* desc = "Router<br/>Management<br/>Utility";
	char* dname = "Device Name";
	char* wait_txt = "Please Wait While Settings Are Applied";
	char** package_variables_to_load = NULL;
	int c;
	
	while((c = getopt(argc, argv, "MmHhFfS:s:P:p:C:c:J:j:Z:z:T:t:IiNnUu")) != -1) //section, page, css includes, javascript includes, title, output interface variables, hostnames, usage
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
			case 'Z':
			case 'z':
				langstr_js_includes = strdup(optarg);
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
				printf("\t-m Generate Minimal Header (for popup boxes used for editing table rows)\n"
				       "\t-h Generate Standard Header\n"
				       "\t-f Generate Footer\n"
				       "\t-s [SECTION-ID] Section/Main Menu Id\n"
				       "\t-p [PAGE-ID] Page Id\n"
				       "\t-c [CSS FILES] List of additional css files necessary for page\n"
				       "\t-z [JS FILES] List of additional i18n language-string javascript files necessary for page\n"
				       "\t-j [JS FILES] List of additional javascript files necessary for page\n"
				       "\t-t [TITLE] Title of page\n"
				       "\t-i Include output of javascript variables that specify network interface ips and MAC addresses\n"
				       "\t-n Include output of javascript variables specifying association of ips with hostnames\n"
				       "\t-u print usage and exit\n");
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
		printf("Content-Type: text/html; charset=utf-8\n\n");

		if(uci_load(ctx, "gargoyle", &p) != UCI_OK)
		{
			printf("ERROR: no gargoyle package defined!\n");
			return 0;
		}

		char* theme_root = "/themes";
		char* theme = "default";
		char* js_root = "js";
		char* web_root = "/www";
		char* bin_root = ".";
		char* gargoyle_version = "default";
		char* fallback_lang = "";
		char* active_lang = "";
		char** translation_strings = NULL;
		
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "theme_root") == UCI_OK)
		{
			theme_root=get_option_value_string(uci_to_option(e));
			if(theme_root[0] != '/')
			{
				char* tmp = theme_root;
				theme_root = dynamic_strcat(2, "/", theme_root);
				free(tmp);
			}
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "theme") == UCI_OK)
		{
			theme=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "js_root") == UCI_OK)
		{
			js_root=get_option_value_string(uci_to_option(e));
			if(js_root[0] != '/')
			{
				char* tmp = js_root;
				js_root = dynamic_strcat(2, "/", js_root);
				free(tmp);
			}

		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "web_root") == UCI_OK)
		{
			web_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "bin_root") == UCI_OK)
		{
			bin_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "version") == UCI_OK)
		{
			char* raw_version = get_option_value_string(uci_to_option(e));

			/* adjust version to ensure it is valid query string */
			int ri;
			gargoyle_version = strdup(raw_version); /* just for allocating memory */
			for(ri = 0; raw_version[ri] != '\0'; ri++)
			{
				unsigned char ch= raw_version[ri];
				if( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '_' || ch == '~' ) /* valid query string characters */
				{
					gargoyle_version[ri] = (char)ch;
				}
				else
				{
					gargoyle_version[ri] = '-';
				}
			}
			gargoyle_version[ri] = '\0';
			free(raw_version);
		}


		char** all_css;
		char** all_js;
		char** all_lstr_js;
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
#ifndef LOCALIZED_BUILD
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "fallback_lang") == UCI_OK)
		{
			fallback_lang=get_option_value_string(uci_to_option(e));
			if(get_uci_option(ctx, &e, p, "gargoyle", "global", "language") == UCI_OK)
			{
				active_lang=get_option_value_string(uci_to_option(e));
				unsigned long num_pieces;
				all_lstr_js=split_on_separators(langstr_js_includes, whitespace_separators, 2, -1, 0, &num_pieces);
				translation_strings=ParseGHF_TranslationStrings(web_root, active_lang, fallback_lang);
			}
		}
#endif
		
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
		


		printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
		       "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
		       "<head>\n"
		       "\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
		       "\t<title>%s</title>\n", translation_strings == NULL ? title : translation_strings[0]);
		printf("\t<link rel=\"shortcut icon\" href=\"%s/%s/images/favicon.png\" type=\"image/png\"/>\n", theme_root, theme);
		int css_index, js_index, lstr_js_index;

		//older versions of gargoyle don't have some CSS classes, so this fixes compatibility with old themes
		printf("\t<style>\n"
		       "\t\t.unselected_submenu_container	{ display:none; }\n"
		       "\t\t#nav_internal_container2 #selected_header_link	{ display:inline; padding:0; color:#000; cursor:default; }\n"
		       "\t\t#nav_internal_container2 #selected_header_link:hover	{ color:#000; cursor:default; }\n"
		       "\t\t#nav_internal_container2 #selected_header_link:visited	{ color:#000; cursor:default; }\n"
		       "\t\t#nav_internal_container2 #selected_header_link:active	{ color:#000; cursor:default; }\n"
		       "\t</style>\n");
		for(css_index=0; all_css[css_index] != NULL; css_index++)
		{
			printf("\t<link rel=\"stylesheet\" href=\"%s/%s/%s?%s\" type=\"text/css\"/>\n", theme_root, theme, all_css[css_index], gargoyle_version);
		}
		for(js_index=0; all_js[js_index] != NULL; js_index++)
		{
			printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"%s/%s?%s\"></script>\n", js_root, all_js[js_index], gargoyle_version);
		}
#ifndef LOCALIZED_BUILD
		if(active_lang[0] != '\0' && fallback_lang[0] != '\0')
		{
			if (memcmp(fallback_lang, active_lang, strlen(active_lang)) != 0)
			{
				printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"/i18n/%s/strings.js?%s\"></script>\n", fallback_lang, gargoyle_version);
			}
			printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"i18n/%s/strings.js?%s\"></script>\n", active_lang, gargoyle_version);
			for(lstr_js_index=0; all_lstr_js[lstr_js_index] != NULL; lstr_js_index++)
			{
				if (memcmp(fallback_lang, active_lang, strlen(active_lang)) != 0)
				{
					printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"/i18n/%s/%s?%s\"></script>\n", fallback_lang, all_lstr_js[lstr_js_index], gargoyle_version);
				}
				char* tran_file = dynamic_strcat(5, web_root, "/i18n/", active_lang, "/", all_lstr_js[lstr_js_index]);
				if (path_exists(tran_file) == PATH_IS_REGULAR_FILE || path_exists(tran_file) == PATH_IS_SYMLINK)
				{
					printf("\t<script language=\"javascript\" type=\"text/javascript\" src=\"/i18n/%s/%s?%s\"></script>\n", active_lang, all_lstr_js[lstr_js_index], gargoyle_version);
				}
				free(tran_file);
			}
		}
#endif
		printf("</head>\n"
		       "<body>\n");

		if(display_type == HEADER)
		{
			printf("\t<div id=\"darken\"><iframe id=\"d_iframe\" class=\"select_free\"></iframe></div>\n"
			       "\t<div id=\"wait_msg\">\n"
			       "\t\t<div id=\"wait_txt\">\n"
			       "\t\t\t%s\n"
			       "\t\t</div>\n"
			       "\t\t<div id=\"wait_icon\">\n"
			       "\t\t\t<img src=\"%s/%s/images/wait_icon.gif\"/>\n", translation_strings == NULL ? wait_txt : translation_strings[3], theme_root, theme);

			printf("\t\t</div>\n"
			       "\t\t<iframe id=\"m_iframe\" class=\"select_free\"></iframe>\n"
			       "\t</div>\n"
			       "\t<div id=\"outer_logo\">\n"
			       "\t\t<div id=\"inner_logo\">\n"
			       "\t\t\t<div id=\"garg_title\">Gargoyle</div>\n"
			       "\t\t\t<div id=\"garg_desc\">%s</div>\n"
			       "\t\t\t<div id=\"garg_host\">%s: %s</div>\n",	translation_strings == NULL ? desc : translation_strings[1], 
																	translation_strings == NULL ? dname : translation_strings[2],
																	hostname);

			printf("\t\t</div>\n"
			       "\t</div>\n"
			       "\t<div id=\"outer_header\"></div>\n"
			       "\t<div id=\"outer_container\">\n"
			       "\t\t<div id=\"main_external_container\">\n"
			       "\t\t\t<div id=\"main_top\"></div>\n"
			       "\t\t\t<div id=\"main_internal_container\">\n"
			       "\n\n");
		}

		printf("<script>\n"
		       "<!--\n"
		       "\tvar gargoyleBinRoot = \"%s/%s\";\n", web_root, bin_root);

		if(display_interface_vars == 1)
		{
			print_interface_vars();
		}
		if(display_hostname_map == 1)
		{
			print_hostname_map();
		}
		define_package_vars(package_variables_to_load);
		printf("\n\tsetBrowserTimeCookie();\n"
		       "\n\tvar testAjax = getRequestObj();\n"
		       "\tif(!testAjax) { window.location = \"no_ajax.sh\"; }\n"
		       "//-->\n"
		       "</script>\n"
		       "\n\n");
		free_null_terminated_string_array(translation_strings);
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


		printf("\t\t\t</div>\n"
		       "\t\t\t<div id=\"main_bottom\"></div>\n"
		       "\t\t</div>\n"
		       "\t\t<div id=\"nav_external_container\" onClick=\"return true\">\n"
		       "\t\t\t<div id=\"nav_top\"></div>\n"
		       "\t\t\t<div id=\"nav_internal_container1\">\n"
		       "\t\t\t\t<div id=\"nav_internal_container2\">\n"
		       "\t\t\t\t\t<div class=\"nav_internal_end1\"></div>\n");

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
					printf("\t\t\t\t\t<div class=\"nav_selected_divider_end1\"></div>\n"
					       "\t\t\t\t\t<div class=\"nav_selected_end1\">\n"
					       "\t\t\t\t\t\t<div class=\"nav_selected_container_end1\">\n");
				}
				else
				{
					printf("\t\t\t\t\t<div class=\"nav_selected_divider1\"></div>\n"
					       "\t\t\t\t\t<div class=\"nav_selected\">\n"
					       "\t\t\t\t\t\t<div class=\"nav_selected_container\">\n");
				}



				if(peek_priority_queue_node(section_pages) == NULL)
				{
					printf("\t\t\t\t\t\t\t<div class=\"selected_single_header\">%s</div>\n", section_display);
				}
				else
				{
					printf("\t\t\t\t\t\t\t<a id=\"selected_header_link\" a onClick=\"return true\">\n"
					       "\t\t\t\t\t\t\t\t<div class=\"selected_header\">%s</div>\n", section_display);
					printf("\t\t\t\t\t\t\t</a>\n"
					       "\t\t\t\t\t\t\t<div id=\"submenu_container\">\n");
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
							char* bin_slash = bin_root[0] == '/' ? strdup("") : strdup("/");
							char* page_slash = page_script[0] == '/' ? strdup("") : strdup("/");
							if(strcmp(bin_root, ".") == 0)
							{
								printf("\t\t\t\t\t\t\t\t<a href=\"%s%s\">%s</a>\n", page_slash, page_script, page_display);
							}
							else
							{
								printf("\t\t\t\t\t\t\t\t<a href=\"%s%s%s%s\">%s</a>\n", bin_slash, bin_root, page_slash, page_script, page_display);
							}
							free(bin_slash);
							free(page_slash);
						}
						free(lookup);
					}
					printf("\t\t\t\t\t\t\t</div>\n");
				}
				printf("\t\t\t\t\t\t</div>\n"
				       "\t\t\t\t\t</div>\n");
				prev_section_selected=1;
			}
			else
			{
				if(first_section == 1)
				{
					printf("\t\t\t\t\t<div class=\"nav_unselected_divider_end1\"></div>\n"
					       "\t\t\t\t\t<div class=\"nav_unselected_end1\">\n"
					       "\t\t\t\t\t\t<div class=\"nav_unselected\">\n");
				}
				else
				{
					printf("\t\t\t\t\t<div class=\"nav_unselected_container\">\n"
					       "\t\t\t\t\t\t<div class=\"nav_unselected\">\n");
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


				char* bin_slash = bin_root[0] == '/' ? strdup("") : strdup("/");
				char* script_slash = next_section_script[0] == '/' ? strdup("") : strdup("/");
				if(strcmp(bin_root, ".") == 0)
				{
					printf("\t\t\t\t\t\t\t<a href=\"%s%s\" onClick=\"return true\">%s</a>\n", script_slash, next_section_script, section_display);
				}
				else
				{
					printf("\t\t\t\t\t\t\t<a href=\"%s%s%s%s\" onClick=\"return true\">%s</a>\n", bin_slash, bin_root, script_slash, next_section_script, section_display);
				}
				printf("\t\t\t\t\t\t</div>\n");
				
				int empty_section = 0;
				if (next_section_page == NULL)
				{
					empty_section = 1;
				}
				if (empty_section == 0)
				{
					printf("\t\t\t\t\t\t<div class=\"unselected_submenu_container\">\n");
				}
				while(next_section_page != NULL)
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
					char* bin_slash = bin_root[0] == '/' ? strdup("") : strdup("/");
					char* page_slash = page_script[0] == '/' ? strdup("") : strdup("/");
					if(strcmp(bin_root, ".") == 0)
					{
						printf("\t\t\t\t\t\t\t<a href=\"%s%s\">%s</a>\n", page_slash, page_script, page_display);
					}
					else
					{
						printf("\t\t\t\t\t\t\t<a href=\"%s%s%s%s\">%s</a>\n", bin_slash, bin_root, page_slash, page_script, page_display);
					}
					free(bin_slash);
					free(page_slash);
					free(lookup);
					next_section_page=shift_priority_queue_node(section_pages);
				}
				if (empty_section == 0)
				{
					printf("\t\t\t\t\t\t</div>\n");
				}


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

		printf("\t\t\t\t\t<div class=\"nav_internal_end2\"></div>\n"
		       "\t\t\t\t</div>\n"
		       "\t\t\t</div>\n"
		       "\t\t\t<div id=\"nav_bottom\"></div>\n"
		       "\t\t</div>\n"
		       "\t</div>\n"
		       "\t<div id=\"outer_footer\"></div>\n"
		       "</body>\n"
		       "</html>\n");
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
			struct uci_context *ctx = uci_alloc_context();
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
						struct uci_option* uopt = uci_to_option(e2);
						if(uopt->type == UCI_TYPE_STRING)
						{
							printf("\tuciOriginal.set('%s', '%s', '%s', \"%s\");\n", package_vars_to_load[package_index], section->e.name, e2->name, get_option_value_string(uopt));
						}
						else if(uopt->type == UCI_TYPE_LIST)
						{
							printf("\tuciOriginal.createListOption('%s', '%s', '%s', true);\n", package_vars_to_load[package_index], section->e.name, e2->name);
							struct uci_element* e;
							uci_foreach_element(&uopt->v.list, e)
							{
								char* list_str = strdup(e->name);
								char* tmp = list_str;
								list_str = dynamic_replace(list_str, "\\", "\\\\");
								free(tmp);
								tmp = list_str;
								list_str = dynamic_replace(list_str, "\"", "\\\"");
								free(tmp);

								/* when last parameter of set is true, value gets appended to list instead of replacing it */
								printf("\tuciOriginal.set('%s', '%s', '%s', \"%s\", true);\n", package_vars_to_load[package_index], section->e.name, e2->name, list_str);
									
								free(list_str);
							}

						}
					}
				
				}
			}
			uci_free_context(ctx);
		}
	}	
}

void print_interface_vars(void)
{


	list* wireless_ifs  = initialize_list();
	list* wireless_uci = initialize_list();

	char* default_lan_if  = NULL;
	char* default_wan_if  = NULL;
	char* default_wan_mac = NULL;
	int   loaded_default_ifs = load_saved_default_interfaces( &default_lan_if, &default_wan_if, &default_wan_mac);

	/*
	 * A few notes on "if" vs "dev" vs "bridge" variable names:
	 * When doing load "if" is the ifname variable loaded from the /etc/config/network file,
	 * while "dev" is the "device" loaded from /var/state/network, and "bridge" is the
	 * "ifname" from /var/state/network.  
	 *
	 * "Bridge" is labeled that way because if the interface
	 * is a bridge, this will be the only place the name of the bridge interface shows up, the
	 * other variables will just have the names of the interfaces within the bridge.
	 *
	 * Note also that "bridge" interface variables will contain the pppoe device name if we're using pppoe.
	 * Therefore we print these out in addition to the interface list as javascript variables labeled 
	 * currentLanName and currentWanName
	 */
	
	char* uci_wan_mac     = NULL;
	char* uci_wan_if      = NULL;
	char* uci_wan_dev     = NULL;
	char* uci_lan_if      = NULL;
	char* uci_lan_dev     = NULL;
	char* uci_lan_bridge  = NULL;
	char* uci_wan_bridge  = NULL;
	char* uci_wan_type    = NULL;

	char* uci_wan_gateway = NULL;
	char* uci_lan_ip      = NULL;
	char* uci_lan_mask    = NULL;
	char* uci_wan_ip      = NULL;
	char* uci_wan_mask    = NULL;




	struct uci_context *ctx = uci_alloc_context();
	struct uci_context *state_ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	struct uci_element *e = NULL;

	
	char** wireless_if_arr = load_interfaces_from_proc_file("/proc/net/wireless");
	int wif_index;
	for(wif_index = 0; wireless_if_arr[wif_index] != NULL; wif_index++)
	{
		push_list(wireless_ifs, (void*)strdup(wireless_if_arr[wif_index]));
	}
	free_null_terminated_string_array(wireless_if_arr);

	
	if(uci_load(ctx, "wireless", &p) == UCI_OK)
	{
		uci_foreach_element( &p->sections, e)
		{
			struct uci_section *section = uci_to_section(e);
			if(strstr(section->type, "wifi-device") != NULL)
			{
				push_list(wireless_uci, (void*)strdup(section->e.name) );
			}
		}
	}

	if(uci_load(ctx, "network", &p) == UCI_OK)
	{
		if(get_uci_option(ctx, &e, p, "network", "wan", "macaddr") == UCI_OK)
		{
			uci_wan_mac=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "network", "wan", "ifname") == UCI_OK)
		{
			uci_wan_if = get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "network", "wan", "type") == UCI_OK)
		{
			uci_wan_type = get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "network", "lan", "ifname") == UCI_OK)
		{
			uci_lan_if = get_option_value_string(uci_to_option(e));
		}
	}



	uci_add_delta_path(state_ctx, state_ctx->savedir);
      	uci_set_savedir(state_ctx, "/var/state"); 
	if(uci_load(state_ctx, "network", &p) == UCI_OK)
	{
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "device") == UCI_OK)
		{
			uci_wan_dev=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "device") == UCI_OK)
		{
			uci_lan_dev=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "ifname") == UCI_OK)
		{
			uci_lan_bridge=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "ifname") == UCI_OK)
		{
			uci_wan_bridge=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "gateway") == UCI_OK)
		{
			uci_wan_gateway=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "ipaddr") == UCI_OK)
		{
			uci_lan_ip=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "netmask") == UCI_OK)
		{
			uci_lan_mask=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "ipaddr") == UCI_OK)
		{
			uci_wan_ip=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "netmask") == UCI_OK)
		{
			uci_wan_mask=get_option_value_string(uci_to_option(e));
		}

	}


	char* current_lan_bridge = uci_lan_bridge != NULL ? strdup(uci_lan_bridge) : strdup("br-lan");
	char* current_lan_if = uci_lan_if != NULL ? strdup(uci_lan_if) : strdup(uci_lan_dev);
	char* current_wan_if = uci_wan_if != NULL ? strdup(uci_wan_if) : strdup(uci_wan_dev);
	if(safe_strcmp(uci_wan_type, "bridge") == 0 && (uci_wan_if == NULL || safe_strcmp(uci_wan_if, "") == 0))
	{
		current_wan_if = (uci_wan_bridge != NULL && strcmp(uci_wan_bridge, "") != 0) ? strdup(uci_wan_bridge) : strdup("br-wan");
	}
	char* current_wan_mac = get_interface_mac(current_wan_if);
	char* current_lan_mac = get_interface_mac(current_lan_bridge);

	if(loaded_default_ifs == 0)
	{
		default_lan_if = current_lan_if != NULL ? strdup(current_lan_if) : NULL;
		default_wan_if = current_wan_if != NULL ? strdup(current_wan_if) : NULL;
		default_wan_mac = current_wan_mac != NULL ? strdup(current_wan_mac) : NULL;

		if(default_wan_if == NULL && current_lan_if != NULL)
		{
			default_wan_if = strdup(current_lan_if);
		}
		if(default_wan_mac == NULL && uci_wan_mac != NULL)
		{
			default_wan_mac = strdup(uci_wan_mac);
		}
		if(default_wan_mac == NULL && current_lan_mac != NULL)
		{
			default_wan_mac = strdup(current_lan_mac);
		}
		if(default_wan_mac == NULL)
		{
			default_wan_mac = strdup("00:11:22:33:44:55");
		}
		save_default_interfaces(default_lan_if, default_wan_if, default_wan_mac);
	}

	


	char* current_lan_ip      = uci_lan_ip != NULL   ? strdup(uci_lan_ip)   : get_interface_ip(current_lan_bridge);
	char* current_lan_mask    = uci_lan_mask != NULL ? strdup(uci_lan_mask) : get_interface_netmask(current_lan_bridge);

	char* current_wan_ip      = uci_wan_ip != NULL   ? strdup(uci_wan_ip)   : get_interface_ip(current_wan_if);
	char* current_wan_mask    = uci_wan_mask != NULL ? strdup(uci_wan_mask) : get_interface_netmask(current_wan_if);

	char* current_wan_gateway = uci_wan_gateway != NULL ? strdup(uci_wan_gateway) : get_interface_gateway(current_wan_if);


	list* tmp_list = initialize_list();
	list* wireless_macs = initialize_list();
	int wireless_if_num = 1;
	while(wireless_ifs->length > 0)
	{
		char* wireless_if = (char*)shift_list(wireless_ifs);
		char* wireless_mac = get_interface_mac(wireless_if);
		if(wireless_mac == NULL)
		{
			wireless_mac = (char*)malloc(20);
			sprintf(wireless_mac, "00:11:22:33:44:%02x",
				wireless_if_num & 0xff);
		}
		push_list(wireless_macs, (void*)wireless_mac);
		push_list(tmp_list, (void*)wireless_if);
		wireless_if_num++;
	}
	unsigned long num_destroyed;
	destroy_list(wireless_ifs, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	wireless_ifs = tmp_list;




	print_js_list_var("wirelessIfs", &wireless_ifs);
	print_js_list_var("uciWirelessDevs", &wireless_uci);
	print_js_list_var("currentWirelessMacs", &wireless_macs);


	print_js_var("defaultLanIf", default_lan_if);
	print_js_var("currentLanIf", uci_lan_if);
	print_js_var("currentLanName", uci_lan_bridge);
	print_js_var("currentLanMac", current_lan_mac);
	print_js_var("currentLanIp", current_lan_ip);
	print_js_var("currentLanMask", current_lan_mask);


	print_js_var("defaultWanIf", default_wan_if);
	print_js_var("defaultWanMac", default_wan_mac);
	print_js_var("currentWanIf", current_wan_if);
	print_js_var("currentWanName", uci_wan_bridge);
	print_js_var("currentWanMac", current_wan_mac);
	print_js_var("currentWanIp", current_wan_ip);
	print_js_var("currentWanMask", current_wan_mask);
	print_js_var("currentWanGateway", current_wan_gateway);

	uci_free_context(ctx);
	uci_free_context(state_ctx);

	/* there are an assload of other variables to free... but, eh, fuck it, this thing doesn't run as a daemon 
	 *
	 * variables are defined/copied with strdups, so we won't get any double-free errors if we do decide to implement
	 * code to free them at a later time
	 */
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


void print_js_list_var(char* var, list** value)
{
	int first = 1;
	printf("\tvar %s = [ ", var);
	if(value != NULL)
	{
		if(*value != NULL)
		{
			list* tmp = initialize_list();
			list* vals = *value;
			unsigned long num_destroyed;
			while(vals->length > 0)
			{
				char* val = (char*)shift_list(vals);
				printf("%s \"%s\"", first==0 ? "," : "", val);
				first = 0;
				push_list(tmp, val);
			}
			destroy_list(vals, DESTROY_MODE_FREE_VALUES, &num_destroyed);
			*value = tmp;
		}
	}
	printf(" ];\n");

}

void print_js_var(char* var, char* value)
{
	printf("\tvar %s = \"%s\";\n",
	       var,
	       (value != NULL) ? value : "");
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

char* get_interface_gateway(char* if_name)
{
	char* gateway = NULL;
	if(if_name != NULL)
	{

		FILE* route_file = fopen("/proc/net/route", "r");
		if(route_file != NULL)
		{
			char newline_terminator[]= {'\n', '\r'};
			char spaces[] = {' ', '\t'};
			unsigned long read_length;
			unsigned long num_lines;
			int line_index;
	
			char* route_data = read_entire_file(route_file, 100, &read_length);
			char** route_lines = split_on_separators(route_data, newline_terminator, 2, -1, 0, &num_lines);
			fclose(route_file);
			free(route_data);
		
			for(line_index=0; line_index < num_lines && gateway == NULL; line_index++)
			{
				unsigned long line_pieces = 0;
				char** split_line;
				trim_flanking_whitespace(route_lines[line_index]);
				split_line = split_on_separators(route_lines[line_index], spaces, 2, -1, 0, &line_pieces);
				if(line_pieces >=4)
				{
					if(strcmp(split_line[0], if_name) == 0)
					{
						unsigned int flags = 0;
						if( sscanf(split_line[3], "%u", &flags) > 0)
						{
							if((flags | 2) != 0)
							{
								unsigned int ip[4];
								sscanf(split_line[2], "%2X%2X%2X%2X", ip, ip+1, ip+2, ip+3); 
								gateway = (char*)malloc(20);
								sprintf(gateway, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
							}
						}
					}
				}
				free_null_terminated_string_array(split_line);
			}
			free_null_terminated_string_array(route_lines);
		}
	}
	return gateway;
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

void save_default_interfaces(char* default_lan_if, char* default_wan_if, char* default_wan_mac)
{
	FILE *interface_file = fopen(DEFAULT_IF_FILE, "w");
	if(interface_file != NULL)
	{
		fprintf(interface_file, "default_lan_if\t%s\n", default_lan_if);
		fprintf(interface_file, "default_wan_if\t%s\n", default_wan_if);
		fprintf(interface_file, "default_wan_mac\t%s\n", default_wan_mac);
		fclose(interface_file);
	}

}

int load_saved_default_interfaces( char** default_lan_if, char** default_wan_if, char** default_wan_mac)
{
	FILE *interface_file = fopen(DEFAULT_IF_FILE, "r");
	char newline_terminator[]= {'\n', '\r'};
	char spaces[] = {' ', '\t'};
	
	*default_lan_if = NULL;
	*default_wan_if = NULL;


	if(interface_file != NULL)
	{
		unsigned long read_length;
		char* file_data = read_entire_file(interface_file, 100, &read_length);
		unsigned long num_lines;
		char** file_lines = split_on_separators(file_data, newline_terminator, 2, -1, 0, &num_lines);
		fclose(interface_file);
		free(file_data);

		int line_index=0;
		for(line_index=0; line_index < num_lines; line_index++)
		{
			unsigned long line_pieces = 0;
			char** split_line;
			trim_flanking_whitespace(file_lines[line_index]);
			split_line = split_on_separators(file_lines[line_index], spaces, 2, -1, 0, &line_pieces);
			if(line_pieces >1)
			{
				if(safe_strcmp(split_line[0], "default_lan_if") == 0)
				{
					*default_lan_if = join_strs(" ", split_line+1, -1, 0, 0);
				}
				if(safe_strcmp(split_line[0], "default_wan_if") == 0)
				{
					*default_wan_if = join_strs(" ", split_line+1, -1, 0, 0);
				}
				if(safe_strcmp(split_line[0], "default_wan_mac") == 0)
				{
					*default_wan_mac = join_strs(" ", split_line+1, -1, 0, 0);
				}


			}
			free_null_terminated_string_array(split_line);
		}
		free_null_terminated_string_array(file_lines);

	}

	return *default_lan_if != NULL || *default_wan_if != NULL ? 1 : 0;
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
			uci_add_delta_path(state_ctx, state_ctx->savedir);
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

char** ParseGHF_TranslationStrings(char* web_root, char* active_lang, char* fallback_lang)
{
#ifdef LOCALIZED_BUILD
	return NULL;
#else
	unsigned char ghf_idx=0;
	char** GHFstrings = (char**)calloc(5, sizeof(char*)); //4 strings + emptyval
	//same order as in file:
	//index=0: ghf.title="Gargoyle Router Management Utility";
	//index=1: ghf.desc="Router<br/>Management<br/>Utility";
	//index=2: ghf.devn="Device Name";
	//index=3: ghf.waits="Please Wait While Settings Are Applied";
	char* ghfs_path = dynamic_strcat(4, web_root, "/i18n/", active_lang, "/ghf.js");
	
	if (path_exists(ghfs_path) == PATH_IS_REGULAR_FILE || path_exists(ghfs_path) == PATH_IS_SYMLINK )
	{
		unsigned long num_lines = 0;
		char** ghf_js_lines = get_file_lines(ghfs_path, &num_lines);
		
		if (memcmp(ghf_js_lines[0], "\xEF\xBB\xBF", 3) == 0)
		{ //Found a UTF8 BOM
			unsigned long ghf_line;
			
			for (ghf_line=0; ghf_line < num_lines; ghf_line++)
			{
				char* this_line = ghf_js_lines[ghf_line];
				unsigned char idx, start_str, end_str = 0;
				
				for (idx=0; idx < strlen(this_line); idx++)
				{
					//UTF8-BOM+ start of comment (/*) or just start of comment (/*)? skip the line
					if (memcmp(this_line+idx, "\xEF\xBB\xBF/*", 5) == 0 || memcmp(this_line+idx, "/*", 2) == 0)
					{
						continue;
					}
					//skip lines shorter than 4 bytes (ghf. is 4 bytes, which is malformed) + skip empty lines
					if (strlen(this_line) < 5 || this_line[idx] == '\n')
					{
						continue;
					}
					if (this_line[idx] == '"' && this_line[idx-1] == '=')
					{
						start_str=idx+1;
					}
					if (this_line[idx] == '"' && this_line[idx+1] == ';')
					{
						end_str=idx;
					}
				}
				if (start_str > 0 && end_str > 0)
				{
					char* val=(char*)calloc(end_str-start_str<8?8:(end_str-start_str)+1, sizeof(char));
					memcpy(val, this_line+start_str, end_str-start_str);
					GHFstrings[ghf_idx]=val;
					ghf_idx++;
				}
			}
		}
		free_null_terminated_string_array(ghf_js_lines);
	}
	
	free(ghfs_path);
	return GHFstrings;
	// don't forget to free_null_terminated_string_array(GHFstrings-or whatever);
#endif
}

#endif
