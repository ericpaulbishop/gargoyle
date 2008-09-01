#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "status" -p "overview" -c "internal.css" -j "overview.js" -i network wireless qos_gargoyle system 
?>

<script>
<!--
<?
	uptime=$(cat /proc/uptime)
	echo "uptime = \"$uptime\";"

	if [ -h /etc/rc.d/S50qos_gargoyle ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi
	
	gargoyle_version=$(cat data/gargoyle_version.txt)
	echo "var gargoyleVersion=\"$gargoyle_version\""

	current_time=$(date "+%D %H:%M %Z")
	echo "var currentTime=\"$current_time\";"

	total_mem=$(cat /proc/meminfo | grep "MemTotal:" | awk ' { print $2 } ')
	free_mem=$(cat /proc/meminfo | grep "MemFree:" | awk ' { print $2 } ')
	echo "var totalMemory=$total_mem;"
	echo "var freeMemory=$free_mem;"


?>
//-->
</script>

<fieldset>
	<legend class="sectionheader">Status</legend>
	
	<div>
		<span class='leftcolumn'>Device Name:</span><span id="device_name" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>Gargoyle Version:</span><span id="gargoyle_version" class='rightcolumn'></span>

	</div>

	<div>
		<span class='leftcolumn'>Memory Usage:</span><span id="memory" class='rightcolumn'></span>
	</div>
	<div class="internal_divider"></div>

	<div>
		<span class='leftcolumn'>Uptime:</span><span id="uptime" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>Current Date &amp; Time:</span><span id="current_time" class='rightcolumn'></span>
	</div>

	<div class="internal_divider"></div>
	
	<div>
		<span class='leftcolumn'>LAN IP Address:</span><span id="lan_ip" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>LAN Netmask:</span><span id="lan_mask" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>LAN MAC Address:</span><span id="lan_mac" class='rightcolumn'></span>
	</div>

	<div class="internal_divider"></div>

	<div>
		<span class='leftcolumn'>WAN IP Address:</span><span id="wan_ip" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>WAN Netmask:</span><span id="wan_mask" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>WAN MAC Address:</span><span id="wan_mac" class='rightcolumn'></span>
	</div>

	<div class="internal_divider"></div>

	<div>
		<span class='leftcolumn'>Wireless Mode:</span><span id="wireless_mode" class='rightcolumn'></span>
	</div>
	<div id="wireless_mac_div">
		<span class='leftcolumn'>Wireless MAC address:</span><span id="wireless_mac" class='rightcolumn'></span>
	</div>
	<div id="wireless_apssid_div">
		<span class='leftcolumn' id="wireless_apssid_label">Access Point SSID:</span><span id="wireless_apssid" class='rightcolumn'></span>
	</div>
	<div id="wireless_otherssid_div">
		<span class='leftcolumn' id="wireless_otherssid_label">SSID Joined By Client:</span><span id="wireless_otherssid" class='rightcolumn'></span>
	</div>



	<div class="internal_divider"></div>

	<div>
		<span class='leftcolumn'>QoS Upload:</span><span id="qos_upload" class='rightcolumn'></span>
	</div>
	<div>
		<span class='leftcolumn'>QoS Download:</span><span id="qos_download" class='rightcolumn'></span>
	</div>

</fieldset>

<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "status" -p "overview"
?>
