#!/usr/bin/haserl --upload-limit=5120 --upload-dir=/tmp/
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) )	
	
	echo "Content-type: text/html"
	echo ""

	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
	echo '<html xmlns="http://www.w3.org/1999/xhtml">'
	echo '<body>'
	
	mkdir -p /tmp/up	
	mv $FORM_upgrade_file /tmp/up/upgrade
	mv $FORM_upgrade_file2 /tmp/up/upgrade2

	upgrade2_size=$(du /tmp/up/upgrade2 | awk ' { print $1 } ' 2>/dev/null)
	vmlinux_mtd=""
	if [ ! "$upgrade2_size" = "0" ] ; then
		vmlinux_mtd=$(cat /proc/mtd | awk 'BEGIN {FS="\"" ; } { print $2 ; }' | grep vmlinux)
		if [ -z "$vmlinux_mtd" ] ; then
			vmlinux_mtd=$(cat /proc/mtd | grep 000b0000 |  awk 'BEGIN {FS="\"" ; } { print $2 ; }')
		fi
		if [ -z "$vmlinux_mtd" ] ; then
			vmlinux_mtd=$(cat /proc/mtd | grep 000a0000 |  awk 'BEGIN {FS="\"" ; } { print $2 ; }')
		fi
		if [ -z "$vmlinux_mtd" ] ; then
			vmlinux_mtd=$(cat /proc/mtd | grep 000c0000 |  awk 'BEGIN {FS="\"" ; } { print $2 ; }')
		fi
		if [ -z "$vmlinux_mtd" ] ; then
			vmlinux_mtd=$(cat /proc/mtd | grep 000d0000 |  awk 'BEGIN {FS="\"" ; } { print $2 ; }')
		fi		
		if [ -z "$vmlinux_mtd" ] ; then
			vmlinux_mtd=$(cat /proc/mtd | grep 000e0000 |  awk 'BEGIN {FS="\"" ; } { print $2 ; }')
		fi			
		if [ -z "$vmlinux_mtd" ] ; then
			echo "<script type=\"text/javascript\">top.failure();</script>"
			echo "</body></html>"
			exit
		fi
	else
		bin2trx /tmp/up/upgrade >/tmp/up/trxtest 2>&1
		trx_test=$(cat /tmp/up/trxtest)
		if [ -n "$trx_test" ] ; then
			echo "<script type=\"text/javascript\">top.failure();</script>"
			echo "</body></html>"
			exit
		fi
	fi

	cd /tmp/up/
	echo "<script type=\"text/javascript\">top.uploaded();</script>"

	if [ ! "$upgrade2_size" = "0" ] ; then
		mtd write upgrade rootfs
		mtd write upgrade2 $vmlinux_mtd ; echo "<script type=\"text/javascript\">top.upgraded();</script></body></html>"; reboot
	else
		mtd write upgrade linux ; echo "<script type=\"text/javascript\">top.upgraded();</script></body></html>"; reboot
	fi
?>
