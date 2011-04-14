#!/bin/sh

out_file="/var/cached_basic_vars"
if [ -e "$out_file" ] ; then
	noDriver=$(cat "$out_file" | grep "wirelessDriver=..;" 2>/dev/null)
	echo "no driver = $noDriver"
	if [ -z "$noDriver" ] ; then exit ; else rm -rf "$out_file" ; fi
fi
touch "$out_file"

# determine if this board is a bcm94704, for which the uci wan macaddr variable must ALWAYS be set
PART="$(grep "nvram" /proc/mtd | awk -F: '{print $1}')"
if [ -n "$PART" ] ; then
	PREFIX=/dev/mtdblock
	PART="${PART##mtd}"
	[ -d /dev/mtdblock ] && PREFIX=/dev/mtdblock/ 
	nvrampath="${PART:+$PREFIX$PART}"
	boardtype=$(strings $nvrampath | grep boardtype | awk 'BEGIN {FS="="}; {print $2}')
	boardnum=$(strings $nvrampath | grep boardnum | awk 'BEGIN {FS="="}; {print $2}')
	#echo "boardnum = $boardnum, boardtype = $boardtype"
	isbcm94704='false'
	if [ "$boardtype" = "0x0472" ] || [ "$boardtype" = "0x042f" ] ; then
		if [ "$boardnum" != "45" ] ; then
			isbcm94704='true'
		fi
	fi
else
	isbcm94704='false'
fi
echo "var isBcm94704 = $isbcm94704;" >> "$out_file"
echo "var allLanMacs = [];" >> "$out_file"
brctl showmacs br-lan | grep "yes" | awk ' { print "allLanMacs.push(\"" $2 "\");" } ' >> "$out_file"



echo "var wifiDevG=uciWirelessDevs.length > 0 ? uciWirelessDevs[0] : \"\";" >> "$out_file"
echo "var wifiDevA=\"\";" >> "$out_file"

if [ -e /lib/wifi/broadcom.sh ] ; then
	echo "var wirelessDriver=\"broadcom\";" >> "$out_file"
	echo "var wifiN = false;" >> "$out_file"
elif [ -e /lib/wifi/mac80211.sh ] && [ -e "/sys/class/ieee80211/phy0" ] ; then
	echo "var wirelessDriver=\"mac80211\";" >> "$out_file"
	echo 'var mac80211Channels = [];' >> "$out_file"
	echo "var nextCh=[];" >> "$out_file"
	ncapab="$ncapab"$( uci get wireless.@wifi-device[0].htmode 2>/dev.null; uci get wireless.@wifi-device[0].ht_capab 2>/dev/null | grep 40 ; )
	if [ -n "$ncapab" ] ; then echo "var wifiN = true ;"  >> "$out_file"; else echo "var wifiN = false ;"  >> "$out_file" ; fi
	
	#test for dual band
	if [ `uci show wireless | grep wifi-device | wc -l`"" = "2" ] && [ -e "/sys/class/ieee80211/phy1" ] && [ ! `uci get wireless.@wifi-device[0].hwmode`"" = `uci get wireless.@wifi-device[1].hwmode`""  ] ; then
		echo "var dualBandWireless=true;" >> "$out_file"
		radios=$(uci show wireless | grep wifi-device | sed 's/^.*\.//g' | sed 's/=.*$//g')
		rnum=0;
		for r in $radios ; do
			echo "nextCh = [];" >> "$out_file"
			mode=$(uci get wireless.$r.hwmode)
			[ "$mode" = "11na" ] &&  mode="11an"
			if [ "$mode" = "11an" ] ; then
				chId="A"
				echo "wifiDevA=\"$r\";" >> "$out_file"
			else
				mode="11bgn"
				chId="G"
				echo "wifiDevG=\"$r\";" >> "$out_file"
			fi


			cur_if=$(iwconfig 2>/dev/null | grep "wlan" | grep "$mode" | awk ' { print $1 }' | head -n 1)
			if [ -n "$cur_if" ] ; then
				iwlist $cur_if channel |  grep -v "total;" | awk '{print $2 ; }' | egrep "^[0-9]+$" | awk ' { print "nextCh.push(parseInt(\"" $0 "\", 10)+\"\");" ; } ' >> "$out_file"
			else		
				iw phy phy$rnum interface add tmpmon type monitor
				ifconfig tmpmon up
				iwlist tmpmon channel |  grep -v "total;" | awk '{print $2 ; }' | egrep "^[0-9]+$" | awk ' { print "nextCh.push(parseInt(\"" $0 "\", 10)+\"\");" ; } ' >> "$out_file"
				ifconfig tmpmon down
				iw dev tmpmon del
			fi
			rnum=$(( $rnum+1 ))
			echo "mac80211Channels[\"$chId\"] = nextCh ;" >> "$out_file"
		done

	else
		echo "var dualBandWireless=false;" >> "$out_file"
		#use iw to get available channels
		cur_if=$(iwconfig 2>/dev/null | grep "wlan" | awk ' { print $1 }' | head -n 1)
		if [ -n "$cur_if" ] ; then
			iwlist $cur_if channel |  grep -v "total;" | awk '{print $2 ; }' | egrep "^[0-9]+$" | awk ' { print "nextCh.push(parseInt(\"" $0 "\", 10)+\"\");" ; } ' >> "$out_file"
		else		
			iw phy phy0 interface add tmpmon type monitor
			ifconfig tmpmon up
			iwlist tmpmon channel |  grep -v "total;" | awk '{print $2 ; }' | egrep "^[0-9]+$" | awk ' { print "nextCh.push(parseInt(\"" $0 "\", 10)+\"\");" ; } ' >> "$out_file"
			ifconfig tmpmon down
			iw dev tmpmon del
		fi
		echo "mac80211Channels[\"G\"] = nextCh ;" >> "$out_file"

	fi

elif [ -e /lib/wifi/madwifi.sh ] && [ -e "/sys/class/net/wifi0" ] ; then
	echo "var wirelessDriver=\"atheros\";" >> "$out_file"
	echo "var wifiN = false;" >> "$out_file"
else
	echo "var wirelessDriver=\"\";" >> "$out_file"
	echo "var wifiN = false;" >> "$out_file"
fi

if [ -e /proc/bus/usb/devices ]; then 
	echo "var hasUSB=true;" >> "$out_file"
else
	echo "var hasUSB=false;" >> "$out_file"
fi

# cache default interfaces if we haven't already
# this script is run on first boot by hotplug, so
# this will make sure the defaults get cached right
# away
gargoyle_header_footer -i >/dev/null 2>&1

