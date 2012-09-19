# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information


#make sure all settings have been written to file
uci commit

#force write of webmon & bwmon
bwmon_enabled=$(ls /etc/rc.d/*bwmon* 2>/dev/null)
webmon_enabled=$(ls /etc/rc.d/*webmon* 2>/dev/null)
if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle stop
fi
if [ -n "$webmon_enabled" ] ; then
	/etc/init.d/webmon_gargoyle stop
fi

backup_locations='/etc/passwd /etc/shadow /etc/config /etc/rc.d /etc/TZ /etc/firewall.user /etc/ethers /etc/hosts /etc/webmon_ips /etc/crontabs /etc/dropbear  /tmp/data /usr/data '
existing_locations=""
for bl in $backup_locations ; do
	if [ -e "$bl" ] ; then
		existing_locations="$existing_locations $bl"
	fi
done

if [ -e /tmp/backup ] ; then
	rm -rf /tmp/backup
fi
mkdir -p /tmp/backup
cd /tmp/backup
tar cvzf backup.tar.gz $existing_locations
chmod 777 backup.tar.gz
garg_web_root=$(uci get gargoyle.global.web_root)
if [ -z "$garg_web_root" ] ; then
	garg_web_root = "/www"
fi

if [ -n "$bwmon_enabled" ] ; then
	/etc/init.d/bwmon_gargoyle start
fi
if [ -n "$webmon_enabled" ] ; then
	/etc/init.d/webmon_gargoyle start
fi


