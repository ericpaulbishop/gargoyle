#!/bin/sh

local plugin_root="/plugin_root" #doesn't change, we switch this to a symlink if we want to put files on USB drive
local escaped_plugin_root=$(echo "$plugin_root" | sed 's/\//\\\//g' )
IFS_ORIG="$IFS"
IFS_LINEBREAK="$(printf '\n\r')"


remove_single_package()
{
	local remove_pkg="$1"
	

	##################################################################
	# Remove symlinks from package files in plugin root to real root
	##################################################################
	IFS="$IFS_LINEBREAK"
	local remove_pkg_files=$(cat "$plugin_root/usr/lib/opkg/info/${remove_pkg}.list" 2>/dev/null)
	local rpf=""
	for rpf in $remove_pkg_files ; do
		local rpf_link=$(echo "$rpf" | sed 's/\/plugin_root//g')
		local in_tmp=$(echo "$rpf_link" | grep "^\/tmp\/")
		local in_etc=$(echo "$rpf_link" | grep "^\/etc\/")
		local in_var=$(echo "$rpf_link" | grep "^\/var\/")
		if [ -e "$rpf" ] && [ -z "$in_etc" ] && [ -z "$in_tmp" ] && [ -z "$in_var" ] ; then 
			if [ -h "$rpf_link" ] ; then
				rm  -rf "$rpf_link"
			fi
		fi
	done
	rm -rf "$plugin_root/usr/lib/opkg/info/${remove_pkg}.linked"
	IFS="$IFS_ORIG"

	###############################################
	# Call opkg to remove package 
	###############################################
	echo "removing $remove_pkg"
	opkg remove -d plugin_root "$remove_pkg" >/dev/null 2>&1
}



local pkg="$1"
if [ -z "$pkg" ] ; then
	echo "ERROR: you must specify package to remove"
	echo "Usage: $0 [PACKAGE_NAME]"
	echo ""
	exit
fi


############################################################################
# Remove main package we were called with
############################################################################
remove_single_package "$pkg"



##################################################################################################
# Cleanup installed dependencies by removing all packages installed in plugin_root that:
#
# (1) Were not explicitly installed by user (they were installed as dependencies) and
# (2) Have no other installed packages depending on them
#
# If a package is removed, perform this check again, in case we missed a package that had 
# the package we just removed as a dependency, and nothing else.  Iterate until no packages
# are removed.
##################################################################################################
local remove_found="1"
while [ "$remove_found" = "1" ] ; do
	remove_found="0"
	local installed_plugin_packages=$(ls "$plugin_root/usr/lib/opkg/info/"*.control | sed 's/\.control.*$//g' | sed 's/^.*\///g')
	for ipp in $installed_plugin_packages ; do
		local user_installed=$(opkg info "$ipp" | grep "^Status:.* user *" )
		if [ -z "$user_installed" ] ; then
			local whatdepends_lines=$(opkg whatdepends "$ipp" | wc -l) #only 3 lines of output means nothing depends on this package
			if [ "$whatdepends_lines" = "3" ] ; then
				remove_found="1"				
				remove_single_package "$ipp"
			fi
		fi
	done
done



exit 0

