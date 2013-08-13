#!/bin/sh

#this is a much more limited internationalization script for use in environments where python is not present
#this script will not handle localization AT ALL - localization is *completely* dependent upon python
#
#the scope of internationalization is limited to setting the language to the language present in the i18n plugin
#you could change it there - to change to another language - but its probably easier to install a python interpreter in $PATH

translation_type="$1"
requested_lang="$2"
target_lang=''
top_dir=$(pwd)

cp -r "$top_dir/package" "$top_dir/package-prepare"

[ "$translation_type" = "localize" ] && {
	echo "Error: python was not found. Please install python to run the localization script."
	exit 1
}

echo "Warning: python was not found."
echo "$top_dir"

[ ! -f "$top_dir/package/plugin-gargoyle-i18n/files/etc/uci-defaults/zzz-plugin-gargoyle-i18n" ] && {
	echo "Error: the internationalization plugin was not found."
	exit 1
}

target_lang=$(awk -F '=' '/uci set gargoyle.global.language/ {print $2}' "$top_dir/package/plugin-gargoyle-i18n/files/etc/uci-defaults/zzz-plugin-gargoyle-i18n")

echo "$target_lang"

[ ! -e "$top_dir/package/plugin-gargoyle-i18n-$target_lang/files/www/i18n/$target_lang/strings.js" ] && {
	echo "Error: the language $target_lang set in plugin-gargoyle-i18n is not available."
	exit 1
}

[ ! "$requested_lang" = "$target_lang" ] && {
	echo "Warning: the required $target_lang language is not set in plugin-gargoyle-i18n"
	echo "Warning: using the language that was found: $target_lang"
}

echo "$target_lang"
