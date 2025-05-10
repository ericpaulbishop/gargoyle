#!/bin/sh

. /lib/functions.sh

outfile=/tmp/dhcpv4configleases.gargoyle

handle_host() {
	local cfg="$1"
	local hostname ip mac hostid duid

	config_get hostname "$cfg" name
	config_get ip "$cfg" ip
	config_get mac "$cfg" mac
	config_get hostid "$cfg" hostid
	config_get duid "$cfg" duid

	echo "$mac $ip $hostname $hostid $duid" >> $outfile
}

rm -f $outfile
touch $outfile
config_load dhcp
config_foreach handle_host host
