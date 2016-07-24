#!/usr/bin/haserl
<%
	# This program is copyright © 2008,2009,2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "time" -c "internal.css" -j "time.js" -z "time.js" system gargoyle
%>
<script>
<!--
<%
	echo "var timezoneLines = new Array();"
	if [ -e ./data/timezones.txt ] ; then
		awk '{gsub(/"/, "\\\""); print "timezoneLines.push(\""$0"\");"}' ./data/timezones.txt
	fi
	echo "var timezoneData = parseTimezones(timezoneLines);"

	. /usr/lib/gargoyle/current_time.sh

	timeformat=$(uci -q get gargoyle.global.hour_style)
	echo "var timeformat=$([[ -z $timeformat ]] && echo 0 || echo $timeformat);"
%>
//-->
</script>

<h1 class="page-header"><%~ time.Section %></h1>
<div class="row">

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-body">
				<div class="form-group form-inline">
					<label id="current_time_label" for="current_time"><%~ CurrTime %>:&nbsp;&nbsp;&nbsp;&nbsp;</label>
					<span id="current_time"></span>
				</div>

				<div class="internal_divider"></div>

				<div class="form-group form-inline">
					<label id="timezone_label" for="timezone"><%~ TimeZone %>:</label>
				</div>

				<div class="form-group form-inline">
					<div>
						<select id="timezone" class="form-control" onchange="timezoneChanged()"></select>
					</div>
				</div>

				<div class="form-group form-inline">
					<label id="timezone_label" for="date_format"><%~ DateForm %>:</label>
					<select id="date_format" class="form-control">
						<option value="usa">mm/dd/yy</option>
						<option value="russia">dd.mm.yyyy</option>
						<option value="australia">dd/mm/yy</option>
						<option value="argentina">dd/mm/yyyy</option>
						<option value="iso">yyyy/mm/dd</option>
						<option value="iso8601">yyyy-mm-dd</option>
					</select>
				</div>

				<div class="form-group form-inline">
					<label id="timezone_label" for="time_format"><%~ TimeForm %>:</label>
					<select id="time_format" class="form-control">
						<option value=12>12 <%~ hour %></option>
						<option value=24>24 <%~ hour %></option>
					</select>
				</div>

				<div class="form-group form-inline">
					<label id="region_label" for="region"><%~ TServers %>:</label>
					<select id="region" class="form-control" onchange="updateServerList()">
						<option value="global"><%~ Global %></option>
						<option value="us"><%~ US %></option>
						<option value="north-america"><%~ NA %></option>
						<option value="south-america"><%~ SA %></option>
						<option value="europe"><%~ EU %></option>
						<option value="africa"><%~ Af %></option>
						<option value="asia"><%~ As %></option>
						<option value="oceania"><%~ Oc %></option>
						<option value="custom"><%~ Cust %></option>
					</select>
				</div>

				<div class="form-group form-inline">
					<div><input type="text" id="server1" class="form-control" size="35" /></div>
					<div><input type="text" id="server2" class="form-control" size="35" /></div>
					<div><input type="text" id="server3" class="form-control" size="35" /></div>
				</div>
			</div>

		</div>
	</div>

</div>
<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick="resetData()"><%~ Reset %></button>
</div>
<span id="update_container" ><%~ WaitSettings %></span>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "time"
%>
