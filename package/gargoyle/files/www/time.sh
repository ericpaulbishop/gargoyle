#!/usr/bin/haserl
<?
	# This program is copyright © 2008,2009 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -h -s "system" -p "time" -c "internal.css" -j "time.js" system gargoyle 
?>
<script>
<!--
<?
	echo "var timezoneLines = new Array();"
	if [ -e ./data/timezones.txt ] ; then
		awk '{gsub(/"/, "\\\""); print "timezoneLines.push(\""$0"\");"}' ./data/timezones.txt
	fi
	echo "var timezoneData = parseTimezones(timezoneLines);"

	dateformat=$(uci get gargoyle.global.dateformat 2>/dev/null)
	if [ "$dateformat" == "iso" ]; then
		current_time=$(date "+%Y/%m/%d %H:%M %Z")
	elif [ "$dateformat" == "iso8601" ]; then
		current_time=$(date "+%Y-%m-%d %H:%M %Z")
	elif [ "$dateformat" == "australia" ]; then
		current_time=$(date "+%d/%m/%y %H:%M %Z")
	elif [ "$dateformat" == "russia" ]; then
		current_time=$(date "+%d.%m.%Y %H:%M %Z")
	else
		current_time=$(date "+%D %H:%M %Z")
	fi
	timezone_is_utc=$(uci get system.@system[0].timezone | grep "^UTC" | sed 's/UTC//g')
	if [ -n "$timezone_is_utc" ] ; then
		current_time=$(echo $current_time | sed "s/UTC/UTC-$timezone_is_utc/g" | sed 's/\-\-/+/g')
	fi
	echo "var currentTime = \"$current_time\";"
?>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader">Time</legend>
	
		<div>
			<label id='current_time_label' for='timezone'>Current Date &amp; Time:&nbsp;&nbsp;&nbsp;&nbsp;</label>
			<span id="current_time"></span>
		</div>
				
		<div class="internal_divider"></div>

		<div>
			<label class='nocolumn' id='timezone_label' for='timezone'>Time Zone:</label>
		</div>
		<div class="indent">
			<div><select class='nocolumn' id='timezone' onchange="timezoneChanged()"></select></div>
		</div>
		
		<div>
			<label class='nocolumn' id='timezone_label' for='timezone'>Date Format:</label>
		</div>
		<div class="indent">
			<div>
				<select class='nocolumn' id='date_format'>
					<option value="usa">mm/dd/yy</option>
					<option value="russia">dd.mm.yyyy</option>
					<option value="australia">dd/mm/yy</option>
					<option value="iso">yyyy/mm/dd</option>
					<option value="iso8601">yyyy-mm-dd</option>
				</select>
			</div>
		</div>

			
		<div>
			<label class='leftcolumn' id='region_label' for='region'>NTP Servers:</label>
			<div class="indent">
				<div>
					<select class='leftcolumn' id='region' onchange='updateServerList()'>
						<option value="global">Global</option>
						<option value="us">United States</option>
						<option value="north-america">North America</option>
						<option value="south-america">South America</option>
						<option value="europe">Europe</option>
						<option value="africa">Africa</option>
						<option value="asia">Asia</option>
						<option value="oceania">Oceania</option>
						<option value="custom">Custom</option>
					</select>
				</div>
				<div class="indent">
					<div><input type='text' class='leftcolumn' id="server1" size="35" /></div>
					<div><input type='text' class='leftcolumn' id="server2" size="35" /></div>
					<div><input type='text' class='leftcolumn' id="server3" size="35" /></div>
				</div>
			</div>
		</div>
	</fieldset>
	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>
	<span id="update_container" >Please wait while new settings are applied. . .</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "system" -p "time"  
?>
