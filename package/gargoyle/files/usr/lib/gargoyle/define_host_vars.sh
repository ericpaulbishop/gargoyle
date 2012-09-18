#!/bin/sh

# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

echo "dhcpLeaseLines = new Array();"
if [ -e /tmp/dhcp.leases ] ; then
	cat /tmp/dhcp.leases | awk '{print "dhcpLeaseLines.push(\""$0"\");"}'
fi
	
echo "wifiLines = new Array();"
if [ -e /lib/wifi/broadcom.sh ] ; then
	echo "var wirelessDriver=\"broadcom\";"
	wl assoclist | awk '{print "wifiLines.push(\""$0"\");"}'
elif [ -e /lib/wifi/mac80211.sh ] && [ -e "/sys/class/ieee80211/phy0" ] ; then
	echo "var wirelessDriver=\"mac80211\";"
	aps=$( iwconfig 2>/dev/null | grep "Mode:Master" | awk ' { print $1 ; } ' )
	if [ -n "$aps" ] ; then
		for ap in $aps ; do
			iw $ap station dump | awk ' /^Station/ { printf "wifiLines.push(\""$2" " ;} /signal:/ {printf ""$2" "} /rx.*bitrate:/ {print ""$3"\");"}'
		done
	fi
elif [ -e /lib/wifi/madwifi.sh ] && [ -e "/sys/class/net/wifi0" ] ; then
	echo "var wirelessDriver=\"atheros\";"
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
else
	echo "var wirelessDriver=\"\";"
fi

echo "conntrackLines = new Array();"
cat /proc/net/nf_conntrack | awk '{print "conntrackLines.push(\""substr($0,index($0,$3))"\");"}'

echo "arpLines = new Array();"
cat /proc/net/arp | awk '{print "arpLines.push(\""$0"\");"}'

current_time=$(date +%s)
echo "currentTime=$current_time;"

