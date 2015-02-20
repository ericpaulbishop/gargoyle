#/bin/bash

# $1 is the platform
# $2 is the profile
# $3 [for distribution type builds] output directory

# creates a massive 500k+ HTML file of (basic) licensing, copyright & binary distribution info for packages used in the profile

gargoyle_path="${BASH_SOURCE[0]%/*/*}"
folders=(linux-*_generic target* toolchain*)
completed_packages=()
openwrt_GPL_pkgs=(base-files gpio-button-hotplug lzma-loader root* swconfig)
skip_dirs=(tmp packages squashfs-64k stamp toolchain)
binary_distribution_search=1
redistribution_clause_limit=10
copyright_limit=10
outFile="$gargoyle_path/images/$1/Licenses $1-$2.html"

[[ -n "$3" ]] && {
	outFile="$gargoyle_path/Distribution/LICENSES/Licenses $1-$2.html"
}


ScrapeLine() {
	rslt=
	GPL_idx=$(echo "$1" | awk '{for (i=1;i<=NF;i++) {if ($i ~ /GNU/) {print i}}}')
	[[ -n "$GPL_idx" ]] && {
		rslt=$(echo "$1" | awk -v g_idx=$GPL_idx '{for (i=g_idx;i<=NF;i++) printf("%s%s", $i, i==NF?"\n":" ") }')		
	}
	echo "$rslt"
}

ScrapeGPL() {
	GPLtype=$(grep -s -m 1 -i "GENERAL PUBLIC LICENSE" "$1")
	echo "$GPLtype"
}
ScrapgeGPL_Version() {
	GPL_ver=$(grep -s -i "version" "$1" | grep -v valid | grep -m 1 -v this | awk  'BEGIN{FS=""} {for (i=1;i<=NF;i++) {if ($i ~ /[0-9]/) {print $i; exit}}}')
	echo "$GPL_ver"
}

SimpleLicenseScrape() {
	lsc=
	source_license=$(grep -s -l -m 1 "License" -r "$1")
	r=1
	while true; do
		afile=$(echo "$source_license" | awk -v rec=$r 'NR==rec {print $0}')
		[[ -z "$afile" ]] && break 
		rslt=$(grep -m 1 "License" "$afile")
		lsc=$(ScrapeLine "$rslt")
		[[ -n "$lsc" ]] && {
			break
		}
		
		let r++
	done
	echo "$lsc"
}

IncludedScrape() {
	ret_val=
	suppl_file="COPYING"
	[[ "$2" == lua* ]] && suppl_file="COPYRIGHT"
	
	ifiles=$(find "$1" -name "$suppl_file" -or -name "LICENSE" -or -name "COPYRIGHT" -or -name "*.*" -and -not -name "*.o" -print0 | xargs -0 grep -s -l -i "Copyright")
	r=1
	while true; do
		ifile=$(echo "$ifiles" | awk -v rec=$r 'NR==rec {print $0}')
		fname=$(basename "$ifile")
		[[ "$fname" == "COPYING" ]] || [[ "$fname" == "LICENSE" ]] || [[ "$fname" == "COPYRIGHT" ]] && {
			ret_val=$(cat "$ifile")
			break
		}
		[[ -z "$ifile" ]] && break
		[[ -f "$ifile" ]] && {
			initial_lines=$(awk '/^$/{exit} {print}' "$ifile" | grep -v '#include')
			[[ -n "$initial_lines" ]] && {
				ret_val="$initial_lines"
				break
			}
		}
		let r++
	done
	echo "$ret_val"
}

