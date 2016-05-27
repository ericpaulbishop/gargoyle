#include "gpkg.h"

string_map* parse_parameters(int argc, char** argv);

void print_usage(void);



int main(int argc, char** argv)
{
	
	string_map* parameters = parse_parameters(argc, argv);

	opkg_conf *conf = load_conf((char*)get_string_map_element(parameters, "config"));

	char* run_type                   = get_string_map_element(parameters, "run-type");
	int force_overwrite_other_files  = get_string_map_element(parameters, "force-overwrite")         != NULL ? 1 : 0;
	int force_overwrite_configs      = get_string_map_element(parameters, "force-overwrite-configs") != NULL ? 1 : 0;
	int force_depends                = get_string_map_element(parameters, "force-depends")           != NULL ? 1 : 0;
	int force_reinstall              = get_string_map_element(parameters, "force-reinstall")         != NULL ? 1 : 0;

	int remove_orphaned_depends      = get_string_map_element(parameters, "autoremove")                    != NULL ? REMOVE_ALL_ORPHANED_DEPENDENCIES : REMOVE_NO_ORPHANED_DEPENDENCIES;
	remove_orphaned_depends          = get_string_map_element(parameters, "autoremove-same-destination")   != NULL ? REMOVE_ORPHANED_DEPENDENCIES_IN_SAME_DEST : remove_orphaned_depends;

	char* install_root               = get_string_map_element(parameters, "install-destination");
	install_root                     = install_root == NULL ? strdup("root") : install_root;

	char* link_root                  = get_string_map_element(parameters, "link-destination");
	char* tmp_root                   = get_string_map_element(parameters, "tmp_dir");
	tmp_root                         = tmp_root == NULL ? strdup("/tmp") : tmp_root;
	string_map* pkgs                 = get_string_map_element(parameters, "package-list");
	
	char* format_str                 = get_string_map_element(parameters, "output-format");
	int format                       = OUTPUT_HUMAN_READABLE;
	if(format_str != NULL)
	{
		format = strcmp(format_str, "json") == 0 ? OUTPUT_JSON : format;
		format = strcmp(format_str, "js") == 0 || strcmp(format_str, "javascript") == 0 ? OUTPUT_JAVASCRIPT : format;
	}


	


	if(strcmp(run_type, "install") == 0)
	{
		do_install(conf, pkgs, install_root, link_root, 0, force_overwrite_configs, force_overwrite_other_files, force_reinstall, tmp_root);
	}
	else if(strcmp(run_type, "remove") == 0)
	{
		do_remove(conf, pkgs, !force_overwrite_configs, remove_orphaned_depends, force_depends, 1, tmp_root);
	}
	else if(strcmp(run_type, "upgrade") == 0)
	{
		do_upgrade(conf, pkgs, !force_overwrite_configs, install_root, link_root, tmp_root);
	}
	else if(strcmp(run_type, "update") == 0)
	{
		update(conf);
	}
	else if((strcmp(run_type, "list") == 0) || strcmp(run_type, "list-installed") == 0 || strcmp(run_type, "list_installed") == 0)
	{
		do_list(conf, parameters, format);
	}
	else if(strcmp(run_type, "dest-info") == 0 || strcmp(run_type, "dest_info") == 0)
	{
		do_print_dest_info(conf, format);
	}
	else if(strcmp(run_type, "info") == 0)
	{
		do_print_info(conf, parameters, install_root, format);
	}

	return(0);

}



