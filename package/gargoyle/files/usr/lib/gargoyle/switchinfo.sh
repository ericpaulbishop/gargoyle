#!/bin/sh

#
# (c) 2013 Cezary Jackiewicz, http://eko.one.pl
#

[ -e /sbin/swconfig ] || exit 0

board=""
if [ -e /lib/ar71xx.sh ]; then
	. /lib/ar71xx.sh
	board=$(ar71xx_board_name)
elif [ -e /lib/mvebu.sh ]; then
	. /lib/mvebu.sh
	board=$(mvebu_board_name)
elif [ -e /lib/ramips.sh ]; then
	. /lib/ramips.sh
	board=$(ramips_board_name)
fi

# PORTS="LAN1 LAN2 LAN3 LAN4"

case "$board" in
gl-inet)
	PORTS="1";;
routerstation-pro)
	PORTS="4 3 2";;
tl-mr3220 | \
tl-mr3420 | \
tl-wr1043nd | \
tl-wr741nd | \
tl-wr841n-v7)
	PORTS="1 2 3 4";;
tl-mr3220-v2 | \
tl-mr3420-v2 | \
tl-wr741nd-v4 | \
tl-wr841n-v8)
	PORTS="2 3 4 1";;
archer-c5 | \
archer-c7 | \
tl-wdr4300)
	PORTS="2 3 4 5";;
dir-835-a1 | \
tl-wdr3500 | \
tl-wr841n-v9 | \
tl-wr1043nd-v2 | \
wndr4300)
        PORTS="4 3 2 1";;
armada-xp-linksys-mamba | \
armada-385-linksys-caiman | \
armada-385-linksys-cobra | \
armada-385-linksys-shelby | \
armada-385-linksys-rango | \
wndr3700 | \
wrt160nl | \
wzr-hp-g300nh)
	PORTS="3 2 1 0";;
mpr-a2)
	PORTS="0";;
rut5xx)
	PORTS="3 2 1";;
px4885 | \
wt3020)
	PORTS="4";;
*)
	PORTS="";;
esac

IFLAN=$(awk '/default_lan_if/ {print $2}' /etc/gargoyle_default_ifs)
VLAN=$(echo $IFLAN | cut -f2 -d.)
[ "$VLAN" = "$IFLAN" ] && VLAN=""

counter=0
for P in $PORTS; do
	counter=$((counter + 1))
	[ "$P" = "-1" ] && continue
	[ -n "$VLAN" ] && {
		PVID=$(swconfig dev switch0 port $P get pvid)
		[ "$PVID" != "$VLAN" ] && continue
	}
	LINK=$(swconfig dev switch0 port $P get link | cut -f2,3 -d" ")
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
	case "$counter" in
		1) PORT="LAN1";;
		2) PORT="LAN2";;
		3) PORT="LAN3";;
		4) PORT="LAN4";;
	esac
	echo "ports.push([\"$PORT\",\"$STATUS\"]);"
done

exit 0
