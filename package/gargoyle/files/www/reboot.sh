#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "reboot" -c "internal.css" -j "reboot.js"
?>

<script>
<?
	echo "var cronLine = \"\";"
	if [ -e /etc/crontabs/root ] ; then
		awk ' $0 ~ /usr\/lib\/gargoyle\/reboot.sh/ {print "cronLine=\""$0"\";"};' /etc/crontabs/root
	fi
?>
</script>

<fieldset>
	<legend class="sectionheader">Reboot</legend>
	<center><input type='button' value='Reboot Now' id="reboot_button" class="big_button" onclick='reboot()' /></center>

</fieldset>
<fieldset>
	<legend class="sectionheader">Scheduled Reboot</legend>
	<select class="leftcolumn" id="sched_reboot" onchange="setVisibility()">
		<option value="none">No Scheduled Reboot</option>
		<option value="scheduled">Reboot Scheduled</option>
	</select>
	<br/>

	<div id="schedule_reboot_container" class="indent">
		<div>
			<label class="narrowleftcolumn" for="reboot_interval">Router Will Reboot:</label>
			<select class="widerightcolumn" id="reboot_interval" onchange="setVisibility()">
				<option value="day">Every Day</option>
				<option value="week">Every Week</option>
				<option value="month">Every Month</option>
			</select>
		</div>

		<div id="reboot_day_container">
			<label class='narrowleftcolumn' id="reboot_day_label" for='reboot_day'>Reboot Day:</label>
			<select class="widerightcolumn" id='reboot_day' style='width:125px'></select>
		</div>

		<div id="reboot_hour_container">
			<label class="narrowleftcolumn" id="reboot_hour_label" for='reboot_hour'>Reboot Hour:</label>

			<select class="widerightcolumn" id='reboot_hour' style='width:125px'>
				<option value="0">12:00 AM</option>
				<option value="1">01:00 AM</option>
				<option value="2">02:00 AM</option>
				<option value="3">03:00 AM</option>
				<option value="4">04:00 AM</option>
				<option value="5">05:00 AM</option>
				<option value="6">06:00 AM</option>
				<option value="7">07:00 AM</option>
				<option value="8">08:00 AM</option>
				<option value="9">09:00 AM</option>
				<option value="10">10:00 AM</option>
				<option value="11">11:00 AM</option>
				<option value="12">12:00 PM</option>
				<option value="13">01:00 PM</option>
				<option value="14">02:00 PM</option>
				<option value="15">03:00 PM</option>
				<option value="16">04:00 PM</option>
				<option value="17">05:00 PM</option>
				<option value="18">06:00 PM</option>
				<option value="19">07:00 PM</option>
				<option value="20">08:00 PM</option>
				<option value="21">09:00 PM</option>
				<option value="22">10:00 PM</option>
				<option value="23">11:00 PM</option>
			</select>
		</div>
	</div>
</fieldset>
<div id="bottom_button_container">
	<input type='button' value='Save Changes' id="save_button" class="bottom_button"  onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button"  onclick='resetData()'/>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
	resetData();
</script>

<?
	gargoyle_header_footer -f -s "system" -p "reboot"
?>