string_map* parse_parameters(int argc, char** argv)
{
	string_map* parameters = initialize_string_map(1);
	if(argc == 1)
	{
		print_usage();  //exits
	}
	char* run_type = argv[1];
	if(	strcmp(run_type, "list") != 0 && 
		strcmp(run_type, "list-installed") != 0 && 
		strcmp(run_type, "list_installed") != 0 && 
		strcmp(run_type, "dest-info") != 0 && 
		strcmp(run_type, "dest_info") != 0 && 
		strcmp(run_type, "info") != 0 && 
		strcmp(run_type, "update") != 0 &&
		strcmp(run_type, "install") != 0 &&
	       	strcmp(run_type, "remove") != 0 &&
	       	strcmp(run_type, "upgrade") != 0
	  )
	{
		print_usage();  //exits
	}
	set_string_map_element(parameters, "run-type", run_type);


	static struct option long_options[] = {
		{"force-depends",           0, 0, 'p'},
		{"force_depends",           0, 0, 'p'},
		{"force-overwrite",         0, 0, 'w'},
		{"force_overwrite",         0, 0, 'w'},
		{"force-maintainer",        0, 0, 'm'},
		{"force_maintainer",        0, 0, 'm'},
		{"force-overwrite-configs", 0, 0, 'm'},
		{"force_overwrite-configs", 0, 0, 'm'},
		{"force-reinstall",         0, 0, 'e'},
		{"force_reinstall",         0, 0, 'e'},
		{"autoremove",              0, 0, 'a'},
		{"autoremove-same-dest",    0, 0, 's'},
		{"autoremove_same_dest",    0, 0, 's'},
		{"conf",                    1, 0, 'c'},
		{"dest",                    1, 0, 'd'},
		{"link-dest",               1, 0, 'l'},
		{"link_dest",               1, 0, 'l'},
		{"only-dest",               1, 0, 'n'},
		{"only_dest",               1, 0, 'n'},
		{"tmp-dir",                 1, 0, 't'},
		{"tmp_dir",                 1, 0, 't'},
		{"output-format",           1, 0, 'o'},
		{"output_format",           1, 0, 'o'},
		{"matching-regex",          0, 0, 'r'},
		{"matching_regex",          0, 0, 'r'},
		{"regex",                   0, 0, 'r'},
		{"package-variables",       1, 0, 'v'},
		{"package_variables",       1, 0, 'v'},
		{"help",                    0, 0, 'h'},
		{NULL, 0, NULL, 0}
	};

	int expect_regex =0;

	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "pwmeasc:d:l:n:t:o:rv:h", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'p':
				set_string_map_element(parameters, "force-depends", strdup("D"));
				break;
			case 'w':
				set_string_map_element(parameters, "force-overwrite", strdup("D"));
				break;
			case 'm':
				set_string_map_element(parameters, "force-overwrite-configs", strdup("D"));
				break;	
			case 'e':
				set_string_map_element(parameters, "force-reinstall", strdup("D"));
				break;
			case 'a':
				set_string_map_element(parameters, "autoremove", strdup("D"));
				break;
			case 's':
				set_string_map_element(parameters, "autoremove-same-destination", strdup("D"));
				break;
			case 'c':
				set_string_map_element(parameters, "conf", strdup(optarg));
				break;
			case 'd':
				set_string_map_element(parameters, "install-destination", strdup(optarg));
				break;
			case 'l':
				set_string_map_element(parameters, "link-destination", strdup(optarg));
				break;
			case 'n':
				set_string_map_element(parameters, "only-destination", strdup(optarg));
				break;
			case 't':
				set_string_map_element(parameters, "tmp-dir", strdup(optarg));
				break;
			case 'o':
				set_string_map_element(parameters, "output-format", strdup(optarg));
				break;
			case 'v':
				if(strlen(optarg) >0)
				{

					unsigned long num_pieces=0;
					char seps[] = {',', ';', ':', ' ', '\r', '\n', '\t' }; //include newlines to strip them off end of line
					char** split_line = split_on_separators(optarg, seps, 7, -1, 0, &num_pieces);
					if(num_pieces > 0)
					{
						string_map* package_variables = initialize_string_map(1); 
						int piece_index;
						for(piece_index=0;piece_index < num_pieces; piece_index++)
						{
							if(strlen(split_line[piece_index]) > 0)
							{
								set_string_map_element(package_variables, split_line[piece_index], strdup("D"));
							}
						}
						if(package_variables->num_elements > 0)
						{
							set_string_map_element(parameters, "package-variables", package_variables);
						}
					}
					free_null_terminated_string_array(split_line);
				}
				break;

			case 'r': 
				//--matching-regex, or just --regex
				expect_regex = 1;
				break;
			case 'h':
			default:
				print_usage(); //exits
				break;
		}
	}

	if( expect_regex == 1 && strcmp(run_type, "info") != 0 && strcmp(run_type, "list") != 0 && strcmp(run_type, "list-installed") != 0 && strcmp(run_type, "list_installed") )
	{
		fprintf(stderr, "ERROR: package list can only specified with regular expression when using 'list', 'list-installed' or 'info' commands\n\n");
		exit(1);
	}


	
	string_map* pkg_list = initialize_string_map(1);
	set_string_map_element(parameters, "package-list", pkg_list);
	for(option_index=1; option_index < argc; option_index++)
	{
		int option_str_len = strlen(argv[option_index]);
		if(argv[option_index][0] == '-' && option_str_len > 1)
		{
			int has_arg=0;
			int test_index;
			const char* name;
			const char* test_name = argv[option_index] + 1;
			int is_long_opt=0;
			if(argv[option_index][1] == '-')
			{
				test_name = argv[option_index] + 2;
				is_long_opt = 1;
			}
			for(test_index=0; (name = (long_options[test_index]).name) != NULL ; test_index++)
			{
				if( (is_long_opt == 1 && strcmp(test_name, name) == 0) || (is_long_opt == 0 && test_name[0] == (long_options[test_index]).val) )
				{
					has_arg = (long_options[test_index]).has_arg;
				}
			}
			if(has_arg)
			{
				option_index++;
			}
		}
		else if(strcmp(argv[option_index], run_type) != 0)
		{
			if(expect_regex)
			{
				regex_t* packages_matching_regex = (regex_t*)malloc(sizeof(regex_t));
				char* regex_str = strdup(argv[option_index]);
				int regex_len = strlen(regex_str);
				if(get_string_map_element(parameters, "package-regex") != NULL)
				{
					fprintf(stderr, "ERROR: Only one regular expression can be specified at a time.\n\n");
					exit(1); 

				}
				if(regex_str[0] != '/' || regex_str[regex_len-1] != '/')
				{
					free(regex_str);
					regex_str=dynamic_strcat(3, "/", argv[option_index], "/");
				}
				if( convert_to_regex(regex_str, packages_matching_regex) )
				{
					set_string_map_element(parameters, "package-regex", packages_matching_regex);
				}
				else
				{
					fprintf(stderr, "ERROR: Invalid regular expression \"%s\"\n\n", regex_str);
					exit(1); 
				}
				free(regex_str);
			}
			else
			{
				set_string_map_element(pkg_list, argv[option_index], alloc_depend_def(NULL));
			}
		}
	}
	if(	strcmp(run_type, "install") == 0 ||
	       	strcmp(run_type, "remove")  == 0 ||
	       	strcmp(run_type, "upgrade") == 0
		)
	{
		if(pkg_list->num_elements == 0)
		{
			printf("ERROR: No packages specified to %s.\n\n", run_type);
			exit(1);
		}
	}


	return parameters;
}

