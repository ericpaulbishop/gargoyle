#!/bin/bash

set_constant_variables()
{
	#working directories
	scriptpath="$(readlink -f "$0")"
	top_dir="${scriptpath%/${0##*/}}"

	targets_dir="$top_dir/targets"
	patches_dir="$top_dir/patches-generic"
	compress_js_dir="$top_dir/compressed_javascript"

	#script for building netfilter patches
	netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules.sh"
	
	#set date here, so it's guaranteed the same for all images
	#even though build can take several hours
	build_date=$(date +"%B %d, %Y")

	gargoyle_git_revision=$(git log -1 --pretty=format:%h )

}


set_constant_variables()
{
	#working directories
	scriptpath="$(readlink -f "$0")"
	top_dir="${scriptpath%/${0##*/}}"
	targets_dir="$top_dir/targets"
	patches_dir="$top_dir/patches-generic"
	compress_js_dir="$top_dir/compressed_javascript"
	
	#script for building netfilter patches
	netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules.sh"


	#cores / build threads
	num_cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null)
	if [ -z "$num_cores" ] ; then num_cores=1 ; fi
	
	#################################################################################################
	# Starging in Attitude Adjustment r36470 multi-threaded builds often fail due to race conditions 
	# somewhere. As of Attitude Adjustment r7838 these issues seem to have been resolved.
	#
	# However, if there is trouble with parallel builds, or you start getting mysterious non-obvious
	# build errors in the future try setting num_build_threads to 1 below.
	#################################################################################################

	#
	# Aaaand... this little bugger is causing problems again...
	#
	#num_build_threads=$(($num_cores + 2)) # more threads than cores, since each thread will sometimes block for i/o
	num_build_threads=1
}

