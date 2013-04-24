#!/bin/sh

local plugin_root="/plugin_root" #doesn't change, we switch this to a symlink if we want to put files on USB drive
local escaped_plugin_root=$(echo "$plugin_root" | sed 's/\//\\\//g' )


local pkg="$1"
if [ -z "$pkg" ] ; then
	echo "ERROR: you must specify package to install"
	echo "Usage: $0 [PACKAGE_NAME]"
	echo ""
	exit
fi

##############################################################
# Prepare to install, create /plugin_root if it doesn't exist
##############################################################
local have_plugin_root=$(cat /etc/opkg.conf | grep "dest.*$escaped_plugin_root" )
if [ -z "$have_plugin_root" ] ; then
	echo "dest plugin_root $plugin_root" >>/etc/opkg.conf
fi
if [ ! -e "$plugin_root" ] ; then  #tests if plugin_root exists, use -e since this may be directory or symlink
	mkdir -p "$plugin_root"
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
gpkg update
gpkg install --dest plugin_root --link-dest root "$pkg"



exit 0
