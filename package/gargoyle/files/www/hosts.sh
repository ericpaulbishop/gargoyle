#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" )
	gargoyle_header_footer -h -s "status" -p "hosts" -c "internal.css" -j "hosts.js table.js" -i dhcp wireless
?>

<script>
<!--
var dhcpLeaseLines;
var wifiLines;
var isBrcm;
var conntrackLines;
var arpLines;
var currentTime;
<?
	sh /usr/lib/gargoyle/define_host_vars.sh
	wan_ip=$(uci -p /tmp/state get network.wan.ipaddr) 
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr) 
	echo "currentWanIp=\"$wan_ip\";"
	echo "currentLanIp=\"$lan_ip\";"
?>
//-->
</script>

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
