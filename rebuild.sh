#working directories
top_dir=$(pwd)
targets_dir="$top_dir/targets"
patches_dir="$top_dir/patches-generic"

#script for building netfilter patches
netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules_backfire.sh"




#parse parameters
targets=$1
if [ "$targets" = "ALL" ]  || [ -z "$targets" ] ; then
	targets=$(ls $targets_dir | sed 's/custom//g' 2>/dev/null)
fi

version_name=$2
gargoyle_version="unknown"
if [ -n "$version_name" ] ; then
	gargoyle_version="$version_name"
fi
verbosity=$3

js_compress=$4
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






#get version that should be all numeric
adj_num_version=$(echo "$version_name" | sed 's/X/0/g' | sed 's/x/0/g' | sed 's/[^\.0123456789]//g' )

for target in $targets ; do

	#if user tries to build brcm-2.4 warn them that this has been removed in favor of brcm47xx and build that instead
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

	if [ -d "$target-src" ] ; then
		echo ""
		echo ""	
		echo "**************************************************************"
		echo "        Gargoyle is now rebuilding target: $target"
		echo "**************************************************************"
		echo ""
		echo ""

		#remove old packages and images
		rm -rf "built/$target"
		rm -rf "images/$target"
	
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


		#copy this target configuration to build directory
		cp "$targets_dir/$target/profiles/default/config" "$target-src/.config"
	

		#enter build directory and make sure we get rid of all those pesky .svn files, 
		#and any crap left over from editing
		cd "$target-src"
		find . -name ".svn"  | xargs rm -rf
		find . -name "*~"    | xargs rm -rf
		find . -name ".*sw*" | xargs rm -rf
	
		#if version name specified, set gargoyle official version parameter in gargoyle package
		if [ -n "$version_name" ] ; then
			echo "OFFICIAL_VERSION:=$version_name" > .ver
			cat .ver "$package_dir/gargoyle/Makefile" >.vermake
			rm .ver
			mv .vermake "$package_dir/gargoyle/Makefile"
		fi

		#build, if verbosity is 0 dump most output to /dev/null, otherwise dump everything
		if [ "$verbosity" = "0" ] ; then
			make -j 4  GARGOYLE_VERSION="$adj_num_version"
		else
			make -j 4 V=99 GARGOYLE_VERSION="$adj_num_version"
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
					version_str=$(echo "$gargoyle_version" | tr 'A-Z' 'a-z' | sed 's/ *(.*$//g' | sed 's/ /_/g')
					newname=$(echo "$i" | sed "s/openwrt/gargoyle_$version_str/g")
					cp "bin/$arch/$i" "../images/$target/$newname"
				fi
			done
		else
			profile_images=$(cat $targets_dir/$target/profiles/default/profile_images 2>/dev/null)
			for pi in $profile_images ; do
				candidates=$(ls bin/$arch/*$pi* 2>/dev/null | sed 's/^.*\///g')
				for c in $candidates ; do
					if [ ! -d "bin/$arch/$c" ] ; then
						version_str=$(echo "$gargoyle_version" | tr 'A-Z' 'a-z' | sed 's/ *(.*$//g' | sed 's/ /_/g')
						newname=$(echo "$c" | sed "s/openwrt/gargoyle_$version_str/g")
						cp "bin/$arch/$c" "../images/$target/$newname"
					fi
				done
			done
		fi

		#if we didn't build anything, die horribly
		if [ -z "$image_files" ] ; then
			exit
		fi

		other_profiles=$(ls $targets_dir/$target/profiles | grep -v "^default$" )
		for p in $other_profiles ; do
	
			#copy profile config and rebuild
			cp $targets_dir/$target/profiles/$p/config .config
			if [ "$verbosity" = "0" ] ; then
				make -j 4 GARGOYLE_VERSION="$adj_num_version"
			else
				make -j 4 V=99 GARGOYLE_VERSION="$adj_num_version"
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
						version_str=$(echo "$gargoyle_version" | tr 'A-Z' 'a-z' | sed 's/ *(.*$//g' | sed 's/ /_/g')
						newname=$(echo "$c" | sed "s/openwrt/gargoyle_$version_str/g")
						cp "bin/$arch/$c" "../images/$target/$newname"
					fi
				done
			done
		done

      
		#cd back to parent directory for next target (if there is one)
		cd ..
	fi
done

