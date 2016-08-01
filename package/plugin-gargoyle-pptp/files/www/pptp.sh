#!/usr/bin/haserl
<%
	# This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "pptp" -c "internal.css" -j "pptp.js" -z "pptp.js" network firewall
%>

<script>
<%
	echo "var pptpIp=\""$(ifconfig pptp-vpnpptp 2>/dev/null | awk ' { if ( $0 ~ /inet addr:/) { gsub(/^.*:/, "", $2) ; print $2 } }')"\";"
%>
	var uci = uciOriginal.clone();
</script>

<h1 class="page-header"><%~ pptp.PCfg %></h1>
<div class="row">

	<div id="pptp_config_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ pptp.PCfg %></h3>
			</div>
			<div class="panel-body">
				<div id="pptp_config_container" class="form-group">
					<label for='pptp_config' id='pptp_config_label'><%~ PCfg %>:</label>
					<select id='pptp_config' class="form-control" onchange='setpptpVisibility()'>
						<option value='disabled'><%~ PDis %></option>
						<option value='client'><%~ PClt %></option>
					</select>
				</div>

				<div id='pptp_config_status_container' class="form-group form-inline" style="display:none">
					<span><%~ PSts %>:</span>
					<span id='pptp_config_status'></span>
				</div>
			</div>

		</div>
	</div>

	<div id="pptp_client_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ PClt %></h3>
			</div>
			<div class="panel-body">

				<div class="form-group">
					<button id="pptp_reconnect_button" class="btn btn-primary" onclick="pptpReconnect()"><%~ ReCnt %>
				</div>

				<div id='pptp_server_container' class="form-group">
					<label for='pptp_server' id='pptp_server_label'><%~ PHostNm %>:</label>
					<input type='text' name='pptp_server' id='pptp_server' class="form-control" size='20' maxlength='50' />
				</div>

				<div id='pptp_user_container' class="form-group">
					<label for='pptp_username' id='pptp_username_label'><%~ PUser %>:</label>
					<input type='text' id='pptp_username' class="form-control" size='20' maxlength='50'/>
				</div>

				<div id='pptp_password_container' class="form-group">
					<label for='pptp_password' id='pptp_password_label'><%~ PPass %>:</label>
					<input type='text' id='pptp_password' class="form-control" size='20' maxlength='50'/>
				</div>

			</div>

		</div>
	</div>

</div>
<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-danger" onclick="resetData()"><%~ Reset %></button>
</div>

<script>
	resetData()
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "pptp"
%>
