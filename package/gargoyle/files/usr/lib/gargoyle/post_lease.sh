# This program is copyright © 2015 John Brown and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL.
# See http://gargoyle-router.com/faq.html#qfoss for more information

# This script was written as part of the Device Groups implementation (see: /www/device.sh)
# Know Devices may be assigned to a Group (see: uci show known.device)
# An ipset is created for each Group (see: ipset list -n)
# Each new dhcp lease must update the ipset (see: uci show dnsmasq.dhcp-script)
# This script searches Know Devices for a matching MAC and will add/del the IP to/from the Group's ipset


event = $1
mac = $2
ip = $3
host = $4

OIFS=$IFS;

deviceStr=$(uci get known.device | grep 'known.device=' | awk ' -F= { print $2 ; } ');
IFS="\n";
devices = ($deviceStr);

for ((d=0; d<${#devices[@]}; ++d)); do
	device  = devices[$d];
	group = $(uci get known.$device.group);
	macStr = $(uci get known.$device.mac);
	IFS=" ";
	macs = $(macStr);

	for ((m=0; m<${#macs[@]}; ++m)); do
		if [macs[$m] = mac]; then
			if [event = "add"]; then
				ipset add $group $ip
			fi
			if [event = "old"]; then
			fi
			if [event = "del"]; then
				ipset del $group $ip -exist
			fi
		fi
	done
done


IFS=$OIFS;
