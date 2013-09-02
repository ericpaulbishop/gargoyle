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
%>
</script>

<fieldset>
	<legend class="sectionheader"><%~ reboot.RbSect %></legend>
	<center><input type='button' value='<%~ Reboot %>' id="reboot_button" class="big_button" onclick='reboot()' /></center>

</fieldset>
<fieldset>
	<legend class="sectionheader"><%~ SchRb %></legend>
	<select class="leftcolumn" id="sched_reboot" onchange="setVisibility()">
		<option value="none"><%~ NoSch %></option>
		<option value="scheduled"><%~ RbSch %></option>
	</select>
	<br/>

	<div id="schedule_reboot_container" class="indent">
		<div>
			<label class="narrowleftcolumn" for="reboot_interval"><%~ WillR %>:</label>
			<select class="widerightcolumn" id="reboot_interval" onchange="setVisibility()">
				<option value="day"><%~ EDay %></option>
				<option value="week"><%~ EWek %></option>
				<option value="month"><%~ EMnh %></option>
			</select>
		</div>

		<div id="reboot_day_container">
			<label class='narrowleftcolumn' id="reboot_day_label" for='reboot_day'><%~ RDay %>:</label>
			<select class="widerightcolumn" id='reboot_day' style='width:125px'></select>
		</div>

		<div id="reboot_hour_container">
			<label class="narrowleftcolumn" id="reboot_hour_label" for='reboot_hour'><%~ RHr %>:</label>

			<select class="widerightcolumn" id='reboot_hour' style='width:125px'>
				<option value="0"><%~ twelveam %></option>
				<option value="1"><%~ oneam %></option>
				<option value="2"><%~ twoam %></option>
				<option value="3"><%~ threeam %></option>
				<option value="4"><%~ fouram %></option>
				<option value="5"><%~ fiveam %></option>
				<option value="6"><%~ sixam %></option>
				<option value="7"><%~ sevenam %></option>
				<option value="8"><%~ eightam %></option>
				<option value="9"><%~ nineam %></option>
				<option value="10"><%~ tenam %></option>
				<option value="11"><%~ elevenam %></option>
				<option value="12"><%~ twelvepm %></option>
				<option value="13"><%~ onepm %></option>
				<option value="14"><%~ twopm %></option>
				<option value="15"><%~ threepm %></option>
				<option value="16"><%~ fourpm %></option>
				<option value="17"><%~ fivepm %></option>
				<option value="18"><%~ sixpm %></option>
				<option value="19"><%~ sevenpm %></option>
				<option value="20"><%~ eightpm %></option>
				<option value="21"><%~ ninepm %></option>
				<option value="22"><%~ tenpm %></option>
				<option value="23"><%~ elevenpm %></option>
			</select>
		</div>
	</div>
</fieldset>
<div id="bottom_button_container">
	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button"  onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button"  onclick='resetData()'/>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "system" -p "reboot"
%>
