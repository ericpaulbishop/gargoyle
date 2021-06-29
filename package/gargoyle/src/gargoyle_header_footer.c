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
#include <mntent.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>


#include <erics_tools.h>
#include <uci.h>
#include <json-c/json.h>
#define malloc safe_malloc
#define strdup safe_strdup

#define MINIMAL_HEADER 1
#define HEADER 2
#define FOOTER 3

#define DEFAULT_IF_FILE "/etc/gargoyle_default_ifs"

void define_package_vars(char** package_vars_to_load);
void print_interface_vars(char** ptr_lan_ip, char** ptr_wan_ip, list** ptr_lan_ip6, list** ptr_lan_mask6, list** ptr_wan_ip6, list** ptr_wan_mask6);
void print_hostname_map(char* ptr_lan_ip, char* ptr_wan_ip, list* ptr_lan_ip6, list* ptr_lan_mask6, list* ptr_wan_ip6, list* ptr_wan_mask6);
void print_js_var(char* var, char* value);
void print_js_list_var(char* var, list** value);
char* get_interface_mac(char* if_name);
void* get_interface_ip(char* if_name, int family);
void* get_interface_netmask(char* if_name, int family);
char* get_interface_gateway(char* if_name, int family);
int load_saved_default_interfaces( char** default_lan_if, char** default_wan_if, char** default_wan_mac);
void save_default_interfaces(char* default_lan_if, char* default_wan_if, char* default_wan_mac);
char** load_interfaces_from_proc_file(char* filename);
string_map* get_hostnames(char* ptr_lan_ip, char* ptr_wan_ip, list* ptr_lan_ip6, list* ptr_lan_mask6, list* ptr_wan_ip6, list* ptr_wan_mask6);
char* get_option_value_string(struct uci_option* uopt);
int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name);
char** ParseGHF_TranslationStrings(char* web_root, char* active_lang, char* fallback_lang);
bool is_unusable_overlayfs(void);
char* ip6_mask(char* addr, long mask);
char* ip6_combine_prefix_hostid(char* ip6, char* hostid);
json_object* getInterfaceDump(void);
void get_ifstatus_ip6addrs(json_object* iface, list* ifstatus_ip6, list* ifstatus_mask6);
void get_ifstatus_gateway(json_object* iface, char** ifstatus_gateway);
void get_ifstatus_ipaddrs(json_object* iface, char** ifstatus_ip, char* ifstatus_mask);


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
	char* desc = "Router Management Utility";
	char* dname = "Device Name";
	char* wait_txt = "Please Wait While Settings Are Applied";
	char* unusable_overlay = "Router storage full (read only) or is mounted in volatile storage (RAM). It will not function correctly, or will revert to a previous state after rebooting. Restore default configuration or flash new firmware (without preserving settings).";
	char** package_variables_to_load = NULL;
	int c;

	while((c = getopt(argc, argv, "MmHhFfS:s:P:p:C:c:J:j:Z:z:T:t:IiNnUu")) != -1)	 //section, page, css includes, javascript includes, title, output interface variables, hostnames, usage
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

	char* web_root = "/www";
	char* theme_root = "/themes";
	char* theme = "default";

	int collapsible_menus=0;
	int theme_js=0;
	char* test_collapse ;
	char* test_theme_js ;

	if(uci_load(ctx, "gargoyle", &p) != UCI_OK)
	{
		printf("ERROR: no gargoyle package defined!\n");
		return 0;
	}
	if(get_uci_option(ctx, &e, p, "gargoyle", "global", "web_root") == UCI_OK)
	{
		web_root=get_option_value_string(uci_to_option(e));
	}
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

	test_collapse = dynamic_strcat(5, web_root, theme_root, "/", theme, "/collapseMenus.txt");
	test_theme_js = dynamic_strcat(5, web_root, theme_root, "/", theme, "/theme.js");
	if (path_exists(test_collapse) == PATH_IS_REGULAR_FILE || path_exists(test_collapse) == PATH_IS_SYMLINK)
	{
		collapsible_menus=1;
	}
	if (path_exists(test_theme_js) == PATH_IS_REGULAR_FILE || path_exists(test_theme_js) == PATH_IS_SYMLINK)
	{
		theme_js=1;
	}

	if(display_type == HEADER || display_type == MINIMAL_HEADER)
	{
		printf("Content-Type: text/html; charset=utf-8\n\n");



		char* js_root = "js";
		char* bin_root = ".";
		char* gargoyle_version = "default";
		char* fallback_lang = "";
		char* active_lang = "";
		char* test_theme = "";
		char* selected_section_page = "";
		char* page_subtitle = "";
		char** translation_strings = NULL;


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
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "bin_root") == UCI_OK)
		{
			bin_root=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "version") == UCI_OK)
		{
			char* raw_version = get_option_value_string(uci_to_option(e));

			/* adjust version to ensure it is valid query string */
			int ri;
			gargoyle_version = strdup(raw_version);			 /* just for allocating memory */
			for(ri = 0; raw_version[ri] != '\0'; ri++)
			{
				unsigned char ch= raw_version[ri];
				if( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '_' || ch == '~' )				 /* valid query string characters */
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

		selected_section_page = dynamic_strcat(3, selected_section, "_", selected_page);
		if(get_uci_option(ctx, &e, p, "gargoyle", "display", selected_section_page) == UCI_OK)
		{
			page_subtitle = dynamic_strcat(2, " - ", get_option_value_string(uci_to_option(e)));
		}

		printf("<!DOCTYPE html>\n"
			   "<head>\n"
			   "\t<meta charset=\"utf-8\">\n"
			   "\t<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n"
			   "\t<meta name=\"description\" content=\"Gargoyle Firmware Webgui for router management.\">\n"
			   "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
			   "\t<title>%s%s</title>\n", translation_strings == NULL ? title : translation_strings[0], page_subtitle);
		test_theme = dynamic_strcat(5, web_root, theme_root, "/", theme, "/images/favicon.png");
		//if the theme includes this file, use it, otherwise fallback to default gargoyle file
		if (path_exists(test_theme) == PATH_IS_REGULAR_FILE || path_exists(test_theme) == PATH_IS_SYMLINK)
		{
			printf("\t<link rel=\"shortcut icon\" href=\"%s/%s/images/favicon.png\"/>\n", theme_root, theme);
		}
		else
		{
			printf("\t<link rel=\"shortcut icon\" href=\"%s/Gargoyle/images/favicon.png\"/>\n", theme_root);
		}
		test_theme = dynamic_strcat(5, web_root, theme_root, "/", theme, "/bootstrap.min.css");
		//if the theme includes this file, use it, otherwise fallback to default gargoyle file
		if (path_exists(test_theme) == PATH_IS_REGULAR_FILE || path_exists(test_theme) == PATH_IS_SYMLINK)
		{
			printf("\t<link rel=\"stylesheet\" href=\"%s/%s/bootstrap.min.css?%s\">\n", theme_root, theme, gargoyle_version);
		}
		else
		{
			printf("\t<link rel=\"stylesheet\" href=\"%s/Gargoyle/bootstrap.min.css?%s\">\n", theme_root, gargoyle_version);
		}

		int css_index, js_index, lstr_js_index;
		for(css_index=0; all_css[css_index] != NULL; css_index++)
		{
			test_theme = dynamic_strcat(6, web_root, theme_root, "/", theme, "/", all_css[css_index]);
			//if the theme includes this file, use it, otherwise fallback to default gargoyle file
			if (path_exists(test_theme) == PATH_IS_REGULAR_FILE || path_exists(test_theme) == PATH_IS_SYMLINK)
			{
				printf("\t<link rel=\"stylesheet\" href=\"%s/%s/%s?%s\"/>\n", theme_root, theme, all_css[css_index], gargoyle_version);
			}
			else
			{
				printf("\t<link rel=\"stylesheet\" href=\"%s/Gargoyle/%s?%s\"/>\n", theme_root, all_css[css_index], gargoyle_version);
			}
		}
		//We won't test if this theme.css doesn't exist because each theme must now at a minimum include this file
		printf("\t<link rel=\"stylesheet\" href=\"%s/%s/theme.css?%s\">\n", theme_root, theme, gargoyle_version);
		for(js_index=0; all_js[js_index] != NULL; js_index++)
		{
			printf("\t<script src=\"%s/%s?%s\"></script>\n", js_root, all_js[js_index], gargoyle_version);
		}
#ifndef LOCALIZED_BUILD
		if(active_lang[0] != '\0' && fallback_lang[0] != '\0')
		{
			if (memcmp(fallback_lang, active_lang, strlen(active_lang)) != 0)
			{
					printf("\t<script src=\"/i18n/%s/strings.js?%s\"></script>\n", fallback_lang, gargoyle_version);
			}
			printf("\t<script src=\"i18n/%s/strings.js?%s\"></script>\n", active_lang, gargoyle_version);
			for(lstr_js_index=0; all_lstr_js[lstr_js_index] != NULL; lstr_js_index++)
			{
				if (memcmp(fallback_lang, active_lang, strlen(active_lang)) != 0)
				{
					printf("\t<script src=\"/i18n/%s/%s?%s\"></script>\n", fallback_lang, all_lstr_js[lstr_js_index], gargoyle_version);
				}
				char* tran_file = dynamic_strcat(5, web_root, "/i18n/", active_lang, "/", all_lstr_js[lstr_js_index]);
				if (path_exists(tran_file) == PATH_IS_REGULAR_FILE || path_exists(tran_file) == PATH_IS_SYMLINK)
				{
					printf("\t<script src=\"/i18n/%s/%s?%s\"></script>\n", active_lang, all_lstr_js[lstr_js_index], gargoyle_version);
				}
				free(tran_file);
			}
		}
#endif
		if(theme_js)
		{
			printf("\t<script src=\"%s/%s/theme.js?%s\"></script>\n", theme_root, theme, gargoyle_version);
		}

		printf("</head>\n"
			   "<body>\n");
		if(display_type == HEADER)
		{
			printf("\t<div id=\"darken\"><iframe id=\"d_iframe\" class=\"select_free\"></iframe></div>\n"
				   "\t<div id=\"wait_msg\">\n"
				   "\t\t<div id=\"wait_txt\">\n"
				   "\t\t\t%s\n"
				   "\t\t</div>\n"
				   "\t\t<div id=\"wait_icon\">\n", translation_strings == NULL ? wait_txt : translation_strings[3]);
			test_theme = dynamic_strcat(5, web_root, theme_root, "/", theme, "/images/wait_icon.gif");
			//if the theme includes this file, use it, otherwise fallback to default gargoyle file
			if (path_exists(test_theme) == PATH_IS_REGULAR_FILE || path_exists(test_theme) == PATH_IS_SYMLINK)
			{
				printf("\t\t\t<img src=\"%s/%s/images/wait_icon.gif\"/>\n", theme_root, theme);
			}
			else
			{
				printf("\t\t\t<img src=\"%s/Gargoyle/images/wait_icon.gif\"/>\n", theme_root);
			}
			printf("\t\t</div>\n"
				   "\t\t<iframe id=\"m_iframe\" class=\"select_free\"></iframe>\n"
				   "\t</div>\n"
				   "\t<div id=\"row-offcanvas\" class=\"row-offcanvas full-height\">\n"
				   "\t\t<div id=\"wrapper\" class=\"container-fluid full-height\">\n");

			printf("\t\t\t<div id=\"content\" class=\"col-xs-12 col-md-10 col-lg-10 col-md-push-2 col-lg-push-2 full-height\">\n"
				   "\t\t\t\t<div id=\"topnavbar\" class=\"navbar navbar-default\">\n"
				   "\t\t\t\t\t<div class=\"container-fluid\">\n"
				   "\t\t\t\t\t\t<div class=\"navbar-header\">\n"
				   "\t\t\t\t\t\t\t<button type=\"button\" class=\"btn btn-default sidebar-toggle navbar-toggle\" onclick=\"sidebar()\">\n"
				   "\t\t\t\t\t\t\t\t<span class=\"sr-only\">Toggle navigation</span>\n"
				   "\t\t\t\t\t\t\t\t<span class=\"icon-bar\"></span>\n"
				   "\t\t\t\t\t\t\t\t<span class=\"icon-bar\"></span>\n"
				   "\t\t\t\t\t\t\t\t<span class=\"icon-bar\"></span>\n"
				   "\t\t\t\t\t\t\t</button>\n"
				   "\t\t\t\t\t\t\t<span class=\"navbar-brand\">%s</span>\n", translation_strings == NULL ? desc : translation_strings[1]);

			printf("\t\t\t\t\t\t</div>\n"//navbar-header end
				   "\t\t\t\t\t</div>\n"//container-fluid end
				   "\t\t\t\t</div>\n"//topnavbar end
				   "\t\t\t\t<div class=\"row\">\n"
				   "\t\t\t\t\t<div class=\"col-lg-12\">\n"
				   "\t\t\t\t\t\t<div class=\"alert alert-danger\" style=\"display:%s;\">%s</div>\n",is_unusable_overlayfs() ? "block" : "none", translation_strings == NULL ? unusable_overlay : translation_strings[4]);
		}

		printf("<script>\n");
		printf("\tvar gargoyleBinRoot = \"%s/%s\";\n", web_root, bin_root);
		printf("\tvar haveCollapsibleMenus = %d;\n", collapsible_menus);
		printf("\tvar haveThemeJs = %d;\n", theme_js);


		char* ptr_lan_ip = NULL;
		char* ptr_wan_ip = NULL;
		list* ptr_lan_ip6 = NULL;
		list* ptr_lan_mask6 = NULL;
		list* ptr_wan_ip6 = NULL;
		list* ptr_wan_mask6 = NULL;
		if(display_interface_vars == 1)
		{
			print_interface_vars(&ptr_lan_ip, &ptr_wan_ip, &ptr_lan_ip6, &ptr_lan_mask6, &ptr_wan_ip6, &ptr_wan_mask6);
		}
		if(display_hostname_map == 1)
		{
			print_hostname_map(ptr_lan_ip, ptr_wan_ip, ptr_lan_ip6, ptr_lan_mask6, ptr_wan_ip6, ptr_wan_mask6);
		}
		define_package_vars(package_variables_to_load);
		printf("\n\tsetBrowserTimeCookie();\n"
			   "\n\tvar testAjax = getRequestObj();\n"
			   "\tif(!testAjax) { window.location = \"no_ajax.sh\"; }\n"
			   "</script>\n"
			   "\n\n");
		free_null_terminated_string_array(translation_strings);
	}
	else if(display_type == FOOTER)
	{
		uci_unload(ctx, p);
		if(uci_load(ctx, "system", &p) != UCI_OK)
		{
			printf("ERROR: no system package defined!\n");
			return 0;
		}
		char* bin_root = ".";
		char* hostname = "";
		char* fallback_lang = "";
		char* active_lang = "";
		char* test_theme = "";
		char** translation_strings = NULL;
		char** all_lstr_js;
		char whitespace_separators[] = { '\t', ' ' };
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
		uci_unload(ctx, p);
		if(uci_load(ctx, "gargoyle", &p) != UCI_OK)
		{
			printf("ERROR: no gargoyle package defined!\n");
			return 0;
		}
		if(get_uci_option(ctx, &e, p, "gargoyle", "global", "bin_root") == UCI_OK)
		{
			bin_root=get_option_value_string(uci_to_option(e));
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

		printf("\t\t\t\t\t</div>\n"				//col-lg-12
			   "\t\t\t\t</div>\n"				//row
			   "\t\t\t</div>\n");			      //content

		printf("\t\t\t<div id=\"sidebar\" class=\"col-xs-12 col-md-2 col-lg-2 col-md-pull-10 col-lg-pull-10 full-height\">\n"
			   "\t\t\t\t<ul class=\"nav sidebar\" >\n"
			   "\t\t\t\t\t<li class=\"sidebar-header\">\n"//sidebar header begin
			   "\t\t\t\t\t\t<span id=\"garg_title\">Gargoyle</span><br/>\n");
		test_theme = dynamic_strcat(5, web_root, theme_root, "/", theme, "/images/gargoyle-logo.png");
		//if the theme includes this file, use it, otherwise fallback to default gargoyle file
		if (path_exists(test_theme) == PATH_IS_REGULAR_FILE || path_exists(test_theme) == PATH_IS_SYMLINK)
		{
			printf("\t\t\t\t\t\t<img src=\"%s/%s/images/gargoyle-logo.png\" class=\"avatar\" alt=\"Gargoyle Logo\"><br/>\n", theme_root, theme);
		}
		else
		{
			printf("\t\t\t\t\t\t<img src=\"%s/Gargoyle/images/gargoyle-logo.png\" class=\"avatar\" alt=\"Gargoyle Logo\"><br/>\n", theme_root);
		}
		printf("\t\t\t\t\t\t<span id=\"garg_host\">%s: %s</span>\n", translation_strings == NULL ? dname : translation_strings[2], hostname);
		printf("\t\t\t\t\t</li>\n");//<!-- sidebar-header end-->


		int maj_counter=1;
		int min_counter=0;
		int first_section=1;
		int prev_section_selected=0;
		priority_queue_node *next_section;
		while( (next_section=shift_priority_queue_node(sections)) != NULL)
		{
			char* section_display = "";
			priority_queue* section_pages;
			section_pages = (priority_queue*)next_section->value;
			if(get_uci_option(ctx, &e, p, "gargoyle", "display", next_section->id) == UCI_OK)
			{
				section_display = get_option_value_string(uci_to_option(e));
			}
			if(strcmp(selected_section, next_section->id) == 0)
			{
					//ACTIVE Major heading e.g. "Status"
					if(peek_priority_queue_node(section_pages) == NULL)
					{
						//ZERO sub-pages for this section
						printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item active\">%s</li>\n", maj_counter, min_counter, section_display);
					}
					else
					{
						char* top_class_added = strdup("sidebar-top-subelement");
						//ONE OR MORE sub-pages for this section
						if(collapsible_menus == 1)
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item active\"><a href=\"#\" >%s</a>\n", maj_counter, min_counter, section_display);
						}
						else
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item active\"><a onclick=\"return true\">%s</a>\n", maj_counter, min_counter, section_display);
						}

						//Start of sub-page links
						printf("\t\t\t\t\t\t<ul class=\"sidebar-list active\">\n");
						priority_queue_node *next_section_page;
						while( (next_section_page=shift_priority_queue_node(section_pages)) != NULL)
						{
							min_counter++;
							char* page_display="";
							char* page_script="";
							char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
							if(section_pages->length == 0)
							{
								char* old = top_class_added;
								char* new = dynamic_strcat(2, top_class_added, " sidebar-bottom-subelement");
								top_class_added = new;
								free(old);
							}
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
								//ACTIVE sub-page e.g. "Overview"
								printf("\t\t\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item minor-sidebar-item active %s\">%s</li>\n", maj_counter, min_counter, top_class_added, page_display);
							}
							else
							{
								//INACTIVE sub-page e.g. "Overview"
								char* bin_slash = bin_root[0] == '/' ? strdup("") : strdup("/");
								char* page_slash = page_script[0] == '/' ? strdup("") : strdup("/");
								if(strcmp(bin_root, ".") == 0)
								{
									//DEFAULT web-root
									printf("\t\t\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item minor-sidebar-item %s\"><a href=\"%s%s\">%s</a>", maj_counter, min_counter, top_class_added, page_slash, page_script, page_display);
								}
								else
								{
									//CUSTOM web-root
									printf("\t\t\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item minor-sidebar-item %s\"><a href=\"%s%s%s%s\">%s</a>", maj_counter, min_counter, bin_slash, bin_root, top_class_added, page_slash, page_script, page_display);
								}
								free(bin_slash);
								free(page_slash);
								printf("</li>\n");
							}
							free(top_class_added);
							top_class_added = strdup("");
							free(lookup);
					
						}
						printf("\t\t\t\t\t\t</ul>\n");//End of sub-page links
						printf("\t\t\t\t\t</li>\n");//End of MAJOR heading
						free(top_class_added);
				}
				prev_section_selected=1;
			}
			else
			{
				//INACTIVE Major heading e.g. "Status"
				priority_queue_node *next_section_page;
				char* next_section_script = "";
				char* top_class_added = strdup("sidebar-top-subelement");

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

				int empty_section = 0;
				if (next_section_page == NULL)
				{
					empty_section = 1;
				}
				char* bin_slash = bin_root[0] == '/' ? strdup("") : strdup("/");
				char* script_slash = next_section_script[0] == '/' ? strdup("") : strdup("/");
				if(strcmp(bin_root, ".") == 0)
				{
					//DEFAULT web-root
					if (collapsible_menus == 1)
					{
						if (empty_section == 0)
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item \"><a href=\"#\" onclick=\"uncollapseNavThis(this);return false\">%s</a>\n", maj_counter, min_counter, section_display);
						}
						else
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item \"><a href=\"%s%s\" onclick=\"return true\">%s</a>\n", maj_counter, min_counter, script_slash, next_section_script, section_display);
						}
					}
					else
					{
						printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item \"><a href=\"%s%s\" onclick=\"return true\">%s</a>\n", maj_counter, min_counter, script_slash, next_section_script, section_display);
					}
				}
				else
				{
					//CUSTOM web-root
					if (collapsible_menus == 1)
					{
						if (empty_section == 0)
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item \"><a href=\"#\" onclick=\"uncollapseNavThis(this);return false\">%s</a>\n", maj_counter, min_counter, section_display);
						}
						else
						{
							printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item \"><a href=\"%s%s%s%s\" onclick=\"return true\">%s</a>\n", maj_counter, min_counter, bin_slash, bin_root, script_slash, next_section_script, section_display);
						}
					}
					else
					{
						printf("\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item major-sidebar-item\"><a href=\"%s%s%s%s\" onclick=\"return true\">%s</a>\n", maj_counter, min_counter, bin_slash, bin_root, script_slash, next_section_script, section_display);
					}
				}

				if (empty_section == 0)
				{
					//Start of sub-page links
					printf("\t\t\t\t\t\t<ul class=\"sidebar-list\">\n");//<!-- sidebar-list 2nd list -->
				}
				while(next_section_page != NULL)
				{
					//INACTIVE sub-page e.g. "Overview"
					//ONE OR MORE sub-pages for this section
					min_counter++;
					char* page_display="";
					char* page_script="";
					char* lookup = dynamic_strcat(3, next_section->id, "_", next_section_page->id);
					if(section_pages->length == 0)
					{
						char* old = top_class_added;
						char* new = dynamic_strcat(2, top_class_added, " sidebar-bottom-subelement");
						top_class_added = new;
						free(old);
					}

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
					{	//DEFAULT web-root
						printf("\t\t\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item minor-sidebar-item %s\"><a href=\"%s%s\">%s</a>", maj_counter, min_counter, top_class_added, page_slash, page_script, page_display);
						printf("</li>\n");
					}
					else
					{
						//CUSTOM web-root
						printf("\t\t\t\t\t\t\t<li id=\"nav_MAJ%02d_MIN%02d\" class=\"sidebar-item minor-sidebar-item %s\"><a href=\"%s%s%s%s\">%s</a>", maj_counter, min_counter, top_class_added, bin_slash, bin_root, page_slash, page_script, page_display);
						printf("</li>\n");
					}
					free(top_class_added);
					top_class_added = strdup("");

					free(bin_slash);
					free(page_slash);
					free(lookup);
					next_section_page=shift_priority_queue_node(section_pages);
				}
				if (empty_section == 0)
				{
					printf("\t\t\t\t\t\t</ul>\n");//End of sub-page links
				}
				printf("\t\t\t\t\t</li>\n");//End of MAJOR heading
				prev_section_selected=0;
				free(top_class_added);
			}
			first_section = 0;
			maj_counter++;
			min_counter=0;
		}

		printf("\t\t\t\t</ul>\n"//sidebar end
			   "\t\t\t\t<div class=\"sidebar-footer\">\n"
			   "\t\t\t\t\t<div class=\"col-xs-6\">\n"
			   "\t\t\t\t\t\t<a href=\"/logout.sh\">Logout</a>\n"
			   "\t\t\t\t\t</div>\n"
			   "\t\t\t\t\t<div class=\"col-xs-6\">\n"
			   "\t\t\t\t\t\t<a href=\"https://www.gargoyle-router.com/\" target=\"_blank\">Support</a>\n"
			   "\t\t\t\t\t</div>\n"
			   "\t\t\t\t</div>\n" //sidebar-footer
			   "\t\t\t</div>\n" //sidebar
			   "\t\t</div>\n" //wrapper
			   "\t</div>\n"); //row-offcanvas


		printf("\t</body>\n"
			   "</html>\n");
		free_null_terminated_string_array(translation_strings);
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

