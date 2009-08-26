#!/usr/bin/haserl --upload-limit=5120 --upload-dir=/tmp/
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	


	echo "Content-type: text/html"
	echo ""

	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
	echo '<html xmlns="http://www.w3.org/1999/xhtml">'
	echo '<body>'

	if [ -e /tmp/restore ] ; then
		rm -rf /tmp/restore
	fi
	mkdir -p /tmp/restore	
	mv $FORM_restore_file /tmp/restore/restore.tar.gz
	cd /tmp/restore

	#make sure all existing settings are saved to file
	uci commit

	# bwmon & webmon write everything when they shut down
	# we therefore need to shut them down, otherwise
	# all the new data gets over-written when we restart it
	/etc/init.d/bwmon_gargoyle stop 2>/dev/null
	/etc/init.d/webmon_gargoyle stop 2>/dev/null
	
	mv /etc/config/gargoyle /tmp/gargoyle.bak
	rm -rf /etc/config/*
	rm -rf /etc/rc.d/*
	rm -rf /tmp/data/*
	rm -rf /usr/data/*
	rm -rf /etc/crontabs/*
	rm -rf /etc/hosts
	rm -rf /etc/ethers

	tar xzf restore.tar.gz -C / 2>error
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
	uci commit

	#deal with firewall include file path being swapped
	cat /etc/config/firewall | sed 's/\/etc\/parse_remote_accept\.firewall/\/usr\/lib\/gargoyle_firewall_util\/gargoyle_additions\.firewall/g' >/etc/config/firewall.tmp
	mv /etc/config/firewall.tmp /etc/config/firewall


	cd /tmp
	if [ -e /tmp/restore ] ; then
		rm -rf /tmp/restore
	fi

	new_ip=$(uci get network.lan.ipaddr)

	#overwrite old date, then restart ntpclient and then bwmon
	#this makes sure that we don't restore crontab that tries
	#to save bwmon save files when rules don't exist, wiping old bwmon data
	date -u  +"%Y.%m.%d-%H:%M:%S" >/usr/data/time_backup
	/etc/init.d/ntpclient restart 2>/dev/null
	/etc/init.d/bwmon_gargoyle start 2>/dev/null


	if [ -n "$error" ] ; then
		echo "<script type=\"text/javascript\">top.restoreFailed();</script>"
	else
		echo "<script type=\"text/javascript\">top.restoreSuccessful(\"$new_ip\");</script>"
	fi

	echo "</body></html>"
?>
