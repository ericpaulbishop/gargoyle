#!/bin/bash



user=$1
if [ -z "$user" ] ; then
	echo "Error: must specify user as first argument"
	exit
fi
version="1.7.0"
major_version="1.8"

if [ -n "$2" ] ; then
	version="$2"
fi
if [ -n "$3" ] ; then
	major_version="$3"
fi


scp_pub='scp -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes -r'
ssh_pub='ssh -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes'




#upload packages and images
cd built
package_dirs=$(ls)
for i in $package_dirs ; do
	if [ "$i" != "brcm-2.4" ] ; then
		echo "uploading packages for $i, version=$version, major_version=$major_version"
		$scp_pub $i/* $user@gargoyle-router.com:gargoyle_site/downloads/images/$i/
		$ssh_pub $user@gargoyle-router.com "rm -rf   gargoyle_site/packages/gargoyle-$version/$i"
		$ssh_pub $user@gargoyle-router.com "rm -rf   gargoyle_site/packages/gargoyle-$major_version/$i"
		$ssh_pub $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/gargoyle-$version/$i"
		$ssh_pub $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/gargoyle-$major_version"
		$ssh_pub $user@gargoyle-router.com "cd gargoyle_site/packages/gargoyle-$major_version/ ; ln -s ../gargoyle-$version/$i"

		$scp_pub ../built/$i/* $user@gargoyle-router.com:gargoyle_site/packages/gargoyle-$version/$i/

	fi
done
