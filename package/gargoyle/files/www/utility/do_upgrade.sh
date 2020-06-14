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

	echo '<!DOCTYPE html>'
	echo '<body>'

	mkdir -p /tmp/up
	mv $FORM_upgrade_file /tmp/up/upgrade

	cd /tmp/up/
	fwtool -i fwtool.json upgrade
	echo "<span id=\"fwtool\">$(cat fwtool.json)</span>"
	. /usr/share/libubox/jshn.sh
	json_load "$(/usr/libexec/validate_firmware_image upgrade)" || {
		# Image check failed
		echo "<script type=\"text/javascript\">top.failureByBootloader();</script>"
		echo "</body></html>"
		rm -rf /tmp/up
		exit
	}
	json_dump > validate_firmware_image.json
	echo "<span id=\"validate_firmware_image\">$(cat validate_firmware_image.json)</span>"

	md5sum upgrade 2>/dev/null | cut -d ' ' -f 1 > hash.md5
	sha1sum upgrade 2>/dev/null | cut -d ' ' -f 1 > hash.sha1
	sha256sum upgrade 2>/dev/null | cut -d ' ' -f 1 > hash.sha256
	echo "<span id=\"md5sum\">$(cat hash.md5)</span>"
	echo "<span id=\"sha1sum\">$(cat hash.sha1)</span>"
	echo "<span id=\"sha256sum\">$(cat hash.sha256)</span>"

	echo "<span id=\"firmware_hash\">$FORM_firmware_hash</span>"

	echo "<span id=\"firmware_size\">$(wc -c upgrade | cut -f1 -d' ')</span>"

	is_tplink=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo | grep "TL\-[WM][DR]")
	if [ -n "$is_tplink" ]; then
		boot_size=$(dd bs=4 count=1 skip=37 if=/tmp/up/upgrade 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"')
		[ "$boot_size" != "00000000" ] && [ "$boot_size" != "ffffffff" ] && {
			# Invalid image, it contains a bootloader
			echo "<script type=\"text/javascript\">top.failureByBootloader();</script>"
			echo "</body></html>"
			rm -rf /tmp/up
			exit
		}
	fi

	echo "<script type=\"text/javascript\">top.uploaded();</script>"

	echo "</body></html>"
?>
