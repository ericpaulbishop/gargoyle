#!/bin/bash

git_branch()
{
	local g="$(git rev-parse --git-dir 2>/dev/null)"
	if [ -n "$g" ]; then
		local r
		local b
		if [ -d "$g/../.dotest" ] ; then
			r="|AM/REBASE"
			b="$(git symbolic-ref HEAD 2>/dev/null)"
		elif [ -f "$g/.dotest-merge/interactive" ] ; then
			r="|REBASE-i"
			b="$(cat $g/.dotest-merge/head-name)"
		elif [ -d "$g/.dotest-merge" ] ; then
			r="|REBASE-m"
			b="$(cat $g/.dotest-merge/head-name)"
		elif [ -f "$g/MERGE_HEAD" ] ; then
			r="|MERGING"
			b="$(git symbolic-ref HEAD 2>/dev/null)"
		else
			if [ -f $g/BISECT_LOG ] ; then
				r="|BISECTING"
			fi
			if  ! b="$(git symbolic-ref HEAD 2>/dev/null)"  ; then
				b="$(cut -c1-7 $g/HEAD)..."
			fi
		fi
		if [ -n "$1" ] ; then
			printf "$1" "${b##refs/heads/}$r"
		else
			printf "%s" "${b##refs/heads/}$r"
		fi
	fi
}


user=$1
if [ -z "$user" ] ; then
	echo "Error: must specify user as first argument"
	exit
fi

scp_pub='scp -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes -r'
ssh_pub='ssh -o StrictHostKeyChecking=no -o PubkeyAuthentication=yes -o BatchMode=yes'

if [ ! -d "images" ] ; then
	echo "ERROR: images directory does not exist"
	echo "Aborting Update"
	exit;
fi

cd images
rm -rf src custom

#make sure we can clone latest source to upload
gargoyle_checkout_branchname=$( git_branch )
if [ -z  "$gargoyle_checkout_branchname" ] ; then
	gargoyle_checkout_branchname="master"
fi
mkdir src
cd src
git clone git://github.com/ericpaulbishop/gargoyle.git
cd gargoyle
git checkout "$gargoyle_checkout_branchname"
cd ..
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
echo "Updating for gargoyle branch = $gargoyle_checkout_branchname"
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
		$ssh_pub $user@gargoyle-router.com "rm -rf   gargoyle_site/packages/gargoyle-$version/$i"
		$ssh_pub $user@gargoyle-router.com "rm -rf   gargoyle_site/packages/gargoyle-$major_version/$i"
		$ssh_pub $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/gargoyle-$version/$i"
		$ssh_pub $user@gargoyle-router.com "mkdir -p gargoyle_site/packages/gargoyle-$major_version"
		$ssh_pub $user@gargoyle-router.com "cd gargoyle_site/packages/gargoyle-$major_version/ ; ln -s ../gargoyle-$version/$i"

		$scp_pub ../built/$i/* $user@gargoyle-router.com:gargoyle_site/packages/gargoyle-$version/$i/

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