json_object* getInterfaceDump(void)
{
	FILE *fp;	
	unsigned long read_length;
	struct json_object* jobj;

	fp = popen("ubus -S call network.interface dump", "r");
	if (fp == NULL)
	{
		//printf("network interface dump failed\n");
		return NULL;
	}

	char* ifstatus = read_entire_file(fp, 100, &read_length);
	jobj = json_tokener_parse(ifstatus);
	free(ifstatus);
	
	//printf("<!--\ninterface dump:\n%s\n-->\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
	
	return jobj;
}

void get_ifstatus_ip6addrs(json_object* iface, list* ifstatus_ip6, list* ifstatus_mask6)
{
	struct json_object* tmpobj = NULL;
	struct json_object* ip6arr = NULL;
	struct json_object* localaddrobj = NULL;
	struct json_object* addressobj = NULL;
	struct json_object* maskobj = NULL;
	const char* address = NULL;
	const char* mask = NULL;
	int ip6arrlen = 0;
	json_object_object_get_ex(iface, "ipv6-address", &ip6arr);
	if(ip6arr != NULL)
	{
		ip6arrlen = json_object_array_length(ip6arr);
		
		for(int idx = 0; idx < ip6arrlen; idx++)
		{
			tmpobj = json_object_array_get_idx(ip6arr, idx);
			json_object_object_get_ex(tmpobj, "address", &addressobj);
			address = json_object_get_string(addressobj);
			json_object_object_get_ex(tmpobj, "mask", &maskobj);
			mask = json_object_get_string(maskobj);
			
			push_list(ifstatus_ip6, (void*)strdup(address));
			push_list(ifstatus_mask6, (void*)strdup(mask));
		}
	}
	
	json_object_object_get_ex(iface, "ipv6-prefix-assignment", &ip6arr);
	if(ip6arr != NULL)
	{
		ip6arrlen = json_object_array_length(ip6arr);
		
		for(int idx = 0; idx < ip6arrlen; idx++)
		{
			tmpobj = json_object_array_get_idx(ip6arr, idx);
			json_object_object_get_ex(tmpobj, "local-address", &localaddrobj);
			json_object_object_get_ex(localaddrobj, "address", &addressobj);
			address = json_object_get_string(addressobj);
			json_object_object_get_ex(localaddrobj, "mask", &maskobj);
			mask = json_object_get_string(maskobj);
			
			push_list(ifstatus_ip6, (void*)strdup(address));
			push_list(ifstatus_mask6, (void*)strdup(mask));
		}
	}
}

