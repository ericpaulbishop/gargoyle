#!/bin/sh

local pkg="$1"
local plugin_root="/plugin_root" #doesn't change, we switch this to a symlink if we want to put files on USB drive

IFS_ORIG="$IFS"
IFS_LINEBREAK="$(printf '\n\r')"

if [ -z "$pkg" ] ; then
	echo "ERROR: you must specify package to install"
	echo "Usage: $0 [PACKAGE_NAME]"
	echo ""
	exit
fi

##############################################################
# Prepare to install, create /plugin_root if it doesn't exist
##############################################################
local have_plugin_root=$(cat /etc/opkg.conf | grep "dest.*plugin_root" )
if [ -z "$have_plugin_root" ] ; then
	echo 'dest plugin_root /plugin_root' >>/etc/opkg.conf
fi
if [ ! -e /plugin_root ] ; then  #tests if plugin_root exists, use -e since this may be directory or symlink
	mkdir -p /plugin_root
fi
local link_dirs="etc tmp var"
for link_dir in $link_dirs ; do
	if [ ! -h "$plugin_root/$link_dir" ] ; then
		rm -rf "$plugin_root/$link_dir" 
		ln -s "/$link_dir" "$plugin_root/$link_dir" 
	fi
done



###############################################
# Call opkg to install package to plugin_root
###############################################
opkg update
opkg install -d plugin_root "$pkg"



###########################################################
# Symlink package files from plugin root to real root
###########################################################
IFS="$IFS_LINEBREAK"
local pkg_files=$(cat "/plugin_root/usr/lib/opkg/info/$pkg.list")
local pf=""
for pf in $pkg_files ; do
	local pf_link=$(echo "$pf" | sed 's/\/plugin_root//g')
	local in_tmp=$(echo "$pf_link" | grep "^\/tmp\/")
	local in_etc=$(echo "$pf_link" | grep "^\/etc\/")
	local in_var=$(echo "$pf_link" | grep "^\/var\/")
	if [ -e "$pf" ] && [ -z "$in_etc" ] && [ -z "$in_tmp" ] && [ -z "$in_var" ] ; then 
		local pf_dir=$( echo "$pf_link" | sed 's/\/[^\/]*$/\//g')
		if [ ! -d "$pf_dir" ] ; then
			mkdir -p "$pf_dir"
		fi
		rm -rf "$pf_link"
		ln -s "$pf" "$pf_link"
	fi
done
IFS="$IFS_ORIG"

exit 0
