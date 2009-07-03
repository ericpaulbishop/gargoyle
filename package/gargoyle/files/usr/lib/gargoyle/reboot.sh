# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

webmon_enabled=$(ls /etc/rc.d/*webmon_gargoyle 2>/dev/null)
bwmon_enabled=$(ls /etc/rc.d/*bwmon_gargoyle 2>/dev/null)

if [ -n "$webmon_enabled" ] ; then
	/etc/init.d/webmon_gargoyle stop >/dev/null 2>&1
fi
if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle stop >/dev/null 2>&1
fi
backup_quotas >/dev/null 2>&1
date -u  +"%Y.%m.%d-%H:%M:%S" >/usr/data/time_backup
reboot
