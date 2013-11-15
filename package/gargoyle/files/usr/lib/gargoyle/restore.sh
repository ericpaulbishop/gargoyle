#!/bin/sh

# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
# version 2.0 with a special clarification/exception that permits adapting the program to 
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL. 
# See http://gargoyle-router.com/faq.html#qfoss for more information

restore_file="$1"
restore_password="$2"
if [ -z "$restore_password" ] ; then
	restore_password=0
fi

echo "Content-Type: text/html; charset=utf-8"
echo ""

echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
echo '<html xmlns="http://www.w3.org/1999/xhtml">'
echo '<body>'

if [ -e /tmp/restore_lock_file ] ; then
	echo "<script type=\"text/javascript\">top.restoreFailed();</script>"
	echo "</body></html>"
	exit
fi
touch /tmp/restore_lock_file

#test that restore file is valid
if [ ! -e "$restore_file" ] ; then
	echo "<script type=\"text/javascript\">top.restoreFailed();</script>"
	echo "</body></html>"
	rm -rf /tmp/restore_lock_file
	exit
else
	tar xzf "$restore_file" -O >/dev/null 2>error
	error=$(cat error)
	if [ -n "$error" ] ; then
		echo "<script type=\"text/javascript\">top.restoreFailed();</script>"
		echo "</body></html>"
		rm -rf /tmp/restore_lock_file
		exit
	fi
fi




if [ -e /tmp/restore ] ; then
	rm -rf /tmp/restore
fi
mkdir -p /tmp/restore	
cd /tmp/restore


#make sure all existing settings are saved to file
uci commit

# bwmon & webmon write everything when they shut down 
# we therefore need to shut them down, otherwise 
# all the new data gets over-written when we restart it 
/etc/init.d/bwmon_gargoyle stop 2>/dev/null
/etc/init.d/webmon_gargoyle stop 2>/dev/null
/etc/init.d/cron stop 2>/dev/null

# tor eats up memory, stop it before proceeding
if [ -e /etc/init.d/tor ] ; then
	/etc/init.d/tor stop 2>/dev/null
fi


