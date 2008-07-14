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


if [ ! -d "kamikaze-7.09-src" ] ; then
	echo "fetching kamikaze 7.09 source"
	wget http://downloads.openwrt.org/kamikaze/7.09/kamikaze_7.09.tar.bz2
	tar xjf kamikaze_7.09.tar.bz2
	mv kamikaze_7.09 kamikaze-7.09-src
fi

if [ -d built ] ; then
	rm -r built
fi
if [ -d images ] ; then
	rm -r images
fi


targets=$(ls targets 2>/dev/null)
for target in $targets ; do
	cp -r kamikaze-7.09-src "$target-src"
	cp -r package/* "$target-src/package/"
	cp -r dependencies/* "$target-src/package/"
	cp "targets/$target/.config" "$target-src"
	cp "targets/$target/buildroot.patch" "$target-src"
	cp "targets/$target/packages.patch" "$target-src"	
	cd "$target-src"
	patch -p 0 < buildroot.patch
	patch -p 0 < packages.patch
	
	make


	mkdir -p ../built/$target
	if [ -d "bin/packages/" ] ; then
		package_files=$(ls bin/packages)
		for p in $package_files ; do
			cp "bin/packages/$p" ../built/$target
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

	
	cd ..
	if [ -n "$image_files" ] ;  then
		rm -rf "$target-src"
	else
		return 1
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

