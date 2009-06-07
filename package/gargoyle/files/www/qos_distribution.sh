#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) )	
	gargoyle_header_footer -h -s "status" -p "qos" -c "internal.css" -j "qos_distribution.js" qos_gargoyle
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
<style>
	.plot_header
	{
		font-family: verdana, sans-serif;
		font-size:18px;
		font-weight:bold;
		color:#888AB8;
		margin-left:20px;
		margin-top:10px;
		margin-bottom:10px;
	}
</style>
<form>
	<fieldset id="upload_container">
		<legend class="sectionheader">QoS Upload Bandwidth Distribution</legend>
		<div>
			<label class="leftcolumn" for="upload_timeframe">Upload Time Frame:</label>
			<select class="rightcolumn" id="upload_timeframe" onchange="setQosTimeframes()">
				<option value="15m">15 Minutes</option>
				<option value="15h">15 Hours</option>
				<option value="15d">15 Days</option>
			</select>
		</div>
		<div><embed id="upload_pie" style="margin-left:10px; width:475px; height:400px;" src="pie.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	</fieldset>
	
	<fieldset id="download_container">
		<legend class="sectionheader">QoS Download Bandwidth Distribution</legend>
		<div>
		<label class="leftcolumn" for="download_timeframe">Download Time Frame:</label>
			<select class="rightcolumn" id="download_timeframe" onchange="setQosTimeframes()">
				<option value="15m">15 Minutes</option>
				<option value="15h">15 Hours</option>
				<option value="15d">15 Days</option>
			</select>
		</div>
		<div><embed id="download_pie" style="margin-left:10px; width:475px; height:400px;" src="pie.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	</fieldset>

</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<script>
<!--
	initializePieCharts();	
//-->
</script>


<?
	gargoyle_header_footer -f -s "status" -p "qos" 
?>