mv /etc/config/gargoyle /tmp/gargoyle.bak
cp /etc/passwd /tmp/passwd
cp /etc/shadow /tmp/shadow
rm -rf /etc/rc.d/*
rm -rf /tmp/data/*
rm -rf /usr/data/*
rm -rf /etc/crontabs/*
rm -rf /etc/ethers
echo "127.0.0.1 localhost." > /etc/hosts  #overwrites old file

have_overlay=$(df | grep "overlay$" 2>/dev/null)
if [ -n "$have_overlay" ] ; then
	mkdir -p /tmp/restore/data
	tar xzf "$restore_file" -C /tmp/restore/data  2>/error
	cp -r /tmp/restore/data/* /
else
	tar xzf "$restore_file" -C / 2>error
fi
error=$(cat error)


# make sure http settings are correct for cookie-based auth
uci set httpd_gargoyle.server.default_page_file="overview.sh" 2>/dev/null
uci set httpd_gargoyle.server.page_not_found_file="login.sh" 2>/dev/null
uci set httpd_gargoyle.server.no_password="1" 2>/dev/null
uci del httpd_gargoyle.server.default_realm_name 2>/dev/null
uci del httpd_gargoyle.server.default_realm_password_file 2>/dev/null
uci commit;


# set proper gargoyle visibility
old_timeout=$(uci get "gargoyle.global.session_timeout")
old_rwp=$(uci get "gargoyle.global.require_web_password")
old_hd1=$(uci get "gargoyle.help.ddns_1")
old_hqu1=$(uci get "gargoyle.help.qos_up_1")
old_hqu2=$(uci get "gargoyle.help.qos_up_2")
old_hqd1=$(uci get "gargoyle.help.qos_down_1")
old_hqd2=$(uci get "gargoyle.help.qos_down_2")
cp /tmp/gargoyle.bak /etc/config/gargoyle
if [ -n "$old_timeout" ] ; then
	uci set gargoyle.global.session_timeout=$old_timeout
fi
if [ -n "$old_rwp" ] ; then
	uci set gargoyle.global.require_web_password=$old_rwp
fi
if [ -n "$old_hd1" ] ; then
	uci set gargoyle.help.ddns_1=$old_hd1
fi
if [ -n "$old_hqu1" ] ; then
	uci set gargoyle.help.qos_up_1=$old_hqu1
fi
if [ -n "$old_hqu2" ] ; then
	uci set gargoyle.help.qos_up_2=$old_hqu2
fi
if [ -n "$old_hqd1" ] ; then
	uci set gargoyle.help.qos_down_1=$old_hqd1
fi
if [ -n "$old_hqd2" ] ; then
	uci set gargoyle.help.qos_down_2=$old_hqd2
fi
	
is_bridge=$(echo $(uci show wireless | grep wds) $(uci show wireless | grep client_bridge))
have_ap=$(echo $(uci show wireless | grep "mode.*ap"))
if [ -n "$have_ap" ] ; then
	is_bridge=""
fi


qos_enabled=$(ls /etc/rc.d/*qos_gargoyle* 2>/dev/null)
quotas_active=""
all_quotas=$(uci show firewall | grep "=quota$" | sed 's/=.*$//' | sed 's/^.*\.//')
for q in $all_quotas ; do
	enabled=$(uci get firewall.$q.enabled)
	active="1"
	if [ "$enabled" = "0" ] ; then
		active=""
	fi
	quotas_active="$quotas_active$active"
done
if [ -z "$is_bridge" ] ; then
	uci set gargoyle.connection.dhcp=200 2>/dev/null
	uci set gargoyle.firewall.portforwarding=100 2>/dev/null
	uci set gargoyle.firewall.restriction=125 2>/dev/null
	uci set gargoyle.firewall.quotas=175 2>/dev/null
else
	uci del gargoyle.connection.dhcp 2>/dev/null
	uci del gargoyle.firewall.portforwarding 2>/dev/null
	uci del gargoyle.firewall.restriction 2>/dev/null
	uci del gargoyle.firewall.quotas 2>/dev/null
fi
if [ -n "$qos_enabled" ] ; then
	uci set gargoyle.status.qos=300 2>/dev/null
else
	uci del gargoyle.status.qos 2>/dev/null
fi
if [ -n "$quotas_active" ] ; then
	uci set gargoyle.status.quotause=225 2>/dev/null
else
	uci del gargoyle.status.quotause 2>/dev/null
fi

for qos_gargoyle_connbytes_rule in $(uci show qos_gargoyle | sed "/^qos_gargoyle\..*\.connbytes=[0-9]*$/!d; s#=.*##g"); do
	connbytes_value="$(uci get ${qos_gargoyle_connbytes_rule})"
	connbytes_kb_value="$(( ${connbytes_value} * 1024 ))"
	uci set ${qos_gargoyle_connbytes_rule}_kb=${connbytes_kb_value}
	uci del ${qos_gargoyle_connbytes_rule}
done; unset qos_gargoyle_connbytes_rule

uci commit

#if tor is around, make sure there is an entry in rc.d -- disabling should be done by uci config
if [ -e /etc/init.d/tor ] ; then 
	/etc/init.d/tor enable 
	total_mem="$(sed -e '/^MemTotal: /!d; s#MemTotal: *##; s# kB##g' /proc/meminfo)"
	if [ "$total_mem" -gt 32000 ] ; then
		uci set gargoyle.display.connection_tor="Tor"
		uci set gargoyle.scripts.connection_tor="tor.sh"
		uci set gargoyle.connection.tor="250"
		uci commit
	fi
fi


#deal with firewall include file path being swapped
cat /etc/config/firewall | sed 's/\/etc\/parse_remote_accept\.firewall/\/usr\/lib\/gargoyle_firewall_util\/gargoyle_additions\.firewall/g' >/etc/config/firewall.tmp
mv /etc/config/firewall.tmp /etc/config/firewall


cd /tmp
if [ -e /tmp/restore ] ; then
	rm -rf /tmp/restore
fi

new_ip=$(uci get network.lan.ipaddr)

#remove obsolete bandwidth monitor data, and convert/rename anything relevant
rm -rf /tmp/data/bwmon/*15m
rm -rf /tmp/data/bwmon/*15h
rm -rf /usr/data/bwmon/*15d
rm -rf /tmp/data/bwmon/*15m.bw
rm -rf /tmp/data/bwmon/*15h.bw
rm -rf /usr/data/bwmon/*15d.bw

if [ -e /usr/data/bwmon/total-upload-1y ] ; then
	bw_convert /usr/data/bwmon/total-upload-1y /usr/data/bwmon/total-upload-1y.bw
	rm -rf /usr/data/bwmon/total-upload-1y 
fi
if [ -e /usr/data/bwmon/total-download-1y ] ; then
	bw_convert /usr/data/bwmon/total-download-1y /usr/data/bwmon/total-download-1y.bw
	rm -rf /usr/data/bwmon/total-download-1y 
fi
if [ -e /usr/data/bwmon/total-upload-1y.bw ] ; then
	mv /usr/data/bwmon/total-upload-1y.bw /usr/data/bwmon/total5A-upload-day-365.bw
fi
if [ -e /usr/data/bwmon/total-download-1y.bw ] ; then
	mv /usr/data/bwmon/total-download-1y.bw /usr/data/bwmon/total5A-download-day-365.bw
fi

if [ "$restore_password" != "1" ] ; then
	cp /tmp/passwd  /etc/passwd 
	cp /tmp/shadow  /etc/shadow
elif [ "$restore_file" = "/etc/original_backup/backup.tar.gz" ] ; then
	uci set gargoyle.global.is_first_boot=1
	uci commit	
fi

rm -rf /tmp/passwd
rm -rf /tmp/shadow


#overwrite old date, then restart ntpclient and then bwmon
#this makes sure that we don't restore crontab that tries
#to save bwmon save files when rules don't exist, wiping old bwmon data
date -u  +"%Y.%m.%d-%H:%M:%S" >/usr/data/time_backup

sleep 3
rm -rf /tmp/restore_lock_file
if [ -n "$error" ] ; then
	echo "<script type=\"text/javascript\">top.restoreFailed();</script>"
else
	echo "<script type=\"text/javascript\">top.restoreSuccessful(\"$new_ip\");</script>"
fi

echo "</body></html>"


# reboot should be handled by calling function, not this script

