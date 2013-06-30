#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "hosts" -c "internal.css" -j "hosts.js table.js" -z "hosts.js" -i -n dhcp wireless
%>

<script>
<!--
var dhcpLeaseLines;
var wifiLines;
var wirelessDriver;
var conntrackLines;
var arpLines;
var currentTime;
<%
	sh /usr/lib/gargoyle/define_host_vars.sh
	wan_ip=$(uci -p /tmp/state get network.wan.ipaddr 2>/dev/null)
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr 2>/dev/null)
	echo "currentWanIp=\"$wan_ip\";"
	echo "currentLanIp=\"$lan_ip\";"
%>
//-->
</script>

<fieldset>
	<legend><%~ hosts.RefreshR %></legend>
	<select id="refresh_rate">
		<option value="2000">2 <%~ seconds %></option>
		<option value="10000">10 <%~ seconds %></option>
		<option value="30000">30 <%~ seconds %></option>
		<option value="60000">60 <%~ seconds %></option>
		<option value="never"><%~ never %></option>
	</select>
	<br/>
	<p><em><%~ RInfo %></em></p>
</fieldset>
<fieldset id="dhcp_data">
	<legend class="sectionHeader"><%~ CurrLeases %></legend>
	<div id="lease_table_container"></div>
</fieldset>
<fieldset id="wifi_data">
	<legend class="sectionheader"><%~ ConWifiHosts %></legend>
	<div id="wifi_table_container"></div>
</fieldset>
<fieldset id="active_data">
	<legend class="sectionheader"><%~ ActiveHosts %></legend>
	<div id="active_table_container"></div>
</fieldset>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "hosts"
%>
