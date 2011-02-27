#!/usr/bin/haserl --upload-limit=8192 --upload-dir=/tmp/
<?
	# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
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
	
	mkdir -p /tmp/up	
	mv $FORM_upgrade_file /tmp/up/upgrade
	

	cd /tmp/up/
	echo "<script type=\"text/javascript\">top.uploaded();</script>"
	
	/sbin/sysupgrade -n /tmp/up/upgrade  2>&1 | awk ' $0 ~ /eboot/ { print "<script type=\"text/javascript\">top.upgraded();</script></body></html>" ; } '
?>
