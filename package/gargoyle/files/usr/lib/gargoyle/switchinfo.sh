#!/bin/sh

#
# (c) 2013 Cezary Jackiewicz, http://eko.one.pl
# (c) 2018 Michael Gray
#

[ -e /sbin/swconfig ] || exit 0
[ -e /usr/share/libubox/jshn.sh ] || exit 0
[ -e /etc/board.json ] || exit 0

. /usr/share/libubox/jshn.sh

# ports.push(["LAN#","STATUS"]);

json_load_file "/etc/board.json"
json_get_keys BOARDKEYS
SWITCHTEST=$(echo $BOARDKEYS | grep "switch")
[ -n "$SWITCHTEST" ] || exit 0

json_select switch
json_get_keys SWITCHKEYS
#handle only a single switch for now
SWITCHID=$(echo $SWITCHKEYS | grep "switch" | cut -d " " -f1)
[ -n "$SWITCHID" ] || exit 0

json_select $SWITCHID
json_select ports
json_get_keys PORTS
[ -n "$PORTS" ] || exit 0

IFLAN=$(awk '/default_lan_if/ {print $2}' /etc/gargoyle_default_ifs)
VLAN=$(echo $IFLAN | cut -f2 -d.)
[ "$VLAN" = "$IFLAN" ] && VLAN=""

for PORT in $PORTS; do
	json_select $PORT
	json_get_var PHYSICAL index
	json_get_var LOGICAL num
	json_select ..
	if [ -n "$PHYSICAL" ] && [ -n "$LOGICAL" ] ; then
		[ "$P" = "-1" ] && continue
		[ -n "$VLAN" ] && {
			PVID=$(swconfig dev $SWITCHID port $LOGICAL get pvid)
			[ "$PVID" != "$VLAN" ] && continue
		}
		LINK=$(swconfig dev switch0 port $LOGICAL get link | cut -f2,3 -d" ")
		case "$LINK" in
			"link:down") STATUS="-";;
			"link:up speed:1000baseT") STATUS="1Gbps";;
			"link:up speed:100baseT") STATUS="100Mbps";;
			"link:up speed:10baseT") STATUS="10Mbps";;
			"1000") STATUS="1Gbps";;
			"100") STATUS="100Mbps";;
			"10") STATUS="10Mbps";;
			"0") STATUS="-";;
			"1") STATUS=$(i18n conn);;
			*) STATUS="?";;
		esac
		echo "ports.push([\"LAN$PHYSICAL\",\"$STATUS\"]);"
	fi
done

exit 0
