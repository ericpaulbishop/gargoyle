#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "tor" -c "internal.css"
?>


	<fieldset>
		<legend class="sectionheader">Tor Anonymization</legend>
		
		
	
	</fieldset>

<?
	gargoyle_header_footer -f -s "connection" -p "tor"
?>
