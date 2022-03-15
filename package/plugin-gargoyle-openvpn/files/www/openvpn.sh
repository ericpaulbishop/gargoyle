#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "openvpn" -j "openvpn.js table.js" -z "openvpn.js" -i openvpn_gargoyle ddns_gargoyle uhttpd dropbear firewall tor gargoyle
%>

<script>
<%

	if [ -e /etc/openvpn/dh.pem ] ; then
		echo "var haveDh = true;"
	else
		echo "var haveDh = false;"
	fi

	client_id=$(uci get openvpn_gargoyle.@client[0].id 2>/dev/null)
	echo "var curClientConf =[]; var curClientCa = []; var curClientCert =[]; var curClientKey = []; var curClientTaKey = []; var curClientSecret = [];"
	if [ -n "$client_id" ] ; then
		if [ -f "/etc/openvpn/${client_id}.conf" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientConf.push(\""$0"\")"}' "/etc/openvpn/${client_id}.conf"      2>/dev/null
		fi
		if [ -f "/etc/openvpn/${client_id}_ca.crt" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientCa.push(\""$0"\");"}'   "/etc/openvpn/${client_id}_ca.crt"   2>/dev/null
		fi
		if [ -f "/etc/openvpn/${client_id}.crt" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientCert.push(\""$0"\");"}' "/etc/openvpn/${client_id}.crt"      2>/dev/null
		fi
		if [ -f "/etc/openvpn/${client_id}.key" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientKey.push(\""$0"\");"}'  "/etc/openvpn/${client_id}.key"      2>/dev/null
		fi
		if [ -f "/etc/openvpn/${client_id}_ta.key" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientTaKey.push(\""$0"\");"}'  "/etc/openvpn/${client_id}_ta.key"      2>/dev/null
		fi
		if [ -f "/etc/openvpn/${client_id}.secret" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientSecret.push(\""$0"\");"}'  "/etc/openvpn/${client_id}.secret"      2>/dev/null
		fi
	fi

	echo "var tunIp=\""$(ifconfig tun0 2>/dev/null | awk ' { if ( $0 ~ /inet addr:/) { gsub(/^.*:/, "", $2) ; print $2 } }')"\";"
	echo "var openvpnProc=\""$(ps | grep openvpn | grep -v grep | grep -v haserl | awk ' { printf $1 }')"\";"

	tab=$(printf "\t")
	config_file=$(uci get openvpn.custom_config.config 2>/dev/null)
	remote_ping=""
	if [ -f "$config_file" ] ; then
		remote=$(egrep "^[$tab ]*remote[$tab ]" "$config_file" | awk ' { print $2 }')
		if [ -n "$remote" ] ;then
			remote_ping=$(ping -c 1 -W 2 8.8.8.8 2>/dev/null | grep "0% packet loss")
		fi
	fi
	echo "var remotePing=\"$remote_ping\";"

%>

	var uci = uciOriginal.clone();
</script>

<h1 class="page-header">OpenVPN</h1>
<div class="row">
	<div id="openvpn_config_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ openvpn.OCfg %></h3>
			</div>

			<div class="panel-body">
				<div id="openvpn_config_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_config' id='openvpn_config_label'><%~ OCfg %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_config' onchange='setOpenvpnVisibility()'>
							<option value='disabled'><%~ ODis %></option>
							<option value='client'><%~ OClt %></option>
							<option value='server'><%~ OSrv %></option>
						</select>
					</span>
				</div>

				<div class="row form-group" id='openvpn_config_status_container' style="display:none">
					<span class="col-xs-5"><%~ OSts %>:</span>
					<span class="col-xs-7" id='openvpn_config_status'></span>
				</div>

				<div class="row form-group" id='openvpn_clear_keys_container' style='display:none'>
					<span class="col-xs-12"><button id='openvpn_clear_keys_button' class='btn btn-default' onclick='clearOpenvpnKeys()'><%~ OClrK %></button></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="openvpn_server_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ OSCfg %></h3>
			</div>

			<div class="panel-body">
				<div id="openvpn_server_ip_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_ip' id='openvpn_server_ip_label'><%~ OInIP %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control" name='openvpn_server_ip' id='openvpn_server_ip' oninput='proofreadIp(this)' size='20' maxlength='15' /></span>
				</div>

				<div id="openvpn_server_mask_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_mask' id='openvpn_server_mask_label'><%~ OIMsk %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control" name='openvpn_server_mask' id='openvpn_server_mask' oninput='proofreadMask(this)' size='20' maxlength='15' /></span>
				</div>

				<div id="openvpn_server_port_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_port' id='openvpn_server_port_label'><%~ OPrt %>:</label>
					<span class="col-xs-7"><input type='text' class="form-control"  id='openvpn_server_port'  size='20' maxlength='5' oninput='proofreadPort(this)'/><br/></span>
				</div>

				<div id="openvpn_server_protocol_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_protocol' id='openvpn_server_protocol_label'><%~ OProto %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_protocol'>
							<option value='udp'>UDP</option>
							<option value='tcp'>TCP</option>
						</select>
					</span>
				</div>

				<div id="openvpn_server_cipher_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_cipher' id='openvpn_server_cipher_label'><%~ OCiph %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_cipher'>
							<option value='AES-128-CBC'>AES-CBC 128bit</option>
							<option value='AES-256-CBC'>AES-CBC 256bit</option>
							<option value='AES-128-GCM'>AES-GCM 128bit</option>
							<option value='AES-256-GCM'>AES-GCM 256bit</option>
						</select>
					</span>
				</div>

				<div id="openvpn_server_client_to_client_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_client_to_client' id='openvpn_server_client_to_client_label'><%~ CCTr %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_client_to_client'>
							<option value='true'><%~ CtoC %></option>
							<option value='false'><%~ CtoS %></option>
						</select>
					</span>
				</div>

				<div id="openvpn_server_subnet_access_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_subnet_access' id='openvpn_server_subnet_access_label'><%~ LSAc %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_subnet_access'>
							<option value='true'><%~ CtoH %></option>
							<option value='false'><%~ CnoL %></option>
						</select>
					</span>
				</div>

				<div id="openvpn_server_duplicate_cn_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_duplicate_cn' id='openvpn_server_duplicate_cn_label'><%~ CredR %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_duplicate_cn' onchange='updateDupeCn()'>
							<option value='false'><%~ CredSC %></option>
							<option value='true'><%~ CredMC %></option>
						</select>
					</span>
				</div>

				<div id="openvpn_server_redirect_gateway_container" class="row form-group">
					<label class="col-xs-5" for='openvpn_server_redirect_gateway' id='openvpn_server_redirect_gateway_label'><%~ CUse %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id='openvpn_server_redirect_gateway'>
							<option value='true'><%~ ATrff %></option>
							<option value='false'><%~ HTrff %></option>
						</select>
					</span>
				</div>
			</div>
		</div>
	</div>

	<div id="openvpn_allowed_client_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ OSAC %></h3>
			</div>

			<div class="panel-body">
				<label class="col-xs-12" id="openvpn_allowed_client_add_label" style="text-decoration:underline"><p><%~ CfgCred %>:</p></label>

				<div class="row form-group">
					<div class="col-xs-12">
						<button id='openvpn_allowed_client_add' class='btn btn-default btn-add' onclick='addOvpnClientModal()'><%~ Add %></button>
					</div>
				</div>

				<div class='internal_divider'></div>

				<label class="col-xs-12" id="openvpn_allowed_client_table_label" style="text-decoration:underline"><p><%~ CClnt %>:</p></label>

				<div id="openvpn_allowed_client_table_container" class="table table-responsive"></div>

				<div><em><%~ ZipCred %></em></div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="openvpn_client_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ OClt %></h3>
			</div>

			<div class="panel-body">
				<form id='openvpn_client_form' enctype="multipart/form-data" method="post" action="utility/openvpn_upload_client.sh" target="client_add_target">
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" id="openvpn_client_config_upload" name="client_config_mode" onclick="setClientVisibility(document)" />
							<label for="openvpn_client_config_upload"><%~ UpCfgF %></label>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" id="openvpn_client_config_manual" name="client_config_mode" onclick="setClientVisibility(document)" />
							<label for="openvpn_client_config_manual"><%~ CfgMan %></label>
						</span>
					</div>

					<div id="openvpn_client_file_controls">
						<div id="openvpn_client_file_type_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_file_type_label" for="openvpn_client_file_type"><%~ UpFmt %>:</label>
							<span class="col-xs-7">
								<select id="openvpn_client_file_type" class="form-control" onchange="setClientVisibility(document)">
									<option value="zip"><%~ SZipF %></option>
									<option value="multi"><%~ CfgF %></option>
									<option value="single"><%~ CfgFOvpn %></option>
								</select>
							</span>
						</div>
						<div id="openvpn_client_zip_file_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_zip_file_label" for="openvpn_client_zip_file"><%~ ZipF %>:</label>
							<span class="col-xs-7"><input type="file" id="openvpn_client_zip_file" name="openvpn_client_zip_file" /></span>
						</div>

						<div id="openvpn_client_conf_file_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_conf_file_label" for="openvpn_client_conf_file"><%~ OCfgF %>:</label>
							<span class="col-xs-7"><input type="file" id="openvpn_client_conf_file" name="openvpn_client_conf_file" /></span>
						</div>

						<div id="openvpn_client_ca_file_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_ca_file_label" for="openvpn_client_ca_file"><%~ CACF %>:</label>
							<span class="col-xs-7"><input type="file" id="openvpn_client_ca_file" name="openvpn_client_ca_file" /></span>
						</div>
						<div id="openvpn_client_cert_file_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_cert_file_label" for="openvpn_client_cert_file"><%~ CCertF %>:</label>
							<span class="col-xs-7"><input type="file" id="openvpn_client_cert_file" name="openvpn_client_cert_file" /></span>
						</div>
						<div id="openvpn_client_key_file_container" class="row form-group">
							<label class="col-xs-5" id="openvpn_client_key_file_label" for="openvpn_client_key_file"><%~ CKeyF %>:</label>
							<span class="col-xs-7"><input type="file" id="openvpn_client_key_file" name="openvpn_client_key_file" /></span>
						</div>
						<div id="openvpn_client_ta_key_file_container">
							<div class="row form-group">
								<span class="col-xs-12">
									<input type='checkbox' id='openvpn_client_use_ta_key_file' name='use_ta_key_file' onclick='enableAssociatedField(this, "openvpn_client_ta_key_file", "")' />
									<label id='openvpn_client_use_ta_key_file_label' for='openvpn_client_use_ta_key_file'><%~ UseTAK %></label>
								</span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" id="openvpn_client_ta_key_file_label" for="openvpn_client_use_ta_key_file"><%~ TAKeyF %>:</label>
								<span class="col-xs-7"><input type="file" id="openvpn_client_ta_key_file" name="openvpn_client_ta_key_file" /></span>
							</div>
						</div>

						<div id="openvpn_client_auth_user_pass_container">
							<div class="row form-group">
								<span class="col-xs-12">
									<input type='checkbox' id='openvpn_client_use_auth_user_pass' name='use_auth_user_pass' onclick='enableAssociatedField(this, "openvpn_client_auth_user_pass_user", "");enableAssociatedField(this, "openvpn_client_auth_user_pass_pass","");' />
									<label id='openvpn_client_use_auth_user_pass_label' for='openvpn_client_use_auth_user_pass'><%~ UseAUP %></label>
								</span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" id="openvpn_client_auth_user_pass_user_label" for="openvpn_client_auth_user_pass_user"><%~ AUPUser %>:</label>
								<span class="col-xs-7"><input type="text" id="openvpn_client_auth_user_pass_user" name="openvpn_client_auth_user_pass_user" class="form-control" /></span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" id="openvpn_client_auth_user_pass_pass_label" for="openvpn_client_auth_user_pass_pass"><%~ AUPPass %>:</label>
								<span class="col-xs-7">
									<input type="password" id="openvpn_client_auth_user_pass_pass" name="openvpn_client_auth_user_pass_pass" class="form-control" autocomplete="off" />
									<input type="checkbox" id="show_pass" onclick="togglePass('openvpn_client_auth_user_pass_pass')" autocomplete="off">
									<label for="show_pass" id="show_pass_label"><%~ rvel %></label>
								</span>
							</div>
						</div>
					</div>

					<div id="openvpn_client_manual_controls">
						<div id='openvpn_client_remote_container' class="row form-group">
							<label class="col-xs-5" for='openvpn_client_remote' id='openvpn_client_remote_label'><%~ OSrvAddr %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control" name='openvpn_client_remote' oninput="updateClientConfigTextFromControls()" id='openvpn_client_remote' size='30' /></span>
						</div>

						<div id='openvpn_client_port_container' class="row form-group">
							<label class="col-xs-5" for='openvpn_client_port' id='openvpn_client_port_label'><%~ OSrvPrt %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control" name='openvpn_client_port' oninput="updateClientConfigTextFromControls();proofreadPort(this);" id='openvpn_client_port' size='30' /></span>
						</div>

						<div id='openvpn_client_protocol_container' class="row form-group">
							<label class="col-xs-5" for='openvpn_client_protocol' id='openvpn_client_protocol_label'><%~ OProto %>:</label>
							<span class="col-xs-7">
								<select class="form-control" onchange="updateClientConfigTextFromControls()" id='openvpn_client_protocol'>
									<option value='udp'>UDP</option>
									<option value='tcp'>TCP</option>
								</select>
							</span>
						</div>

						<div id='openvpn_client_cipher_container' class="row form-group">
							<label class="col-xs-5" for='openvpn_client_cipher' id='openvpn_client_cipher_label'><%~ OCiph %>:</label>
							<span class="col-xs-7">
								<select class="form-control" id='openvpn_client_cipher' onchange="setClientVisibility(document);updateClientConfigTextFromControls();" >
									<option value='AES-128-CBC'>AES-CBC 128bit</option>
									<option value='AES-256-CBC'>AES-CBC 256bit</option>
									<option value='AES-128-GCM'>AES-GCM 128bit</option>
									<option value='AES-256-GCM'>AES-GCM 256bit</option>
									<option value='other'><%~ Othr %></option>
								</select>
							</span>
						</div>

						<div id='openvpn_client_cipher_other_container' class="row form-group">
							<span class="col-xs-7 col-xs-offset-5"><input type='text' class="form-control" oninput="updateClientConfigTextFromControls()" id="openvpn_client_cipher_other" />&nbsp;<em><%~ Cphr %></em></span>
						</div>

						<div id='openvpn_client_block_nonovpn_container' class="row form-group">
							<label class="col-xs-5" for='openvpn_client_block_nonovpn' id='openvpn_client_block_nonovpn_label'><%~ NOVPNT %>:</label>
							<span class="col-xs-7">
								<select class="form-control" id='openvpn_client_block_nonovpn' >
									<option value='allow'><%~ AllowNOVPNT %></option>
									<option value='block'><%~ BlockNOVPNT %></option>
								</select>
							</span>
							<span class="col-xs-12"><em><%~ DescNOVPNT %></em></span>
						</div>

						<div id="openvpn_client_conf_text_container" class="row form-group">
							<br/>
							<label class="col-xs-12" for='openvpn_client_conf_text' id='openvpn_client_conf_text_label' style="text-decoration:underline"><%~ OCfg %>:</label>
							<br/>
							<span class="col-xs-12"><em><%~ CfgUpd %></em></span>
							<br/>
							<span class="col-xs-7 col-xs-offset-5"><textarea class="form-control textarea-lg" id='openvpn_client_conf_text' name='openvpn_client_conf_text' oninput='updateClientControlsFromConfigText()'></textarea></span>
						</div>

						<div id="openvpn_client_ca_text_container" class="row form-group">
							<label class="col-xs-5" for='openvpn_client_ca_text' id='openvpn_client_ca_text_label'><%~ CACert %>:</label>
							<br/>
							<span class="col-xs-7"><textarea class="form-control textarea-lg" id='openvpn_client_ca_text' name='openvpn_client_ca_text' oninput='updateClientControlsFromConfigText()'></textarea></span>
						</div>
						<div id="openvpn_client_cert_text_container" class="row form-group">
							<label class="col-xs-5" for='openvpn_client_cert_text' id='openvpn_client_cert_text_label'><%~ CCert %>:</label>
							<br/>
							<span class="col-xs-7"><textarea class="form-control textarea-lg" id='openvpn_client_cert_text' name='openvpn_client_cert_text' oninput='updateClientControlsFromConfigText()'></textarea></span>
						</div>
						<div id="openvpn_client_key_text_container" class="row form-group">
							<label class="col-xs-5" for='openvpn_client_key_text' id='openvpn_client_key_text_label'><%~ CKey %>:</label>
							<br/>
							<span class="col-xs-7"><textarea class="form-control textarea-lg" id='openvpn_client_key_text' name='openvpn_client_key_text' oninput='updateClientControlsFromConfigText()'></textarea></span>
						</div>

						<div id="openvpn_client_ta_key_text_container">
							<div class="row form-group">
								<span class="col-xs-12">
									<input type='checkbox' id='openvpn_client_use_ta_key_text' name='use_ta_key_text' onclick='enableAssociatedField(this, "openvpn_client_ta_key_text", "");enableAssociatedField(this, "openvpn_client_ta_direction", "1");updateClientConfigTextFromControls()' />
									<label id='openvpn_client_use_ta_key_text_label' for='openvpn_client_use_ta_key_text'><%~ UseTAK %></label>
								</span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" for="openvpn_client_ta_direction"><%~ TADir %>:</label>
								<span class="col-xs-7">
									<select class="form-control" id='openvpn_client_ta_direction' name="openvpn_client_ta_direction" onchange="updateClientConfigTextFromControls()">
										<option value="1">1 (<%~ Clnt %>)</option>
										<option value="omitted"><%~ Symm %></option>
									</select>
								</span>
							</div>
							<div class="row form-group">
								<label class="col-xs-12" id="openvpn_client_ta_key_text_label" for="openvpn_client_use_ta_key_text"><%~ TAKey %>:</label>
								<span class="col-xs-7 col-xs-offset-5"><textarea class="form-control textarea-lg" id="openvpn_client_ta_key_text" name="openvpn_client_ta_key_text"></textarea></span>
							</div>
						</div>
						
						<div id="openvpn_client_auth_user_pass_text_container">
							<div class="row form-group">
								<span class="col-xs-12">
									<input type='checkbox' id='openvpn_client_use_auth_user_pass_text' name='use_auth_user_pass_text' onclick='enableAssociatedField(this, "openvpn_client_auth_user_pass_text_user", "");enableAssociatedField(this, "openvpn_client_auth_user_pass_text_pass","");' />
									<label id='openvpn_client_use_auth_user_pass_text_label' for='openvpn_client_use_auth_user_pass_text'><%~ UseAUP %></label>
								</span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" id="openvpn_client_auth_user_pass_text_user_label" for="openvpn_client_auth_user_pass_text_user"><%~ AUPUser %>:</label>
								<span class="col-xs-7"><input type="text" id="openvpn_client_auth_user_pass_text_user" name="openvpn_client_auth_user_pass_text_user" class="form-control" /></span>
							</div>
							<div class="row form-group">
								<label class="col-xs-5" id="openvpn_client_auth_user_pass_text_pass_label" for="openvpn_client_auth_user_pass_text_pass"><%~ AUPPass %>:</label>
								<span class="col-xs-7">
									<input type="password" id="openvpn_client_auth_user_pass_text_pass" name="openvpn_client_auth_user_pass_text_pass" class="form-control" autocomplete="off" />
									<input type="checkbox" id="show_text_pass" onclick="togglePass('openvpn_client_auth_user_pass_text_pass')" autocomplete="off">
									<label for="show_text_pass" id="show_text_pass_label"><%~ rvel %></label>
								</span>
							</div>
						</div>
					</div>

					<input style="display:none" type="hidden" id="net_mismatch_action" name="net_mismatch_action" value="query"></input>
					<input style="display:none" type="hidden" id="openvpn_client_commands" name="commands"></input>
					<input style="display:none" type="hidden" id="openvpn_client_hash" name="hash"></input>
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

<div class="modal fade" tabindex="-1" role="dialog" id="openvpn_allowed_client_modal" aria-hidden="true" aria-labelledby="openvpn_allowed_client_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="openvpn_allowed_client_modal_title" class="panel-title"><%~ CfgCred %></h3>
			</div>
			<div class="modal-body">
				<%in templates/openvpn_allowed_client_template %>
			</div>
			<div class="modal-footer" id="openvpn_allowed_client_modal_button_container">
			</div>
		</div>
	</div>
</div>

<script>
	resetData()
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "openvpn"
%>
