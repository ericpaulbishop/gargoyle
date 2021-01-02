#!/bin/sh

#Block ads, malware, etc.

#### CONFIG SECTION ####
# Try to transparently serve pixel response?
TRANS=`uci get adblock.config.trans`

# Exempt an ip range
EXEMPT=`uci get adblock.config.exempt`
START_RANGE=`uci get adblock.config.exstart`
END_RANGE=`uci get adblock.config.exend`

# Redirect endpoint
ENDPOINT_IP4=`uci get adblock.config.endpoint4`

#Change the cron command to what is comfortable, or leave as is
CRON="0 4 * * 0 sh /plugin_root/usr/lib/adblock/runadblock.sh"

#Check if Tor is enabled
TOR=`uci -q get tor.global.enabled`
#### END CONFIG ####

#### FUNCTIONS ####

cleanup()
{
	rm -f /tmp/block.build.list
	rm -f /tmp/block.build.before
}

add_config()
{
	mkdir -p "/plugin_root/adblock"
	if [ $? -ne 0 ] ; then
		logger -t ADBLOCK Unable to create folder to store ad list. Attempting to exit gracefully...
		exit 1
	else
		logger -t ADBLOCK Adblock folder has been created or already exists...
	fi

	if [ "$EXEMPT" == 1 ]
	then
		logger -t ADBLOCK Exempting some clients from ad blocking
		FW1="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -p tcp --dport 53 -j REDIRECT --to-ports 53"
		FW2="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -p udp --dport 53 -j REDIRECT --to-ports 53"
	else
		FW1="iptables -t nat -I PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports 53"
		FW2="iptables -t nat -I PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 53"
	fi
 
	#Update DHCP config
	logger -t ADBLOCK Adding hosts file to dnsmasq
	uci add_list dhcp.@dnsmasq[0].addnhosts=/plugin_root/adblock/block.hosts > /dev/null 2>&1 && uci commit

	#Add to crontab
	logger -t ADBLOCK Adding cron entry
	echo "$CRON" >> /etc/crontabs/root

	#Add firewall rules unless TOR is enabled
	if [ "${TOR:-0}" == 1 ]
	then
		logger -t ADBLOCK Tor is enabled, discarding firewall rules
	else
		logger -t ADBLOCK Adding firewall rules
		echo "$FW1" >> /etc/firewall.user
		echo "$FW2" >> /etc/firewall.user
	fi

	# Modifying uHTTPd for transparent pixel support
	if [ "$TRANS" == 1 ]
	then
		logger -t ADBLOCK Pointing uHTTPd error page at transparent pixel
		uci set uhttpd.main.error_page="/transpixel.gif" && uci commit
	fi
}

remove_config()
{ 
	# Remove addnhosts
	logger -t ADBLOCK Removing hosts file from dnsmasq
	uci del_list dhcp.@dnsmasq[0].addnhosts=/plugin_root/adblock/block.hosts > /dev/null 2>&1 && uci commit

	# Remove cron entry
	logger -t ADBLOCK Removing cron entry
	sed -i '/adblock/d' /etc/crontabs/root

	# Remove firewall rules
	logger -t ADBLOCK Removing firewall rules
	sed -i '/--to-ports 53/d' /etc/firewall.user

	# Remove pixel redirect
	logger -t ADBLOCK Reverting uHTTPd error page
	uci set uhttpd.main.error_page="/login.sh" && uci commit
}

update_blocklist()
{
	#Delete the old block.hosts to make room for the updates
	logger -t ADBLOCK Removing old hosts file
	rm -f /plugin_root/adblock/block.hosts

	#Download and process the files needed to make the lists
	logger -t ADBLOCK Retrieving ad lists from remote source
	ewget http://winhelp2002.mvps.org/hosts.txt 2>/dev/null | awk -v r="$ENDPOINT_IP4" '{sub(/^0.0.0.0/, r)} $0 ~ "^"r' > /tmp/block.build.list
	ewget https://adaway.org/hosts.txt 2>/dev/null |awk -v r="$ENDPOINT_IP4" '{sub(/^127.0.0.1/, r)} $0 ~ "^"r' >> /tmp/block.build.list

	#Check we got a hosts file
	if [ -s "/tmp/block.build.list" ]
	then
		logger -t ADBLOCK Successfully retrieved ad list
	else
		logger -t ADBLOCK Retrieve remote ad list failed. Check connection
	fi

	#Add black list, if non-empty
	logger -t ADBLOCK Adding entries from black.list
	if [ -s "/plugin_root/usr/lib/adblock/black.list" ]
	then
		awk -v r="$ENDPOINT_IP4" '{ print r,$1 }' /plugin_root/usr/lib/adblock/black.list >> /tmp/block.build.list
	fi

	#Sort the download/black lists
	awk '{sub(/\r$/,"");print $1,$2}' /tmp/block.build.list|sort -u > /tmp/block.build.before

	#Filter (if applicable)
	logger -t ADBLOCK Removing entries from white.list
	if [ -s "/plugin_root/usr/lib/adblock/white.list" ]
	then
		#Filter the blacklist, supressing whitelist matches
		egrep -v "^[[:space:]]*$" /plugin_root/usr/lib/adblock/white.list | awk '/^[^#]/ {sub(/\r$/,"");print $1}' | grep -vf - /tmp/block.build.before > /plugin_root/adblock/block.hosts
	else
		cat /tmp/block.build.before > /plugin_root/adblock/block.hosts
	fi
		
	#Record when the last time we updated the block list is
	LASTRUN=$(/usr/lib/gargoyle/current_time.sh | cut -f2 -d\")
	uci set adblock.config.lastrun="$LASTRUN" && uci commit
}

restart_firewall()
{
	logger -t ADBLOCK Restarting firewall
	/usr/lib/gargoyle/restart_firewall.sh > /dev/null 2>&1
}

restart_dnsmasq()
{
	if [ "$1" -eq "0" ]
	then
		logger -t ADBLOCK Reloading hosts list
		killall -HUP dnsmasq
	else
		logger -t ADBLOCK Restarting dnsmasq
		/etc/init.d/dnsmasq restart
	fi
}

restart_http()
{
	logger -t ADBLOCK Restarting uHTTPd
	/etc/init.d/uhttpd restart
}

#### END FUNCTIONS ####

### Options parsing ####

case "$1" in
	# Enable script
	"-enable")
		logger -t ADBLOCK Enabling adblock plugin
		remove_config
		add_config
		update_blocklist
		restart_firewall
		restart_dnsmasq 1
		restart_http
		cleanup
	;;
	# Disable script
	"-disable")
		logger -t ADBLOCK Disabling adblock plugin
		remove_config
		restart_firewall
		restart_dnsmasq 1
		restart_http
		cleanup
	;;
	#Default updates blocklist only
	*)
		update_blocklist
		restart_dnsmasq 0
		cleanup
	;;
esac

#### END OPTIONS ####

exit 0
