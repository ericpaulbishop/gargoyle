#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "access" -c "internal.css" -j "table.js access.js" -z "access.js" -i uhttpd dropbear gargoyle firewall network wireless
%>

<script>
<!--
<%
		if [ -e /etc/uhttpd.key ] && [ -e /etc/uhttpd.key ] ; then
			md5sum /etc/uhttpd.key | awk ' {print "var uhttpd_key_md5=\""$1"\";"}'
			md5sum /etc/uhttpd.crt | awk ' {print "var uhttpd_crt_md5=\""$1"\";"}'
		else
			echo "var httpsCertOK = false;"
			echo "var uhttpd_key_md5='';"
			echo "var uhttpd_crt_md5='';"
		fi

		if [ -e /usr/bin/openssl ] ; then
			echo "var opensslInstalled = true;"
		else
			echo "var opensslInstalled = false;"
		fi

		echo "var authorizedKeyMap = new Object();"
		if [ -e /etc/dropbear/authorized_keys ] ; then
			cat /etc/dropbear/authorized_keys | awk -F' ' ' $0 ~ /./ {print "authorizedKeyMap[\""$NF"\"]=\""$0"\";"}'
		fi
%>
//-->
</script>
<h1 class="page-header">Router Access</h1>
<div class="row">

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ access.Section %></h3>
			</div>
			<div class="panel-body">
				<div class="form-group form-inline">
					<label id="local_web_protocol_label" for="local_web_protocol"><%~ WebProtocol %>:</label>
					<select class="form-control" id="local_web_protocol" onchange="updateVisibility()">
						<option value="https">HTTPS</option>
						<option value="http">HTTP</option>
						<option value="both">HTTP & HTTPS</option>
					</select>
				</div>
				<div>
					<div id="local_http_port_container" class="form-group form-inline">
						<label class="leftcolumn" for="local_http_port" id="local_http_port_label"><%~ LocalPort %>:</label>
						<input type="text" class="form-control" id="local_http_port" size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
					</div>

					<div id="local_https_port_container" class="form-group form-inline">
						<label for="local_https_port" id="local_https_port_label"><%~ Local_S_Port %>:</label>
						<input type="text" class="form-control" id="local_https_port" size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
					</div>
				</div>

				<div id="remote_web_protocol_container" class="form-group form-inline">
					<label id="remote_web_protocol_label" for="remote_web_protocol"><%~ RemoteWebAccess %>:</label>
					<select class="form-control" id="remote_web_protocol" onchange="updateVisibility()">
						<option value="disabled"><%~ disabled %></option>
						<option value="https">HTTPS</option>
						<option value="http">HTTP</option>
						<option value="both">HTTP & HTTPS</option>
					</select>
				</div>

				<div id="remote_web_ports_container">
					<div id="remote_http_port_container" class="form-group form-inline">
						<label for="remote_http_port" id="remote_http_port_label"><%~ RemotePort %>:</label>
						<input type="text" class="form-control" id="remote_http_port" size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
					</div>

					<div id="remote_https_port_container" class="form-group form-inline">
						<label for="remote_https_port" id="remote_https_port_label"><%~ Remote_S_Port %>:</label>
						<input type="text" class="form-control" id="remote_https_port" size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
					</div>
				</div>

				<div id="session_length_container" class="form-group form-inline">
					<label id="session_length_label" for="session_length"><%~ Session %>:</label>
					<select class="form-control" id="session_length">
						<option value="15">15 <%~ minutes %></option>
						<option value="30">30 <%~ minutes %></option>
						<option value="60">1 <%~ hour %></option>
						<option value="120">2 <%~ hours %></option>
						<option value="240">4 <%~ hours %></option>
						<option value="720">12 <%~ hours %></option>
						<option value="1440">24 <%~ hours %></option>
					</select>
				</div>

				<div class="form-group form-inline">
					<input type="checkbox" id="disable_web_password"/>
					<label id="disable_web_password_label" for="disable_web_password"><%~ DisablePassword %></label> <em>(<%~ warning %>)</em>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SSHAccess %></h3>
			</div>
			<div class="panel-body">

			<div class="form-group form-inline">
				<label for="local_ssh_port" id="local_ssh_port_label"><%~ LocalSSHPort %>:</label>
				<input type="text" class="form-control" id="local_ssh_port" size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
			</div>

			<div class="form-group form-inline" id="remote_ssh_enabled_container">
				<input type="checkbox" id="remote_ssh_enabled" onclick="updateVisibility()"/>
				<label id="remote_ssh_enabled_label" for="remote_ssh_enabled"><%~ EnableRemoteAccess %></label>
			</div>

			<div class="form-group form-inline" id="remote_ssh_port_container">
				<label for="remote_ssh_port" id="remote_ssh_port_label"><%~ RemoteSSHPort %>:</label>
				<input type="text" class="form-control" id="remote_ssh_port"  size="7" maxlength="5" onkeyup="proofreadNumericRange(this,1,65535)"/>
			</div>

			<div class="form-group form-inline" id="remote_ssh_attempts_container">
				<label for="remote_ssh_attempts" id="remote_ssh_attempts_label"><%~ MaxRemoteTries %>:</label>
				<select class="form-control" id="remote_ssh_attempts">
					<option value="1">1 <%~ Attempts %></option>
					<option value="3">3 <%~ Attempts %></option>
					<option value="5">5 <%~ Attempts %></option>
					<option value="10">10 <%~ Attempts %></option>
					<option value="15">15 <%~ Attempts %></option>
					<option value="unlimited"><%~ Unlimited %></option>
				</select>
			</div>

			<div style="display: block;" id="internal_divider1" class="internal_divider"></div>
			<div class="form-group form-inline" id="pwd_enabled_container">
				<input type="checkbox" id="pwd_auth_enabled" />
				<label id="pwd_auth_label" for="pwd_auth_enabled"><%~ SSHEnablePwd %></label>
			</div>

			<div style="display: block;" id="internal_divider2" class="internal_divider"></div>

			<div class="form-group form-inline" id="authorized_keys_container">
				<label for="add_key" id="add_key_label"><%~ SSHExistKey %>:</label>
				<input type="file" id="public_key_file" name="public_key_file" class="form-control" />
				<input id="file_contents" name="file_contents" type="hidden" value="" />
			</div>

			<div class="form-group form-inline" id="key_name_container">
				<label><%~ SSHName %>:</label>
				<input type="text" id="public_key_name" name="public_key_name" value="" class="form-control" />
				<button class="btn btn-default" id="add_key" name="add_key" onclick="addKey()"><%~ Add %></button>
			</div>

			<div class="form-group form-inline" id="authorised_keys_table_container">
				<label id="authorized_keys_label" class="leftcolumnonly" for="authorized_keys_table_container"><%~ SSHKeys %>:</label>
				<div id="authorized_keys_table_container" class="indent table-responsive"></div>
				<div id="ssh_help" class="indent">
					<span id="ssh_help_txt" style="display:none">
						<p><%~ SSHHelp1 %></p>
						<p><%~ SSHHelp2 %></p>
						<p><%~ SSHHelp3 %></p>
						<ul>
							<li><%~ SSHHelp3a %></li>
							<li><%~ SSHHelp3b %></li>
						</ul>
						<p><%~ SSHHelp4 %></p>
					</span>
					<a onclick="setDescriptionVisibility('ssh_help')"  id="ssh_help_ref" href="#ssh_help"><%~ MoreInfo %></a>
				</div>
			</div>

		</div>
	</div>
	</div>

	<div class="col-lg-4">
			<div class="panel panel-default">
				<div class="panel-heading">
					<h3 class="panel-title"><%~ ChangePass %></h3>
				</div>
			<div class="panel-body">
			<div class="form-group form-inline">
				<label for="password1" id="password1_label"><%~ NewPass %>:</label>
				<input type="password" class="form-control" id="password1" size="25"/>
			</div>

			<div class="form-group form-inline">
				<label for="password2" id="password2_label"><%~ ConfirmPass %>:</label>
				<input type="password" class="form-control" id="password2" size="25"/>
			</div>
			</div>
		</div>
	</div>

</div>
<div id="bottom_button_container" class="form-group form-inline">
	<button id="save_button" class="btn btn-info" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick="resetData()"/><%~ Reset %></button>
</div>

<span id="update_container"><%~ WaitSettings %></span>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>
<%
	gargoyle_header_footer -f -s "system" -p "access"
%>

