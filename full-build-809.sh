#working directories
top_dir=$(pwd)
openwrt_src_dir="$top_dir/kamikaze-8.09-src"
targets_dir="$top_dir/targets-8.09"
netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules_809.sh"


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



# set svn revision number to use 
# you can set this to an alternate revision 
# or empty to checkout latest 8.09 branch
rnum=16622


#download openwrt source if we haven't already
if [ ! -d "$openwrt_src_dir" ] ; then
	revision=""
	if [ -n "$rnum" ] ; then
		revision=" -r $rnum "
	fi
	echo "fetching kamikaze 8.09 source"
	svn checkout $revision svn://svn.openwrt.org/openwrt/branches/8.09/
	if [ ! -d "8.09" ] ; then
		echo "ERROR: could not download source, exiting"
		exit
	fi
	mv 8.09 $openwrt_src_dir
	cd $openwrt_src_dir
	find . -name ".svn" | xargs -r rm -rf
	cd .. 
fi

#create common download directory if it doesn't exist
if [ ! -d "downloaded" ] ; then
	mkdir "downloaded"
fi
if [ ! -e $openwrt_src_dir/dl ] ; then
	ln -s $top_dir/downloaded $openwrt_src_dir/dl
fi


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
	gargoyle_packages=$(ls package ; ls dependencies)
	for gp in $gargoyle_packages ; do
		if [ ! -d "$target-src/package/$gp" ] ; then
			if [ -d "package/$gp" ] ; then
				cp -r package/$gp $target-src/package
			else
				cp -r dependencies/$gp $target-src/package
			fi
		fi
	done
	
	#if target is custom, checkout optional packages and copy all that don't 
	#share names with gargoyle-specific packages to build directory
	if [ "$target" = "custom" ] ; then
		if [ ! -d packages_8.09 ] ; then
			svn checkout $revision svn://svn.openwrt.org/openwrt/branches/packages_8.09
			
			cd packages_8.09
			find . -name ".svn" | xargs rm -rf
			for gp in $gargoyle_packages ; do
				find . -name "$gp" | xargs rm -rf
			done
			cd ..
		fi
		other_packages=$(ls packages_8.09)
		for other in $other_packages ; do
			if [ ! -d "$target-src/package/$other" ] ; then
				cp -r packages_8.09/$other $target-src/package
			fi
		done
	fi

	#copy this target configuration to build directory
	cp "$targets_dir/$target/.config" "$target-src"
	

	#enter build directory
	cd "$target-src"
	
	#if version name specified, set gargoyle official version parameter in gargoyle package
	if [ -n "$version_name" ] ; then
		echo "OFFICIAL_VERSION:=$version_name" > .ver
		cat .ver package/gargoyle/Makefile >.vermake
		rm .ver
		mv .vermake package/gargoyle/Makefile
	fi

	#build, if verbosity is 0 dump most output to /dev/null, otherwise dump everything
	if [ "verbosity" = "0" ] ; then
		scripts/patch-kernel.sh . $targets_dir/$target/patches/ >/dev/null 2>&1
		if [ "$target" = "custom" ] ; then
			sh $netfilter_patch_script . ../netfilter-match-modules 1 0 2>&1
			make menuconfig
			sh $netfilter_patch_script . ../netfilter-match-modules 0 1 2>&1
		else
			sh $netfilter_patch_script . ../netfilter-match-modules 2>&1
		fi
		make
	else
		scripts/patch-kernel.sh . $targets_dir/$target/patches/
		if [ "$target" = "custom" ] ; then
			sh $netfilter_patch_script . ../netfilter-match-modules 1 0 
			make menuconfig
			sh $netfilter_patch_script . ../netfilter-match-modules 0 1 
		else
			sh $netfilter_patch_script . ../netfilter-match-modules 
		fi
		make V=99
	fi

	#copy packages to built/target directory
	mkdir -p ../built/$target
	if [ -d "bin/packages/" ] ; then
		package_files=$(find bin -name "*.ipk")
		for p in $package_files ; do
			cp "$p" ../built/$target
		done
		if [ -e "bin/packages/Packages" ] ; then
			cp "bin/packages/Packages" ../built/$target
		fi
	fi

	#copy images to images/target directory
	mkdir -p ../images/$target
	image_files=$(ls bin 2>/dev/null)
	for i in $image_files ; do
		if [ ! -d "bin/$i" ] ; then
			newname=$(echo "$i" | sed "s/openwrt/gargoyle_$gargoyle_version/g")
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

