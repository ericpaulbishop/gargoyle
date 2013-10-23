#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "webshell" -c "internal.css" -j "webshell.js"
%>

<fieldset id="webshell">
	<legend class="sectionheader"><%~ webshell.Webs %></legend>
	<label class='leftcolumn' for='cmd' id='cmd_label'><%~ Cmd %>:</label>
	<span class="rightcolumn">
		<input id="cmd" class="rightcolumn" onkeydown='checkKey(event)' type="text" size='80'/>
		<input type='button' class='default_button' id='cmd_button' value='<%~ Exe %>' onclick='runCmd()' />
	</span>
	<span class="nocolumn"><em><%~ CmdWarn %></em></span>
	<textarea style="width:100%" rows=30 id='output'></textarea>
</fieldset>

<%
	gargoyle_header_footer -f -s "system" -p "webshell"
%>
