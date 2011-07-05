#!/bin/bash


user=$1
if [ -z "$user" ] ; then
	echo "Error: must specify user as first argument"
	exit
fi

scp_pub='scp -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes'
ssh_pub='ssh -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes'

cd images
rm -rf src custom

#make sure we can clone latest source to upload
mkdir src
cd src
git clone git://gargoyle-router.com/gargoyle.git
if [ ! -d "gargoyle" ] ; then
	echo "ERROR: Cannot clone source tree from: git://gargoyle-router.com/gargoyle.git"
	echo "Aborting Update"
	echo ""
	exit;
fi
cd ..


#prepare for upload by determining current version and stable version branch
#package dir will be current or next stable version branch
version=$(find . -name "gargoyle_*"  | egrep -o "[0-9]+\.[0-9]+\.[0-9]+" | head -n 1)
major_version="1.0"
if [ -n "$version" ] ; then
	major_version=$(echo "$version" | egrep -o "^[0-9]+\.[0-9]+")
	
	major_full_version=$(echo $major_version | sed -e 's/\..*$//g')
	major_point_version=$(echo $major_version | sed -e 's/^.*\.//g')
	is_odd=$(( $major_point_version % 2 ))
	if [ "$is_odd" = "1" ] ; then
		major_point_version=$(( $major_point_version + 1 ))
		major_version="$major_full_version.$major_point_version"
	fi
fi


#give user a chance to cancel
echo "Updating for version = $version"
echo "Upcoming major version (for package naming) = $major_version"
echo ""
echo "Beginning update in 10 seconds (kill the job if version info above is wrong!)"
echo ""
countdown=10
while [ $countdown -gt 0 ] ; do
	echo "$countdown"
	sleep 1
	countdown=$(( $countdown - 1 ))
done

echo "Now Doing Update..."


#upload packages and images
image_dirs=$(ls)
for i in $image_dirs ; do
	if [ "$i" != "brcm-2.4" ] ; then
		echo $i
	
		$scp_pub $i/* $user@gargoyle-router.com:gargoyle_site/downloads/images/$i/
		$ssh_pub $user@gargoyle-router.com "rm -rf   gargoyle_site/packages/gargoyle-$major_version/$i"
		$ssh_pub $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/gargoyle-$major_version/$i"
		$scp_pub ../built/$i/* $user@gargoyle-router.com:gargoyle_site/packages/gargoyle-$major_version/$i/
	fi
done

#upload tarball of latest code
mkdir src
cd src
tar cvzf gargoyle_$version-src.tar.gz gargoyle
rm -rf gargoyle
$scp_pub gargoyle_$version-src.tar.gz $user@gargoyle-router.com:gargoyle_site/downloads/src/

#update download list
$ssh_pub $user@gargoyle-router.com  "./update.sh"


#tag release
git tag "$version" -m "Tag $version"
git push --tags

