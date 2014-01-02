#!/bin/sh
# Copyright (C) 2013 BashfulBladder as part of i18n support for Gargoyle router firmware
# Version 04

#
#  get_i18_3rd_party_menuoption /www/i18n/universal/menu-system_languages.txt /www/i18n/Spanish-ES 
#
get_i18_3rd_party_menuoption() {
	local target_file="$1"
	local full_lang="$2"
	local iso_lang=$(echo "$full_lang" | sed -e 's/^\([^-]*-\)\{1\}//')
	local translation=""

	translation=$(awk -v lang="$iso_lang" -F '=' '$0 ~ lang { gsub(/\r$/,""); printf $2 }' $target_file)
	echo "$translation"
}

#
#  get_i18n_3rd_party_menuname gargoyle.display.connection_dhcp /www/i18n/Spanish-ES
#
get_i18n_3rd_party_menuname() {
	local uci_menu_item="$1"
	local full_lang="$2"
	local web_root=$(uci get gargoyle.global.web_root)
	local menu_filename=$(echo "$uci_menu_item" | awk -F '.' '{print "menu-"$3".txt"}')
	local fallback_lang=$(uci get gargoyle.global.fallback_lang)
	local translation=""

	if [ -e "$web_root/i18n/universal/$menu_filename" ] ; then
		translation=$(get_i18_3rd_party_menuoption "$web_root/i18n/universal/$menu_filename" "$full_lang")
		[ -z "$translation" ] && translation=$(get_i18_3rd_party_menuoption "$web_root/i18n/universal/$menu_filename" "$fallback_lang")
		[ -z "$translation" ] && translation=$(get_i18_3rd_party_menuoption "$web_root/i18n/universal/$menu_filename" "-fallback")
	fi
	echo "$translation"
}

#
#  get_i18n_menuname gargoyle.display.connection_dhcp /www/i18n/Spanish-ES
#  get_i18n_menuname gargoyle.display.system_languages /www/i18n/Arabic-AR
#
get_i18n_menuname() {
	local uci_menu_item="$1"
	local tgt_lang="$2"
	local uci_menu_var=$(echo "$uci_menu_item" | awk '{gsub(/\./, "_"); print $1}')
	local web_root=$(uci get gargoyle.global.web_root)
	local fallback_lang=$(uci get gargoyle.global.fallback_lang)
	local uci_trans=""
	local translation=""

	[ -e "$web_root/i18n/$fallback_lang/menus.txt" ] && . "$web_root/i18n/$fallback_lang/menus.txt"
	[ -e "$web_root/i18n/$tgt_lang/menus.txt" ] && . "$web_root/i18n/$tgt_lang/menus.txt"

	translation="$(eval echo \$$uci_menu_var)"

	[ -z "$translation" ] && translation=$(get_i18n_3rd_party_menuname "$uci_menu_item" "$tgt_lang")
	[ -z "$translation" ] && translation=$(get_i18n_3rd_party_menuname "$uci_menu_item" "$fallback_lang")

	translation=$(echo "$translation" | awk '{printf("%s", $0)}')

	echo "$translation"
}

#
#  change_menu_language plugin-gargoyle-i18n-Spanish-ES_1.0.0-1_all.ipk
#
change_menu_language() {
	local lang_ipk="$1"
	local new_lang=$(echo "$lang_ipk" | awk '{gsub(/\/.*\//,""); gsub("plugin-gargoyle-i18n-",""); gsub(/_[0-9]/, " "); print $1}')
	local uciName
	local uci_val
	local old_ifs=$IFS

	[ -z "$lang_ipk" ] && exit -1

	IFS=$(printf '\n\b')
	#set -x
	for uciMenu in $(uci show gargoyle.display 2>/dev/null | grep -v gargoyle.display=display); do
		uciName=$(echo "$uciMenu" | awk -F '=' '{print $1}')
		uci_val=$(get_i18n_menuname "$uciName" "$new_lang")
		[ -n "$uci_val" ] && uci set "$uciName=$uci_val"
	done
	#set +x
	IFS=$old_ifs
	uci set gargoyle.global.language=$new_lang
	uci commit
}

install_lang_pack() {
	local lang_file="$1"

	gpkg install "$lang_file"
	rm "$lang_file"
}

restart_lang_services() {
	local web_lang_menu=$(uci -q get gargoyle.system.languages)
	local web_root=$(uci get gargoyle.global.web_root)
	local installed_langs=$(find "$web_root/i18n/" -name "menus.txt"  | wc -l)
	local active_lang=$(uci -q get gargoyle.global.language)
	local fallback_lang=$(uci -q get gargoyle.global.fallback_lang)
	local translation

	if [ -z $web_lang_menu ] && [ $installed_langs -gt 1 ] ; then
		#uci set gargoyle.display.system_languages='Languages' #old way
		translation=$(get_i18n_menuname "gargoyle.display.system_languages" "$web_root/i18n/$active_lang")

		# only use translation if not "" - it was something before, so lets keep it
		# uci set gargoyle.display.system_languages="$translation"
		[ ! -z "$translation" ] && uci set gargoyle.display.system_languages="$translation"
		uci set gargoyle.scripts.system_languages='languages.sh'
		uci set gargoyle.system.languages='307'
		uci commit gargoyle
	fi
	if [ -n $web_lang_menu ] && [ $installed_langs -eq 1 ] ; then
		uci -q del gargoyle.system.languages
		uci commit gargoyle
	fi

	#if current language doesn't exist anymore switch back to default
	if [ ! -e "$web_root/i18n/$active_lang/menus.txt"  ] ; then
		if [ -e "$web_root/i18n/$fallback_lang/menus.txt" ] ; then
			#should always have fallback language, but check above to make sure
			uci set gargoyle.global.language="$fallback_lang"
			active_lang="$fallback_lang"
		fi
	fi
	change_menu_language "$active_lang"
}