FindLicenseFile() {
	lfiles=$(find "$1" -name "COPYING" -or -name "LICENSE" -print0 | xargs -0 grep -s -l -i "license")
	lfound=0
	r=1
	
	while true; do
		aline=$(echo "$lfiles" | awk -v rec=$r 'NR==rec {print $0}')
		[[ -z "$aline" ]] && break 
		[[ -f "$aline" ]] && {
			[[ "$2" == e2fsprogs* ]] && {
				lheader=$(awk '/^\t\t\t/{exit} {print}' "$1/COPYING" | awk 1 ORS='</br>\n')
				echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
				echo "$lheader" >> "$outFile"
				lfound=1
				break
			}
			[[ "$2" == openvpn* ]] && {
				lheader=$(awk 1 ORS='</br>\n' "$1/COPYING")
				echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
				echo "$lheader" >> "$outFile"
				lfound=1
				break
			}
			lsc=$(ScrapeGPL "$aline")
			[[ -n "$lsc" ]] && {
				echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
				echo $(ScrapeGPL "$aline") $(ScrapgeGPL_Version "$aline")"</br>" >> "$outFile"
				lfound=1
				break
			}
			[[ -z "$lsc" ]] && {				
				buried_license=$(IncludedScrape "$1" "$2")
				[[ -n "$buried_license" ]] && {
					echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
					cprt=$(echo "$buried_license" | awk 1 ORS='</br>\n')
					echo "<plain>$cprt</plain" >> "$outFile"
					lfound=1
				}
			}
		}
		let r++
	done
	
	for Openwrt_pkg in "${openwrt_GPL_pkgs[@]}"; do
		[[ "$2" == "$Openwrt_pkg" ]] || [[ "$2" == $Openwrt_pkg ]] && {
			echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
			echo "<h2>OpenWrt GPL v2</h2>" >> "$outFile"
			lfound=1
			break
		}
	done
	
	#lets just say that all plugin_gargoyle_* are GPL
	apkg=$(echo "$2" | awk -F '[_-]' '{print $1$2}')
	[[ "$apkg" == "plugingargoyle" ]] || [[ "$apkg" == "shareusers" ]] || [[ "$apkg" == "webmongargoyle" ]] || [[ "$apkg" == "bwmongargoyle" ]] || [[ "$apkg" == "qosgargoyle" ]]&& {
		echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
		echo "<h2>Gargoyle GPL v2</h2>" >> "$outFile"
		lfound=1	
	}
	
	#easily found COPYING or LICENSE file not found...
	[[ $lfound == 0 ]] && {
		discovered_lsc=$(SimpleLicenseScrape "$1")
		[[ -n "$discovered_lsc" ]] && {
			echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
			echo "$discovered_lsc</br>" >> "$outFile"
			lfound=1
		} || {
			buried_license=$(IncludedScrape "$1" "$2")
			[[ -n "$buried_license" ]] && {
				echo "<h1>Package <strong>$2</strong></h1>" >> "$outFile"
				cprt=$(echo "$buried_license" | awk 1 ORS='</br>\n')
				echo "<plain>$cprt</plain" >> "$outFile"
				lfound=1
			} || {
				echo "Skipping package $2"
				echo "<h2>Skipping package $2</h2>" >> "$outFile"
				lfound=1
			}
		}
	}
}

FindBinaryClause() {
	cfound=0
	c=1
	[[ "$2" == busybox* ]] && {
		return #various programs have been relicensed under the GLPv2; original license at tail of file
	}
	[[ "$2" == libncurses* ]] && {
		return #hackers guide will be a false positive
	}
	cfiles=$(find "$1" -name "COPYING" -or -name "LICENSE" -or -name "README" -or -name "*.*" -and -not -type l -print0 | xargs -0 grep -s -l "binary form ")
	while true; do
		cline=$(echo "$cfiles" | awk -v rec=$c 'NR==rec {print $0}')
		[[ -z "$cline" ]] && break
		[[ -f "$cline" ]] && {
			f_name=$(basename "$cline")
			f_ext=$(echo "$f_name" | awk -F'.' '{print $NF}')
			
			[[ $f_ext == c* || $f_ext == h* ]] && {
				clause=$(awk '{ lines[x++] = $0 } END { for (y=0; y<=x;) { print lines[y]; if (index(lines[y],"*\x2F") > 0 && y > 8) { break; } y++; } }' "$cline")
				[[ -n "$clause" ]] && {
					cls=$(echo "$clause" | awk 1 ORS='</br>\n')
					echo "$cls" >> "$outFile"
				}
			}
		}
		let c++
		[[ $c -gt $redistribution_clause_limit ]] && {
   			echo "<strong><emphasis>(only $redistribution_clause_limit redistribution clauses shown)</emphasis></strong></br>" >> "$outFile"
   			break
   		}
	done	
}

ScrapeCopyright() {
	cr=1
	cfiles=$(find "$1" -name "*.*" -and -not -type l -and -not -name ".o" -print0 | xargs -0 grep -s -l "Copyright")
	while true; do
		crfile=$(echo "$cfiles" | awk -v rec=$cr 'NR==rec {print $0}')
   		[[ -z "$crfile" ]] && break
   		[[ -f "$crfile" ]] && {
   			cval=$(grep -s -m 1 --binary-files=without-match "Copyright" -F "$crfile")
   			echo "<h4>$cval</h4>" >> "$outFile"
   			let cr++
   			[[ $cr -gt $copyright_limit ]] && {
   				echo "<strong><emphasis>(only $copyright_limit copyright notices shown)</emphasis></strong></br>" >> "$outFile"
   				break
   			}
   		}
	done
}

