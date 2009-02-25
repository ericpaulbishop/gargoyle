echo "dhcpLeaseLines = new Array();"
if [ -e /tmp/dhcp.leases ] ; then
	cat /tmp/dhcp.leases | awk '{print "dhcpLeaseLines.push(\""$0"\");"}'
fi
	
echo "wifiLines = new Array();"
if [ -e /lib/wifi/broadcom.sh ] ; then
	echo "isBrcm=true;"
	wl assoclist | awk '{print "wifiLines.push(\""$0"\");"}'
else
	echo "isBrcm=false;"
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

echo "conntrackLines = new Array();"
cat /proc/net/ip_conntrack | awk '{print "conntrackLines.push(\""$0"\");"}'

echo "arpLines = new Array();"
cat /proc/net/arp | awk '{print "arpLines.push(\""$0"\");"}'

current_time=$(date +%s)
echo "currentTime=$current_time;"

