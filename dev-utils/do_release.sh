#!/bin/bash

user=$1
if [ -z "$user" ] ; then
	echo "Error: must specify user as first argument"
fi


SCP=scp -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes
SSH=ssh -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes


#upload images and packages
cd images
image_dirs=$(ls)
for i in $image_dirs ; do
	$SCP $i/* $user@gargoyle-router.com:gargoyle_site/downloads/images/$i/
	$SSH $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/backfire/$i"
	$SCP ../built/$i/* $user@gargoyle-router.com:gargoyle_site/packages/backfire/$i/
done

#upload latest code
version=$(find . -name "gargoyle_*" | head -n 1 | egrep -o "[0-9]+\.[0-9]+\.[0-9]+")
mkdir src
cd src
git clone git://gargoyle-router.com/gargoyle.git
tar cvzf gargoyle_$version-src.tar.gz gargoyle
rm -rf gargoyle
$SCP gargoyle_$version-src.tar.gz $user@gargoyle-router.com:gargoyle_site/downloads/src/

#update download list
$SSH $user@gargoyle-router.com  "./update.sh"


#tag release
git tag "$version" -m "Tag $version"
git push --tags

