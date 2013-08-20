#!/usr/bin/haserl --upload-limit=8192 --upload-target=/tmp/ --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -c "internal.css" -j "i18n.js firstboot.js table.js plugins.js" -z "firstboot.js i18n.js" system 
%>

<script>
<!--
<%
	echo "var httpUserAgent = \"$HTTP_USER_AGENT\";"
	echo "var remoteAddr = \"$REMOTE_ADDR\";"

	echo "var timezoneLines = new Array();"
	if [ -e ./data/timezones.txt ] ; then
		awk '{gsub(/"/, "\\\""); print "timezoneLines.push(\""$0"\");"}' ./data/timezones.txt
	fi
	echo "var timezoneData = parseTimezones(timezoneLines);"
%>

//-->
</script>

<fieldset>
	<legend class="sectionheader"><%~ firstboot.ISSect %></legend>
	<p><strong><%~ npass %>:</strong></p>
	<div>
		<label class='leftcolumn' for='password1' id='password1_label'><%~ NPass %>:</label>
		<input type='password' class='rightcolumn' id='password1'  size='25' />
	</div>
	<div>
		<label class='leftcolumn' for='password2' id='password2_label'><%~ CPass %>:</label>
		<input type='password' class='rightcolumn' id='password2'  size='25' />
	</div>
	<p><strong><%~ Stz %>:</strong></p>
	<div>
		<select class='nocolumn' id='timezone'></select>
		<br/>
	</div>
	<br/>
	<div>
		<div id='lang_container' />
	</div>
	<input class="default_button" type="button" value="<%~ SSet %>" onclick="setInitialSettings()" />
</fieldset>

<script>
<!--
var timezoneList = timezoneData[0];
var timezoneDefinitions = timezoneData[2];

removeAllOptionsFromSelectElement(document.getElementById("timezone"));
var tzIndex=0;
for(tzIndex = 0; tzIndex < timezoneList.length; tzIndex++)
{
	var timezone = timezoneList[tzIndex];
	addOptionToSelectElement("timezone", timezone, timezoneDefinitions[timezone]);
}

var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
var currentTimezone = uciOriginal.get("system", systemSections[0], "timezone");
currentTimezone = currentTimezone == "UTC" ? "UTC0" : currentTimezone;
setSelectedValue("timezone", currentTimezone); //set value from config

document.getElementById('password1').focus();

//-->
</script>

<script>
<!--

<%
	echo "var HaveNet=0;"
	HaveNet=$(ping -q -w 1 -c 1 8.8.8.8 2>/dev/null | awk '/transmitted/ {print $4}') #Google's DNS servers; error nulled
	if [ "$HaveNet" == "1" ] ; then
		echo "HaveNet=1;"
		gpkg update > /dev/null 2>&1

		lang_info=$(gpkg info -v 'Status,Description' -d plugin_root -o 'js' -r /^plugin-gargoyle-i18n-/)
		#lang_info=$(gpkg info -v 'Status,Description' -d plugin_root -o 'js' -r /^plugin-gargoyle/)
		if [ -n "$lang_info" ] ; then
			printf "%s\n" "$lang_info" 
		fi
	fi
%>

	GenLangContainer();
//-->
</script>

<%
	gargoyle_header_footer -f
%>
