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

<fieldset id="pptp_config_fieldset">
	<legend class="sectionheader"><%~ pptp.PCfg %></legend>
	<div id= "pptp_config_container">
		<label class='leftcolumn' for='pptp_config' id='pptp_config_label'><%~ PCfg %>:</label>
		<select class='rightcolumn' id='pptp_config' onchange='setpptpVisibility()'>
			<option value='disabled'><%~ PDis %></option>
			<option value='client'><%~ PClt %></option>
		</select>
	</div>

	<div id='pptp_config_status_container' style="display:none" >
		<span class='leftcolumn'><%~ PSts %>:</span>
		<span class='rightcolumn' id='pptp_config_status'></span>
	</div>
</fieldset>

<fieldset id="pptp_client_fieldset">
	<legend class="sectionheader"><%~ PClt %></legend>

	<div class='rightcolumnonly' style="margin-bottom:15px">
		<input type='button' id="pptp_reconnect_button" value="<%~ ReCnt %>" class="default_button" onclick="pptpReconnect()" />
	</div>

	<div id='pptp_server_container'>
		<label class='leftcolumn' for='pptp_server' id='pptp_server_label'><%~ PHostNm %>:</label>
		<input type='text' class='rightcolumn' name='pptp_server' id='pptp_server' size='20' maxlength='50' />
	</div>

	<div id='pptp_user_container'>
		<label class='leftcolumn' for='pptp_username' id='pptp_username_label'><%~ PUser %>:</label>
		<input type='text' id='pptp_username'  size='20' maxlength='50'/>
	</div>

	<div id='pptp_password_container'>
		<label class='leftcolumn' for='pptp_password' id='pptp_password_label'><%~ PPass %>:</label>
		<input type='text' id='pptp_password'  size='20' maxlength='50'/>
	</div>

</fieldset>

<div id="bottom_button_container">
	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
</div>

<script>
	resetData()
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "pptp"
%>
