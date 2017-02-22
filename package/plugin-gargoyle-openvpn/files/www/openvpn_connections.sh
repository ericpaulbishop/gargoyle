#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "openvpn_connections" -c "internal.css" -j "openvpn_connections.js table.js" -z "openvpn.js" openvpn_gargoyle
%>

<script>
var statusFileLines = []

<%

if [ -e /etc/openvpn/current_status ] ; then
	awk '{print "statusFileLines.push(\""$0"\");" ; }' /etc/openvpn/current_status
fi

%>
</script>

<h1 class="page-header"><%~ openvpn.ConnOC %></h1>
<div class="row">
	<div id="openvpn_server_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"<%~ openvpn.ConnOC %></h3>
			</div>

			<div class="panel-body">
				<div id="openvpn_connection_table_container"></div>
			</div>
		</div>
	</div>
</div>

<script>
	resetData()
</script>

<%
	gargoyle_header_footer -f -s "status" -p "openvpn_connections"
%>
