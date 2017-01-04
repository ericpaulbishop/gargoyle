#!/bin/sh
#
# This program is copyright © 2015 John Brown and is distributed under the terms of the GNU GPL
# version 2.0 with a special clarification/exception that permits adapting the program to
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL.
# See http://gargoyle-router.com/faq.html#qfoss for more information

# This script was written as part of the Device Groups implementation (see: /www/dhcp.sh)
# Know Devices may be assigned to a Group (see: uci show known.device)
# An ipset is created for each Group (see: ipset list -n)
# Each new dhcp lease must update the ipset (see: uci show dnsmasq.dhcp-script)
# This script searches Know Devices for a matching MAC and will add/del the IP to/from the Group's ipset


event=$1
mac="$(echo $2 | awk '{print toupper($0)}')"
ip=$3
host=$4

OIFS=$IFS;

#echo "" >>//tmp/post_lease
#echo $event $mac $ip $host >>/tmp/post_lease

uciHosts=$(uci show dhcp 2>>//dev/null | grep '=host' | sed s/dhcp.// | sed s/=host// | awk '{ print $1 ; } ')

for uciHost in $uciHosts; do

	IFS=" "
	uciMacs=$(uci get dhcp.$uciHost.mac 2>>//dev/null)
	for uciMac in $uciMacs ; do
		if [ "$uciMac" = "$mac" ]; then

			uciGroup=$(uci get dhcp.$uciHost.group 2>>//dev/null)
			if [ "${#uciGroup}" -gt 0 ]; then

				if [ "$event" = "add" ] || [ "$event" = "old" ]; then
					ipset create $uciGroup hash:ip hashsize 64 2>>//dev/null	# create ipset if not exist
					ipset add $uciGroup $ip 2>>//dev/null
				fi
				if [ "$event" = "del" ]; then
					ipset del $uciGroup $ip -exist 2>>//dev/null
				fi
				#ipset list $uciGroup >>//tmp/post_lease
				IFS=$OIFS;
				return 0	# remove to continue searching for Macs in other Hosts
			fi
		fi
	done
done
