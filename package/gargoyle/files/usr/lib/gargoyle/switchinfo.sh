#!/bin/sh

#
# (c) 2013 Cezary Jackiewicz, http://eko.one.pl
# (c) 2018 Michael Gray
#

[ -e /usr/share/libubox/jshn.sh ] || exit 0
[ -e /etc/board.json ] || exit 0

. /usr/share/libubox/jshn.sh

# ports.push(["LAN#","STATUS"]);

convert_link_speed()
{
	linkspeed="$1"
	case "$linkspeed" in
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
	echo "$STATUS"
}

json_load_file "/etc/board.json"
json_get_keys BOARDKEYS
for KEY in $BOARDKEYS; do
	[ "$KEY" = "switch" ] && SWITCHTEST="1"
done
json_select network
	json_select lan
		json_get_keys LANKEYS
		for KEY in $LANKEYS; do
			[ "$KEY" = "ports" ] && PORTSTEST="1"
		done
	json_select ..
json_select ..

if [ -n "$PORTSTEST" ]; then
	# DSA
	json_select network
		json_select lan
			json_select ports
				json_get_keys PORTS
				[ -n "$PORTS" ] || exit 0
				for PORT in $PORTS; do
					json_get_var PORTNAME $PORT
					LINK=$(cat /sys/class/net/$PORTNAME/carrier 2>/dev/null)
					[ "$LINK" = "1" ] && LINK=$(cat /sys/class/net/$PORTNAME/speed 2>/dev/null)
					STATUS=$(convert_link_speed "$LINK")
					echo "ports.push([\"LAN${PORTNAME:3}\",\"$STATUS\"]);"
				done
			json_select ..
		json_select ..
	json_select ..
elif [ -n "$SWITCHTEST" ]; then
	# swconfig
	[ -e /sbin/swconfig ] || exit 0
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
						json_get_var ROLE role
					json_select ..
					if [ "$ROLE" = "lan" ] && [ -n "$LOGICAL" ] ; then
						[ "$P" = "-1" ] && continue
						[ -n "$VLAN" ] && {
							PVID=$(swconfig dev $SWITCHID port $LOGICAL get pvid)
							[ "$PVID" != "$VLAN" ] && continue
						}
						LINK=$(swconfig dev switch0 port $LOGICAL get link | cut -f2,3 -d" ")
						STATUS=$(convert_link_speed "$LINK")
						echo "ports.push([\"LAN$PHYSICAL\",\"$STATUS\"]);"
					fi
				done
			json_select ..
		json_select ..
	json_select ..
fi

exit 0

