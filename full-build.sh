#!/bin/bash

set_constant_variables()
{
	#working directories
	top_dir=$(pwd)
	targets_dir="$top_dir/targets"
	patches_dir="$top_dir/patches-generic"
	compress_js_dir="$top_dir/compressed_javascript"

	#script for building netfilter patches
	netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules_backfire.sh"

	#openwrt branch
	branch_name="backfire"

	# set svn revision number to use 
	# you can set this to an alternate revision 
	# or empty to checkout latest 
	rnum=29323

	#set date here, so it's guaranteed the same for all images
	#even though build can take several hours
	build_date=$(date +"%B %d, %Y")

	gargoyle_git_revision=$(git log -1 --pretty=format:%h)

}
set_version_variables()
{
	# Full display version in gargoyle web interface
	full_gargoyle_version="$1"
	if [ -z "$full_gargoyle_version" ] ; then
		full_gargoyle_version="Unknown"
	fi
	
	# Used in gargoyle banner
	short_gargoyle_version=$(echo "$full_gargoyle_version" | awk '{ print $1 ; }' | sed 's/[^0-9^A-Z^a-z^\.^\-^_].*$//g' )
	
	# Used for file naming
	lower_short_gargoyle_version=$(echo "$short_gargoyle_version" | tr 'A-Z' 'a-z' )

	# Used for package versioning/numbering, needs to be a numeric, eg 1.2.3
	numeric_gargoyle_version=$(echo "$short_gargoyle_version" | sed 's/[Xx]/0/' )
	not_numeric=$(echo "$numeric_gargoyle_version" | sed 's/[\.0123456789]//g')
	if [ -n "$not_numeric" ] ; then
		numeric_gargoyle_version="0.9.9"
	fi

	#echo "full        = \"$full_gargoyle_version\""
	#echo "short       = \"$short_gargoyle_version\""
	#echo "short lower = \"$lower_short_gargoyle_version\""
	#echo "numeric     = \"$numeric_gargoyle_version\""

}

get_target_from_config()
{
	config_file_path="$1"
	cat .config | grep "^CONFIG_TARGET_BOARD=" | sed 's/\"$//g' | sed 's/^.*\"//g'
}


