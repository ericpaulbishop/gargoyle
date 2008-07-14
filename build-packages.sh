
get_var()
{
	var_name=$1
	file_name=$2

	lines=$(cat "$file_name" | sed "s/\t/INITIAL_TAB/g" )
	vfound="no"
	IFS='
'
	var=""
	for line in $lines ; do
		if [ "$vfound" = "no" ] ; then
			found=$(printf "%s" "$line" | grep "$var_name" | sed "s/$var_name//g")
			if [ -n "$found" ] ; then
				vfound="yes"
				var=$found
			fi
		else
			t=$(printf "%s" "$line" | egrep "^[^\t ]+")
			if [ -z "$t" ] ; then
				var=$(printf "%s\n%s" "$var" "$line")
			else
				vfound="no"
			fi
		fi
	done
	printf "%s" "$var"
}

build_package_index()
{	
	control_files=$(find . -wholename "./build*/*CONTROL/control")
	for file in $control_files ; do
		package=$(get_var "Package: " "$file")
		version=$(get_var "Version: " "$file")
		depends=$(get_var "Depends: " "$file")
		provides=$(get_var "Provides: " "$file")
		source=$(get_var "Source: " "$file")
		section=$(get_var "Section: " "$file")
		priority=$(get_var "Priority: " "$file")
		maintainer=$(get_var "Maintainer: " "$file")
		arch=$(get_var "Architecture: " "$file")
		description=$(get_var "Description: " "$file")




		ipk_file="./bin/packages/$package"_"$version"_"$arch".ipk
		ipk_file_name=$(echo "$ipk_file" | sed "s/^.*\///g")

		#ipkg_dir=$(echo $file | sed "s/CONTROL.*$//g")
		#size=$(du -bs $ipkg_dir | awk '{ print $1 ; }' )
		size=$(du -bs $ipk_file | awk '{ print $1 ; }' )
		md5=$(md5sum "$ipk_file" | awk '{ print $1 ; }' )

		printf "Package: %s\n" "$package"
		printf "Version: %s\n" "$version"
		printf "Depends: %s\n" "$depends"
		printf "Provides: %s\n" "$provides"
		printf "Source: %s\n" "$source"
		printf "Section: %s\n" "$section"
		printf "Priority: %s\n" "$section"
		printf "Maintainer: %s\n" "$maintainer"
		printf "Architecture: %s\n" "$arch"
		printf "Filename: %s\n" "$ipk_file_name"
		printf "Size: %s\n" "$size"
		printf "MD5Sum: %s\n" "$md5"
		printf "Description: %s\n" "$description"
		echo ""
		echo ""
	done
}



current_dir=$(pwd)
targets=$(ls targets 2>/dev/null)

sdk_dirs=""
for target in $targets ; do
	if [ -d "$target-sdk" ] ; then 
		sdk_dirs="$sdk_dirs $target-sdk"
	fi
done
	

if [ -z "$sdk_dirs" ] ; then
	echo "ERROR: Target SDK directories do not exist."
	echo "       Run full-build.sh to create them."
	exit 0
fi

#sdk_dirs="kamikaze79-sdk-atheros kamikaze79-sdk-brcm"

if [ -e "built" ] ; then
	rm -r "built"
fi
mkdir "built"


for target in $targets ; do
	
	sdk="$target-sdk"
	if [ -d "$sdk" ] ; then

		echo "$target"
		mkdir -p "$current_dir/built/$target"
		
		sdk_package_dirs=$(ls $sdk/package)
		for spd in $sdk_package_dirs ; do
			if [ -d "$sdk/package/$spd" ] || [ -h "$sdk/package/$spd" ] ; then
				rm -r "$sdk/package/$spd"
			fi
		done
		packages=$(ls package)
		for package in $packages ; do
			cp -r "$current_dir/package/$package" "$sdk/package/$package"
		done
		
		cp "targets/$target/packages.patch" "$sdk/"
		cd $sdk
		sdk_dir=$(pwd)
		
		
		patch -p 0 < packages.patch
		rm packages.patch
		


		rm -rf tmp
		make clean
		make

		build_package_index > Packages	
		
		mv Packages $current_dir/built/$target
		mv bin/pack*/*.ipk $current_dir/built/$target
		make clean
		cd $current_dir
	fi
done




