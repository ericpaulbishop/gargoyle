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


if [ ! -d "$openwrt_src_dir" ] ; then
	echo "fetching kamikaze 7.09 source"
	wget http://downloads.openwrt.org/kamikaze/7.09/kamikaze_7.09.tar.bz2
	tar xjf kamikaze_7.09.tar.bz2
	mv kamikaze_7.09 "$openwrt_src_dir"
fi

if [ -d built ] ; then
	rm -r built
fi

if [ -d images ] ; then
	rm -r images
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

		if [ -e $targets_dir/$target/.config ] ; then
			cp "$targets_dir/$target/.config" "$target-src"
		fi
		cp "$targets_dir/$target/buildroot.patch" "$target-src"
		cp "$targets_dir/$target/packages.patch" "$target-src"	
		cd "$target-src"
		patch -p 0 < buildroot.patch
		patch -p 0 < packages.patch
	
		if [ "$target" = "custom" ] ; then
			make menuconfig
		fi

	
		make V=99


		mkdir -p ../built/$target
		if [ -d "bin/packages/" ] ; then
			package_files=$(find bin -name "*.ipk")
			for p in $package_files ; do
				cp "$p" ../built/$target
			done
		fi

		mkdir -p ../images/$target
		image_files=$(ls bin 2>/dev/null)
		for i in $image_files ; do
			if [ ! -d "bin/$i" ] ; then
				newname=$(echo $i | sed 's/openwrt/gargoyle/g')
				cp "bin/$i" ../images/$target/$newname
			fi
		done
		
		#save downloaded files so we don't have to download them again
		dl_files=$(ls ./dl 2>/dev/null)
		if [ -n "$dl_files" ] ; then
			if [ ! -d "../kamikaze-7.09-src/dl" ] ; then
				mkdir "../kamikaze-7.09-src/dl"
			fi
			cp -r dl/* ../kamikaze-7.09-src/dl
		fi
	

		cd ..
		if [ -n "$image_files" ] && [ "$target" != "custom" ] ;  then
			rm -rf "$target-src"
		else
			exit
		fi
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