void print_usage(void)
{
	printf("gpkg: opkg/ipkg compatible package manager\n");
	printf("      written by Eric Bishop\n\n");

	printf("usage: gpkg sub-command [options] [package argument(s)]\n");
	printf("where sub-command is one of:\n");
	printf("\n");
	
	printf("Package Manipulation:\n");
	printf("  update                        Update list of available packages\n");
	printf("  upgrade [pkgs]                Upgrade packages\n");
	printf("  install [pkgs]                Install package(s)\n");
	printf("  remove  [pkgs]                Remove package(s)\n");
	printf("\n");
	
	
	printf("Informational Commands\n");
	printf("  list [pkgs|regexp]            List available packages\n");
	printf("  list-installed [pkgs|regexp]  List installed packages\n");
	printf("  info [pkgs|regexp]            Display info for packages\n");
	printf("  dest-info                     Display info about package destinations\n");
	printf("\n");
	
	printf("Options:\n");
	printf("  --regex,r                     Package argument is a regular expression\n");
	printf("                                Only accepted for list/info commands\n");
	printf("  --conf,-f [conf_file]         Use <conf_file> as opkg/gpkg configuration file\n");
	printf("  --dest,-d [dest_name]         Use <dest_name> as the the root directory for\n");
	printf("                                package installation, removal, upgrading\n");
	printf("                                <dest_name> should be a defined dest name from\n");
	printf("                                the configuration file,\n");
	printf("  --link-dest,-l [dest_name]    After installation, symlink files to a\n");
	printf("                                differentdestination specified by <dest_name>\n");
	printf("  --only-dest,-n [dest_name]    Only display packages installed in <dest_name>\n");
	printf("  --tmp-dir,-t   [dir_path]     Specify path of tmp-dir\n");
	printf("  --output-format,-o [format]   Specify output format of list/info commands\n");
	printf("                                can be 'human-readable', 'json' or 'js'\n");
	printf("                                Default is 'human-readable'\n");
	printf("  --package-variables,-v [vars] Comma seperated list of package variables\n");
	printf("                                to print when info command is called\n");
	printf("\n");


	printf("Force Options:\n");
	printf("  --force-depends,-p            Install/remove despite failed dependencies\n");
	printf("  --force-maintainer,-m         Overwrite preexisting config files\n");
	printf("  --force-reinstall,-e          Reinstall package(s)\n");
	printf("  --force-overwrite,-w          Overwrite files from other package(s)\n");
	printf("  --autoremove,-a               Remove packages that were installed\n");
	printf("                                automatically to satisfy dependencies\n");
	printf("  --autoremove-same-dest,-s     Remove packages that were installed\n");
	printf("                                automatically to satisfy dependencies\n");
	printf("                                only if they were installed to same\n");
	printf("                                destination as package being removed\n");


	
	
	exit(0);
}
