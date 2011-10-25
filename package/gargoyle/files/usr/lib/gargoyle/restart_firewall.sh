# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

. /usr/lib/gargoyle/libgargoylehelper.sh

# Work in progress note:
# Can we stop them only if present in rc.d (enabled to autostart)? 
# Should speedup things if some servers aren't enabled.
/etc/init.d/webmon_gargoyle stop >/dev/null 2>&1
/etc/init.d/bwmon_gargoyle stop >/dev/null 2>&1
/etc/init.d/miniupnpd stop >/dev/null 2>&1
/etc/init.d/qos_gargoyle stop >/dev/null 2>&1

backup_quotas

/etc/init.d/firewall stop >/dev/null 2>&1
/etc/init.d/firewall start >/dev/null 2>&1

. /usr/lib/gargoyle_firewall_util/gargoyle_firewall_util.sh
ifup_firewall

service_enabled qos_gargoyle	&& /etc/init.d/qos_gargoyle start
service_enabled miniupnpd	&& /etc/init.d/miniupnpd start
service_enabled bwmon_gargoyle	&& /etc/init.d/bwmon_gargoyle start
service_enabled webmon_gargoyle	&& /etc/init.d/webmon_gargoyle start