void get_ifstatus_gateway(json_object* iface, char** ifstatus_gateway)
{
	struct json_object* tmpobj = NULL;
	struct json_object* routesarr = NULL;
	struct json_object* targetobj = NULL;
	struct json_object* nexthopobj = NULL;
	const char* target = NULL;
	int routearrlen = 0;
	json_object_object_get_ex(iface, "route", &routesarr);
	
	if(routesarr != NULL)
	{
		routearrlen = json_object_array_length(routesarr);

		for(int idx = 0; idx < routearrlen; idx++)
		{
			tmpobj = json_object_array_get_idx(routesarr, idx);
			json_object_object_get_ex(tmpobj, "target", &targetobj);
			target = json_object_get_string(targetobj);
			if(safe_strcmp(target, "::") == 0 || safe_strcmp(target, "0.0.0.0") == 0)
			{
				json_object_object_get_ex(tmpobj, "nexthop", &nexthopobj);
				*ifstatus_gateway = json_object_get_string(nexthopobj);
			}
		}
	}
}

void get_ifstatus_ipaddrs(json_object* iface, char** ifstatus_ip, char* ifstatus_mask)
{
	struct json_object* tmpobj = NULL;
	struct json_object* tmpobj2 = NULL;
	uint32_t maskbits = 0;

	json_object_object_get_ex(iface, "ipv4-address", &tmpobj);
	if(tmpobj != NULL)
	{
		tmpobj = json_object_array_get_idx(tmpobj, 0);
		json_object_object_get_ex(tmpobj, "address", &tmpobj2);
		*ifstatus_ip = json_object_get_string(tmpobj2);
		json_object_object_get_ex(tmpobj, "mask", &tmpobj2);
		//ifstatus_wan_mask = json_object_get_string(tmpobj2);
		maskbits = json_object_get_int(tmpobj2);
		maskbits = (0xFFFFFFFF << (32 - maskbits)) & 0xFFFFFFFF;
		sprintf(ifstatus_mask, "%u.%u.%u.%u", maskbits >> 24, (maskbits >> 16) & 0xFF, (maskbits >> 8) & 0xFF, maskbits & 0xFF);
	}
}

