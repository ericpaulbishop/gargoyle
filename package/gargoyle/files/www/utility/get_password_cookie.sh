#!/usr/bin/haserl 
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information



	echo "Content-type: text/plain"
	echo ""
	#prevent brute force attacks by forcing this script to take at least 1 second
	sleep 1
	eval $( gargoyle_session_validator -p "$POST_password" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )
?>
