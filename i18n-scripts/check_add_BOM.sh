#!/bin/sh

if [ $# -eq 0 ] ; then
	echo "usage $0 check|add [language]"
	echo "\tcheck|add\tOnly check BOM or add BOM where missing"
	echo "\tlanguage\tOptional check specific language. Defaults to all"
	exit 0
fi

action="$1"
[ "$action" = "add" ] || action="check"

lang="$2"

scriptpath="$(readlink -f "$0")"
i18ndir="${scriptpath%/${0##*/}}"
packagedir="$(readlink -f "$i18ndir/../package")"

if [ -z "$lang" ] ; then
	lang="all"
elif [ -n "$lang" ] && [ "$lang" != "all" ] ; then
	langfolders="$(find $packagedir -type d -name plugin-gargoyle-i18n-$lang | head -n 1)"
	[ -z "$langfolders" ] && { echo "ERROR: No i18n plugin called $lang"; exit 0; }
fi

if [ "$lang" = "all" ] ; then
	langfolders="$(find $packagedir -type d -name plugin-gargoyle-i18n-*)"
fi

echo "$langfolders" | while read -r langfolder;
do
	langfiles="$(find "$langfolder" -type f -name *.js)"
	echo "$langfiles" | while read -r langfile;
	do
		filetype="$(file "$langfile" | cut -d: -f2)"
		bomfound="$(echo $filetype | grep '(with BOM)')"
		if [ -n "$bomfound" ] ; then
			#echo "INFO: $langfile already has BOM"
			:
		else
			echo "WARNING: $langfile missing BOM"
			[ "$action" = "add" ] && { echo "Adding BOM..."; sed -i '1s/^/\xef\xbb\xbf/' $langfile; }
		fi
	done
done