void print_interface_vars(char** ptr_lan_ip, char** ptr_wan_ip, list** ptr_lan_ip6, list** ptr_lan_mask6, list** ptr_wan_ip6, list** ptr_wan_mask6)
{
	struct json_object* ifdump = getInterfaceDump();
	struct json_object* if_lan = NULL;
	struct json_object* if_wan = NULL;
	struct json_object* if_wan6 = NULL;
	
	struct json_object* interfaces_arr;
	int ifacearrlen = 0;
	json_object_object_get_ex(ifdump, "interface", &interfaces_arr);
	if(interfaces_arr != NULL)
	{
		ifacearrlen = json_object_array_length(interfaces_arr);
		
		for(int idx = 0; idx < ifacearrlen; idx++)
		{
			struct json_object* tmpobj = NULL;
			struct json_object* nameobj = NULL;
			const char* name = NULL;
			
			tmpobj = json_object_array_get_idx(interfaces_arr, idx);
			json_object_object_get_ex(tmpobj, "interface", &nameobj);
			name = json_object_get_string(nameobj);
			
			if(safe_strcmp(name, "lan") == 0)
			{
				if_lan = json_object_get(tmpobj);
			}
			else if(safe_strcmp(name, "wan") == 0)
			{
				if_wan = json_object_get(tmpobj);
			}
			else if(safe_strcmp(name, "wan6") == 0)
			{
				if_wan6 = json_object_get(tmpobj);
			}
		}
	}
	
	// Free bits that haven't been grabbed separately
	json_object_put(ifdump);
	
	// GENERIC
	char* default_lan_if  = NULL;
	char* default_wan_if  = NULL;
	char* default_wan_mac = NULL;
	int loaded_default_ifs = load_saved_default_interfaces( &default_lan_if, &default_wan_if, &default_wan_mac);
	
	// WIRELESS
	list* wireless_ifs  = initialize_list();
	list* wireless_uci = initialize_list();
	
	char** wireless_if_arr = load_interfaces_from_proc_file("/proc/net/wireless");
	int wif_index;
	for(wif_index = 0; wireless_if_arr[wif_index] != NULL; wif_index++)
	{
		push_list(wireless_ifs, (void*)strdup(wireless_if_arr[wif_index]));
	}
	free_null_terminated_string_array(wireless_if_arr);

	struct uci_context *ctx = uci_alloc_context();
	struct uci_context *state_ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	struct uci_element *e = NULL;

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
	
	/*
	 * A few notes on "if" vs "dev" vs "l3dev" variable names:
	 * When doing load "if" is the ifname variable loaded from the /etc/config/network file (e.g. eth1.1),
	 * while "dev" is the "device" loaded from ifstatus (e.g. eth1.1, br-lan), and "l3dev" is the
	 * "l3_device" from ifstatus (e.g. br-lan, pppoe-wan).
	 *
	 * "dev" and "l3dev" can be equal, and either one can be missing as well
	 *
	 * Note also that "l3dev" interface variables will contain the pppoe device name if we're using pppoe.
	 * Therefore we print these out in addition to the interface list as javascript variables labeled
	 * currentLanName and currentWanName
	 */
	 
	// NETWORK UCI
	char* uci_wan_mac = NULL;
	char* uci_wan_if = NULL;
	char* uci_wan_type = NULL;
	char* uci_wan_ip = NULL;
	char* uci_wan_mask = NULL;
	char* uci_wan_gateway = NULL;
	char* uci_wan_gateway6 = NULL;
	
	char* uci_lan_if = NULL;
	char* uci_lan_ip = NULL;
	char* uci_lan_mask = NULL;
	char* uci_ula_prefix = NULL;
	char* uci_ula_mask = NULL;
	
	if(uci_load(ctx, "network", &p) == UCI_OK)
	{
		char* wan_dev = dynamic_strcat(3,"wan_",default_wan_if,"_dev");
		char* tmp_dev = dynamic_replace(wan_dev, ".", "_");
		free(wan_dev);
		wan_dev = tmp_dev;
		if(get_uci_option(ctx, &e, p, "network", wan_dev, "macaddr") == UCI_OK)
		{
			uci_wan_mac=get_option_value_string(uci_to_option(e));
		}
		else if(get_uci_option(ctx, &e, p, "network", "wan", "macaddr") == UCI_OK)
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
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "ipaddr") == UCI_OK)
		{
			uci_wan_ip=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "netmask") == UCI_OK)
		{
			uci_wan_mask=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "gateway") == UCI_OK)
		{
			uci_wan_gateway=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "wan", "ip6gw") == UCI_OK)
		{
			uci_wan_gateway6=get_option_value_string(uci_to_option(e));
		}

		if(get_uci_option(ctx, &e, p, "network", "lan", "ifname") == UCI_OK)
		{
			uci_lan_if = get_option_value_string(uci_to_option(e));
		}		
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "ipaddr") == UCI_OK)
		{
			uci_lan_ip=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "lan", "netmask") == UCI_OK)
		{
			uci_lan_mask=get_option_value_string(uci_to_option(e));
		}
		if(get_uci_option(state_ctx, &e, p, "network", "globals", "ula_prefix") == UCI_OK)
		{
			char* tmpulaprefix=get_option_value_string(uci_to_option(e));
			char* subnet_sep = "/";
			char* subnet = strstr(tmpulaprefix, subnet_sep);
			if(subnet != NULL)
			{
				uci_ula_prefix = (char*)malloc((subnet-tmpulaprefix+1)*sizeof(char));
				uci_ula_mask = (char*)malloc((strlen(tmpulaprefix)-(subnet-tmpulaprefix)+1)*sizeof(char));
				strncpy(uci_ula_prefix, tmpulaprefix, (subnet-tmpulaprefix));
				uci_ula_prefix[subnet-tmpulaprefix] = '\0';
				strncpy(uci_ula_mask, tmpulaprefix+(subnet-tmpulaprefix)+1, strlen(tmpulaprefix)-(subnet-tmpulaprefix));
				uci_ula_mask[strlen(tmpulaprefix)-(subnet-tmpulaprefix)] = '\0';
			}
			else
			{
				uci_ula_prefix = strdup(tmpulaprefix);
			}
		}

	}
	
	// NETWORK IFSTATUS
	const char* ifstatus_lan_dev = NULL;
	const char* ifstatus_lan_l3dev = NULL;
	const char* ifstatus_lan_ip = NULL;
	char ifstatus_lan_mask[16] = "";
	list* ifstatus_lan_ip6 = initialize_list();
	list* ifstatus_lan_mask6 = initialize_list();
	
	const char* ifstatus_wan_dev = NULL;
	const char* ifstatus_wan_l3dev = NULL;
	const char* ifstatus_wan_ip = NULL;
	char ifstatus_wan_mask[16] = "";
	const char* ifstatus_wan_gateway = NULL;
	
	list* ifstatus_wan_ip6 = initialize_list();
	list* ifstatus_wan_mask6 = initialize_list();
	const char* ifstatus_wan_gateway6 = NULL;
	
	if(if_lan != NULL)
	{
		struct json_object* tmpobj = NULL;
		struct json_object* tmpobj2 = NULL;
		uint32_t maskbits = 0;
		
		json_object_object_get_ex(if_lan, "device", &tmpobj);
		ifstatus_lan_dev = json_object_get_string(tmpobj);
		
		json_object_object_get_ex(if_lan, "l3_device", &tmpobj);
		ifstatus_lan_l3dev = json_object_get_string(tmpobj);
		
		get_ifstatus_ipaddrs(if_lan, &ifstatus_lan_ip, ifstatus_lan_mask);
		get_ifstatus_ip6addrs(if_lan, ifstatus_lan_ip6, ifstatus_lan_mask6);
	}
	if(if_wan != NULL)
	{
		struct json_object* tmpobj = NULL;
		struct json_object* tmpobj2 = NULL;
		uint32_t maskbits = 0;
		
		json_object_object_get_ex(if_wan, "device", &tmpobj);
		ifstatus_wan_dev = json_object_get_string(tmpobj);
		
		json_object_object_get_ex(if_wan, "l3_device", &tmpobj);
		ifstatus_wan_l3dev = json_object_get_string(tmpobj);
		
		get_ifstatus_ipaddrs(if_wan, &ifstatus_wan_ip, ifstatus_wan_mask);
		get_ifstatus_gateway(if_wan, &ifstatus_wan_gateway);
	}
	if(if_wan6 != NULL)
	{
		get_ifstatus_ip6addrs(if_wan6, ifstatus_wan_ip6, ifstatus_wan_mask6);
		get_ifstatus_gateway(if_wan6, &ifstatus_wan_gateway6);
	}
	
	char* current_lan_l3dev = NULL;
	if(ifstatus_lan_l3dev != NULL)
	{
		current_lan_l3dev = strdup(ifstatus_lan_l3dev);
	}
	else if(ifstatus_lan_dev != NULL)
	{
		current_lan_l3dev = strdup(ifstatus_lan_dev);
	}
	else if(uci_lan_if != NULL)
	{
		current_lan_l3dev = strdup(uci_lan_if);
	}
	else
	{
		current_lan_l3dev = strdup("br-lan");
	}
	char* current_lan_if = uci_lan_if != NULL ? strdup(uci_lan_if) : strdup(ifstatus_lan_dev);
	char* current_wan_if = uci_wan_if != NULL ? strdup(uci_wan_if) : strdup(ifstatus_wan_dev);
	char* current_wan_l3dev = NULL;
	if(ifstatus_wan_l3dev != NULL)
	{
		current_wan_l3dev = strdup(ifstatus_wan_l3dev);
	}
	else if(ifstatus_wan_dev != NULL)
	{
		current_wan_l3dev = strdup(ifstatus_wan_dev);
	}
	else if(uci_wan_if != NULL)
	{
		current_wan_l3dev = strdup(uci_wan_if);
	}
	else if(safe_strcmp(uci_wan_type, "bridge") == 0)
	{
		current_wan_l3dev = strdup("br-wan");
	}
	char* current_wan_mac = get_interface_mac(current_wan_l3dev);
	if(safe_strcmp(current_wan_mac, "00:00:00:00:00:00") == 0 || current_wan_mac == NULL)
	{
		current_wan_mac = get_interface_mac(ifstatus_wan_dev);
	}
	char* current_lan_mac = get_interface_mac(current_lan_l3dev);

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
	
	// Get canonical answers
	char* current_lan_ip = NULL;
	if(ifstatus_lan_ip != NULL)
	{
		current_lan_ip = strdup(ifstatus_lan_ip);
	}
	else if(ifstatus_lan_ip != NULL)
	{
		current_lan_ip = strdup(uci_lan_ip);
	}
	else
	{
		current_lan_ip = (char*)get_interface_ip(current_lan_l3dev, AF_INET);
	}
	char* current_lan_mask = NULL;
	if(ifstatus_lan_mask != NULL)
	{
		current_lan_mask = strdup(ifstatus_lan_mask);
	}
	else if(uci_lan_mask != NULL)
	{
		current_lan_mask = strdup(uci_lan_mask);
	}
	else
	{
		current_lan_mask = (char*)get_interface_netmask(current_lan_l3dev, AF_INET);
	}
	list* current_lan_ip6 = NULL;
	if(ifstatus_lan_ip6->length > 0)
	{
		current_lan_ip6 = ifstatus_lan_ip6;
	}
	else
	{
		current_lan_ip6 = (list*)get_interface_ip(current_lan_l3dev, AF_INET6);
	}
	list* current_lan_mask6 = NULL;
	if(ifstatus_lan_mask6->length > 0)
	{
		current_lan_mask6 = ifstatus_lan_mask6;
	}
	else
	{
		current_lan_mask6 = (list*)get_interface_netmask(current_lan_l3dev, AF_INET6);
	}

	char* current_wan_ip = NULL;
	if(ifstatus_wan_ip != NULL)
	{
		current_wan_ip = strdup(ifstatus_wan_ip);
	}
	else if(uci_wan_ip != NULL)
	{
		current_wan_ip = strdup(uci_wan_ip);
	}
	else
	{
		current_wan_ip = (char*)get_interface_ip(current_wan_l3dev, AF_INET);
	}
	char* current_wan_mask = NULL;
	if(ifstatus_wan_mask != NULL)
	{
		current_wan_mask = strdup(ifstatus_wan_mask);
	}
	else if(uci_wan_mask != NULL)
	{
		current_wan_mask = strdup(uci_wan_mask);
	}
	else
	{
		current_wan_mask = (char*)get_interface_netmask(current_wan_l3dev, AF_INET);
	}
	char* current_wan_gateway = NULL;
	if(ifstatus_wan_gateway != NULL)
	{
		current_wan_gateway = strdup(ifstatus_wan_gateway);
	}
	else if(uci_wan_gateway != NULL)
	{
		current_wan_gateway = strdup(uci_wan_gateway);
	}
	else
	{
		current_wan_gateway = (char*)get_interface_gateway(current_wan_l3dev, AF_INET);
	}
	list* current_wan_ip6 = NULL;
	if(ifstatus_wan_ip6->length > 0)
	{
		current_wan_ip6 = ifstatus_wan_ip6;
	}
	else
	{
		current_wan_ip6 = (list*)get_interface_ip(current_wan_l3dev, AF_INET6);
	}
	list* current_wan_mask6 = NULL;
	if(ifstatus_wan_mask6->length > 0)
	{
		current_wan_mask6 = ifstatus_wan_mask6;
	}
	else
	{
		current_wan_mask6 = (list*)get_interface_netmask(current_wan_l3dev, AF_INET6);
	}
	char* current_wan_gateway6 = NULL;
	if(ifstatus_wan_gateway6 != NULL)
	{
		current_wan_gateway6 = strdup(ifstatus_wan_gateway6);
	}
	else if(uci_wan_gateway6 != NULL)
	{
		current_wan_gateway6 = strdup(uci_wan_gateway6);
	}
	else
	{
		current_wan_gateway6 = (char*)get_interface_gateway(current_wan_l3dev, AF_INET6);
	}
	
	// Free ifstatus vars, we should be done with these now
	json_object_put(if_lan);
	json_object_put(if_wan);
	json_object_put(if_wan6);

	// Print variables
	print_js_list_var("wirelessIfs", &wireless_ifs);
	print_js_list_var("uciWirelessDevs", &wireless_uci);
	print_js_list_var("currentWirelessMacs", &wireless_macs);

	print_js_var("defaultLanIf", default_lan_if);
	print_js_var("currentLanIf", current_lan_if);
	print_js_var("currentLanName", current_lan_l3dev);
	print_js_var("currentLanMac", current_lan_mac);
	print_js_var("currentLanIp", current_lan_ip);
	print_js_var("currentLanMask", current_lan_mask);
	print_js_list_var("currentLanIp6", &current_lan_ip6);
	print_js_list_var("currentLanMask6", &current_lan_mask6);
	print_js_var("currentULAPrefix", uci_ula_prefix);
	print_js_var("currentULAMask", uci_ula_mask);

	print_js_var("defaultWanIf", default_wan_if);
	print_js_var("defaultWanMac", default_wan_mac);
	print_js_var("currentWanIf", current_wan_if);
	print_js_var("currentWanName", current_wan_l3dev);
	print_js_var("currentWanMac", current_wan_mac);
	print_js_var("currentWanIp", current_wan_ip);
	print_js_var("currentWanMask", current_wan_mask);
	print_js_var("currentWanGateway", current_wan_gateway);
	print_js_list_var("currentWanIp6", &current_wan_ip6);
	print_js_list_var("currentWanMask6", &current_wan_mask6);
	print_js_var("currentWanGateway6", current_wan_gateway6);

	uci_free_context(ctx);
	uci_free_context(state_ctx);

	*ptr_lan_ip = current_lan_ip;
	*ptr_lan_ip6 = current_lan_ip6;
	*ptr_lan_mask6 = current_lan_mask6;
	*ptr_wan_ip = current_wan_ip;
	*ptr_wan_ip6 = current_wan_ip6;
	*ptr_wan_mask6 = current_wan_mask6;
	/* there are an assload of other variables to free... but, eh, fuck it, this thing doesn't run as a daemon
	 *
	 * variables are defined/copied with strdups, so we won't get any double-free errors if we do decide to implement
	 * code to free them at a later time
	 */
}


