#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "openvpn" -c "internal.css" -j "openvpn.js table.js" -z "openvpn.js" openvpn_gargoyle ddns_gargoyle httpd_gargoyle dropbear firewall tor -i
%>

<script>
<%

	if [ -e /etc/openvpn/dh1024.pem ] ; then
		echo "var haveDh = true;"
	else
		echo "var haveDh = false;"
	fi

	client_id=$(uci get openvpn_gargoyle.@client[0].id 2>/dev/null)
	echo "var curClientConf =[]; var curClientCa = []; var curClientCert =[]; var curClientKey = []; var curClientTaKey = [];"
	if [ -n "$client_id" ]  && [ -f "/etc/openvpn/${client_id}.conf" ] && [ -f "/etc/openvpn/${client_id}_ca.crt" ] && [ -f "/etc/openvpn/${client_id}.crt" ] && [ -f "/etc/openvpn/${client_id}.key" ] ; then
		awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientConf.push(\""$0"\")"}' "/etc/openvpn/${client_id}.conf"      2>/dev/null
		awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientCa.push(\""$0"\");"}'   "/etc/openvpn/${client_id}_ca.crt"   2>/dev/null
		awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientCert.push(\""$0"\");"}' "/etc/openvpn/${client_id}.crt"      2>/dev/null
		awk '{ gsub(/[\r\n]+$/, "") ; gsub(/\"/, "\\\"") ; print "curClientKey.push(\""$0"\");"}'  "/etc/openvpn/${client_id}.key"      2>/dev/null
		if [ -f "/etc/openvpn/${client_id}_ta.key" ] ; then
			awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientTaKey.push(\""$0"\");"}'  "/etc/openvpn/${client_id}_ta.key"      2>/dev/null
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
			remote_ping=$(ping -c 1 -W 2 www.google.com 2>/dev/null | grep "0% packet loss")
		fi
	fi
	echo "var remotePing=\"$remote_ping\";"

%>

	var uci = uciOriginal.clone();
</script>

<fieldset id="openvpn_config_fieldset">
	<legend class="sectionheader"><%~ openvpn.OCfg %></legend>
	<div id= "openvpn_config_container">
		<label class='leftcolumn' for='openvpn_config' id='openvpn_config_label'><%~ OCfg %>:</label>
		<select class='rightcolumn' id='openvpn_config' onchange='setOpenvpnVisibility()'>
			<option value='disabled'><%~ ODis %></option>
			<option value='client'><%~ OClt %></option>
			<option value='server'><%~ OSrv %></option>
		</select>
	</div>

	<div id='openvpn_config_status_container' style="display:none" >
		<span class='leftcolumn'><%~ OSts %>:</span>
		<span class='rightcolumn' id='openvpn_config_status'></span>
	</div>

	<div id='openvpn_clear_keys_container' style='display:none' >
		<span class="rightcolumnonly"><input type='button' id='openvpn_clear_keys_button' value='<%~ OClrK %>' class='default_button' onclick='clearOpenvpnKeys()' /></span>
	</div>

</fieldset>

