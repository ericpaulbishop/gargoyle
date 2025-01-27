#!/bin/sh

#
# (c) 2024 Michael Gray
#

convert_link_speed()
{
	linkspeed="$1"
	linkspeed="${linkspeed%"${linkspeed##*[![:space:]]}"}"
	case "$linkspeed" in
		"link:down") STATUS="-";;
		"link:up speed:1000baseT") STATUS="1Gbps";;
		"link:up speed:100baseT") STATUS="100Mbps";;
		"link:up speed:10baseT") STATUS="10Mbps";;
		"10000") STATUS="10Gbps";;
		"5000") STATUS="5Gbps";;
		"2500") STATUS="2.5Gbps";;
		"1000") STATUS="1Gbps";;
		"100") STATUS="100Mbps";;
		"10") STATUS="10Mbps";;
		"0") STATUS="-";;
		"1") STATUS=$(i18n conn);;
		*) STATUS="?";;
	esac
	echo "$STATUS"
}

get_status_speed()
{
	ethport=$1

	LINK=$(cat /sys/class/net/$ethport/carrier 2>/dev/null)
	[ "$LINK" = "1" ] && LINK=$(cat /sys/class/net/$ethport/speed 2>/dev/null)
	STATUS=$(convert_link_speed "$LINK")
	echo "$STATUS"
}
