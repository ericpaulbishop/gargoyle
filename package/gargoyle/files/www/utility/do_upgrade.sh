#!/usr/bin/haserl --upload-limit=1048576 --upload-dir=/tmp/
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )

	echo "Content-Type: text/html; charset=utf-8"
	echo ""

	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
	echo '<html xmlns="http://www.w3.org/1999/xhtml">'
	echo '<body>'

	mkdir -p /tmp/up
	mv $FORM_upgrade_file /tmp/up/upgrade

	cd /tmp/up/
	if [ -e /usr/bin/bin2trx ] ; then
		/usr/bin/bin2trx /tmp/up/upgrade >/tmp/up/trxtest 2>&1
		trx_test=$(cat /tmp/up/trxtest)
		if [ -n "$trx_test" ] ; then
			echo "<script type=\"text/javascript\">top.failure();</script>"
			echo "</body></html>"
			exit
		else
			echo "<script type=\"text/javascript\">top.uploaded();</script>"
			#make sure output buffer gets flushed by writing lots of whitespace to output
			inc=1
			while [ $inc -lt 500 ] ; do
				echo "     "
				inc=$(($inc + 1))
			done

			#tor eats up memory like crazy and can make an upgrade crash, kill it if it exists
			if [ -e /etc/init.d/tor ] ; then
				/etc/init.d/tor stop >/dev/null 2>&1
			fi
			mtd write upgrade linux ; echo "<script type=\"text/javascript\">top.upgraded();</script></body></html>" ; reboot
		fi
	else

		is_tplink=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo | grep "TL\-[WM][DR]")
		if [ -n "$is_tplink" ]; then
			boot_size=$(dd bs=4 count=1 skip=37 if=/tmp/up/upgrade 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"')
			[ "$boot_size" != "00000000" ] && {
				# Invalid image, it contains a bootloader
				echo "<script type=\"text/javascript\">top.failureByBootloader();</script>"
				echo "</body></html>"
				rm /tmp/up/upgrade
				exit
			}
		fi

		echo "<script type=\"text/javascript\">top.uploaded();</script>"
		#make sure output buffer gets flushed by writing lots of whitespace to output
		inc=1
		while [ $inc -lt 500 ] ; do
			echo "     "
			inc=$(($inc + 1))
		done

		#tor eats up memory like crazy and can make an upgrade crash, kill it if it exists
		if [ -e /etc/init.d/tor ] ; then
			/etc/init.d/tor stop >/dev/null 2>&1
		fi

		#if attempting to preserve settings, don't pass -n
		if [ "$FORM_upgrade_preserve" = "on" ] ; then
			/sbin/sysupgrade    /tmp/up/upgrade  2>&1 | awk ' $0 ~ /eboot/ { print "<script type=\"text/javascript\">top.upgraded();</script></body></html>" ; } '
		else
			/sbin/sysupgrade -n /tmp/up/upgrade  2>&1 | awk ' $0 ~ /eboot/ { print "<script type=\"text/javascript\">top.upgraded();</script></body></html>" ; } '
		fi
	fi
?>
