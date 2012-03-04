#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
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

	dateformat=$(uci get gargoyle.global.dateformat 2>/dev/null)
	if [ "$dateformat" == "iso" ]; then
		current_time=$(date "+%Y/%m/%d %H:%M %Z")
	elif [ "$dateformat" == "iso8601" ]; then
		current_time=$(date "+%Y-%m-%d %H:%M %Z")
	elif [ "$dateformat" == "australia" ]; then
		current_time=$(date "+%d/%m/%y %H:%M %Z")
	elif [ "$dateformat" == "russia" ]; then
		current_time=$(date "+%d.%m.%Y %H:%M %Z")
	else
		current_time=$(date "+%D %H:%M %Z")
	fi
	timezone_is_utc=$(uci get system.@system[0].timezone | grep "^UTC" | sed 's/UTC//g')
	if [ -n "$timezone_is_utc" ] ; then
		current_time=$(echo $current_time | sed "s/UTC/UTC-$timezone_is_utc/g" | sed 's/\-\-/+/g')
	fi
	echo "var currentTime = \"$current_time\";"

	total_mem="$(sed -e '/^MemTotal: /!d; s#MemTotal: *##; s# kB##g' /proc/meminfo)"
	buffers_mem="$(sed -e '/^Buffers: /!d; s#Buffers: *##; s# kB##g' /proc/meminfo)"
	cached_mem="$(sed -e '/^Cached: /!d; s#Cached: *##; s# kB##g' /proc/meminfo)"
	free_mem="$(sed -e '/^MemFree: /!d; s#MemFree: *##; s# kB##g' /proc/meminfo)"
	free_mem="$(( ${free_mem} + ${buffers_mem} + ${cached_mem} ))"
	echo "var totalMemory=parseInt($total_mem);"
	echo "var freeMemory=parseInt($free_mem);"


	total_swap="$(sed -e '/^SwapTotal: /!d; s#SwapTotal: *##; s# kB##g' /proc/meminfo)"
	cached_swap="$(sed -e '/^SwapCached: /!d; s#SwapCached: *##; s# kB##g' /proc/meminfo)"
	free_swap="$(sed -e '/^SwapFree: /!d; s#SwapFree: *##; s# kB##g' /proc/meminfo)"
	free_swap="$(( ${free_swap} + ${cached_swap} ))"
	echo "var totalSwap=parseInt($total_swap);"
	echo "var freeSwap=parseInt($free_swap);"


	
	load_avg="$(awk '{print $1 " / " $2 " / " $3}' /proc/loadavg)"
	echo "var loadAvg=\"$load_avg\";"

	curconn="$(wc -l < /proc/net/ip_conntrack)"
	maxconn=$(cat /proc/sys/net/ipv4/ip_conntrack_max 2>/dev/null)
	if [ -z "$maxconn" ] ; then
		maxconn=$(cat /proc/sys/net/ipv4/netfilter/ip_conntrack_max 2>/dev/null )
	fi
	if [ -z "$maxconn" ] ; then
		maxconn="4096"
	fi
	echo "var curConn=\"$curconn\";"
	echo "var maxConn=\"$maxconn\";"


	echo "var wanDns=\""$(sed -e '/nameserver/!d; s#nameserver ##g' /tmp/resolv.conf.auto)"\";"

?>
//-->
</script>

<fieldset>
	<legend class="sectionheader">Status</legend>

	<div id="device_container">	
		<div>
			<span class='leftcolumn'>Device Name:</span><span id="device_name" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Gargoyle Version:</span><span id="gargoyle_version" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Device Configuration:</span><span id="device_config" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Memory Usage:</span><span id="memory" class='rightcolumn'></span>
		</div>
		<div id="swap_container">
			<span class='leftcolumn'>Swap Memory Usage:</span><span id="swap" class='rightcolumn'></span>
		</div>

		<div>
			<span class='leftcolumn'>Connections:</span><span id="connections" class='rightcolumn'></span>
		</div>
 		<div>
 			<span class='leftcolumn'>CPU Load Averages:</span><span id="load_avg" class='rightcolumn'></span><span>&nbsp;&nbsp;(1/5/15 minutes)
 		</div>
		<div class="internal_divider"></div>
	</div>


	<div id="time_container">
		<div>
			<span class='leftcolumn'>Uptime:</span><span id="uptime" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Current Date &amp; Time:</span><span id="current_time" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>


	<div id="bridge_container">
		<div>
			<span class='leftcolumn'>Bridge IP Address:</span><span id="bridge_ip" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Bridge Netmask:</span><span id="bridge_mask" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Bridge MAC Address:</span><span id="bridge_mac" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>LAN Gateway IP:</span><span id="bridge_gateway" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Connected Via:</span><span id="bridge_mode" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>Bridge SSID:</span><span id="bridge_ssid" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>


	<div id="lan_container">
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
	</div>


	<div id="wan_container">
		<div>
			<span class='leftcolumn'>WAN IP Address:</span><span id="wan_ip" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>WAN Netmask:</span><span id="wan_mask" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>WAN MAC Address:</span><span id="wan_mac" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>WAN Gateway IP:</span><span id="wan_gateway" class='rightcolumn'></span>
		</div>
		<div id="wan_dns_container">
			<span class='leftcolumn'>WAN DNS Server(s):</span><span id="wan_dns" class='rightcolumn'></span>
		</div>
		<div id="wan_3g_container">
			<span class='leftcolumn'>Signal Strength:</span><span id="wan_3g" class='rightcolumn'>
<?
	if [ -e /tmp/strength.txt ]; then
		awk -F[,\ ] '/^\+CSQ:/ {if ($2>31) {C=0} else {C=$2}} END {if (C==0) {printf "(no data)"} else {printf "%d%%, %ddBm\n", C*100/31, C*2-113}}' /tmp/strength.txt
	fi
?>
			</span>
		</div>
		<div class="internal_divider"></div>
	</div>


	<div id="wifi_container">
		<div>
			<span class='leftcolumn'>Wireless Mode:</span><span id="wireless_mode" class='rightcolumn'></span>
		</div>
		<div id="wireless_mac_div">
			<span class='leftcolumn'>Wireless MAC Address:</span><span id="wireless_mac" class='rightcolumn'></span>
		</div>
		<div id="wireless_apssid_div">
			<span class='leftcolumn' id="wireless_apssid_label">Access Point SSID:</span><span id="wireless_apssid" class='rightcolumn'></span>
		</div>
		<div id="wireless_apssid_5ghz_div">
			<span class='leftcolumn' id="wireless_apssid_5ghz_label">5GHz Access Point SSID:</span><span id="wireless_apssid_5ghz" class='rightcolumn'></span>
		</div>
		<div id="wireless_otherssid_div">
			<span class='leftcolumn' id="wireless_otherssid_label">SSID Joined By Client:</span><span id="wireless_otherssid" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>



	
	<div id="services_container">
		<div>
			<span class='leftcolumn'>QoS Upload:</span><span id="qos_upload" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'>QoS Download:</span><span id="qos_download" class='rightcolumn'></span>
		</div>
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
