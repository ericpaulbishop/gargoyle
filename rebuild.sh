#!/bin/bash

set_constant_variables()
{
	#working directories
	scriptpath="$(readlink -f "$0")"
	top_dir="${scriptpath%/${0##*/}}"

	targets_dir="$top_dir/targets"
	patches_dir="$top_dir/patches-generic"
	compress_js_dir="$top_dir/compressed_javascript"
	compress_css_dir="$top_dir/compressed_css"
	
	node_version_tag="v10.19.0"
	npm_version_tag="v6.14.4"


	#script for building netfilter patches
	netfilter_patch_script="$top_dir/netfilter-match-modules/integrate_netfilter_modules.sh"
	
	#set date here, so it's guaranteed the same for all images
	#even though build can take several hours
	build_date=$(LC_ALL=C date +"%B %d, %Y")

	gargoyle_git_revision=$(git log -1 --pretty=format:%h )

}


set_version_variables()
{
	#set date here, so it is guaranteed the same for all images
	#even though build can take several hours
	build_date=$(LC_ALL=C date +"%B %d, %Y")

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

get_subtarget_from_config()
{
	config_file_path="$1"
	cat .config | grep "^CONFIG_TARGET_SUBTARGET" | sed 's/^.*="\(.*\)"/\1/g'
}

get_pkg_arch_from_config()
{
	config_file_path="$1"
	cat .config | grep "^CONFIG_TARGET_ARCH_PACKAGES" | sed 's/^.*="\(.*\)"/\1/g'
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
	local middle_line=$(printf "| %-26s| %-35s|" "Gargoyle revision $gargoyle_commit" "OpenWrt commit $openwrt_revision")
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
	num_terser_threads=1
	if [ "$num_build_threads" != "unspecified" ]; then
		num_terser_threads=$num_build_threads
	fi
	echo "Compressing/Mangling JavaScript files ($num_terser_threads threads)..."
	terser_arg1="$1"
	terser_arg2="$2"
	package_dir="$top_dir/package-prepare"
	rm -rf "$compress_js_dir"
	mkdir "$compress_js_dir"

	terser_batch=""
	cd "$package_dir"
	for jsf in $(find . -path '*.js' -a -not -path '*/www/i18n/*')
	do
		compress_jsf="$compress_js_dir/$jsf"
		terser_batch="$terser_batch $package_dir/$jsf -o $compress_jsf -c -m"
		mkdir -p "$(dirname "$compress_jsf")"
	done
	cd "$top_dir"

	echo "$terser_batch" | xargs -n 5 -P $num_terser_threads $terser_arg1 $terser_arg2
	cp -r "$compress_js_dir"/* "$package_dir"
	echo "Done!"
}

do_css_compress()
{
	num_uglifycss_threads=1
	if [ "$num_build_threads" != "unspecified" ]; then
		num_uglifycss_threads=$num_build_threads
	fi
	echo "Compressing CSS files ($num_uglifycss_threads threads)..."
	uglifycss_arg1="$1"
	uglifycss_arg2="$2"
	package_dir="$top_dir/package-prepare"
	rm -rf "$compress_css_dir"
	mkdir "$compress_css_dir"

	uglifycss_batch=""
	cd "$package_dir"
	for cssf in $(find . -path "*.css")
	do
		compress_cssf="$compress_css_dir/$cssf"
		uglifycss_batch="$uglifycss_batch --output $compress_cssf $package_dir/$cssf"
		mkdir -p "$(dirname "$compress_cssf")"
	done
	cd "$top_dir"

	echo "$uglifycss_batch" | xargs -n 3 -P $num_uglifycss_threads $uglifycss_arg1 $uglifycss_arg2
	cp -r "$compress_css_dir"/* "$package_dir"
	echo "Done!"
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
css_compress="$5"
specified_profile="$6"
translation_type="$7"
fallback_lang="$8"
active_lang="$9"
num_build_threads="${10}"
distribution="${11}"



num_build_thread_str=""
if [ "$num_build_threads" = "single" ] ; then
	num_build_threads="1"
	num_build_thread_str="-j1"
elif [ "$num_build_threads" = "" ] || [ "$num_build_threads" = "unspecified" ] ; then
	num_build_threads="unspecified"
	num_build_thread_str=""
elif [ "$num_build_threads" = "auto" ] ; then
	num_cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null)
	if [ -z "$num_cores" ] ; then num_cores=1 ; fi
	num_build_threads=$(($num_cores + 2)) # more threads than cores, since each thread will sometimes block for i/o
	num_build_thread_str="-j$num_build_threads"
elif [ "$num_build_threads" -lt 1 ] ; then
	num_build_threads="1"
	num_build_thread_str="-j1"
else
	num_build_thread_str="-j$num_build_threads"
fi





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



#compress javascript AND css
if [ "$js_compress" = "true" ] || [ "$js_compress" = "TRUE" ] || [ "$js_compress" = "1" ] ; then

	cd "$top_dir/minifiers/node_modules/.bin" 2>/dev/null

	npm_binary="npm"
	npm_test=$( "$npm_binary" -v 2>/dev/null )
	nodeglobal=$npm_test
	node_binary="node"
	node_version=$( "$node_binary" -v 2>/dev/null)
	if [ -n "$npm_test" ] && [ -z "$node_version" ] ; then
		node_binary="nodejs"
		node_version=$( "$node_binary" -v 2>/dev/null)
	fi
	if [ -z "$node_version" ] ; then
		npm_test=""
		nodeglobal="$npm_test"
	fi


	if [ -z "$npm_test" ] ; then
		echo ""
		echo "**************************************************************************"
		echo "** node/npm not installed globally! Attempting to build them            **"
		echo "**************************************************************************"
		echo ""
		
		#node
		if [ ! -e "$top_dir/node/bin/node" ] ; then
			rm -rf "$top_dir"/node
			cd "$top_dir"
			git clone git://github.com/nodejs/node.git
			cd node
			git checkout "$node_version_tag"
			./configure 
			make
			mkdir bin
			cp node bin/
			cd "$top_dir"
		fi
	
		PATH="$PATH:$top_dir/node/bin"
		export PATH

		#npm
		if [ ! -e "$top_dir/node/bin/npm" ] ; then
			rm -rf "$top_dir"/npm
			cd "$top_dir"
			git clone git://github.com/npm/cli.git "$top_dir/npm"
			cd npm
			git checkout "$npm_version_tag"
			npm_binary="$node_binary $top_dir/npm/bin/npm-cli.js"
		fi

		npm_test=$( npm -v 2>/dev/null )


	else
		echo "node/npm ok!"
	fi

	if [  -z "npm_test" ] ; then
		echo ""
		echo "**************************************************************************"
		echo "**  WARNING: node/npm could not be installed, cannot compress css/js    **"
		echo "**************************************************************************"
		echo ""

	else
	
		terser_bin="$top_dir/minifiers/node_modules/.bin/terser"
		if [ ! -e "$terser_bin" ] ; then
			echo ""
			echo "**************************************************************************"
			echo "**  Terser is not installed, attempting to install from npm             **"
			echo "**************************************************************************"
			echo ""
			
			mkdir -p "$top_dir/minifiers/node_modules/.bin"
	
			cd "$top_dir"
			${npm_binary} install terser --prefix minifiers > /dev/null 2>&1
		else
			echo "terser ok!"
		fi
		cd "$top_dir/minifiers/node_modules/.bin"
		uglify_test=$( echo 'var abc = 1;' | ${nodeglobal:+$node_binary} "$terser_bin"  2>/dev/null )
		if [ "$uglify_test" = 'var abc=1' ] ||  [ "$uglify_test" = 'var abc=1;' ]  ; then
			js_compress="true"
			do_js_compress ${nodeglobal:+"$node_binary"} "$terser_bin"
		else
			js_compress="false"
			echo ""
			echo "**************************************************************************"
			echo "** WARNING: Cannot compress JavaScript, terser could not be installed   **"
			echo "**************************************************************************"
			echo ""
		fi
		if [ "$css_compress" = "true" ] || [ "$css_compress" = "TRUE" ] || [ "$css_compress" = "1" ] ; then
			uglifycss_bin="$top_dir/minifiers/node_modules/.bin/uglifycss"
			if [ ! -e "$uglifycss_bin" ] ; then
				echo ""
				echo "**************************************************************************"
				echo "**  UglifyCSS is not installed, attempting to install from npm          **"
				echo "**************************************************************************"
				echo ""
	
				cd "$top_dir"
				${npm_binary} install uglifycss -q --prefix minifiers > /dev/null 2>&1
			else
				echo "uglifycss ok!"
			fi

			cd "$top_dir/minifiers/node_modules/.bin"
			uglify_test=$( echo -e '#test {\nabc: 1;\ndef: 1;\n}' | ${nodeglobal:+$node_binary} "$uglifycss_bin"  2>/dev/null )
			if [ "$uglify_test" = '#test{abc:1;def:1}' ] ; then
				css_compress="true"
				do_css_compress  ${nodeglobal:+"$node_binary"} "$uglifycss_bin"
			else
				css_compress="false"
				echo ""
				echo "**************************************************************************"
				echo "** WARNING: Cannot compress css, uglifycss could not be installed       **"
				echo "**************************************************************************"
				echo ""
			fi
		fi
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
			IFS_ORIG="$IFS"
			IFS_LINEBREAK="$(printf '\n\r')"
			IFS="$IFS_LINEBREAK"
			matching_packages=$(find "$target-src/package" -name "$gp")
			for mp in $matching_packages ; do
				if [ -d "$mp" ] && [ -e "$mp/Makefile" ] ; then
					rm -rf "$mp" 
				fi
			done
			IFS="$IFS_ORIG"
			cp -pr "$package_dir/$gp" "$target-src/package"
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
		echo "                 (with $num_build_threads build threads)"
		echo "**************************************************************************"
		echo ""
		echo ""

	
		#copy this target configuration to build directory
		cp "$targets_dir/$target/profiles/$default_profile/config" "$top_dir/${target}-src/.config"
		#clean out old bin folder to prevent contamination between profiles
		arch=$(ls "$top_dir/${target}-src/bin/targets")
		profile_images=$(cat "$targets_dir/$target/profiles/$profile_name/profile_images" 2>/dev/null)
		for pi in $profile_images ; do
			escaped_pi=$(echo $pi | sed 's/-/\\-/g')
			candidates=$(find "$top_dir/${target}-src/bin/targets/$arch/" 2>/dev/null | grep "$escaped_pi" )
			for c in $candidates ; do
				if [ ! -d "$c" ] ; then
					rm "$c"
				fi
			done
		done
		
		
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
		openwrt_commit=$(cat "OPENWRT_REVISION")
		openwrt_abbrev_commit=$( echo "$openwrt_commit" | cut -b 1-7 )

	
		#if version name specified, set gargoyle official version parameter in gargoyle package
		echo "OFFICIAL_VERSION:=$full_gargoyle_version" > .ver
		cat .ver "$package_dir/gargoyle/Makefile" >.vermake
		rm .ver
		mv .vermake "$top_dir/$target-src/package/gargoyle/Makefile"
		
		#build, if verbosity is 0 dump most output to /dev/null, otherwise dump everything
		openwrt_target=$(get_target_from_config "./.config")
		create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "${openwrt_abbrev_commit}" "package/base-files/files/etc/banner" "."
		if [ "$verbosity" = "0" ] ; then
			make $num_build_thread_str GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$default_profile"

		else
			make $num_build_thread_str V=99 GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$default_profile"
		fi
		
		if [ "$distribution" = "true" ] || [ "$distribution" = "TRUE" ] || [ "$distribution" = "1" ] ; then
			distribution="true"
			distrib_init
			mkdir -p "$top_dir/Distribution/Images/$target-$default_profile"
		fi



		#copy packages to build/target directory
		pkg_arch=$(get_pkg_arch_from_config "./.config")
		mkdir -p "$top_dir/built/$target/$profile_name"
		package_base_dir=$(find "bin/packages/$pkg_arch" -name "base")
		package_files=$(find "$package_base_dir" -name "*.ipk")
		index_files=$(find "$package_base_dir" -name "Packa*")
		if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
			for pf in $package_files ; do
				cp "$pf" "$top_dir/built/$target/$profile_name/"
			done
			for inf in $index_files ; do
				cp "$inf" "$top_dir/built/$target/$profile_name/"
			done
		fi
		#copy build specific packages to build/target specific directory
		openwrt_target=$(get_target_from_config "./.config")
		subtarget_arch=$(get_subtarget_from_config "./.config")
		mkdir -p "$top_dir/built/$target/$profile_name"_kernelspecific
		package_base_dir=$(find "bin/targets/$openwrt_target/$subtarget_arch" -name "packages")
		package_files=$(find "$package_base_dir" -name "*.ipk")
		index_files=$(find "$package_base_dir" -name "Packa*")
		if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
			for pf in $package_files ; do
				cp "$pf" "$top_dir/built/$target/$profile_name"_kernelspecific/
			done
			for inf in $index_files ; do
				cp "$inf" "$top_dir/built/$target/$profile_name"_kernelspecific/
			done
		fi
	
		#copy images to images/target directory
		mkdir -p "$top_dir/images/$target"
		arch=$(ls bin/targets)
		image_files=$(find "bin/targets/$arch/$subtarget_arch" 2>/dev/null)
		if [ ! -e "$targets_dir/$target/profiles/$default_profile/profile_images"  ]  ; then 
			for imf in $image_files ; do
				if [ ! -d "$imf" ] ; then
					newname=$(echo "$imf" | sed 's/^.*\///g' | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
					cp "$imf" "$top_dir/images/$target/$newname"
					if [ "$distribution" = "true" ] ; then
						cp "$imf" "$top_dir/Distribution/Images/$target-$default_profile/$newname"
					fi
				fi
			done
		else
			profile_images=$(cat "$targets_dir/$target/profiles/$default_profile/profile_images" 2>/dev/null)
			for pi in $profile_images ; do
				escaped_pi=$(echo $pi | sed 's/-/\\-/g')
				candidates=$(find "bin/targets/$arch/$subtarget_arch/" 2>/dev/null | grep "$escaped_pi" )
				for c in $candidates ; do
					if [ ! -d "$c" ] ; then
						newname=$(echo "$c" | sed 's/^.*\///g' | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
						cp "$c" "$top_dir/images/$target/$newname"
						if [ "$distribution" = "true" ] ; then
							cp "$c" "$top_dir/Distribution/Images/$target-$default_profile/$newname"
						fi
					fi
				done
			done
		fi

		#copy build config info
		if [ -e "bin/targets/$openwrt_target/$subtarget_arch/config.buildinfo" ] ; then
			cp "bin/targets/$openwrt_target/$subtarget_arch/config.buildinfo" "$top_dir/images/$target/$target-$default_profile.buildinfo"
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
			#clean out old bin folder to prevent contamination between profiles
			arch=$(ls bin/targets)
			profile_images=$(cat "$targets_dir/$target/profiles/$profile_name/profile_images" 2>/dev/null)
			for pi in $profile_images ; do
				escaped_pi=$(echo $pi | sed 's/-/\\-/g')
				candidates=$(find "bin/targets/$arch/" 2>/dev/null | grep "$escaped_pi" )
				for c in $candidates ; do
					if [ ! -d "$c" ] ; then
						rm "$c"
					fi
				done
			done
			
			
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
			create_gargoyle_banner "$openwrt_target" "$profile_name" "$build_date" "$short_gargoyle_version" "$gargoyle_git_revision" "$branch_name" "$openwrt_abbrev_commit" "package/base-files/files/etc/banner" "."

			echo ""
			echo ""	
			echo "**************************************************************************"
			echo "        Gargoyle is now rebuilding target: $target / $profile_name"
			echo "                 (with $num_build_threads build threads)"
			echo "**************************************************************************"
			echo ""
			echo ""



			if [ "$verbosity" = "0" ] ; then
				make $num_build_thread_str GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$profile_name"

			else
				make $num_build_thread_str V=99 GARGOYLE_VERSION="$numeric_gargoyle_version" GARGOYLE_VERSION_NAME="$lower_short_gargoyle_version" GARGOYLE_PROFILE="$profile_name"
			fi


			#if we didn't build anything, die horribly
			image_files=$(ls "bin/targets/$arch/" 2>/dev/null)
			if [ -z "$image_files" ] ; then
				exit
			fi

			#copy packages to build/target directory
			pkg_arch=$(get_pkg_arch_from_config "./.config")
			mkdir -p "$top_dir/built/$target/$profile_name"
			package_base_dir=$(find "bin/packages/$pkg_arch" -name "base")
			package_files=$(find "$package_base_dir" -name "*.ipk")
			index_files=$(find "$package_base_dir" -name "Packa*")
			if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
				for pf in $package_files ; do
					cp "$pf" "$top_dir/built/$target/$profile_name/"
				done
				for inf in $index_files ; do
					cp "$inf" "$top_dir/built/$target/$profile_name/"
				done
			fi
			#copy build specific packages to build/target specific directory
			openwrt_target=$(get_target_from_config "./.config")
			subtarget_arch=$(get_subtarget_from_config "./.config")
			mkdir -p "$top_dir/built/$target/$profile_name"_kernelspecific
			package_base_dir=$(find "bin/targets/$openwrt_target/$subtarget_arch" -name "packages")
			package_files=$(find "$package_base_dir" -name "*.ipk")
			index_files=$(find "$package_base_dir" -name "Packa*")
			if [ -n "$package_files" ] && [ -n "$index_files" ] ; then
				for pf in $package_files ; do
					cp "$pf" "$top_dir/built/$target/$profile_name"_kernelspecific/
				done
				for inf in $index_files ; do
					cp "$inf" "$top_dir/built/$target/$profile_name"_kernelspecific/
				done
			fi
			
			if [ "$distribution" = "true" ] ; then
				mkdir -p "$top_dir/Distribution/Images/$target-$profile_name"
			fi

			#copy relevant images for which this profile applies
			arch=$(ls bin/targets)
			profile_images=$(cat $targets_dir/$target/profiles/$profile_name/profile_images 2>/dev/null)
			for pi in $profile_images ; do
				escaped_pi=$(echo $pi | sed 's/-/\\-/g')
				candidates=$(find "bin/targets/$arch/$subtarget_arch/" 2>/dev/null | grep "$escaped_pi" )
				for c in $candidates ; do
					if [ ! -d "$c" ] ; then
						newname=$(echo "$c" | sed 's/^.*\///g' | sed "s/openwrt/gargoyle_$lower_short_gargoyle_version/g")
						cp "$c" "$top_dir/images/$target/$newname"
						if [ "$distribution" = "true" ] ; then
							cp "$c" "$top_dir/Distribution/Images/$target-$default_profile/$newname"
						fi
					fi
				done

			done

			#copy build config info
			if [ -e "bin/targets/$openwrt_target/$subtarget_arch/config.buildinfo" ] ; then
				cp "bin/targets/$openwrt_target/$subtarget_arch/config.buildinfo" "$top_dir/images/$target/$target-$profile_name.buildinfo"
			fi

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

