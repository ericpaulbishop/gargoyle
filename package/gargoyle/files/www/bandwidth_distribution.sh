#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bandwidth" -c "internal.css" -j "table.js bdist.js" gargoyle
?>


<script>
<!--
<?
	echo 'var monitorNames = new Array();'
	mnames=$(cat /tmp/do_bw_backup.sh | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
	for m in $mnames ; do 
		echo "monitorNames.push(\"$m\");"
	done
?>
//-->
</script>


<form>

	<fieldset>
		<legend class="sectionheader">Bandwidth Display Options</legend>
	


		<div>
			<label for='time_frame' id='time_frame_label'>Distribution Time Frame:</label>
			<select id="time_frame" onchange="resetPlots()">
				<option value="bdist1">Minutes</option>
				<option value="bdist2">Hours</option>
				<option value="bdist3">Days</option>
				<option value="bdist4">Months</option>
			</select>
		</div>

		<div>
			<label for='time_interval' id='time_interval_label'>Distribution Interval:</label>
			<select id="time_interval" onchange="resetPlots()"></select>
		</div>
	</fieldset>
	
	<div class="plot_header">Upload Bandwidth Usage:</div>
	<div><embed id="upload_plot" style="margin-left:10px; width:525px; height:525px;" src="multi_pie.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	
	
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
