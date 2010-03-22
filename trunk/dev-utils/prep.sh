# 1) check if we have src directory, if not download
# 2) for each target in targets directory
# 	A) copy src directory to [target]-src
#	B) copy package/* to [target]-src/package
#	C) copy dependencies/* to [target]-src/package
#	D) copy targets[target]/.config to [target]-src
#	D) patch [target]-src with targets/[target]/buildroot-patchfile
#	E) patch packages with targets/[target]/packages-patchfile
#	F) build image and sdk
#	G) mkdir built & copy ipk packages to it
#	H) mkdir images & copy image files to it
# 3) extract SDKs for building standalone packages with build-packages.sh

openwrt_src_dir="kamikaze-7.09-src"
targets_dir="targets"
targets=$(ls $targets_dir 2>/dev/null)

#get gargoyle version for naming image files
gargoyle_version=$(grep "^PKG_VERSION" package/gargoyle/Makefile | sed "s/^.*=//g" 2>/dev/null)
gargoyle_release=$(grep "^PKG_RELEASE" package/gargoyle/Makefile | sed "s/^.*=//g" 2>/dev/null)
if [ -n "$gargoyle_release" ] ; then
	if [ $gargoyle_release -le 5 ] ; then
		gargoyle_version=$gargoyle_version"_beta"$gargoyle_release
	elif [ $gargoyle_release -le 9 ] ; then
		rc=$(($gargoyle_release - 5))
		gargoyle_version=$gargoyle_version"_rc"$rc
	fi
fi


if [ ! -d "$openwrt_src_dir" ] ; then
	echo "fetching kamikaze 7.09 source"
	wget http://downloads.openwrt.org/kamikaze/7.09/kamikaze_7.09.tar.bz2
	tar xjf kamikaze_7.09.tar.bz2
	mv kamikaze_7.09 "$openwrt_src_dir"
fi


for target in $targets ; do
	if ( [ "$1" = "custom" ] && [ "$target" = "custom" ] ) || ([ "$1" != "custom" ] && [ "$target" != "custom" ]) ; then
		if [ -e "$target-src" ] ; then 
			rm -rf "$target-src"
		fi
		cp -r "$openwrt_src_dir" "$target-src"
	
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

		cp "$targets_dir/$target/.config" "$target-src"
		cp "$targets_dir/$target/buildroot.patch" "$target-src"
		cp "$targets_dir/$target/packages.patch" "$target-src"	
		cd "$target-src"
		patch -p 0 < buildroot.patch
		patch -p 0 < packages.patch

		sh ../netfilter-match-modules/integrate_netfilter_modules.sh . ../netfilter-match-modules
	
		if [ "$target" = "custom" ] ; then
			make menuconfig
		fi

	
		cd ..
	fi
done


for target in $targets ; do
	sdk_bzip=$(ls images/$target/*Open*SDK*.bz2 2>/dev/null)
	if [ -n "$sdk_bzip" ] ; then
		cp "$sdk_bzip" "./$target-sdk.tar.bz2"
		tar xjf "$target-sdk.tar.bz2"
		rm "$target-sdk.tar.bz2"
		mv *Open*SDK* "$target-sdk"
	fi
done

