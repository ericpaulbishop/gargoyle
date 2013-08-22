#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "qos" -c "internal.css" -j "qos_distribution.js" qos_gargoyle
%>

<script>
<!--
<%
	echo 'var monitorNames = new Array();'
	mnames=$(cat /tmp/bw_backup/do_bw_backup.sh  | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
	for m in $mnames ; do 
		echo "monitorNames.push(\"$m\");"
	done
%>
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
		<legend class="sectionheader"><%~ qos.UBSect %></legend>
		<div>
			<label class="leftcolumn" for="up_timeframe"><%~ uTFrm %>:</label>
			<select class="rightcolumn" id="up_timeframe" onchange="setQosTimeframes()">
				<option value="1"><%~ minutes %></option>
				<option value="2"><%~ bandwidth.qhour %></option>
				<option value="3"><%~ hours %></option>
				<option value="4"><%~ days %></option>
				<option value="5"><%~ mnths %></option>

			</select>
		</div>
		<div><embed id="upload_pie" style="margin-left:10px; width:475px; height:400px;" src="pie.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>
	</fieldset>

	<fieldset id="download_container">
		<legend class="sectionheader"><%~ DBSect %></legend>
		<div>
		<label class="leftcolumn" for="down_timeframe"><%~ dTFrm %>:</label>
			<select class="rightcolumn" id="down_timeframe" onchange="setQosTimeframes()">
				<option value="1"><%~ minutes %></option>
				<option value="2"><%~ qhour %></option>
				<option value="3"><%~ hours %></option>
				<option value="4"><%~ days %></option>
				<option value="5"><%~ mnths %></option>
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

<%
	gargoyle_header_footer -f -s "status" -p "qos"
%>