#---------------------------


echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<title>Gargoyle licenses, copyright & binary distribution info for '$1'-'$2'</title>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<style type="text/css">
<!-- 
body { margin-left:15px; margin-right:0px; margin-top:0px; background-color:#fff }
emphasis { font-style:italic; margin-top:0px; margin-bottom:0.1px }
h1 { font-weight:bold; font-size:24px }
h2 { font-weight:bold; font-size:16px }
h3 { font-weight:bold; font-size:14px }
h4 { font-weight:normal; font-style:italic; font-size:11px }
plain { font-weight:normal; font-size:12px }
strong { font-weight:bold }
-->
</style>
</head>
<body>' > "$outFile"


build_dirs=$(find "$gargoyle_path/$1-src/build_dir" -maxdepth 1 -type d -and -not -name "host" -and -not -name "build_dir" | sort)
bf=1
while true; do
	bdir=$(echo "$build_dirs" | awk -v rec=$bf 'NR==rec {print $0}')
	[[ -z "$bdir" ]] && break
   	[[ -d "$bdir" ]] && {
		pkg_folders=$(find "$bdir" -maxdepth 1 -type d | sort)
		pkg=1
		while true; do
			pkgdir=$(echo "$pkg_folders" | awk -v rec=$pkg 'NR==rec {print $0}')
			[[ -z "$pkgdir" ]] && break
			[[ "$pkgdir" != "$bdir" ]] && [[ ! -L "$pkgdir" ]] && {
				fldr=$(echo "$pkgdir" | awk -F'/' '{print $(NF)}')
				skip_pkg=0
				for skp in "${skip_dirs[@]}"; do
					[[ "$fldr" = "$skp" ]] && {
						skip_pkg=1
						break
					}
				done
				bare_pkg_name=$(echo "$fldr" | sed 's/[-_ .][rv0-9].*//g')
				[[ $(basename "$bdir") == target-* ]] && {
					#weed out the =y packages from the =m packages that are built but not included in the image
					found_pkg=0
					[[ $(grep -m 1 "^CONFIG_PACKAGE_$bare_pkg_name" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }') != 'y' ]] && {
						[[ $(grep -m 1 "^CONFIG_PACKAGE_lib$bare_pkg_name" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }') = 'y' ]] && found_pkg=1
						
						[[ $(grep -m 1 "^CONFIG_PACKAGE_$bare_pkg_name" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }') = 'm' ]] && skip_pkg=1
						
						[[ -f "$gargoyle_path/$1-src/package/$bare_pkg_name/Makefile" ]] || {
							bare_pkg_base=$(echo "$bare_pkg_name" | sed 's/_/-/g')
							[[ -e "$gargoyle_path/$1-src/package/$bare_pkg_base" ]] && {
								[[ $(grep -m 1 "^CONFIG_PACKAGE_$bare_pkg_base" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }') = 'y' ]] && {
									found_pkg=1
								}
							}
						}
					} || {
						[[ $(grep -m 1 "^CONFIG_PACKAGE_$bare_pkg_name" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }') = 'y' ]] && {
							found_pkg=1
						}
					}
					
					[[ $found_pkg != 1 ]] && skip_pkg=1 || skip_pkg=0
					
				} || {
					[[ $(grep -m 1 "$bare_pkg_name" "$gargoyle_path/$1-src/.config") == \#* ]] && {
						skip_pkg=1
						[[ "$bare_pkg_name" = 'iw' ]] && skip_pkg=0
						[[ "$bare_pkg_name" = 'uci' ]] && skip_pkg=0
					} || {
						pkg_built_in=$(grep -m 1 "$bare_pkg_name" "$gargoyle_path/$1-src/.config" | awk -F'=' '{ print $2 }')
					}
					[[ "$pkg_built_in" = 'm' ]] || [[ "$fldr" ==  plugin_gargoyle* ]] && skip_pkg=1
				}
				
				[[ $skip_pkg = 0 ]] && {
					echo "Processing package: $fldr"
					FindLicenseFile "$pkgdir" "$fldr"
					
					ScrapeCopyright "$pkgdir"
					
					[[ $binary_distribution_search == 1 ]] && {
						FindBinaryClause "$pkgdir" "$fldr"
					}
				}
			}			
			let pkg++
		done
	}
	let bf++
done

echo '
</body>
</html>' >> "$outFile"
