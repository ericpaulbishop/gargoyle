#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m  -c "internal.css"
%>

<fieldset id="edit_container">
	<legend class="sectionheader"><%~ usb_storage.ChUPass %></legend>

	<div style="clear:both;display:block">
		<label class="leftcolumn" for="share_user_text"><%~ User %>:</label>
		<span class="rightcolumn" id="share_user_text"></span>
	</div>

	<div style="clear:both">
		<label class="leftcolumn" for="new_password"><%~ NPass %>:</label>
		<input class="rightcolumn" type="password" id="new_password" />
	</div>
	<div style="clear:both">
		<label class="leftcolumn" for="new_password_confirm"><%~ ChPass %>:</label>
		<input class="rightcolumn" type="password" id="new_password_confirm" />
	</div>
</fieldset>
<div id="bottom_button_container"></div>

</body>
</html>