<fieldset id="openvpn_server_fieldset">
	<legend class="sectionheader"><%~ OSCfg %></legend>
		
	<div id='openvpn_server_ip_container'>
		<label class='leftcolumn' for='openvpn_server_ip' id='openvpn_server_ip_label'><%~ OInIP %>:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_ip' id='openvpn_server_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
	</div>

	<div id='openvpn_server_mask_container'>
		<label class='leftcolumn' for='openvpn_server_mask' id='openvpn_server_mask_label'><%~ OIMsk %>:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_mask' id='openvpn_server_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
	</div>
	
	<div id='openvpn_server_port_container'>
		<label class='leftcolumn' for='openvpn_server_port' id='openvpn_server_port_label'><%~ OPrt %>:</label>
		<input type='text' id='openvpn_server_port'  size='20' maxlength='5' onkeyup='proofreadPort(this)'/><br/>
	</div>


	<div id= "openvpn_server_protocol_container">
		<label class='leftcolumn' for='openvpn_server_protocol' id='openvpn_server_protocol_label'><%~ OProto %>:</label>
		<select class='rightcolumn' id='openvpn_server_protocol'>
			<option value='udp'>UDP</option>
			<option value='tcp'>TCP</option>
		</select>
	</div>	

	<div id= "openvpn_server_cipher_container">
		<label class='leftcolumn' for='openvpn_server_cipher' id='openvpn_server_cipher_label'><%~ OCiph %>:</label>
		<select class='rightcolumn' id='openvpn_server_cipher'>
			<option value='BF-CBC:128'>Blowfish-CBC 128bit</option>
			<option value='BF-CBC:256'>Blowfish-CBC 256bit</option>
			<option value='AES-128-CBC'>AES-CBC 128bit</option>
			<option value='AES-256-CBC'>AES-CBC 256bit</option>
		</select>
	</div>

	<div id= "openvpn_server_client_to_client_container">
		<label class='leftcolumn' for='openvpn_server_client_to_client' id='openvpn_server_client_to_client_label'><%~ CCTr %>:</label>
		<select class='rightcolumn' id='openvpn_server_client_to_client'>
			<option value='true'><%~ CtoC %></option>
			<option value='false'><%~ CtoS %></option>
		</select>
	</div>

	<div id= "openvpn_server_subnet_access_container">
		<label class='leftcolumn' for='openvpn_server_subnet_access' id='openvpn_server_subnet_access_label'><%~ LSAc %>:</label>
		<select class='rightcolumn' id='openvpn_server_subnet_access'>
			<option value='true'><%~ CtoH %></option>
			<option value='false'><%~ CnoL %></option>
		</select>
	</div>

	<div id= "openvpn_server_duplicate_cn_container">
		<label class='leftcolumn' for='openvpn_server_duplicate_cn' id='openvpn_server_duplicate_cn_label'><%~ CredR %>:</label>
		<select class='rightcolumn' id='openvpn_server_duplicate_cn' onchange='updateDupeCn()'>
			<option value='false'><%~ CredSC %></option>
			<option value='true'><%~ CredMC %></option>
		</select>
	</div>

	<div id= "openvpn_server_redirect_gateway_container">
		<label class='leftcolumn' for='openvpn_server_redirect_gateway' id='openvpn_server_redirect_gateway_label'><%~ CUse %>:</label>
		<select class='rightcolumn' id='openvpn_server_redirect_gateway'>
			<option value='true'><%~ ATrff %></option>
			<option value='false'><%~ HTrff %></option>
		</select>
	</div>
</fieldset>

<fieldset id="openvpn_allowed_client_fieldset">
	<legend class="sectionheader"><%~ OSAC %></legend>

	<span id="openvpn_allowed_client_table_label"><p><%~ CClnt %>:</p></span>

	<div id="openvpn_allowed_client_table_container"></div>

	<div><em><%~ ZipCred %></em></div>

	<div class='internal_divider'></div>

	<span id="openvpn_allowed_client_add_label"><p><%~ CfgCred %>:</p></span>

	<div class="indent">
		<%in /www/templates/openvpn_allowed_client_template %>
		<div>
			<input type='button' id='openvpn_allowed_client_add' value='<%~ Add %>' class='default_button' onclick='addAc()' />
		</div>
	</div>
</fieldset>

