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


#uncomment below to set a specific revision number to check out
#rnum="11800"


revision=""
if [ -n "$rnum" ] ; then
	revision=" -r $rnum "
fi

#always remove old source, we may be checking out different revision
if [ -d "kamikaze-trunk-src" ] ; then
	rm -rf kamikaze-trunk-src
fi
if [ -d "trunk" ] ; then
	rm -rf trunk
fi

#remove old package/image directories
if [ -d built ] ; then
	rm -r built
fi
if [ -d images ] ; then
	rm -r images
fi




#checkout source
echo "fetching kamikaze trunk source"
svn checkout $revision https://svn.openwrt.org/openwrt/trunk/
mv trunk kamikaze-trunk-src
cd kamikaze-trunk-src
find . -name ".svn" | xargs -r rm -rf
cd .. 



targets_dir="targets-trunk"

targets=$(ls $targets_dir 2>/dev/null)

for target in $targets ; do
	if ( [ "$1" = "custom" ] && [ "$target" = "custom" ] ) || ([ "$1" != "custom" ] && [ "$target" != "custom" ]) ; then
	

		if [ -e "$target-src" ] ; then 
			rm -rf "$target-src"
		fi
		cp -r kamikaze-trunk-src "$target-src"
	
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
		pwd		
		
		patch -p 0 < buildroot.patch
		patch -p 0 < packages.patch

		#build netfilter patches
		sh ../netfilter-match-modules/integrate_netfilter_modules_trunk.sh . ../netfilter-match-modules

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