void print_hostname_map(char* ptr_lan_ip, char* ptr_wan_ip, list* ptr_lan_ip6, list* ptr_lan_mask6, list* ptr_wan_ip6, list* ptr_wan_mask6)
{
	string_map* ip_to_hostname = get_hostnames(ptr_lan_ip,ptr_wan_ip,ptr_lan_ip6,ptr_lan_mask6,ptr_wan_ip6,ptr_wan_mask6);
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

char* get_interface_gateway(char* if_name, int family)
{
	char* gateway = NULL;
	if(if_name != NULL)
	{
		FILE* route_file = NULL;
		if(family == AF_INET)
		{
			route_file = fopen("/proc/net/route", "r");
		}
		else if(family == AF_INET6)
		{
			route_file = fopen("/proc/net/ipv6_route", "r");
		}
		
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
				if(family == AF_INET)
				{
					if(line_pieces >=4)
					{
						if(strcmp(split_line[0], if_name) == 0)
						{
							unsigned int flags = 0;
							if( sscanf(split_line[3], "%u", &flags) > 0)
							{
								if((flags | 2) != 0)
								{
									uint32_t ip;
									uint8_t* ipptr;
									sscanf(split_line[2], "%X", &ip);
									ip = htonl(ip);
									ipptr = &ip;
									gateway = (char*)malloc(20);
									sprintf(gateway, "%u.%u.%u.%u", *(ipptr+3), *(ipptr+2), *(ipptr+1), *ipptr);
								}
							}
						}
					}
				}
				else if(family == AF_INET6)
				{
					if(line_pieces >= 10)
					{
						if(strcmp(split_line[9], if_name) == 0 && strcmp(split_line[0], "00000000000000000000000000000000") == 0)
						{
							unsigned int flags = 0;
							if(sscanf(split_line[8], "%u", &flags) > 0)
							{
								if((flags | 2) != 0)
								{
									unsigned int ip[8];
									sscanf(split_line[4], "%4x%4x%4x%4x%4x%4x%4x%4x", ip, ip+1, ip+2, ip+3, ip+4, ip+5, ip+6, ip+7);
									gateway = (char*)malloc(42);
									sprintf(gateway, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7]);
								}
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

void* get_interface_ip(char* if_name, int family)
{
	struct ifaddrs* ifap;
	struct ifaddrs* ifa;
	struct sockaddr_in* in;
	struct sockaddr_in6* in6;
	void* ip = NULL;
	char* addr = NULL;

	if(family == AF_INET)
	{
		addr = (char*)malloc(INET_ADDRSTRLEN);
	}
	else if(family == AF_INET6)
	{
		addr = (char*)malloc(INET6_ADDRSTRLEN);
		ip = initialize_list();
	}
	if(if_name == NULL)
	{
		free(addr);
		return ip;
	}

	getifaddrs(&ifap);
	for(ifa = ifap; ifa; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET && (strcmp(ifa->ifa_name,if_name) == 0) && family==AF_INET)
		{
			in = (struct sockaddr_in*)ifa->ifa_addr;
			inet_ntop(AF_INET, &in->sin_addr, addr, INET_ADDRSTRLEN);
			ip = strdup(addr);
		}
		else if(ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET6 && (strcmp(ifa->ifa_name,if_name) == 0) && family==AF_INET6)
		{
			in6 = (struct sockaddr_in6*)ifa->ifa_addr;
			inet_ntop(AF_INET6, &in6->sin6_addr, addr, INET6_ADDRSTRLEN);
			push_list(ip, (void*)strdup(addr));
		}
	}

	freeifaddrs(ifap);
	free(addr);
	return ip;
}

void* get_interface_netmask(char* if_name, int family)
{
	struct ifaddrs* ifap;
	struct ifaddrs* ifa;
	struct sockaddr_in* in;
	struct sockaddr_in6* in6;
	void* netmask = NULL;
	char* addr = NULL;

	if(family == AF_INET)
	{
		addr = (char*)malloc(INET_ADDRSTRLEN);
	}
	else if(family == AF_INET6)
	{
		addr = (char*)malloc(INET6_ADDRSTRLEN);
		netmask = initialize_list();
	}
	if(if_name == NULL)
	{
		free(addr);
		return netmask;
	}

	getifaddrs(&ifap);
	for(ifa = ifap; ifa; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET && (strcmp(ifa->ifa_name,if_name) == 0) && family==AF_INET)
		{
			in = (struct sockaddr_in*)ifa->ifa_netmask;
			inet_ntop(AF_INET, &in->sin_addr.s_addr, addr, INET_ADDRSTRLEN);
			netmask = strdup(addr);
		}
		else if(ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET6 && (strcmp(ifa->ifa_name,if_name) == 0) && family==AF_INET6)
		{
			in6 = (struct sockaddr_in6*)ifa->ifa_addr;
			inet_ntop(AF_INET6, &in6->sin6_addr, addr, INET6_ADDRSTRLEN);

			unsigned char* c = ((struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr.s6_addr;
			int i = 0, j = 0;
			unsigned char n = 0;
			while(i < 16)
			{
				n = c[i];
				while(n > 0)
				{
					if(n & 1)
					{
						j = j+1;
					}

					n = n/2;
				}

				i = i + 1;
			}

			sprintf(addr, "%d", j);
			push_list(netmask, (void*)strdup(addr));
		}
	}

	freeifaddrs(ifap);
	free(addr);
	return netmask;
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

string_map* get_hostnames(char* ptr_lan_ip, char* ptr_wan_ip, list* ptr_lan_ip6, list* ptr_lan_mask6, list* ptr_wan_ip6, list* ptr_wan_mask6)
{
	string_map* ip_to_hostname = initialize_string_map(1);

	char* hostname_files[] = { "/tmp/dhcp.leases", "/tmp/hosts/odhcpd", NULL };
	int ip_indices[] = { 2, 0 };
	int name_indices[] = { 3, 1 };
	char* subnet_sep = "/";
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
				if(num_line_pieces >= min_line_pieces && safe_strcmp(line_pieces[0], "#") != 0)
				{
					char *name = strdup(line_pieces[ name_index ]);
					trim_flanking_whitespace(name);
					if(name[0] != '*' && name[0] != '\0')
					{
						char* ip  = line_pieces[ ip_index ];
						char* subnet = strstr(ip, subnet_sep);
						if(subnet != NULL)
						{
							ip[subnet - ip] = '\0';
						}
						set_string_map_element(ip_to_hostname, ip, name);
					}
					else
					{
						free(name);
					}
				}
				free_null_terminated_string_array(line_pieces);
			} while(sep != EOF);
			fclose(name_file);
		}
	}

	//create prefixes
	list* ip6_prefixes = initialize_list();
	if(ptr_lan_ip6 != NULL)
	{
		char* local_ip6 = "fe80";
		for(unsigned long idx = 0; idx < ptr_lan_ip6->length && idx < ptr_lan_mask6->length; idx++)
		{
			char* ip6 = (char*)list_element_at(ptr_lan_ip6, idx);
			long mask6 = strtol((char*)list_element_at(ptr_lan_mask6, idx), NULL, 10);
			if(strncmp(local_ip6, ip6, 4) != 0 && mask6 <= 96)
			{
				char* masked = ip6_mask(ip6, mask6);
				if(masked != NULL)
				{
					push_list(ip6_prefixes, (void*)strdup(masked) );
				}
				free(masked);
			}
		}
	}
	//source assignments from dhcp config file
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *p = NULL;
	struct uci_element *e;
	if(uci_load(ctx, "dhcp", &p) == UCI_OK)
	{
		struct uci_ptr ptr;
		int idx = 0;
		char sIdx[4];
		sprintf(sIdx, "%d", idx);
		char* lookup_str = lookup_str = dynamic_strcat(5, "dhcp", ".", "@host[", sIdx, "]");
		int ret_value = uci_lookup_ptr(ctx, &ptr, lookup_str, 1);
		while(ret_value == UCI_OK)
		{
			char* ip4addr = NULL;
			char* ip6addr = NULL;
			char* hostname = NULL;

			struct uci_section *s = ptr.s;
			if(s != NULL)
			{
				uci_foreach_element(&s->options, e)
				{
					char* option_name = strdup(e->name);
					to_lowercase(option_name);
					if(safe_strcmp(option_name, "name") == 0)
					{
						hostname = get_option_value_string(uci_to_option(e));
					}
					else if(safe_strcmp(option_name, "ip") == 0)
					{
						ip4addr = get_option_value_string(uci_to_option(e));
					}
					else if(safe_strcmp(option_name, "hostid") == 0)
					{
						char* hostid = get_option_value_string(uci_to_option(e));
						char tmpstr[12];
						int len = strlen(hostid);
						if(len > 4)
						{
							strncpy(tmpstr+2, hostid, len-4);
							strncpy(tmpstr+len-2, ":", 1);
							strncpy(tmpstr+len-1, hostid+len-4, 4);
							tmpstr[len+3] = '\0';
						}
						else
						{
							strncpy(tmpstr+2, hostid, 4);
							tmpstr[len+2] = '\0';
						}
						strncpy(tmpstr, "::", 2);
						ip6addr = strdup(tmpstr);
						free(hostid);
					}
					free(option_name);
				}
			}
			else
			{
				break;
			}

			if(safe_strcmp(hostname, "") != 0 && hostname != NULL)
			{
				if(safe_strcmp(ip4addr, "") != 0 && ip4addr != NULL)
				{
					set_string_map_element(ip_to_hostname, ip4addr, strdup(hostname));
				}
				if(safe_strcmp(ip6addr, "") != 0 && ip6addr != NULL)
				{
					for(unsigned long idx = 0; idx < ip6_prefixes->length; idx++)
					{
						char* ip6 = (char*)list_element_at(ip6_prefixes, idx);
						char* tmpip6 = ip6_combine_prefix_hostid(ip6, ip6addr);
						if(tmpip6 != NULL)
						{
							set_string_map_element(ip_to_hostname, tmpip6, strdup(hostname));
						}
					}
				}
			}

			idx = idx + 1;
			sprintf(sIdx, "%d", idx);
			lookup_str = dynamic_strcat(5, "dhcp", ".", "@host[", sIdx, "]");
			ret_value = uci_lookup_ptr(ctx, &ptr, lookup_str, 1);
		}
	}
	uci_free_context(ctx);
	unsigned long num_destroyed;
	destroy_list(ip6_prefixes, DESTROY_MODE_FREE_VALUES, &num_destroyed);

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
			//Load LAN IPs
			if(ptr_lan_ip != NULL)
			{
				set_string_map_element(ip_to_hostname, ptr_lan_ip, strdup(line));
			}
			if(ptr_lan_ip6 != NULL)
			{
				char* local_ip6 = "fe80";
				for(unsigned long idx = 0; idx < ptr_lan_ip6->length; idx++)
				{
					char* ip6 = (char*)list_element_at(ptr_lan_ip6, idx);
					if(strncmp(local_ip6, ip6, 4) != 0)
					{
						set_string_map_element(ip_to_hostname, ip6, strdup(line));
					}
				}
			}
			//Load WAN IPs
			if(ptr_wan_ip != NULL)
			{
				set_string_map_element(ip_to_hostname, ptr_wan_ip, strdup(line));
			}
			if(ptr_wan_ip6 != NULL)
			{
				char* local_ip6 = "fe80";
				for(unsigned long idx = 0; idx < ptr_wan_ip6->length; idx++)
				{
					char* ip6 = (char*)list_element_at(ptr_wan_ip6, idx);
					if(strncmp(local_ip6, ip6, 4) != 0)
					{
						set_string_map_element(ip_to_hostname, ip6, strdup(line));
					}
				}
			}

			//127.0.0.1 and ::1 are always local, so add it unconditionally
			set_string_map_element(ip_to_hostname, "127.0.0.1", strdup(line));
			set_string_map_element(ip_to_hostname, "::1", strdup(line));

		}
		free(line);
		fclose(hostname_file);
	}


	return ip_to_hostname;



}

char* ip6_mask(char* addr, long mask)
{
	unsigned char address[sizeof(struct in6_addr)];
	char addrstr[INET6_ADDRSTRLEN];
	unsigned char maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

	int res = 0;
	res = inet_pton(AF_INET6, addr, address);
	if(res > 0)
	{
		int index = mask / 8;
		if(index < 16)
		{
			unsigned char* ptr = address;
			int offset = mask % 8;
			ptr[index] &= maskbit[offset];
			index++;

			while(index < 16)
			{
				ptr[index++] = 0;
			}
		}

		if(inet_ntop(AF_INET6, address, addrstr, INET6_ADDRSTRLEN) != NULL)
		{
			return strdup(addrstr);
		}
	}
	
	return NULL;
}


char* ip6_combine_prefix_hostid(char* ip6, char* hostid)
{
	unsigned char prefix[sizeof(struct in6_addr)];
	unsigned char host[sizeof(struct in6_addr)];
	char addrstr[INET6_ADDRSTRLEN];

	int res = 0;
	res = inet_pton(AF_INET6, ip6, prefix);
	if(res > 0)
	{
		res = inet_pton(AF_INET6, hostid, host);
		if(res > 0)
		{
			//This function assumes hostid is a maximum of 32bits (i.e. prefix is a maximum of 96)
			//so we can just do this straight assignment. If we ever want to get more tricky, revisit this
			prefix[12] = host[12];
			prefix[13] = host[13];
			prefix[14] = host[14];
			prefix[15] = host[15];

			if(inet_ntop(AF_INET6, prefix, addrstr, INET6_ADDRSTRLEN) != NULL)
			{
				return strdup(addrstr);
			}
		}
	}
	
	return NULL;
}

bool is_unusable_overlayfs(void)
{
	FILE* procmounts = setmntent("/proc/mounts", "r");
	if(!procmounts)
	{
		return false;
	}
	struct mntent* mnt_p;
	
	while (mnt_p = getmntent(procmounts))
	{
		if(strstr(mnt_p->mnt_fsname, "overlayfs") != NULL)
		{
			if(strstr(mnt_p->mnt_fsname, "/tmp/root") != NULL || strstr(mnt_p->mnt_opts, "ro,") != NULL)
			{
				endmntent(procmounts);
				return true;
			}
		}
	}
	endmntent(procmounts);
	return false;
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
	return uci_lookup(ctx, e, p, section_name, option_name);
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
	char** GHFstrings = (char**)calloc(6, sizeof(char*));	 //5 strings + emptyval
	//same order as in file:
	//index=0: ghf.title="Gargoyle Router Management Utility";
	//index=1: ghf.desc="Router<br/>Management<br/>Utility";
	//index=2: ghf.devn="Device Name";
	//index=3: ghf.waits="Please Wait While Settings Are Applied";
	//index=4: ghf.badoverlay="Router storage full (read only) or is mounted in volatile storage (RAM). It will not function correctly, or will revert to a previous state after rebooting. Restore default configuration or flash new firmware (without preserving settings).";
	char* ghfs_path = dynamic_strcat(4, web_root, "/i18n/", active_lang, "/ghf.js");

	if (path_exists(ghfs_path) == PATH_IS_REGULAR_FILE || path_exists(ghfs_path) == PATH_IS_SYMLINK )
	{
		unsigned long num_lines = 0;
		char** ghf_js_lines = get_file_lines(ghfs_path, &num_lines);

		if (memcmp(ghf_js_lines[0], "\xEF\xBB\xBF", 3) == 0)
		{		 //Found a UTF8 BOM
			unsigned long ghf_line;

			for (ghf_line=0; ghf_line < num_lines; ghf_line++)
			{
				char* this_line = ghf_js_lines[ghf_line];
				unsigned short idx, start_str, end_str = 0;

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
					char* val=(char*)calloc(end_str-start_str<8 ? 8 : (end_str-start_str)+1, sizeof(char));
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
