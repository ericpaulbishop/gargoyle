#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m  -c "internal.css" -j "ddns.js" -z "ddns.js"
%>
<fieldset id="edit_container">
	<legend class="sectionheader"><%~ ddns.EDSect %></legend>

	<div>
		<div>
			<label class='leftcolumn' for='ddns_provider' id='ddns_provider_label'><%~ SvPro %>:</label>
			<span class='rightcolumn' id="ddns_provider_text" ></span>
		</div>
	</div>

	<div id="ddns_variable_container"></div>

	<div>
		<div>
			<label class='leftcolumn' for='ddns_check' id='ddns_check_label'><%~ ChItv %>:</label>
			<input type='text' class='rightcolumn' id='ddns_check'  size='8' onkeyup='proofreadNumeric(this)'/>
			<em><%~ minutes %></em>
			<div class='indent'>
				<p><%~ HelpCI %></p>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='ddns_force' id='ddns_force_label'><%~ FUItv %>:</label>
			<input type='text' class='rightcolumn' id='ddns_force'  size='8' onkeyup='proofreadNumeric(this)'/>
			<em><%~ days %></em>
			<div class='indent'>
				<p><%~ HelpFI %></p>
			</div>
		</div>
	</div>

</fieldset>
<div id="bottom_button_container"></div>

</body>
</html>
