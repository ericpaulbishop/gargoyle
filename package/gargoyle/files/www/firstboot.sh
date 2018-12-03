#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -j "firstboot.js time.js" -z "firstboot.js time.js" system
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

<h1 class="page-header"><%~ firstboot.ISSect %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-body">
				<p><strong><%~ npass %>:</strong></p>

				<div class="row form-group">
					<label class="col-xs-4 col-md-3 col-lg-2" for="password1" id="password1_label"><%~ NPass %>:</label>
					<span class="col-xs-7 col-md-8 col-lg-8"><input type="password" id="password1" class="form-control" size="25" /></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-4 col-md-3 col-lg-2" for="password2" id="password2_label"><%~ CPass %>:</label>
					<span class="col-xs-7 col-md3 col-lg-2"><input type="password" id="password2" class="form-control" size="25" /></span>
				</div>

				<p><strong><%~ Stz %>:</strong></p>

				<div class="row form-group">
					<span class="col-xs-12"><select class="form-control" id="timezone"></select></span>
					<br/>
				</div>

				<br/>
				<button class="btn btn-primary btn-lg" onclick="setInitialSettings()"><%~ SSet %></button>
			</div>
		</div>
	</div>
</div>

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

document.getElementById("password1").focus();

//-->
</script>

<%
	gargoyle_header_footer -f
%>

