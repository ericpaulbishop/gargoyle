#!/bin/sh

local pkg="$1"
local plugin_root="/plugin_root" #doesn't change, we switch this to a symlink if we want to put files on USB drive

IFS_ORIG="$IFS"
IFS_LINEBREAK="$(printf '\n\r')"

if [ -z "$pkg" ] ; then
	echo "ERROR: you must specify package to remove"
	echo "Usage: $0 [PACKAGE_NAME]"
	echo ""
	exit
fi




##################################################################
# Remove symlinks from package files in plugin root to real root
##################################################################
IFS="$IFS_LINEBREAK"
local pkg_files=$(cat "/plugin_root/usr/lib/opkg/info/$pkg.list")
local pf=""
for pf in $pkg_files ; do
	local pf_link=$(echo "$pf" | sed 's/\/plugin_root//g')
	local in_tmp=$(echo "$pf_link" | grep "^\/tmp\/")
	local in_etc=$(echo "$pf_link" | grep "^\/etc\/")
	local in_var=$(echo "$pf_link" | grep "^\/var\/")
	if [ -e "$pf" ] && [ -z "$in_etc" ] && [ -z "$in_tmp" ] && [ -z "$in_var" ] ; then 
		if [ -h "$pf_link" ] ; then
			rm  "$pf_link" >/dev/null 2>&1
		fi
	fi
done
IFS="$IFS_ORIG"


###############################################
# Call opkg to remove package 
###############################################
opkg remove -d plugin_root "$pkg" >/dev/null 2>&1

exit 0

