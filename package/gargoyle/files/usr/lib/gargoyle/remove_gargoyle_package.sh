#!/bin/sh


local pkg="$1"
if [ -z "$pkg" ] ; then
	echo "ERROR: you must specify package to remove"
	echo "Usage: $0 [PACKAGE_NAME]"
	echo ""
	exit
fi


gpkg remove --autoremove-same-dest "$pkg"


exit 0