<fieldset id="openvpn_client_fieldset">
	<legend class="sectionheader"><%~ OClt %></legend>

	<form id='openvpn_client_form' enctype="multipart/form-data" method="post" action="utility/openvpn_upload_client.sh" target="client_add_target">
		<div>
			<input type="radio" id="openvpn_client_config_upload" name="client_config_mode" onclick="setClientVisibility(document)" />
			<label for="openvpn_client_config_upload"><%~ UpCfgF %></label>
			<br/>
			<input type="radio" id="openvpn_client_config_manual" name="client_config_mode" onclick="setClientVisibility(document)" />
			<label for="openvpn_client_config_manual"><%~ CfgMan %></label>
		</div>

		<div id="openvpn_client_file_controls" class="indent">
			
			<div id="openvpn_client_file_type_container">
				<label id="openvpn_client_file_type_label" class="leftcolumn" for="openvpn_client_file_type"><%~ UpFmt %>:</label>
				<select id="openvpn_client_file_type" class="rightcolumn" onchange="setClientVisibility(document)">
					<option value="zip"><%~ SZipF %></option>
					<option value="multi" ><%~ CfgF %></option>
				</select>
			</div>
			<div id="openvpn_client_zip_file_container">
				<label id="openvpn_client_zip_file_label" class='leftcolumn' for="openvpn_client_zip_file"><%~ ZipF %>:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_zip_file" name="openvpn_client_zip_file" />
			</div>
			
			<div id="openvpn_client_conf_file_container">
				<label id="openvpn_client_conf_file_label" class='leftcolumn' for="openvpn_client_conf_file"><%~ OCfgF %>:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_conf_file" name="openvpn_client_conf_file" />
			</div>
			<div id="openvpn_client_ca_file_container">
				<label id="openvpn_client_ca_file_label" class='leftcolumn' for="openvpn_client_ca_file"><%~ CACF %>:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_ca_file" name="openvpn_client_ca_file" />
			</div>
			<div id="openvpn_client_cert_file_container">
				<label id="openvpn_client_cert_file_label" class='leftcolumn' for="openvpn_client_cert_file"><%~ CCertF %>:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_cert_file" name="openvpn_client_cert_file" />
			</div>
			<div id="openvpn_client_key_file_container">
				<label id="openvpn_client_key_file_label" class='leftcolumn' for="openvpn_client_key_file"><%~ CKeyF %>:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_key_file" name="openvpn_client_key_file" />
			</div>
			<div id="openvpn_client_ta_key_file_container">
				<label id="openvpn_client_ta_key_file_label" class='leftcolumn' for="openvpn_client_use_ta_key_file"><%~ TAKeyF %>:</label>
				<input type='checkbox' class='rightcolumn' id='openvpn_client_use_ta_key_file' name='use_ta_key_file' onclick='enableAssociatedField(this, "openvpn_client_ta_key_file", "")'  >&nbsp;&nbsp;
				<label id='openvpn_client_use_ta_key_file_label' for='openvpn_client_use_ta_key_file'><%~ UseTAK %></label>
				<br/>
				<input class='rightcolumnonly' type="file" id="openvpn_client_ta_key_file" name="openvpn_client_ta_key_file" />
			</div>
		</div>

		<div id="openvpn_client_manual_controls" class="indent">

			<div id='openvpn_client_remote_container'>
				<label class='leftcolumn' for='openvpn_client_remote' id='openvpn_client_remote_label'><%~ OSrvAddr %>:</label>
				<input type='text' class='rightcolumn' name='openvpn_client_remote' onkeyup="updateClientConfigTextFromControls()" id='openvpn_client_remote' size='30' />
			</div>

			<div id='openvpn_client_port_container'>
				<label class='leftcolumn' for='openvpn_client_port' id='openvpn_client_port_label'><%~ OSrvPrt %>:</label>
				<input type='text' class='rightcolumn' name='openvpn_client_port' onkeyup="updateClientConfigTextFromControls();proofreadPort(this);" id='openvpn_client_port' size='30' />
			</div>

			<div id='openvpn_client_protocol_container'>
				<label class='leftcolumn' for='openvpn_client_protocol' id='openvpn_client_protocol_label'><%~ OProto %>:</label>
				<select class='rightcolumn' onchange="updateClientConfigTextFromControls()" id='openvpn_client_protocol'>
					<option value='udp'>UDP</option>
					<option value='tcp'>TCP</option>
				</select>
			</div>

			<div id='openvpn_client_cipher_container'>
				<label class='leftcolumn' for='openvpn_client_cipher' id='openvpn_client_cipher_label'><%~ OCiph %>:</label>
				<select class='rightcolumn' id='openvpn_client_cipher' onchange="setClientVisibility(document);updateClientConfigTextFromControls();" >
					<option value='BF-CBC:128'>Blowfish-CBC 128bit</option>
					<option value='BF-CBC:256'>Blowfish-CBC 256bit</option>
					<option value='AES-128-CBC'>AES-CBC 128bit</option>
					<option value='AES-256-CBC'>AES-CBC 256bit</option>
					<option value='other'><%~ Othr %></option>

				</select>
			</div>

			<div id='openvpn_client_block_nonovpn_container'>
				<label class='leftcolumn' for='openvpn_client_block_nonovpn' id='openvpn_client_block_nonovpn_label'><%~ NOVPNT %>:</label>
				<select class='rightcolumn' id='openvpn_client_block_nonovpn' >
					<option value='allow'><%~ AllowNOVPNT %></option>
					<option value='block'><%~ BlockNOVPNT %></option>
				</select>
				<br/>
				<span class='rightcolumnonly'><em><%~ DescNOVPNT %></em></span>
			</div>



			<div id='openvpn_client_cipher_other_container'>
				<span class="rightcolumnonly"><input type='text' onkeyup="updateClientConfigTextFromControls()" id="openvpn_client_cipher_other" />&nbsp;<em><%~ Cphr %></em></span>
				<span class="rightcolumnonly"><input type='text' onkeyup="updateClientConfigTextFromControls()" id="openvpn_client_key_other" />&nbsp;<em><%~ Keyopt %></em></span>
			</div>

			<div id="openvpn_client_conf_text_container">
				<br/>
				<label class="leftcolumn" for='openvpn_client_conf_text' id='openvpn_client_conf_text_label'><%~ OCfg %>:</label>
				<br/>
				<span class="leftcolumnonly" style="margin-left:5px;"><em><%~ CfgUpd %></em></span>
				<br/>
				<textarea id='openvpn_client_conf_text' name='openvpn_client_conf_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>

			<div id="openvpn_client_ca_text_container">
				<label class="leftcolumn" for='openvpn_client_ca_text'  id='openvpn_client_ca_text_label'><%~ CACert %>:</label>
				<br/>
				<textarea id='openvpn_client_ca_text' name='openvpn_client_ca_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>
			<div id="openvpn_client_cert_text_container">
				<label class="leftcolumn" for='openvpn_client_cert_text' id='openvpn_client_cert_text_label'><%~ CCert %>:</label>
				<br/>
				<textarea id='openvpn_client_cert_text' name='openvpn_client_cert_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>
			<div id="openvpn_client_key_text_container">
				<label class="leftcolumn" for='openvpn_client_key_text' id='openvpn_client_key_text_label'><%~ CKey %>:</label>
				<br/>
				<textarea id='openvpn_client_key_text' name='openvpn_client_key_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>

			<div id="openvpn_client_ta_key_text_container">
				<label id="openvpn_client_ta_key_text_label" class='leftcolumn' for="openvpn_client_use_ta_key_text"><%~ TAKey %>:</label>
				<input type='checkbox' class='rightcolumn' id='openvpn_client_use_ta_key_text' name='use_ta_key_text' onclick='enableAssociatedField(this, "openvpn_client_ta_key_text", "");enableAssociatedField(this, "openvpn_client_ta_direction", "1");updateClientConfigTextFromControls()' >&nbsp;&nbsp;
				<label id='openvpn_client_use_ta_key_text_label' for='openvpn_client_use_ta_key_text'><%~ UseTAK %></label>
				<br/>
				<label class="leftcolumn" for="openvpn_client_ta_direction"><%~ TADir %>:</label>
				<span class="rightcolumn">
					<select class='rightcolumnonly' id='openvpn_client_ta_direction' name="openvpn_client_ta_direction" onchange="updateClientConfigTextFromControls()">
						<option value="1">1 (<%~ Clnt %>)</option>
						<option value="omitted"><%~ Symm %></option>
					</select>
				</span>
				<br/>
				<textarea class='rightcolumnonly' id="openvpn_client_ta_key_text" name="openvpn_client_ta_key_text" style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>


		</div>
		<input style="display:none" type="hidden" id="net_mismatch_action" name="net_mismatch_action" value="query"></input>
		<input style="display:none" type="hidden" id="openvpn_client_commands" name="commands"></input>
		<input style="display:none" type="hidden" id="openvpn_client_hash" name="hash"></input>
	</form>

	<iframe id="client_add_target" name="client_add_target" src="#" style="display:none"></iframe> 
</fieldset>

<div id="bottom_button_container">
	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
</div>

<script>
	resetData()
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "openvpn"
%>
