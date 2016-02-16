#!/usr/bin/haserl
<%
	# This program is copyright © 2015 Michael Gray and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	# I'd also like to note and thank Mike Bostock for his work on D3.js which is the underlying
	# graphing library in use for this plugin.
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "spectrum_analyser" -c "internal.css" -j "spectrum_analyser.js" -z "spectrum.js"
%>

<script>
<%
	#we need to find out which wireless interfaces exist.
	echo "var wifiLines = new Array();"
	aps=$( iwinfo | grep ESSID | awk ' { print $1 ; } ' )
	for ap in $aps ; do
		iwinfo $ap info | awk ' /^wlan/ { printf "wifiLines.push(\""$1" " ;} /Channel:/ {print ""$4"\");"}'
	done
%>
</script>
<!--add in graphing library-->
<script src="//d3js.org/d3.v3.min.js" charset="utf-8"></script>

<!--Extra CSS Styling-->
<style>
	.axis path {
		fill: none;
		stroke: #777;
		shape-rendering: crispEdges;
	}
	.axis text {
		font-family: Lato;
		font-size: 13px;
	}
</style>

<fieldset id="spectrum-fieldset">
	<legend class="sectionheader"><%~ spectrum.Analyser %></legend>
	
	<select id="interface" onchange="changeBand()">
		<option value="wlan0">wlan0 2.4GHz</option>
		<option value="wlan1">wlan1 5GHz</option>
	</select>
	<br/>
	<svg id="spectrum_plot" width="500" height="500"></svg>
</fieldset>



<script>
<!--
	initialiseAll();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "spectrum_analyser"
%>
