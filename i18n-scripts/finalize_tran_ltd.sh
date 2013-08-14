#!/bin/sh

#this is a vastly simplified script to finalize the config file for i18n builds only without python
#any i18n plugins are crudely commented out, and the language set in the zzz-plugin-gargoyle-i18n is used

target_dir="$1"
target_lang="$2"

echo "Using shell script to set i18n language"

touch "$target_dir/.config2"

awk -v lng="$target_lang" '{ lines[x++] = $0 } END { for (y=0; y<=x;) { if (match(lines[y],"CONFIG_PACKAGE_plugin-gargoyle-i18n")) { lines[y]="# "lines[y]; } print lines[y++]} print "CONFIG_PACKAGE_gargoyle-i18n=y"; printf ("CONFIG_PACKAGE_plugin-gargoyle-i18n-%s=y\n\n", lng)}' "$target_dir/.config" >> "$target_dir/.config2"

mv "$target_dir/.config2" "$target_dir/.config"
