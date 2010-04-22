#working directories
top_dir=$(pwd)
openwrt_src_dir="$top_dir/backfire-src"
targets_dir="$top_dir/targets-backfire"
patches_dir="$top_dir/patches-generic-backfire"
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

#get version that should be all numeric
adj_num_version=$(echo "$version_name" | sed 's/X/0/g' | sed 's/x/0/g' | sed 's/[^\.0123456789]//g' )

# set svn revision number to use 
# you can set this to an alternate revision 
# or empty to checkout latest 
#rnum=18801


#download openwrt source if we haven't already
if [ ! -d "$openwrt_src_dir" ] ; then
	revision=""
	if [ -n "$rnum" ] ; then
		revision=" -r $rnum "
	fi
	echo "fetching backfire source"
	svn checkout $revision svn://svn.openwrt.org/openwrt/tags/backfire_10.03/
	if [ ! -d "backfire_10.03" ] ; then
		echo "ERROR: could not download source, exiting"
		exit
	fi
	mv backfire_10.03 $openwrt_src_dir
	cd $openwrt_src_dir
	find . -name ".svn" | xargs -r rm -rf
	cd .. 
fi


#create common download directory if it doesn't exist
if [ ! -d "downloaded" ] ; then
	mkdir "downloaded"
fi
rm -rf $openwrt_src_dir/dl 
ln -s $top_dir/downloaded $openwrt_src_dir/dl


for target in $targets ; do

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
	if [ -d "package-backfire" ] ; then
		package_dir="package-backfire"
	fi
	gargoyle_packages=$(ls "$package_dir" )
	for gp in $gargoyle_packages ; do
		if [ -d "$target-src/package/$gp" ] ; then
			rm -rf "$target-src/package/$gp" 
		fi
		cp -r "$package_dir/$gp" "$target-src/package"
	done
	
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

	#copy this target configuration to build directory
	cp "$targets_dir/$target/.config" "$target-src"
	

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
		scripts/patch-kernel.sh . "$patches_dir/" >/dev/null 2>&1
		scripts/patch-kernel.sh . "$targets_dir/$target/patches/" >/dev/null 2>&1
		if [ "$target" = "custom" ] ; then
			sh $netfilter_patch_script . ../netfilter-match-modules 1 0 >/dev/null 2>&1
			make menuconfig
			sh $netfilter_patch_script . ../netfilter-match-modules 0 1 >/dev/null 2>&1
		else
			sh $netfilter_patch_script . ../netfilter-match-modules 1 1 >/dev/null 2>&1
		fi
		make  GARGOYLE_VERSION="$adj_num_version"
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
		make V=99 GARGOYLE_VERSION="$adj_num_version"
	fi

	#copy packages to built/target directory
	mkdir -p ../built/$target
	if [ -d "bin/packages/" ] ; then
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
	image_files=$(ls bin 2>/dev/null)
	for i in $image_files ; do
		if [ ! -d "bin/$i" ] ; then
			version_str=$(echo "$gargoyle_version" | tr 'A-Z' 'a-z' | sed 's/ *(.*$//g' | sed 's/ /_/g')
			newname=$(echo "$i" | sed "s/openwrt/gargoyle_$version_str/g")
			cp "bin/$i" "../images/$target/$newname"
		fi
	done

	#if we didn't build anything, die horribly
	bin_contents=$(ls bin/* 2>/dev/null)	
	if [ -z "$bin_contents" ] ; then
		exit
	fi	       

	#cd back to parent directory for next target (if there is one)
	cd ..
done

#extract all target SDKs into parent directory if they exist
for target in $targets ; do
	sdk_bzip=$(ls images/$target/*Open*SDK*.bz2 2>/dev/null)
	if [ -n "$sdk_bzip" ] ; then
		rm -rf "$target-sdk" *Open*SDK*
		cp "$sdk_bzip" "./$target-sdk.tar.bz2"
		tar xjf "$target-sdk.tar.bz2"
		rm "$target-sdk.tar.bz2"
		mv *Open*SDK* "$target-sdk"
	fi
done

