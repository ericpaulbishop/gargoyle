#!/usr/bin/haserl --upload-limit=8192 --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -j "i18n.js firstboot.js table.js time.js" -z "firstboot.js i18n.js time.js" system gargoyle
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
					<span class="col-xs-7 col-md-8 col-lg-8"><input type="password" id="password2" class="form-control" size="25" /></span>
				</div>

				<p><strong><%~ Stz %>:</strong></p>

				<div class="row form-group">
					<span class="col-xs-12"><select class="form-control" id="timezone"></select></span>
					<br/>
				</div>

			</div>
		</div>
	</div>
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-body">
				<p><strong><%~ Sla %>:</strong></p>
				<div>
					<div id='lang_container'>
						<span>
							<img src="i18n/graphics/globe-and-flags.png"  width='80px' height='84px' />
						</span>
						<span style="display:block; clear:right; margin-right:100px" >
							<strong><em>Language / Lengua / Lingua / Langue / Język / Kieli / Sprache / Dil / γλώσσα / язык / زبان / שפה / لغة / भाषा / ภาษา / 언어 / 語</em></strong>
						</span>

						<div id="lang_table_container"></div>

						<form id="lfile_form" action="utility/do_fb_lang.sh" method="post" enctype="multipart/form-data" target="get_lfile" class="form-group">
							<div class="form-group">
								<span>
									<label><%~ ULngF %>:</label>
								</span>
								<span>
									<input id="lfile" type="file" name="lfile" />
								</span>
							</div>
							<div>
								<span>
									<button id="upload_lang_button" class="btn btn-default" onclick="do_get_lfile()">Upload ⇧</button>
								</span>
							</div>
							<input id="lfile_fname" type="hidden" name="fname" value="" />
							<input id="lfile_hash" type="hidden" name="hash" value="" />
						</form>
						<iframe id="get_lfile" style="display: none;" src="#" name="get_lfile"></iframe>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>
<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="setInitialSettings()"><%~ SSet %></button>
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

document.getElementById('password1').focus();

//-->
</script>

<script>
<!--

<%
	echo "var haveNet=0;"
	have_net=$(ping -q -w 1 -c 1 8.8.8.8 2>/dev/null | awk '/transmitted/ {print $4}') #Google's DNS servers; error nulled
	if [ "$have_net" == "1" ] ; then
		echo "haveNet=1;"
		gpkg update > /dev/null 2>&1
	fi

	lang_info=$(gpkg info -v 'Status,Description' -d plugin_root -o 'js' -r /^plugin-gargoyle-i18n-/)
	if [ -n "$lang_info" ] ; then
		printf "%s\n" "$lang_info"
	fi
%>

	resetFirstBootData();
//-->
</script>

<%
	gargoyle_header_footer -f
%>
