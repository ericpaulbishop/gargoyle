#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bdist" -c "internal.css" -j "table.js bdist.js" -n -i gargoyle
?>


<script>
<!--
<?
	echo 'monitorNames = new Array();'
	mnames=$(cat /tmp/bw_backup/do_bw_backup.sh | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
	for m in $mnames ; do 
		echo "monitorNames.push(\"$m\");"
	done

	tz_hundred_hours=$(date "+%z" | sed 's/^+0//' | sed 's/^-0/-/g')
	tz_h=$(($tz_hundred_hours/100))
	tz_m=$(($tz_hundred_hours-($tz_h*100)))
	tz_minutes=$((($tz_h*60)+$tz_m))
	echo "var tzMinutes = $tz_minutes;";
?>
//-->
</script>


<form>

	<fieldset>
		<legend class="sectionheader">Bandwidth Distribution Display Options</legend>
	


		<div>
			<label class="leftcolumn" for='time_frame' id='time_frame_label'>Distribution Time Frame:</label>
			<select class="rightcolumn" id="time_frame" onchange="resetTimeFrame()">
				<option value="bdist1">Minutes</option>
				<option value="bdist2">Quarter Hours</option>
				<option value="bdist3">Hours</option>
				<option value="bdist4">Days</option>
				<option value="bdist5">Months</option>
			</select>
		</div>

		<div>
			<label class="leftcolumn" for='time_interval' id='time_interval_label'>Distribution Interval:</label>
			<select class="rightcolumn" id="time_interval" onchange="resetDisplayInterval()"></select>
		</div>

		<div>
			<label class="leftcolumn" for='host_display' id='time_interval_label'>Host Display:</label>
			<select class="rightcolumn" id="host_display" onchange="resetTimeFrame()">
				<option value="hostname">Display Hostnames</option>
				<option value="ip">Display Host IPs</option>
			</select>
		</div>

	</fieldset>
	
	<div class="plot_header">Bandwidth Distribution:</div>
	<div><embed id="pie_chart" style="margin-left:10px; width:525px; height:525px;" src="multi_pie.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	<fieldset>
		<legend class="sectionheader">Bandwidth Distribution Table</legend>
		<div id="bandwidth_distribution_table_container"></div>	
	</fieldset>
	
</form>




<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	initializePlotsAndTable();
//-->
</script>



<?
	gargoyle_header_footer -f -s "status" -p "bdist"  
?>
