#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "status" -p "hosts" -c "internal.css" -j "hosts.js table.js" -i dhcp wireless
?>

<script>
<!--
<?
        echo "var dhcpLeaseLines = new Array();"
	cat /tmp/dhcp.leases | awk '{print "dhcpLeaseLines.push(\""$0"\");"}'

	echo "var wifiLines = new Array();"
	if [ -e /lib/wifi/broadcom.sh ] ; then
		echo "var isBrcm=true;"
		wl assoclist | awk '{print "wifiLines.push(\""$0"\");"}'
	else
		echo "var isBrcm=false;"
		aths=$(iwconfig 2>/dev/null | grep ath | cut -f 1 -d" ")
		modes=$(iwconfig 2>/dev/null | grep "ode:.a" | cut -f 2 -d":" | cut -f 1 -d" ")
		ath_index=1
		for ath in $aths ; do
			
			mode=$(echo $modes | cut -f$ath_index -d" ")
			if [ "$mode" = "Master" ] ; then
				wlanconfig $ath list 2>/dev/null | awk '{print "wifiLines.push(\""$0"\");"}'
			fi
			ath_index=$(($ath_index+1))
		done
	fi

	echo "var conntrackLines = new Array();"
	cat /proc/net/ip_conntrack | awk '{print "conntrackLines.push(\""$0"\");"}'

	echo "arpLines = new Array();"
	cat /proc/net/arp | awk '{print "arpLines.push(\""$0"\");"}'

	current_time=$(date +%s)
	echo "currentTime=$current_time;"

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
