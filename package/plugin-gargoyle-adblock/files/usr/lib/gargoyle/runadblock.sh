#!/bin/sh

#Block ads, malware, etc.
 
#### CONFIG SECTION ####
# Only block wireless ads? Y/N
ONLY_WIRELESS=`uci get adblock.config.onlywireless`
 
# Try to transparently serve pixel response?
TRANS=`uci get adblock.config.trans`
 
# Exempt an ip range
EXEMPT=`uci get adblock.config.exempt`
START_RANGE=`uci get adblock.config.exstart`
END_RANGE=`uci get adblock.config.exend`
 
# Redirect endpoint
ENDPOINT_IP4=`uci get adblock.config.endpoint4`
 
#Change the cron command to what is comfortable, or leave as is
CRON="0 4 * * 0,3 sh /usr/lib/gargoyle/runadblock.sh"
#### END CONFIG ####
 
#### FUNCTIONS ####
 
cleanup()
{
    rm -f /tmp/block.build.list
    rm -f /tmp/block.build.before
}

add_config()
{
	if [ "$ONLY_WIRELESS" == 1 ]
    then
        logger -t ADBLOCK Only blocking ads for wireless clients
        if [ "$EXEMPT" == 1 ]
        then
            logger -t ADBLOCK Exempting some clients from ad blocking
            FW1="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -i wlan+ -p tcp --dport 53 -j REDIRECT --to-ports 53"
            FW2="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -i wlan+ -p udp --dport 53 -j REDIRECT --to-ports 53"
        else
            FW1="iptables -t nat -I PREROUTING -i wlan+ -p tcp --dport 53 -j REDIRECT --to-ports 53"
            FW2="iptables -t nat -I PREROUTING -i wlan+ -p udp --dport 53 -j REDIRECT --to-ports 53"
        fi
    else
        if [ "$EXEMPT" == 1 ]
        then
            logger -t ADBLOCK Exempting some clients from ad blocking
            FW1="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -p tcp --dport 53 -j REDIRECT --to-ports 53"
            FW2="iptables -t nat -I PREROUTING -m iprange ! --src-range $START_RANGE-$END_RANGE -p udp --dport 53 -j REDIRECT --to-ports 53"
        else
            FW1="iptables -t nat -I PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports 53"
            FW2="iptables -t nat -I PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 53"
        fi
    fi
 
    #Update DHCP config
	logger -t ADBLOCK Adding hosts file to dnsmasq
    uci add_list dhcp.@dnsmasq[0].addnhosts=/etc/block.hosts > /dev/null 2>&1 && uci commit
 
    #Add to crontab
	logger -t ADBLOCK Adding cron entry
    echo "$CRON" >> /etc/crontabs/root
 
    #Add firewall rules
	logger -t ADBLOCK Adding firewall rules
    echo "$FW1" >> /etc/firewall.user
    echo "$FW2" >> /etc/firewall.user
 
    # Determining uhttpd/httpd_gargoyle for transparent pixel support
    if [ "$TRANS" == 1 ]
    then
		logger -t ADBLOCK Pointing server error page at transparent pixel
        ENDPOINT_IP4=$(uci get network.lan.ipaddr)
        if [ ! -e "/www/1.gif" ]
        then
            /usr/bin/wget -O /www/1.gif http://upload.wikimedia.org/wikipedia/commons/c/ce/Transparent.gif  > /dev/null
        fi
        if [ -s "/usr/sbin/uhttpd" ]
        then
            uci set uhttpd.main.error_page="/1.gif" && uci commit
        elif [ -s "/usr/sbin/httpd_gargoyle" ]
        then
            uci set httpd_gargoyle.server.page_not_found_file="1.gif" && uci commit
        else
            ENDPOINT_IP4="0.0.0.0"
        fi
    fi
}

remove_config()
{ 
    # Remove addnhosts
	logger -t ADBLOCK Removing hosts file from dnsmasq
    uci del_list dhcp.@dnsmasq[0].addnhosts=/etc/block.hosts > /dev/null 2>&1 && uci commit
 
    # Remove cron entry
	logger -t ADBLOCK Removing cron entry
    sed -i '/adblock/d' /etc/crontabs/root
 
    # Remove firewall rules
	logger -t ADBLOCK Removing firewall rules
    sed -i '/--to-ports 53/d' /etc/firewall.user
 
	# Remove proxying
	logger -t ADBLOCK Reverting server error page
	if [ -s "/usr/sbin/uhttpd" ]
        then
            uci set uhttpd.main.error_page="/login.sh" && uci commit
        else
            uci set httpd_gargoyle.server.page_not_found_file="login.sh" && uci commit
    fi
}
 
update_blocklist()
{
    #Delete the old block.hosts to make room for the updates
	logger -t ADBLOCK Removing old hosts file
    rm -f /etc/block.hosts
 
    #Download and process the files needed to make the lists
	logger -t ADBLOCK Retrieving ad lists from remote source
    wget -qO- http://www.mvps.org/winhelp2002/hosts.txt| awk -v r="$ENDPOINT_IP4" '{sub(/^0.0.0.0/, r)} $0 ~ "^"r' > /tmp/block.build.list
    wget -qO- "http://adaway.org/hosts.txt"|awk -v r="$ENDPOINT_IP4" '{sub(/^127.0.0.1/, r)} $0 ~ "^"r' >> /tmp/block.build.list
 
    #Add black list, if non-empty
	logger -t ADBLOCK Adding entries from black.list
    if [ -s "/etc/black.list" ]
    then
        awk -v r="$ENDPOINT_IP4" '/^[^#]/ { print r,$1 }' /etc/black.list >> /tmp/block.build.list
    fi
 
    #Sort the download/black lists
    awk '{sub(/\r$/,"");print $1,$2}' /tmp/block.build.list|sort -u > /tmp/block.build.before
 
    #Filter (if applicable)
	logger -t ADBLOCK Removing entries from white.list
    if [ -s "/etc/white.list" ]
    then
        #Filter the blacklist, supressing whitelist matches
        #  This is relatively slow
        egrep -v "^[[:space:]]*$" /etc/white.list | awk '/^[^#]/ {sub(/\r$/,"");print $1}' | grep -vf - /tmp/block.build.before > /etc/block.hosts
    else
        cat /tmp/block.build.before > /etc/block.hosts
    fi
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
    logger -t ADBLOCK Restarting web server
	if [ -s "/usr/sbin/uhttpd" ]
    then
        /etc/init.d/uhttpd restart
    elif [ -s "/usr/sbin/httpd_gargoyle" ]
    then
        /etc/init.d/httpd_gargoyle restart
    fi
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
