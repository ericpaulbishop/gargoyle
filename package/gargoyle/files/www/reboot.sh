#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "reboot" -c "internal.css" -j "reboot.js" -z "reboot.js"
%>

<script>
<%
	echo "var cronLine = \"\";"
	if [ -e /etc/crontabs/root ] ; then
		awk ' $0 ~ /usr\/lib\/gargoyle\/reboot.sh/ {print "cronLine=\""$0"\";"};' /etc/crontabs/root
	fi

	prAM=$(i18n pAM)
	prPM=$(i18n pPM)
	hrAM=$(i18n hAM)
	hrPM=$(i18n hPM)
	tfmt=$(uci -q get gargoyle.global.hour_style)
	tabs=""

	otime() {
		for h in $(seq 0 23); do
			tstr=""
			[ -z $tfmt ] || [ $tfmt = "12" ] && {
				hr=$(date -u -d $h:00 +"%I:%M")
				[ $h -lt 12 ] && {
					tstr="$prAM $hr $hrAM"
				} || {
					tstr="$prPM $hr $hrPM"
				}
			} || {
				tstr=$(date -u -d $h:00 +"%H:%M")
			}
			echo -e "$tabs<option value=\"$h\">$tstr</option>"
			[ $h = 0 ] && tabs="$1"
		done
	}
%>
</script>

<fieldset>
	<legend class="sectionheader"><%~ reboot.RbSect %></legend>
	<button id="reboot_button" class="btn btn-default btn-lg" onclick='reboot()'><%~ Reboot %></button
</fieldset>
<fieldset>
	<div class="form-group">
		<legend class="sectionheader"><%~ SchRb %></legend>
		<select class="leftcolumn form-control" id="sched_reboot" onchange="setVisibility()">
			<option value="none"><%~ NoSch %></option>
			<option value="scheduled"><%~ RbSch %></option>
			</select>
	</div>

	<div id="schedule_reboot_container" class="indent">
		<div class="form-group">
			<label class="narrowleftcolumn" for="reboot_interval"><%~ WillR %>:</label>
			<select class="widerightcolumn form-control" id="reboot_interval" onchange="setVisibility()">
				<option value="day"><%~ EDay %></option>
				<option value="week"><%~ EWek %></option>
				<option value="month"><%~ EMnh %></option>
			</select>
		</div>

		<div id="reboot_day_container" class="form-group">
			<label class='narrowleftcolumn' id="reboot_day_label" for='reboot_day'><%~ RDay %>:</label>
			<select class="widerightcolumn form-control" id='reboot_day' style='width:125px'></select>
		</div>

		<div id="reboot_hour_container" class="form-group">
			<label class="narrowleftcolumn" id="reboot_hour_label" for='reboot_hour'><%~ RHr %>:</label>
			<select class="widerightcolumn form-control" id='reboot_hour' style='width:125px'>
				<% otime '\t\t\t\t' %>
			</select>
		</div>
	</div>
</fieldset>
<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick='saveChanges()'><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick='resetData()'><%~ Reset %></button>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "system" -p "reboot"
%>
