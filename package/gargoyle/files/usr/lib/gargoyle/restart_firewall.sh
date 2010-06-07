# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

miniupnpd_enabled=$(ls /etc/rc.d/*miniupnpd 2>/dev/null)
bwmon_enabled=$(ls /etc/rc.d/*bwmon_gargoyle 2>/dev/null)
qos_enabled=$(ls /etc/rc.d/*qos_gargoyle 2>/dev/null)

/etc/init.d/bwmon_gargoyle stop >/dev/null 2>&1
/etc/init.d/miniupnpd stop >/dev/null 2>&1
/etc/init.d/qos_gargoyle stop >/dev/null 2>&1

backup_quotas

/etc/init.d/firewall stop >/dev/null 2>&1
/etc/init.d/firewall start >/dev/null 2>&1
if [ -n "$qos_enabled" ] ; then
	/etc/init.d/qos_gargoyle start
fi
if [ -n "$miniupnpd_enabled" ] ; then
	/etc/init.d/miniupnpd start
fi

if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle start
fi

