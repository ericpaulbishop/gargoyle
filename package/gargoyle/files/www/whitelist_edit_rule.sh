#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m  -c "internal.css" -j "restrictions.js table.js" -z "restrictions.js"
%>

<div id="edit_container" class="row">

	<div class="col-lg-4">
		<div class="panel panel-default">
		<div class="panel-heading">
			<h3 class="panel-title"><%~ restrictions.EESect %></h3>
		</div>
		<div class="panel-body">

	<div>
		<%in templates/whitelist_template %>
	</div>

</div>
</div>
</div>
</div>
<div id="bottom_button_container"></div>

</body>
</html>
