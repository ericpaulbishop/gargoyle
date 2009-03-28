# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

# This script will effictively restart things if the wifi chip is brcm
# but things get completely FUBAR if the wifi is atheros
# If the wifi is Atheros, the router HAS to be restarted

# for some reason network interfaces are not all down
# after running "/etc/init.d/network stop",
# so we have to explicitly take all of them down 
# after stopping the network
# However, if vlan is active it can be problematic to take
# the switch interface down, so just take down the sub-interfaces

switch_if=$(uci show network | grep switch | sed "s/network\.//g" | sed "s/\=.*//g")
vlan_active=$(cat /proc/net/dev | grep "$switch_if\.")
ifs=$(cat /proc/net/dev 2>/dev/null | awk 'BEGIN {FS = ":"}; $0 ~ /:/ { print $1 }')


dnsmasq_enabled=$(ls /etc/rc.d/*dnsmasq 2>/dev/null)
restricter_enabled=$(ls /etc/rc.d/*restricter_gargoyle 2>/dev/null)
bwmon_enabled=$(ls /etc/rc.d/*bwmon_gargoyle 2>/dev/null)
qos_enabled=$(ls /etc/rc.d/*qos_gargoyle 2>/dev/null)


#stop firewall,dnsmasq,qos,bwmon
if [ -n "$restricter_enabled" ] ; then
	/etc/init.d/restricter_gargoyle stop >/dev/null 2>&1
fi
if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle stop >/dev/null 2>&1
fi
if [ -n "$qos_enabled" ] ; then
	/etc/init.d/qos_gargoyle stop >/dev/null 2>&1
fi
/etc/init.d/firewall stop >/dev/null 2>&1 
/etc/init.d/dnsmasq stop >/dev/null 2>&1 



/etc/init.d/network stop >/dev/null 2>&1 

for i in $ifs ; do
	is_switch=$(echo "$i" | egrep "$switch_if[^\.]*$")
	if [ -z "$is_switch" ] || [ -z "$vlan_active" ] ; then
		ifconfig $i down 2>/dev/null
	fi
done

/etc/init.d/network start >/dev/null 2>&1 


#restart everything
/etc/init.d/firewall start >/dev/null 2>&1
if [ -n "$qos_enabled" ] ; then
	/etc/init.d/qos_gargoyle start
fi
if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle start
fi
if [ -n "$restricter_enabled" ] ; then
	/etc/init.d/restricter_gargoyle start
fi
if [ -n "$dnsmasq_enabled" ] ; then
	/etc/init.d/dnsmasq start >/dev/null 2>&1 
fi

