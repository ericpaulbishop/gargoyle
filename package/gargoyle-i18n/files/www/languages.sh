#!/usr/bin/haserl
<%
	# Copyright © 2013 BashfulBladder and Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "languages" -c "internal.css" -j "table.js i18n.js" -z "i18n.js" gargoyle
%>

<script>
<!--
<%
	echo "var langs = new Array();"
	lang_info=$(gpkg info -v 'Status,Description' -d plugin_root -o 'js' -r /^plugin-gargoyle-i18n-/)
	if [ -n "$lang_info" ] ; then
		printf "%s\n" "$lang_info" 
	fi
%>
//-->
</script>

<fieldset>
	<legend class="sectionheader"><%~ i18n.LMSect %></legend>
	
	<span class='narrowleftcolumn'>
		<img src="i18n/graphics/globe-and-flags.png"  width='80px' height='84px' />
	</span>
	<span class='widerightcolumn' style="display:block; clear:right; margin-right:100px" >
		<strong><em>Language / Lengua / Lingua / Langue / Język / Kieli / Sprache / Språk / Dil / γλώσσα / Язык / زبان / שפה / لغة / भाषा / ภาษา / 언어 / 語</em></strong>
	</span>
	<div id="lang_table_container"></div>
</fieldset>

<script>
<!--
	resetLanguagesData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "languages"
%>
