#/bin/bash

filelist="/tmp/license_files.txt"
item=1

scr_path="${BASH_SOURCE[0]}"
gargoyle_path=$(dirname $(dirname "$scr_path"))

find "$gargoyle_path"/*-src/build_dir/target* -name "COPYING" -or -name "LICENSE" -or -name "README" -or -name "*.*" -and -not -type l | xargs grep -s -l "binary form " > "$filelist"

[[ ! -d "$gargoyle_path/LICENSES/packages" ]] && {
	mkdir "$gargoyle_path/LICENSES/packages"
}

while true; do
	[[ ! -e "$filelist" ]] && {
		break
	}
	
	aline=$(awk -v rec=$item 'NR==rec {print $0}' "$filelist")
	let item++
	
	[[ "$aline" == *busybox* ]] && {
		continue #various programs have been relicensed under the GLPv2; original license at tail of file
	}
	
	[[ -z "$aline" ]] && {
		break
	}
	
	echo "$aline"
	f_name=$(basename "$aline")
	f_ext=$(echo "$f_name" | awk -F'.' '{print $NF}')
	package=$(echo "$aline" | awk -F '/' '{for (i=1;i<=NF;i++) {if ($i ~ /-src/) {print $(i+3)}}}')
	f_bytes=$(ls -l "$aline" | awk '{print $5}')
	
	[[ ! -f "$gargoyle_path/LICENSES/packages/$package-license.txt" ]] && {
	
		[[ "$f_ext" == "$f_name" && $f_bytes -gt 50 ]] && {
			cp "$aline" "$gargoyle_path/LICENSES/packages/$package-license.txt"
		}
	
		[[ $f_ext == c* || $f_ext == h* ]] && {
			awk '{ lines[x++] = $0 } END { for (y=0; y<=x;) { print lines[y]; if (index(lines[y],"*\x2F") > 0 && y > 8) { break; } y++; } }' "$aline" > "$gargoyle_path/LICENSES/packages/$package-license.txt"
		}
	}
	
done