create_gargoyle_banner()
{
	echo "BUILDING BANNER"
	local target="$1"
	local profile="$2"
	local date="$3"
	local gargoyle_version="$4"
	local gargoyle_commit="$5"
	local openwrt_branch="$6"
	local openwrt_revision="$7"
	local banner_file_path="$8"
	local revision_save_dir="$9"

	local openwrt_branch_str="OpenWrt $openwrt_branch branch"
	if [ "$openwrt_branch" = "trunk" ] ; then
		openwrt_branch_str="OpenWrt trunk"
	fi

	local top_line=$(printf "| %-26s| %-32s|" "Gargoyle version $gargoyle_version" "$openwrt_branch_str")
	local middle_line=$(printf "| %-26s| %-32s|" "Gargoyle revision $gargoyle_commit" "OpenWrt revision r$openwrt_revision")
	local bottom_line=$(printf "| %-26s| %-32s|" "Built $date" "Target  $target/$profile")

	cat << 'EOF' >"$banner_file_path"
---------------------------------------------------------------
|          _____                             _                |
|         |  __ \                           | |               |
|         | |  \/ __ _ _ __ __ _  ___  _   _| | ___           |
|         | | __ / _` | '__/ _` |/ _ \| | | | |/ _ \          |
|         | |_\ \ (_| | | | (_| | (_) | |_| | |  __/          |
|          \____/\__,_|_|  \__, |\___/ \__, |_|\___|          |
|                           __/ |       __/ |                 |
|                          |___/       |___/                  |
|                                                             |
|-------------------------------------------------------------|
EOF
	echo "$top_line"    >> "$banner_file_path"
	echo "$middle_line" >> "$banner_file_path"
	echo "$bottom_line" >> "$banner_file_path"
	echo '---------------------------------------------------------------' >> "$banner_file_path"

	#save openwrt variables for rebuild
	echo "$openwrt_revision" > "$revision_save_dir/OPENWRT_REVISION"
	echo "$openwrt_branch"  > "$revision_save_dir/OPENWRT_BRANCH"

}


#initialize constants
set_constant_variables


#parse parameters
targets=$1
if [ "$targets" = "ALL" ]  || [ -z "$targets" ] ; then
	targets=$(ls $targets_dir | sed 's/custom//g' 2>/dev/null)
fi

set_version_variables "$2"

verbosity=$3
custom_template=$4
js_compress=$5
if [ -z "$js_compress" ] ; then
	js_compress="true"
fi




#compress javascript
if [ "$js_compress" = "true" ] || [ "$js_compress" = "TRUE" ] || [ "$js_compress" = "1" ] ; then
	uglify_test=$( echo 'var abc = 1;' | uglifyjs  2>/dev/null )
	if [ "$uglify_test" != 'var abc=1' ] ; then
		js_compress="false"
		echo ""
		echo "**************************************************************************"
		echo "**  WARNING: Cannot compress javascript -- uglifyjs is not installed!   **"
		echo "**************************************************************************"
		echo ""
	else
		js_compress="true"
		rm -rf "$compress_js_dir"
		cp -r "package/gargoyle/files/www/js" "$compress_js_dir"
		cd "$compress_js_dir"
		jsfiles=*.js
		for jsf in $jsfiles ; do
			uglifyjs "$jsf" > "$jsf.cmp"
			mv "$jsf.cmp" "$jsf"
		done
		cd "$top_dir"
	fi
fi






#create common download directory if it doesn't exist
if [ ! -d "downloaded" ] ; then
	mkdir "downloaded"
fi

openwrt_src_dir="$top_dir/downloaded/$branch_name-$rnum"

#download openwrt source if we haven't already
if [ ! -d "$openwrt_src_dir" ] ; then
	revision=""
	if [ -n "$rnum" ] ; then
		revision=" -r $rnum "
	fi
	echo "fetching openwrt source"
	rm -rf "$branch_name"
	svn checkout $revision svn://svn.openwrt.org/openwrt/branches/$branch_name/
	if [ ! -d "$branch_name" ] ; then
		echo "ERROR: could not download source, exiting"
		exit
	fi
	cd "$branch_name"
	find . -name ".svn" | xargs -r rm -rf
	cd .. 
	mv "$branch_name" "$openwrt_src_dir"
fi


rm -rf $openwrt_src_dir/dl 
ln -s $top_dir/downloaded $openwrt_src_dir/dl


for target in $targets ; do

	#if user tries to build brcm-2.4 warn them that this has been removed in favor of brcm47xx and build that instead
	if [ "$target" = "brcm-2.4" ] || [ "$target" = "brcm" ] ; then
		echo ""
		echo ""	
		echo "*************************************************************************"
		echo "  WARNING: brcm-2.4 target has been deprecated in favor of newer brcm47xx"
		echo "           Setting target to brcm47xx"
		echo "*************************************************************************"
		target="brcm47xx"
	fi

	echo ""
	echo ""	
	echo "**************************************************************"
	echo "        Gargoyle is now building target: $target"
	echo "**************************************************************"
	echo ""
	echo ""

	#remove old build files
	rm -rf "$target-src"
	rm -rf "built/$target"
	rm -rf "images/$target"

	#copy source to new, target build directory
	cp -r "$openwrt_src_dir" "$target-src"

	
	#copy gargoyle-specific packages to build directory
	package_dir="package"
	gargoyle_packages=$(ls "$package_dir" )
	for gp in $gargoyle_packages ; do
		if [ -d "$target-src/package/$gp" ] ; then
			rm -rf "$target-src/package/$gp" 
		fi
		cp -r "$package_dir/$gp" "$target-src/package"
	done

	#copy compressed javascript to build directory
	if [ "$js_compress" = "true" ] ; then
		rm -rf "$target-src/package/gargoyle/files/www/js"
		cp -r  "$compress_js_dir" "$target-src/package/gargoyle/files/www/js"
	fi


	default_profile="default"
	profile_target_dir="$target"
	if [ "$target" = "custom" ] && [ -n "$custom_template" ] ; then
		profile_target_dir="$custom_template"
	fi
	if [ ! -e "$targets_dir/$profile_target_dir/profiles/$default_profile/config" ] ; then
		profile_dir=""
		profile_dirs="$targets_dir/$profile_target_dir/profiles"/*
		for pd in $profile_dirs ; do
			if [ -z "$profile_dir" ] && [ -e "$pd/config" ] ; then
				profile_dir="$pd"
				default_profile=$(echo "$profile_dir" | sed 's/^.*\///g' | sed 's/^.*\\//g')
			fi
		done
	fi


	#copy this target configuration to build directory
	cp "$targets_dir/$target/profiles/$default_profile/config" "$target-src/.config"


	#if target is custom, checkout optional packages and copy all that don't 
	#share names with gargoyle-specific packages to build directory
	if [ "$target" = "custom" ] ; then
		if [ ! -d packages ] ; then
			svn checkout $revision svn://svn.openwrt.org/openwrt/packages
			
			cd packages
			find . -name ".svn" | xargs rm -rf
			for gp in $gargoyle_packages ; do
				find . -name "$gp" | xargs rm -rf
			done
			cd ..
		fi
		other_packages=$(ls packages)
		for other in $other_packages ; do
			if [ ! -d "$target-src/package/$other" ] ; then
				cp -r packages/$other $target-src/package
			fi
		done
	fi

	profile_name="$default_profile"
	if [ "$target" = "custom" ] ; then
		profile_name="custom"
	fi

	#enter build directory and make sure we get rid of all those pesky .svn files, 
	#and any crap left over from editing
	cd "$target-src"
	find . -name ".svn"  | xargs rm -rf
	find . -name "*~"    | xargs rm -rf
	find . -name ".*sw*" | xargs rm -rf
	
	#Set gargoyle official version parameter in gargoyle package
	echo "OFFICIAL_VERSION:=$full_gargoyle_version" > .ver
	cat .ver "$package_dir/gargoyle/Makefile" >.vermake
	rm .ver
	mv .vermake "$package_dir/gargoyle/Makefile"

	#build, if verbosity is 0 dump most output to /dev/null, otherwise dump everything
	if [ "$verbosity" = "0" ] ; then
		scripts/patch-kernel.sh . "$patches_dir/" >/dev/null 2>&1
		scripts/patch-kernel.sh . "$targets_dir/$target/patches/" >/dev/null 2>&1
		if [ "$target" = "custom" ] ; then
			sh $netfilter_patch_script . ../netfilter-match-modules 1 0 >/dev/null 2>&1
			make menuconfig
			sh $netfilter_patch_script . ../netfilter-match-modules 0 1 >/dev/null 2>&1
		else
			sh $netfilter_patch_script . ../netfilter-match-modules 1 1 >/dev/null 2>&1
		fi
	
		openwrt_target=$(get_target_from_config "./.config")
		create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$rnum" "package/base-files/files/etc/banner" "."

		make -j 4 GARGOYLE_VERSION="$numeric_gargoyle_version"
	else
		scripts/patch-kernel.sh . "$patches_dir/" 
		scripts/patch-kernel.sh . "$targets_dir/$target/patches/" 
		if [ "$target" = "custom" ] ; then
			sh $netfilter_patch_script . ../netfilter-match-modules 1 0  
			make menuconfig
			sh $netfilter_patch_script . ../netfilter-match-modules 0 1  
		else
			sh $netfilter_patch_script . ../netfilter-match-modules 1 1 
		fi

		openwrt_target=$(get_target_from_config "./.config")
		create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$rnum" "package/base-files/files/etc/banner" "."

		make -j 4 V=99 GARGOYLE_VERSION="$numeric_gargoyle_version"
	fi

	#copy packages to built/target directory
	mkdir -p ../built/$target
	arch=$(ls bin)
	if [ -d "bin/$arch/packages/" ] ; then
		package_files=$(find bin -name "*.ipk")
		index_files=$(find bin -name "Packa*")
		for p in $package_files ; do
			cp "$p" ../built/$target
		done
		for i in $index_files ; do
			cp "$i" ../built/$target
		done
	fi
	
	#copy images to images/target directory
	mkdir -p ../images/$target
	arch=$(ls bin)
	image_files=$(ls bin/$arch/ 2>/dev/null)
	if [ ! -e $targets_dir/$target/profiles/default/profile_images  ]  ; then 
		for i in $image_files ; do
			if [ ! -d "bin/$arch/$i" ] ; then
				newname=$(echo "$i" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
				cp "bin/$arch/$i" "../images/$target/$newname"
			fi
		done
	else
		profile_images=$(cat $targets_dir/$target/profiles/default/profile_images 2>/dev/null)
		for pi in $profile_images ; do
			candidates=$(ls bin/$arch/*$pi* 2>/dev/null | sed 's/^.*\///g')
			for c in $candidates ; do
				if [ ! -d "bin/$arch/$c" ] ; then
					newname=$(echo "$c" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
					cp "bin/$arch/$c" "../images/$target/$newname"
				fi
			done
		done
	fi

	#if we didn't build anything, die horribly
	if [ -z "$image_files" ] ; then
		exit
	fi

	other_profiles=""
	if [ "$target" != "custom" ] ; then
		other_profiles=$(ls $targets_dir/$target/profiles | grep -v "^$default_profile$" )
	fi
	for p in $other_profiles ; do

		profile_name="$p"

		#copy profile config and rebuild
		cp $targets_dir/$target/profiles/$p/config .config
		
		openwrt_target=$(get_target_from_config "./.config")
		create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$rnum" "package/base-files/files/etc/banner" "."

		
		if [ "$verbosity" = "0" ] ; then
			make -j 4  GARGOYLE_VERSION="$numeric_gargoyle_version"
		else
			make -j 4 V=99 GARGOYLE_VERSION="$numeric_gargoyle_version"
		fi


		#if we didn't build anything, die horribly
		image_files=$(ls bin/$arch/ 2>/dev/null)	
		if [ -z "$image_files" ] ; then
			exit
		fi

		#copy relevant images for which this profile applies
		profile_images=$(cat $targets_dir/$target/profiles/$p/profile_images 2>/dev/null)
		for pi in $profile_images ; do
			candidates=$(ls bin/$arch/*$pi* 2>/dev/null | sed 's/^.*\///g')
			for c in $candidates ; do
				if [ ! -d "bin/$arch/$c" ] ; then
					newname=$(echo "$c" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
					cp "bin/$arch/$c" "../images/$target/$newname"
				fi
			done
		done
	done

       

	#cd back to parent directory for next target (if there is one)
	cd ..
done

