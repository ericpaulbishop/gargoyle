#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "hosts" -c "internal.css" -j "hosts.js table.js" -i -n dhcp wireless
?>

<script>
<!--
var dhcpLeaseLines;
var wifiLines;
var wirelessDriver;
var conntrackLines;
var arpLines;
var currentTime;
<?
	sh /usr/lib/gargoyle/define_host_vars.sh
	wan_ip=$(uci -p /tmp/state get network.wan.ipaddr 2>/dev/null)
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr 2>/dev/null)
	echo "currentWanIp=\"$wan_ip\";"
	echo "currentLanIp=\"$lan_ip\";"
?>
//-->
</script>

<fieldset>
	<legend>Refresh Rate</legend>
	<select id="refresh_rate">
		<option value="2000">2 Seconds</option>
		<option value="10000">10 Seconds</option>
		<option value="30000">30 Seconds</option>
		<option value="60000">60 Seconds</option>
		<option value="never">Never</option>
	</select>
	<br/>
	<p><em>This specifies how frequently data on this page is reloaded</em></p>
</fieldset>
<fieldset id="dhcp_data">
	<legend class="sectionHeader">Current DHCP Leases</legend>
	<div id="lease_table_container"></div>
</fieldset>
<fieldset id="wifi_data">
	<legend class="sectionheader">Connected Wireless Hosts</legend>
	<div id="wifi_table_container"></div>
</fieldset>
<fieldset id="active_data">
	<legend class="sectionheader">Hosts With Active Connections</legend>
	<div id="active_table_container"></div>
</fieldset>

<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "status" -p "hosts"
?>
