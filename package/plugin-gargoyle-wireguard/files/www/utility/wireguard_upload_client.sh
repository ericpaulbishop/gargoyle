#!/usr/bin/haserl --upload-limit=1048576 --upload-dir=/tmp/
<?
	# This program is copyright © 2020 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )

	echo "Content-Type: text/html; charset=utf-8"
	echo ""

	echo '<!DOCTYPE html>'
	echo '<body>'

	mkdir -p /tmp/wgcfg
	mv $FORM_wireguard_client_config_file /tmp/wgcfg/upcfg

	cd /tmp/wgcfg/
	echo "<span id=\"cfgcontents\">$(cat upcfg)</span>"
	rm -rf /tmp/wgcfg

	echo "<script type=\"text/javascript\">top.uploaded();</script>"

	echo "</body></html>"
?>

