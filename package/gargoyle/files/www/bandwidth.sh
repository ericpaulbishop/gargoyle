#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bandwidth" -c "internal.css" -j "table.js bandwidth.js" gargoyle qos_gargoyle 
?>


<script>
<!--
<?
	echo 'var monitorNames = new Array();'
	mnames=$(cat /tmp/do_bw_backup.sh | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
	for m in $mnames ; do 
		echo "monitorNames.push(\"$m\");"
	done

	tz_hundred_hours=$(date "+%z" | sed 's/^+0//' | sed 's/^-0/-/g')
	tz_minutes=$((tz_hundred_hours*60/100))
	echo "var tzMinutes = $tz_minutes;";
?>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader">Bandwidth Graph Display Options</legend>
		<div>
			<label for='plot_time_frame' id='time_frame_label'>Time Frame:</label>
			<select id="plot_time_frame" onchange="resetPlots()">
				<option value="1">15 Minutes</option>
				<option value="2">24 Hours</option>
				<option value="3">30 Days</option>
				<option value="4">1 Year</option>
			</select>
		</div>

		<div id="control_column_container">
			<div id="plot1_control_column">
				<div><span  id="plot1_title">Plot 1</span></div>
				<div><select id="plot1_type" onchange="resetPlots()" ><option value="total">Total Bandwidth</option></select></div>
				<div><select id="plot1_id" onchange="resetPlots()"></select></div>

			</div>
			<div id="plot2_control_column">
				<div><span   id="plot2_title">Plot 2</span></div>
				<div><select id="plot2_type" onchange="resetPlots()" ><option value="none">None</option></select></div>
				<div><select id="plot2_id" onchange="resetPlots()"></select></div>
			</div>
			<div id="plot3_control_column">
				<div><span   id="plot3_title">Plot 3</span></div>
				<div><select id="plot3_type" onchange="resetPlots()"><option value="none">None</option></select></div>
				<div><select id="plot3_id" onchange="resetPlots()"></select></div>
			</div>
		</div>
		<br/>All bandwidth usage reported is via the WAN interface only.  
		<br/>Traffic between local hosts is not reported.
	</fieldset>
	

	<div class="plot_header">Total Bandwidth Usage:</div>
	<div><embed id="total_plot" style="margin-left:10px; width:525px; height:400px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>	
	
	<div class="plot_header">Download Bandwidth Usage:</div>
	<div><embed id="download_plot" style="margin-left:10px; width:525px; height:400px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	
	<div class="plot_header">Upload Bandwidth Usage:</div>
	<div><embed id="upload_plot" style="margin-left:10px; width:525px; height:400px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	
	<fieldset id="total_bandwidth_use">
		<legend class="sectionheader">Bandwidth Usage Table</legend>
		<div>
			<label for='table_time_frame' class="narrowleftcolumn" id='table_time_frame_label'>Display Interval:</label>
			<select id="table_time_frame" class="rightcolumn" onchange="resetPlots()">
				<option value="1">Minutes</option>
				<option value="2">Hours</option>
				<option value="3">Days</option>
				<option value="4">Months</option>
			</select>
		</div>
		<div>
			<label for='table_type' class="narrowleftcolumn" id='total_type_label'>Display Type:</label>
			<select id="table_type" class="rightcolumn" onchange="resetPlots()">
				<option value="total">Total Bandwidth</option>
			</select>
		</div>
		<div id="table_id_container" style="display:none" >
			<label for='table_id' class="narrowleftcolumn" id='total_id_label'>Display Id:</label>
			<select id="table_id" class="rightcolumn" onchange="resetPlots()"></select>
		</div>
		<div class="bottom_gap">
			<label for='table_units' class="narrowleftcolumn" id='table_units_label'>Table Units:</label>
			<select id="table_units" class="rightcolumn" onchange="resetPlots()">
				<option value="mixed">Auto (Mixed)</option>
				<option value="KBytes">KBytes</option>
				<option value="MBytes">MBytes</option>
				<option value="GBytes">GBytes</option>
				<option value="TBytes">TBytes</option>
			</select>
		</div>

		<div id="bandwidth_table_container"></div>

	</fieldset>

	<fieldset id="download_bandwidth_data" >
		<legend class="sectionheader">Download Bandwidth Data</legend>
		<div>
			<span style='text-decoration:underline'>Data is comma separated:</span>
			<br/>
			<em>[IP],[Direction],[Time Frame],[Interval Start],[Interval End],[Bytes Used]</em>
			<br/>
		</div>
		<div>
			<center><input type='button' id='download_data_button' class='big_button' value='Download Now' onclick='window.location="bandwidth.csv";' /></center>
		</div>
	</fieldset>
</form>


<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	initializePlotsAndTable();
//-->
</script>



<?
	gargoyle_header_footer -f -s "status" -p "bandwidth"  
?>
