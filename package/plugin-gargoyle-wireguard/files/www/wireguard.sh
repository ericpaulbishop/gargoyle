#!/usr/bin/haserl
<%
	# This program is copyright © 2020 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "wireguard" -j "wireguard.js table.js" -z "wireguard.js" -i wireguard_gargoyle network firewall ddns_gargoyle
%>

<script>
<%
	echo "var wgIp=\""$(ifconfig wg0 2>/dev/null | awk ' { if ( $0 ~ /inet addr:/) { gsub(/^.*:/, "", $2) ; print $2 } }')"\";"
	echo "var wgStatus='"$(ifstatus wg0)"';"
%>
	var uci = uciOriginal.clone();
</script>

<h1 class="page-header"><%~ wireguard.wg %></h1>
<div class="row">
	<div id="wireguard_config_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ wgCfg %></h3>
			</div>

			<div class="panel-body">
				<div id="wireguard_config_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_config' id='wireguard_config_label'><%~ wgCfg %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='wireguard_config' onchange='setWireguardVisibility()'>
							<option value='disabled'><%~ wgDis %></option>
							<option value='client'><%~ wgClt %></option>
							<option value='server'><%~ wgSrv %></option>
						</select>
					</span>
				</div>

				<div class="row form-group" id='wireguard_config_status_container' style="display:none">
					<span class="col-xs-5"><%~ wgSts %>:</span>
					<span class="col-xs-7" id='wireguard_config_status'></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="wireguard_server_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ wgSCfg %></h3>
			</div>

			<div class="panel-body">
				<div id="wireguard_server_privkey_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_privkey' id='wireguard_server_privkey_label'><%~ wgPrivKey %>:</label>
					<span class="col-xs-7">
						<input type='password' class="form-control" name='wireguard_server_privkey' id='wireguard_server_privkey' style="width:100%" autocomplete="new-password" />
						<input type="checkbox" id="show_server_privkey" onclick="togglePass('wireguard_server_privkey')" autocomplete="off"/>
						<label for="show_server_privkey" id="show_server_privkey_label"><%~ rvel %></label>
					</span>
				</div>

				<div id="wireguard_server_pubkey_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_pubkey' id='wireguard_server_pubkey_label'><%~ wgPubKey %>:</label>
					<span class="col-xs-7">
						<input type='text' class="form-control" name='wireguard_server_pubkey' id='wireguard_server_pubkey' style="width:100%" readonly />
						<button class="btn btn-default" id="generate_keys_button" onclick="generateKeyPair('server')"><%~ keygen %></button>
					</span>
				</div>

				<div id="wireguard_server_ip_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_ip' id='wireguard_server_ip_label'><%~ wgInIP %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control" name='wireguard_server_ip' id='wireguard_server_ip' oninput='proofreadIp(this)' size='20' maxlength='15' /></span>
				</div>

				<div id="wireguard_server_mask_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_mask' id='wireguard_server_mask_label'><%~ wgIMsk %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control" name='wireguard_server_mask' id='wireguard_server_mask' oninput='proofreadMask(this)' size='20' maxlength='15' /></span>
				</div>

				<div id="wireguard_server_port_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_port' id='wireguard_server_port_label'><%~ wgPrt %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control"  id='wireguard_server_port'  size='20' maxlength='5' oninput='proofreadPort(this)'/><br/></span>
				</div>

				<div id="wireguard_server_client_to_client_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_client_to_client' id='wireguard_server_client_to_client_label'><%~ CCTr %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='wireguard_server_client_to_client'>
							<option value='true'><%~ CtoC %></option>
							<option value='false'><%~ CtoS %></option>
						</select>
					</span>
				</div>

				<div id="wireguard_server_subnet_access_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_subnet_access' id='wireguard_server_subnet_access_label'><%~ LSAc %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='wireguard_server_subnet_access'>
							<option value='true'><%~ CtoH %></option>
							<option value='false'><%~ CnoL %></option>
						</select>
					</span>
				</div>

				<div id="wireguard_server_redirect_gateway_container" class="row form-group">
					<label class="col-xs-5" for='wireguard_server_redirect_gateway' id='wireguard_server_redirect_gateway_label'><%~ CUse %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='wireguard_server_redirect_gateway'>
							<option value='true'><%~ ATrff %></option>
							<option value='false'><%~ HTrff %></option>
						</select>
					</span>
				</div>
			</div>
		</div>
	</div>

	<div id="wireguard_allowed_client_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ wgSAC %></h3>
			</div>

			<div class="panel-body">
				<label class="col-xs-12" id="wireguard_allowed_client_add_label" style="text-decoration:underline"><p><%~ CfgClnt %>:</p></label>

				<div class="row form-group">
					<div class="col-xs-12">
						<button id='wireguard_allowed_client_add' class='btn btn-default btn-add' onclick='addWgClientModal()'><%~ Add %></button>
					</div>
				</div>

				<div class='internal_divider'></div>

				<label class="col-xs-12" id="wireguard_allowed_client_table_label" style="text-decoration:underline"><p><%~ CClnt %>:</p></label>

				<div id="wireguard_allowed_client_table_container" class="table table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="wireguard_client_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ wgCCfg %></h3>
			</div>

			<div class="panel-body">
				<form id='wireguard_client_form' enctype="multipart/form-data" method="post" action="utility/wireguard_upload_client.sh" target="client_add_target">
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" id="wireguard_client_config_upload" name="client_config_mode" onclick="setClientVisibility(document)" />
							<label for="wireguard_client_config_upload"><%~ UpCfgF %></label>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" id="wireguard_client_config_manual" name="client_config_mode" onclick="setClientVisibility(document)" />
							<label for="wireguard_client_config_manual"><%~ CfgMan %></label>
						</span>
					</div>

					<div id="wireguard_client_upload_config">
						<div id="wireguard_client_config_file_container" class="row form-group">
							<label class="col-xs-5" id="wireguard_client_config_file_label" for="wireguard_client_config_file"><%~ CfgF %>:</label>
							<span class="col-xs-7"><input type="file" id="wireguard_client_config_file" name="wireguard_client_config_file" /></span>
						</div>

						<div class="row form-group">
							<span class="col-xs-12"><button id="upload_button" class="btn btn-primary btn-lg" onclick="doUpload()"><%~ UploadCfg %></button></span>
						</div>
					</div>

					<div id="wireguard_client_manual_config">
						<div id="wireguard_client_server_pubkey_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_server_pubkey' id='wireguard_client_server_pubkey_label'><%~ ServerPubKey %>:</label>
							<span class="col-xs-7">
								<input type='text' class="form-control" name='wireguard_client_server_pubkey' id='wireguard_client_server_pubkey' style="width:100%" />
							</span>
						</div>

						<div id="wireguard_client_server_host_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_server_host' id='wireguard_client_server_host_label'><%~ ServerHost %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control"  id='wireguard_client_server_host'  size='20' /><br/></span>
						</div>

						<div id="wireguard_client_server_port_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_server_port' id='wireguard_client_server_port_label'><%~ ServerPort %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control"  id='wireguard_client_server_port'  size='20' maxlength='5' oninput='proofreadPort(this)'/><br/></span>
						</div>

						<div class="internal_divider"></div>

						<div id="wireguard_client_privkey_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_privkey' id='wireguard_client_privkey_label'><%~ wgPrivKey %>:</label>
							<span class="col-xs-7">
								<input type='password' class="form-control" name='wireguard_client_privkey' id='wireguard_client_privkey' style="width:100%" autocomplete="new-password" />
								<input type="checkbox" id="show_client_privkey" onclick="togglePass('wireguard_client_privkey')" autocomplete="off"/>
								<label for="show_client_privkey" id="show_client_privkey_label"><%~ rvel %></label>
							</span>
						</div>

						<div id="wireguard_client_pubkey_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_pubkey' id='wireguard_client_pubkey_label'><%~ wgPubKey %>:</label>
							<span class="col-xs-7">
								<input type='text' class="form-control" name='wireguard_client_pubkey' id='wireguard_client_pubkey' style="width:100%" readonly />
								<button class="btn btn-default" id="generate_keys_button" onclick="generateKeyPair('client')"><%~ keygen %></button>
							</span>
						</div>

						<div id="wireguard_client_ip_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_ip' id='wireguard_client_ip_label'><%~ wgInIP %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control" name='wireguard_client_ip' id='wireguard_client_ip' oninput='proofreadIp(this)' size='20' maxlength='15' /></span>
						</div>

						<div id="wireguard_client_allow_nonwg_traffic_container" class="row form-group">
							<label class="col-xs-5" for='wireguard_client_allow_nonwg_traffic' id='wireguard_client_allow_nonwg_traffic_label'><%~ NOWGT %>:</label>
							<span class="col-xs-7">
								<select class="form-control" id='wireguard_client_allow_nonwg_traffic'>
									<option value='true'><%~ AllowNOWGT %></option>
									<option value='false'><%~ BlockNOWGT %></option>
								</select>
							</span>
							<span class="col-xs-12"><em><%~ DescNOWGT %></em></span>
						</div>

						<div id="wireguard_client_allowed_ips_container" class="row form-group" style="display: none">
							<label class="col-xs-5" for='wireguard_client_ip' id='wireguard_client_allowed_ips_label'><%~ wgInIP %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control" name='wireguard_client_allowed_ips' id='wireguard_client_allowed_ips' /></span>
						</div>
					</div>

					<input style="display:none" type="hidden" id="wireguard_client_hash" name="hash"></input>
				</form>

				<iframe id="client_add_target" name="client_add_target" src="#" style="display:none"></iframe>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="wireguard_allowed_client_modal" aria-hidden="true" aria-labelledby="wireguard_allowed_client_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="wireguard_allowed_client_modal_title" class="panel-title"><%~ CfgCred %></h3>
			</div>
			<div class="modal-body">
				<%in templates/wireguard_allowed_client_template %>
			</div>
			<div class="modal-footer" id="wireguard_allowed_client_modal_button_container">
			</div>
		</div>
	</div>
</div>

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "wireguard"
%>

