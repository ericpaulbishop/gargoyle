#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" )
	gargoyle_header_footer -h -s "status" -p "bandwidth" -c "internal.css" -j "table.js bandwidth.js" qos_gargoyle 
?>


<script>
<!--
<?
	echo 'var monitorNames = new Array();'
	if [ -e /etc/bwmond.conf ] ; then
		cat /etc/bwmond.conf | awk ' { if($0 ~ /^monitor/){ print "monitorNames.push(\""$2"\");" ; }} '
	fi
?>
//-->
</script>


<form>

	<fieldset>
		<legend class="sectionheader">Bandwidth Graph Display Options</legend>
	

		<div>
			<label for='time_frame' id='time_frame_label'>Time Frame:</label>
			<select id="time_frame" onchange="resetPlots()">
				<option value="15m">15 Minutes</option>
				<option value="15h">15 Hours</option>
				<option value="15d">15 Days</option>
				<option value="1y">1 Year</option>
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
	
	<div class="plot_header">Upload Bandwidth Usage:</div>
	<div><embed id="upload_plot" style="margin-left:10px; width:525px; height:400px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	
	
	<div class="plot_header">Download Bandwidth Usage:</div>
	<div><embed id="download_plot" style="margin-left:10px; width:525px; height:400px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>

	<fieldset id="total_bandwidth_use" style="display:none" >
		<legend class="sectionheader">Total Bandwidth Usage</legend>
		<div>
			<label for='total_time_frame' class="narrowleftcolumn" id='total_time_frame_label'>Display Time Interval:</label>
			<select id="total_time_frame" class="rightcolumn" onchange="updateTotalTable()"></select>
		</div>	
		<div class="bottom_gap">
			<label for='total_table_units' class="narrowleftcolumn" id='total_table_units_label'>Table Units:</label>
			<select id="total_table_units" class="rightcolumn" onchange="updateTotalTable()"></select>
		</div>
		<div id="total_bandwidth_table_container">
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
