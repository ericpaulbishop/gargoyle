#!/bin/sh

print_mac80211_channels_for_wifi_dev()
{
	wifi_dev="$1"
	dev_num="$2"
	out="$3"
	
	echo "nextCh     = [];" >> "$out"
	echo "nextChFreq = [];" >> "$out"
	echo "nextChPwr  = [];" >> "$out"
	mode=$(uci get wireless.$wifi_dev.hwmode)
	[ "$mode" = "11na" ] &&  mode="11an"
	if [ "$mode" = "11an" ] ; then
		chId="A"
		echo "wifiDevA=\"$wifi_dev\";" >> "$out"
	else
		mode="11bgn"
		chId="G"
		echo "wifiDevG=\"$wifi_dev\";" >> "$out"
	fi
	
	# we are about to screen-scrape iw output, which the tool specifically says we should NOT do
	# however, as far as I can tell there is no other way to get max txpower for each channel
	# so... here it goes.
	# If stuff gets FUBAR, take a look at iw output, and see if this god-awful expression still works
	iw "phy${dev_num}" info 2>&1 | sed -e '/MHz/!d; /GI/d; /disabled/d; /radar detect/d;s/[:blank:]*\*[:blank:]*//g; s:[]()[]::g; s/\..*$//g' | awk ' { print "nextCh.push("$3"); nextChFreq["$3"] = \""$1"MHz\"; nextChPwr["$3"] = "$4";"   ; } ' >> "$out"

	echo "mac80211Channels[\"$chId\"] = nextCh ;"     >> "$out"
	echo "mac80211ChFreqs[\"$chId\"]  = nextChFreq ;" >> "$out"
	echo "mac80211ChPwrs[\"$chId\"]   = nextChPwr ;"  >> "$out"

}



out_file="/var/cached_basic_vars"
if [ -e "$out_file" ] ; then
	noDriver="$(grep "wirelessDriver=..;" "$out_file" 2>/dev/null)"
	echo "no driver = $noDriver"
	if [ -z "$noDriver" ] ; then exit ; else rm -f "$out_file" ; fi
fi
touch "$out_file"

# determine if this board is a bcm94704, for which the uci wan macaddr variable must ALWAYS be set
PART="$(grep 'nvram' /proc/mtd)"
PART="${PART%%:*}"
if [ -n "$PART" ] ; then
	PREFIX=/dev/mtdblock
	PART="${PART##mtd}"
	[ -d /dev/mtdblock ] && PREFIX=/dev/mtdblock/ 
	nvrampath="${PART:+$PREFIX$PART}"
	boardtype="$(strings "${nvrampath}" | sed -e '/boardtype/!d; s#boardtype=##g')"
	boardnum="$(strings "${nvrampath}" | sed -e '/boardnum/!d; s#boardnum=##g')"
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
	echo 'var wirelessDriver="mac80211";' >> "$out_file"
	echo 'var mac80211Channels = [];' >> "$out_file"
	echo 'var mac80211ChFreqs = [];' >> "$out_file"
	echo 'var mac80211ChPwrs = [];' >> "$out_file"



	echo "var nextCh=[];" >> "$out_file"
	ncapab="$ncapab"$( uci get wireless.@wifi-device[0].htmode 2>/dev/null; uci get wireless.@wifi-device[0].ht_capab 2>/dev/null | grep 40 ; )
	if [ -n "$ncapab" ] ; then echo "var wifiN = true ;"  >> "$out_file"; else echo "var wifiN = false ;"  >> "$out_file" ; fi
	
	#test for dual band
	if [ `uci show wireless | grep wifi-device | wc -l`"" = "2" ] && [ -e "/sys/class/ieee80211/phy1" ] && [ ! `uci get wireless.@wifi-device[0].hwmode`"" = `uci get wireless.@wifi-device[1].hwmode`""  ] ; then
		echo "var dualBandWireless=true;" >> "$out_file"
	else
		echo "var dualBandWireless=false;" >> "$out_file"
	fi
	
	radios=$(uci show wireless | grep wifi-device | sed 's/^.*\.//g' | sed 's/=.*$//g')
	radios="$(uci show wireless | sed -e '/wifi-device/!d; s/^.*\.//g; s/=.*$//g')"
	rnum=0;
	for r in $radios ; do
		print_mac80211_channels_for_wifi_dev "$r" "$rnum" "$out_file"
		rnum=$(( $rnum+1 ))
	done


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

