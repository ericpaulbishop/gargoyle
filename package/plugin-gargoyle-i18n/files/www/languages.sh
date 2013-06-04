#!/usr/bin/haserl
<?

	# Copyright © 2011 Eric Bishop and Cezary Jackiewicz <cezary@eko.one.pl>
	# and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "languages" -c "internal.css" -j "table.js i18n.js" gargoyle
?>

<script>
<!--
<?
	echo "var langs = new Array();"
	webroot="$(uci get gargoyle.global.web_root 2>/dev/null)"
	for lang in "${webroot:-/www}/i18n"/*; do printf 'langs.push("%s");\n' "${lang##*/}"; done
?>
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader">Language manager</legend>
		<div class='indent'>
		<div id="lang_table_container"></div>
		</div>
	</fieldset>

	<span id="update_container" >Plase wait while new settings are applied...</span>
</form>

<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "system" -p "languages"
?>
