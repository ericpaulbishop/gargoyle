#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "logread" -c "internal.css" -j "logread.js" -z "logread.js"

%>

<fieldset id="logread">
	<legend class="sectionheader"><%~ logread.SLogs %></legend>
	<textarea style="width:100%" rows=30 id='output'></textarea>
</fieldset>

<div id="bottom_button_container">
	<input type='button' value='<%~ Rfsh %>' id="refresh_button" class="bottom_button" onclick='resetData()' />
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "logread"
%>