set_version_variables()
{
	#set date here, so it is guaranteed the same for all images
	#even though build can take several hours
	build_date=$(date +"%B %d, %Y")

	gargoyle_git_revision=$(git log -1 --pretty=format:%h )

	# Full display version in gargoyle web interface
	if [ -z "$full_gargoyle_version" ] ; then
		full_gargoyle_version="$1"
	fi
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

	local top_line=$(printf "| %-26s| %-35s|" "Gargoyle version $gargoyle_version" "$openwrt_branch_str")
	local middle_line=$(printf "| %-26s| %-35s|" "Gargoyle revision $gargoyle_commit" "OpenWrt revision r$openwrt_revision")
	local bottom_line=$(printf "| %-26s| %-35s|" "Built $date" "Target  $target/$profile")

	cat << 'EOF' >"$banner_file_path"
------------------------------------------------------------------
|            _____                             _                 |
|           |  __ \                           | |                |
|           | |  \/ __ _ _ __ __ _  ___  _   _| | ___            |
|           | | __ / _` | '__/ _` |/ _ \| | | | |/ _ \           |
|           | |_\ \ (_| | | | (_| | (_) | |_| | |  __/           |
|            \____/\__,_|_|  \__, |\___/ \__, |_|\___|           |
|                             __/ |       __/ |                  |
|                            |___/       |___/                   |
|                                                                |
|----------------------------------------------------------------|
EOF
	echo "$top_line"    >> "$banner_file_path"
	echo "$middle_line" >> "$banner_file_path"
	echo "$bottom_line" >> "$banner_file_path"
	echo '------------------------------------------------------------------' >> "$banner_file_path"

	#save openwrt variables for rebuild
	echo "$openwrt_revision" > "$revision_save_dir/OPENWRT_REVISION"
	echo "$openwrt_branch"  > "$revision_save_dir/OPENWRT_BRANCH"

}


do_js_compress()
{
	uglifyjs_arg1="$1"
	uglifyjs_arg2="$2"

	rm -rf "$compress_js_dir"
	mkdir "$compress_js_dir"
	escaped_package_dir=$(echo "$top_dir/package-prepare/" | sed 's/\//\\\//g' | sed 's/\-/\\-/g' ) ;
	for jsdir in $(find "${top_dir}/package-prepare" -path "*/www/js") ; do
		pkg_rel_path=$(echo $jsdir | sed "s/$escaped_package_dir//g");
		mkdir -p "$compress_js_dir/$pkg_rel_path"
		cp "$jsdir/"*.js "$compress_js_dir/$pkg_rel_path/"
		cd "$compress_js_dir/$pkg_rel_path/"
	 	
		for jsf in *.js ; do
	 		if [ -n "$uglifyjs_arg2" ] ; then
				"$uglifyjs_arg1" "$uglifyjs_arg2" "$jsf" > "$jsf.cmp"
			else
				"$uglifyjs_arg1" "$jsf" > "$jsf.cmp"
			fi
	 		mv "$jsf.cmp" "$jsf"
	 	done
	done
	cp -r "$compress_js_dir"/* "$top_dir/package-prepare/"

	cd "$top_dir"
}

distrib_copy_arch_ind_ipk()
{
	local tgt="$1"
	local ltype="$2"
	local di=1
	
	local dpkgs=$(find "$top_dir/package" -path '*plugin-gargoyle-*' -and -name 'Makefile' -and -not -path '*-i18n-*' | xargs grep -s -l "DEPENDS:=+gargoyle$" | xargs grep -s -l "PKGARCH:=all$" | awk -F'/' '{print $(NF-1)}')
	
	# printf -- '%s\n' "${dpkgs[@]}"
	
	if [ ! -d "$top_dir/Distribution/architecture-independent packages ]" ] ; then
		mkdir -p "$top_dir/Distribution/architecture-independent packages"
	fi
	#if [ ! -d "$top_dir/Distribution/theme packages ]" ] ; then
	#	mkdir -p "$top_dir/Distribution/theme packages"
	#fi
	
	if [ ! -d "$top_dir/Distribution/theme packages ]" ] && [ "$ltype" = 'internationalize' ] ; then
		mkdir -p "$top_dir/Distribution/language packages"
	fi
	
	while true; do
		local apkg=$(echo "$dpkgs" | awk -v rec=$di 'NR==rec {print $0}')
		[[ -z "$apkg" ]] && 
		{
			break
		} || {
			ipkg=("$top_dir/$tgt-src/bin/$tgt/packages/${apkg}"*"ipk")
			[[ -f "${ipkg[0]}" ]] &&
			{
				cp -f "$top_dir/$tgt-src/bin/$tgt/packages/$apkg"*".ipk" "$top_dir/Distribution/architecture-independent packages/"
			}
		}
		let di++
	done
	#cp -f "$top_dir/$tgt-src/bin/$tgt/packages/plugin-gargoyle-theme-"*".ipk" "$top_dir/Distribution/theme packages/"
	
	if [ "$ltype" = 'internationalize' ] ; then
		cp -f "$top_dir/$tgt-src/bin/$tgt/packages/plugin-gargoyle-i18n-"*".ipk" "$top_dir/Distribution/language packages/"
	fi
}

distrib_init ()
{
	if [ ! -d "$top_dir/Distribution" ] ; then
		mkdir "$top_dir/Distribution"
	fi
	#git log --since=5/16/2013 $(git log -1 --pretty=format:%h) --pretty=format:"%h%x09%ad%x09%s" --date=short > "$top_dir/Distribution/changelog.txt"
	git log $(git log $(git describe --abbrev=0 --tags)..$(git log -1 --pretty=format:%h) --no-merges --pretty=format:"%h%x09%ad%x09%s" --date=short)..$(git log -1 --pretty=format:%h) --no-merges --pretty=format:"%h%x09%ad%x09%s" --date=short > "$top_dir/Distribution/Gargoyle changelog.txt"
	svn log -r "$rnum":36425 svn://svn.openwrt.org/openwrt/branches/attitude_adjustment/ > "$top_dir/Distribution/OpenWrt changelog.txt"
	cp -fR "$top_dir/LICENSES" "$top_dir/Distribution/"
}


######################################################################################################
## Begin Main Body of Build Script                                                                  ##
######################################################################################################


if [ -z "${BASH_VERSION}" ] || [ "${BASH_VERSION:0:1}" -lt '4' ]; then
	echo 'Build script was designed to work with bash in version 4 (at least). Exiting...'
	exit 1
fi



#initialize constants
set_constant_variables
cd "$top_dir"

#parse parameters
targets="$1"
full_gargoyle_version="$2"
verbosity="$3"
js_compress="$4"
specified_profile="$5"
translation_type="$6"
fallback_lang="$7"
active_lang="$8"
distribution="$9"

if [ "$targets" = "ALL" ]  || [ -z "$targets" ] ; then
	targets=$(ls $targets_dir | sed 's/custom//g' 2>/dev/null)
fi
if [ -z "$js_compress" ] ; then
	js_compress="true"
fi
set_version_variables "$full_gargoyle_version"


if [ -d "$top_dir/package-prepare" ] ; then	
	rm -rf "$top_dir/package-prepare"
fi

#NOTE: because 'make ALL' calls this script in a for loop, rm -rf "$top_dir/Distribution" cannot be done here

[ ! -z $(which python 2>&1) ] && {
	#whether localize or internationalize, the packages directory is going to be modified
	#default behavior is internationalize; defined in Makefile
	[ "$translation_type" = "localize" ] 	&& "$top_dir/i18n-scripts/localize.py" "$fallback_lang" "$active_lang" \
											|| "$top_dir/i18n-scripts/internationalize.py" "$active_lang"
} || {
	active_lang=$(sh ./i18n-scripts/intl_ltd.sh "$translation_type" "$active_lang")
}


#compress javascript
if [ "$js_compress" = "true" ] || [ "$js_compress" = "TRUE" ] || [ "$js_compress" = "1" ] ; then

	cd "$top_dir"

	uglify_test=$( echo 'var abc = 1;' | uglifyjs  2>/dev/null )
	if [ "$uglify_test" != 'var abc=1' ] &&  [ "$uglify_test" != 'var abc=1;' ]  ; then
		
		node_bin="$top_dir/node/node"
		uglifyjs_bin="$top_dir/UglifyJS/bin/uglifyjs"
		if [ ! -e "$node_bin" ] && [ ! -e "$uglifyjs_bin" ] ; then
			echo ""
			echo "**************************************************************************"
			echo "**  uglifyjs is not installed globally, attempting to build it          **"
			echo "**************************************************************************"
			echo ""

			#node
			git clone git://github.com/joyent/node.git
			cd node
			git checkout v0.7.12
			./configure 
			make
			cd "$top_dir"


			#uglifyjs
			git clone git://github.com/mishoo/UglifyJS.git
			cd UglifyJS/bin
			git checkout v1.3.1
			cd "$top_dir"
		fi
		uglify_test=$( echo 'var abc = 1;' | "$node_bin" "$uglifyjs_bin"  2>/dev/null )
		if [ "$uglify_test" = 'var abc=1' ] ||  [ "$uglify_test" = 'var abc=1;' ]  ; then
			js_compress="true"
			do_js_compress "$node_bin" "$uglifyjs_bin"
	
		else
			js_compress="false"
			echo ""
			echo "**************************************************************************"
			echo "**  WARNING: Cannot compress javascript -- uglifyjs could not be built  **"
			echo "**************************************************************************"
			echo ""
		fi
	else
		js_compress="true"
		do_js_compress "uglifyjs"
	fi
	cd "$top_dir"
fi





for target in $targets ; do

	if [ -d "$target-src" ] ; then

		#remove old build files
		if [ -z "$specified_profile" ] ; then
			rm -rf "$top_dir/built/$target"
			rm -rf "$top_dir/images/$target"
		else
			profile_images=$(cat "$targets_dir/$target/profiles/$specified_profile/profile_images" 2>/dev/null)
			mkdir -p "$top_dir/images/$target/"
			for pi in $profile_images ; do
				rm -rf "$top_dir/images/$target/"*"$pi"*
			done
		fi

	
		#copy gargoyle-specific packages to build directory
		package_dir="$top_dir/package-prepare"
		if [ ! -d "$package_dir" ] ; then
			package_dir="$top_dir/package"
		fi
		gargoyle_packages=$(ls "$package_dir" )
		for gp in $gargoyle_packages ; do
			if [ -d "$target-src/package/$gp" ] ; then
				rm -rf "$target-src/package/$gp" 
			fi
			cp -r "$package_dir/$gp" "$target-src/package"
		done
	


		# specify default build profile	
		default_profile="default"
		if [ -n "$specified_profile" ] ; then
			default_profile="$specified_profile" 
		fi
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
		profile_name="$default_profile"
		if [ "$target" = "custom" ] ; then
			profile_name="custom"
		fi

		echo ""
		echo ""	
		echo "**************************************************************************"
		echo "        Gargoyle is now rebuilding target: $target / $profile_name"
		echo "**************************************************************************"
		echo ""
		echo ""

	
		#copy this target configuration to build directory
		cp "$targets_dir/$target/profiles/$default_profile/config" "$top_dir/${target}-src/.config"
		
		
		[ ! -z $(which python 2>&1) ] && {
			#finish internationalization by setting the target language & adding the i18n plugin to the config file
			#finish localization just deletes the (now unnecessary) language packages from the config file
			[ "$translation_type" = "localize" ] 	&& "$top_dir/i18n-scripts/finalize_translation.py" 'localize' "$target" \
													|| "$top_dir/i18n-scripts/finalize_translation.py" 'internationalize' "$active_lang" "$target"
		} || {
			#NOTE: localize is not supported because it requires python
			"$top_dir/i18n-scripts/finalize_tran_ltd.sh" "$target-src" "$active_lang"
		}


		#enter build directory and make sure we get rid of all those pesky .svn files, 
		#and any crap left over from editing
		cd "$top_dir/$target-src"
		find . -name ".svn"  | xargs rm -rf
		find . -name "*~"    | xargs rm -rf
		find . -name ".*sw*" | xargs rm -rf
		
		branch_name=$(cat "OPENWRT_BRANCH")
		rnum=$(cat "OPENWRT_REVISION")

	
		#if version name specified, set gargoyle official version parameter in gargoyle package
		echo "OFFICIAL_VERSION:=$full_gargoyle_version" > .ver
		cat .ver "$package_dir/gargoyle/Makefile" >.vermake
		rm .ver
		mv .vermake "$top_dir/$target-src/package/gargoyle/Makefile"
		
		#build, if verbosity is 0 dump most output to /dev/null, otherwise dump everything
		openwrt_target=$(get_target_from_config "./.config")
		create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$rnum" "package/base-files/files/etc/banner" "."
		if [ "$verbosity" = "0" ] ; then
			make -j $num_build_threads  GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$default_profile"

		else
			make -j $num_build_threads V=99 GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$default_profile"
		fi
		
		if [ "$distribution" = "true" ] || [ "$distribution" = "TRUE" ] || [ "$distribution" = "1" ] ; then
			distribution="true"
			distrib_init
			mkdir -p "$top_dir/Distribution/Images/$target-$default_profile"
		fi



		#copy packages to build/target directory
		mkdir -p "$top_dir/built/$target/$profile_name"
		arch=$(ls bin)
		package_files=$(find bin -name "*.ipk")
		index_files=$(find bin -name "Packa*")
		if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
			for pf in $package_files ; do
				cp "$pf" "$top_dir/built/$target/$profile_name/"
			done
			for inf in $index_files ; do
				cp "$inf" "$top_dir/built/$target/$profile_name/"
			done
		fi
	
		#copy images to images/target directory
		mkdir -p "$top_dir/images/$target"
		arch=$(ls bin)
		image_files=$(ls bin/$arch/ 2>/dev/null)
		if [ ! -e "$targets_dir/$target/profiles/$default_profile/profile_images"  ]  ; then 
			for imf in $image_files ; do
				if [ ! -d "bin/$arch/$imf" ] ; then
					newname=$(echo "$imf" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
					cp "bin/$arch/$imf" "$top_dir/images/$target/$newname"
					if [ "$distribution" = "true" ] ; then
						cp "bin/$arch/$imf" "$top_dir/Distribution/Images/$target-$default_profile/$newname"
					fi
				fi
			done
		else
			profile_images=$(cat "$targets_dir/$target/profiles/$default_profile/profile_images" 2>/dev/null)
			for pi in $profile_images ; do
				candidates=$(ls "bin/$arch/"*"$pi"* 2>/dev/null | sed 's/^.*\///g')
				for c in $candidates ; do
					if [ ! -d "bin/$arch/$c" ] ; then
						newname=$(echo "$c" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
						cp "bin/$arch/$c" "$top_dir/images/$target/$newname"
						if [ "$distribution" = "true" ] ; then
							cp "bin/$arch/$c" "$top_dir/Distribution/Images/$target-$default_profile/$newname"
						fi
					fi
				done
			done
		fi

		#if we didn't build anything, die horribly
		if [ -z "$image_files" ] ; then
			exit
		fi
		
		if [ "$distribution" = "true" ] ; then
			#Generate licenses file for each profile
			#Copy architecture independent packages & themes to Distribution folder
			echo "Generating Licenses file (expect it to take 5+ minutes)"
			"$top_dir/dev-utils/GenLicences.sh" "$target" "$profile_name" 1 2>&1
			distrib_copy_arch_ind_ipk "$target" "$translation_type"
		fi
	
		other_profiles=""
		if [ "$target" != "custom" ] && [ -z "$specified_profile" ] ; then
			other_profiles=$(ls "$targets_dir/$target/profiles" | grep -v "^$default_profile$" )
		fi
		for profile_name in $other_profiles ; do
		

			#copy profile config and rebuild
			cp "$targets_dir/$target/profiles/$profile_name/config" .config
			
			
			[ ! -z $(which python 2>&1) ] && {
				#finish internationalization by setting the target language & adding the i18n plugin to the config file
				#finish localization just deletes the (now unnecessary) language packages from the config file
				[ "$translation_type" = "localize" ] 	&& "$top_dir/i18n-scripts/finalize_translation.py" 'localize' "$target" \
														|| "$top_dir/i18n-scripts/finalize_translation.py" 'internationalize' "$active_lang" "$target"
			} || {
				#NOTE: localize is not supported because it requires python
				"$top_dir/i18n-scripts/finalize_tran_ltd.sh" "$target-src" "$active_lang"
			}
			

			openwrt_target=$(get_target_from_config "./.config")
			create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$rnum" "package/base-files/files/etc/banner" "."

			echo ""
			echo ""	
			echo "**************************************************************************"
			echo "        Gargoyle is now rebuilding target: $target / $profile_name"
			echo "**************************************************************************"
			echo ""
			echo ""



			if [ "$verbosity" = "0" ] ; then
				make -j $num_build_threads GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$profile_name"

			else
				make -j $num_build_threads V=99 GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$profile_name"
			fi


			#if we didn't build anything, die horribly
			image_files=$(ls "bin/$arch/" 2>/dev/null)	
			if [ -z "$image_files" ] ; then
				exit
			fi

			#copy packages to build/target directory
			mkdir -p "$top_dir/built/$target/$profile_name"
			arch=$(ls bin)
			package_files=$(find bin -name "*.ipk")
			index_files=$(find bin -name "Packa*")
			if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
				for pf in $package_files ; do
					cp "$pf" "$top_dir/built/$target/$profile_name/"
				done
				for inf in $index_files ; do
					cp "$inf" "$top_dir/built/$target/$profile_name/"
				done
			fi
			
			if [ "$distribution" = "true" ] ; then
				mkdir -p "$top_dir/Distribution/Images/$target-$profile_name"
			fi

			#copy relevant images for which this profile applies
			profile_images=$(cat $targets_dir/$target/profiles/$profile_name/profile_images 2>/dev/null)
			for pi in $profile_images ; do
				candidates=$(ls "bin/$arch/"*"$pi"* 2>/dev/null | sed 's/^.*\///g')
				for c in $candidates ; do
					if [ ! -d "bin/$arch/$c" ] ; then
						newname=$(echo "$c" | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
						cp "bin/$arch/$c" "$top_dir/images/$target/$newname"
						if [ "$distribution" = "true" ] ; then
							cp "bin/$arch/$c" "$top_dir/Distribution/Images/$target-$profile_name/$newname"
						fi
					fi
				done
			done
			if [ "$distribution" = "true" ] ; then
				#Generate licenses file for each profile
				#Copy architecture independent packages & themes to Distribution folder
				echo "Generating Licenses file (expect it to take 5+ minutes)"
				"$top_dir/dev-utils/GenLicences.sh" "$target" "$profile_name" 1 2>&1
				distrib_copy_arch_ind_ipk "$target" "$translation_type"
			fi
		done

      
		#cd back to parent directory for next target (if there is one)
		cd "$top_dir"
	fi
done

