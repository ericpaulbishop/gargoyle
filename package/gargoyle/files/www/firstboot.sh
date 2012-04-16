#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -c "internal.css" -j "firstboot.js" system 
?>

<script>
<!--
<?
	echo "var httpUserAgent = \"$HTTP_USER_AGENT\";"
	echo "var remoteAddr = \"$REMOTE_ADDR\";"

	
	echo "var timezoneLines = new Array();"
	if [ -e ./data/timezones.txt ] ; then
		awk '{gsub(/"/, "\\\""); print "timezoneLines.push(\""$0"\");"}' ./data/timezones.txt
	fi
	echo "var timezoneData = parseTimezones(timezoneLines);"
?>


//-->
</script>


<fieldset>
	<legend class="sectionheader">Initial Settings</legend>
	<p><strong>Enter a new password now:</strong></p>
	<div>
		<label class='leftcolumn' for='password1' id='password1_label'>New Password:</label>
		<input type='password' class='rightcolumn' id='password1'  size='25' />
	</div>
	<div>
		<label class='leftcolumn' for='password2' id='password2_label'>Confirm Password:</label>
		<input type='password' class='rightcolumn' id='password2'  size='25' />
	</div>
	<p><strong>Select your timezone:</strong></p>
	<div>
		<select class='nocolumn' id='timezone'></select>
		<br/>
	</div>
	<br/>
	<input class="default_button" type="button" value="Save Settings" onclick="setInitialSettings()" />
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

//-->
</script>


<?
	gargoyle_header_footer -f 
?>
