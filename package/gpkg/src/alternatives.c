
#include "gpkg.h"


int update_alternatives(list* pkg_alternatives, string_map* package_data, char* fs_terminated_link_root, char* fs_terminated_overlay_root, char* consider_uninstalled)
{
	int warn = 0;
	if(pkg_alternatives != NULL)
	{
		unsigned long num_pkg_alts = 0;
		unsigned long pkg_alts_index = 0;
		char* pkg_alt = list_element_at(pkg_alternatives, pkg_alts_index);
		while(pkg_alt != NULL)
		{
			char* best_altpath = NULL;
			char* best_altaltpath = NULL;
			int best_altprio = 0;
			char* pkg_alt_prio = NULL;
			char* pkg_alt_path = NULL;
			char* pkg_alt_altpath = NULL;
			num_pkg_alts = pkg_alts_index;
			// Process the alt - priority:path:altpath
			unsigned long num_alt_parts;
			char alt_separators[] = {':'};
			char** alt_parts = split_on_separators(pkg_alt, alt_separators, 1, 3, 0, &num_alt_parts);
			if(num_alt_parts == 3)
			{
				pkg_alt_prio = alt_parts[0];
				pkg_alt_path = alt_parts[1];
				pkg_alt_altpath = alt_parts[2];
				
				unsigned long num_pkg;
				char** pkg_names_list = get_string_map_keys(package_data, &num_pkg);
				int pkg_index = 0;
				for(pkg_index = 0; pkg_index < num_pkg; pkg_index++)
				{
					int have_current;
					char* current_version = NULL;
					char* installed_pkg_status = NULL;
					string_map* installed_pkg = get_package_current_or_latest(package_data, pkg_names_list[pkg_index], &have_current, &current_version);
					if(installed_pkg != NULL)
					{
						installed_pkg_status = get_string_map_element(installed_pkg, "Status");
					}
					// We want installed, half-installed, but not packages that are currently being uninstalled
					// The status in package_data isn't updated before this runs so we also check the package name
					if(
						installed_pkg_status != NULL &&
						strstr(installed_pkg_status, "installed") != NULL &&
						strstr(installed_pkg_status, "deinstall") == NULL &&
						strcmp(pkg_names_list[pkg_index], consider_uninstalled) != 0
					)
					{
						// pkg installed, check
						list* installed_pkg_alts = get_string_map_element(installed_pkg, "Alternatives");
						if(installed_pkg_alts != NULL)
						{
							//printf("Package: %s, status: %s\n", pkg_names_list[pkg_index], installed_pkg_status);
							unsigned long num_installed_pkg_alts = 0;
							unsigned long installed_pkg_alts_index = 0;
							char* installed_pkg_alt = list_element_at(installed_pkg_alts, installed_pkg_alts_index);
							while(installed_pkg_alt != NULL)
							{
								char* installed_pkg_alt_prio = NULL;
								char* installed_pkg_alt_path = NULL;
								char* installed_pkg_alt_altpath = NULL;

								// Process the alt - priority:path:altpath
								char** installed_alt_parts = split_on_separators(installed_pkg_alt, alt_separators, 1, 3, 0, &num_alt_parts);
								if(num_alt_parts == 3)
								{
									installed_pkg_alt_prio = installed_alt_parts[0];
									installed_pkg_alt_path = installed_alt_parts[1];
									installed_pkg_alt_altpath = installed_alt_parts[2];
									if(strcmp(pkg_alt_path,installed_pkg_alt_path) == 0)
									{
										if(best_altpath == NULL || atoi(pkg_alt_prio) < atoi(installed_pkg_alt_prio))
										{
											best_altpath = strdup(installed_pkg_alt_path);
											best_altaltpath = strdup(installed_pkg_alt_altpath);
											best_altprio = atoi(installed_pkg_alt_prio);
										}
									}
								}
								free_null_terminated_string_array(installed_alt_parts);
								
								installed_pkg_alts_index += 1;
								installed_pkg_alt = list_element_at(installed_pkg_alts, installed_pkg_alts_index);
							}
						}
					}
				}
			}
			free_null_terminated_string_array(alt_parts);
			
			char* symlinkpath = NULL;
			if(best_altpath != NULL)
			{
				// Put together real file location and real symlink location
				// Alternatives should always be absolute, but lets be careful
				// We already took care of fs_terminated_link_root and fs_terminated_overlay_root earlier
				char* linktargetpath = NULL;
				symlinkpath = dynamic_strcat(2, (fs_terminated_link_root != NULL ? fs_terminated_link_root : fs_terminated_overlay_root), (best_altpath[0] == '/' ? (best_altpath+1) : best_altpath));
				linktargetpath = dynamic_strcat(2, (fs_terminated_link_root != NULL ? fs_terminated_link_root : fs_terminated_overlay_root), (best_altaltpath[0] == '/' ? (best_altaltpath+1) : best_altaltpath));
				
				int bail = 0;
				int r = path_exists(symlinkpath);
				if(r > 0)
				{
					char* realpath;
					if(r != 3) // PATH_IS_SYMLINK
					{
						fprintf(stderr, "%s exists but is not a symlink\n", symlinkpath);
						bail = 1;
						warn = 1;
					}
					else
					{
						realpath = xreadlink(symlinkpath);
						if(realpath && strcmp(realpath, linktargetpath))
						{
							unlink(symlinkpath);
						}
					}
				}
				if(!bail)
				{
					char* tmp = strdup(symlinkpath);
					char* dir = dirname(tmp);
					r = mkdir_p(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
					free(tmp);
					r = symlink(linktargetpath, symlinkpath);
					if(r && path_exists(symlinkpath) != 3)
					{
						fprintf(stderr, "Failed simlinking %s -> %s\n", linktargetpath, symlinkpath);
						warn = 1;
					}
				}
				
				free(symlinkpath);
				free(linktargetpath);
			}
			else
			{
				// Put together real symlink location
				// Alternatives should always be absolute, but lets be careful
				// We already took care of fs_terminated_link_root and fs_terminated_overlay_root earlier
				symlinkpath = dynamic_strcat(2, (fs_terminated_link_root != NULL ? fs_terminated_link_root : fs_terminated_overlay_root), (pkg_alt_path[0] == '/' ? (pkg_alt_path+1) : pkg_alt_path));
				int r = path_exists(symlinkpath);
				if(r == 3)
				{
					unlink(symlinkpath);
				}
				else if(r == 0)
				{
					// Swing and a miss. This is a massive crapshoot on the uninstall path. Let's try the regular root location and hope for the best
					r = path_exists(pkg_alt_path);
					if(r == 3)
					{
						unlink(pkg_alt_path);
					}
				}
				free(symlinkpath);
			}
			
			pkg_alts_index += 1;
			pkg_alt = list_element_at(pkg_alternatives, pkg_alts_index);
		}
	}
	
	return warn;
}



