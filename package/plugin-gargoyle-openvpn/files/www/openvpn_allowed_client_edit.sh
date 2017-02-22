#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m  -c "internal.css" -j "openvpn.js" -z "openvpn.js"
%>

<div class="row">
	<div id="edit_container" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ openvpn.EditOCS %></h3>
			</div>

			<div class="panel-body">
				<%in /www/templates/openvpn_allowed_client_template %>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default"></div>

</body>
</html>
