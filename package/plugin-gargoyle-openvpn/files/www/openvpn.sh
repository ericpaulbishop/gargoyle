#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "openvpn" -c "internal.css" -j "openvpn.js table.js" openvpn_gargoyle ddns_gargoyle httpd_gargoyle dropbear firewall tor -i
?>

<script>
<?

if [ -e /etc/openvpn/dh1024.pem ] ; then
	echo "var haveDh = true;"
else
	echo "var haveDh = false;"
fi

client_id=$(uci get openvpn_gargoyle.@client[0].id 2>/dev/null)
echo "var curClientConf =[]; var curClientCa = []; var curClientCert =[]; var curClientKey = [];"
if [ -n "$client_id" ]  && [ -f "/etc/openvpn/${client_id}.conf" ] && [ -f "/etc/openvpn/${client_id}_ca.crt" ] && [ -f "/etc/openvpn/${client_id}.crt" ] && [ -f "/etc/openvpn/${client_id}.key" ] ; then
	awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientConf.push(\""$0"\")"}' "/etc/openvpn/${client_id}.conf"      2>/dev/null
	awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientCa.push(\""$0"\");"}'   "/etc/openvpn/${client_id}_ca.crt"   2>/dev/null
	awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientCert.push(\""$0"\");"}' "/etc/openvpn/${client_id}.crt"      2>/dev/null
	awk '{ gsub(/[\r\n]+$/, "") }; {print "curClientKey.push(\""$0"\");"}'  "/etc/openvpn/${client_id}.key"      2>/dev/null
fi

 	echo "var tunIp=\""$(ifconfig tun0 2>/dev/null | awk ' { if ( $0 ~ /inet addr:/) { gsub(/^.*:/, "", $2) ; print $2 } }')"\";"
	echo "var openvpnProc=\""$(ps | grep openvpn | grep -v grep | awk ' { printf $1 }')"\";"
	
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

?>

	var uci = uciOriginal.clone();
</script>



<fieldset id="openvpn_config_fieldset">
	<legend class="sectionheader">OpenVPN Configuration</legend>
	<div id= "openvpn_config_container">
		<label class='leftcolumn' for='openvpn_config' id='openvpn_config_label'>OpenVPN Configuration:</label>
		<select class='rightcolumn' id='openvpn_config' onchange='setOpenvpnVisibility()'>
			<option value='disabled'>OpenVPN Disabled</option>
			<option value='client'>OpenVPN Client</option>
			<option value='server'>OpenVPN Server</option>
		</select>
	</div>

	<div id='openvpn_config_status_container' style="display:none" >
		<span class='leftcolumn'>OpenVPN Status:</span>
		<span class='rightcolumn' id='openvpn_config_status'></span>
	</div>


</fieldset>

<fieldset id="openvpn_server_fieldset">
	<legend class="sectionheader">OpenVPN Server: Configuration</legend>
		
	<div id='openvpn_server_ip_container'>
		<label class='leftcolumn' for='openvpn_server_ip' id='openvpn_server_ip_label'>OpenVPN Internal IP:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_ip' id='openvpn_server_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
	</div>

	<div id='openvpn_server_mask_container'>
		<label class='leftcolumn' for='openvpn_server_mask' id='openvpn_server_mask_label'>OpenVPN Internal Subnet Mask:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_mask' id='openvpn_server_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
	</div>
	
	<div id='openvpn_server_port_container'>
		<label class='leftcolumn' for='openvpn_server_port' id='openvpn_server_port_label'>OpenVPN Port:</label>
		<input type='text' id='openvpn_server_port'  size='20' maxlength='5' onkeyup='proofreadPort(this)'/><br/>
	</div>


	<div id= "openvpn_server_protocol_container">
		<label class='leftcolumn' for='openvpn_server_protocol' id='openvpn_server_protocol_label'>OpenVPN Protocol:</label>
		<select class='rightcolumn' id='openvpn_server_protocol'>
			<option value='udp'>UDP</option>
			<option value='tcp'>TCP</option>
		</select>
	</div>	

	<div id= "openvpn_server_cipher_container">
		<label class='leftcolumn' for='openvpn_server_cipher' id='openvpn_server_cipher_label'>OpenVPN Cipher:</label>
		<select class='rightcolumn' id='openvpn_server_cipher'>
			<option value='BF-CBC:128'>Blowfish-CBC 128bit</option>
			<option value='BF-CBC:256'>Blowfish-CBC 256bit</option>
			<option value='AES-128-CBC'>AES-CBC 128bit</option>
			<option value='AES-256-CBC'>AES-CBC 256bit</option>
		</select>
	</div>

	<div id= "openvpn_server_client_to_client_container">
		<label class='leftcolumn' for='openvpn_server_client_to_client' id='openvpn_server_client_to_client_label'>Client-To-Client Traffic:</label>
		<select class='rightcolumn' id='openvpn_server_client_to_client'>
			<option value='true'>Allow Clients To Communicate With Each Other</option>
			<option value='false'>Clients Can Only Communicate With Server</option>
		</select>
	</div>
	
	<div id= "openvpn_server_subnet_access_container">
		<label class='leftcolumn' for='openvpn_server_subnet_access' id='openvpn_server_subnet_access_label'>LAN Subnet Access:</label>
		<select class='rightcolumn' id='openvpn_server_subnet_access'>
			<option value='true'>Allow Clients To Access Hosts on LAN</option>
			<option value='false'>Clients Can Not Access LAN</option>
		</select>
	</div>
	
	<div id= "openvpn_server_duplicate_cn_container">
		<label class='leftcolumn' for='openvpn_server_duplicate_cn' id='openvpn_server_duplicate_cn_label'>Credential Re-Use:</label>
		<select class='rightcolumn' id='openvpn_server_duplicate_cn' onchange='setOpenvpnVisibility()'>
			<option value='false'>Credentials Are Specific to Each Client</option>
			<option value='true'>Credentials Can Be Used By Multiple Clients</option>
		</select>
	</div>

	<div id= "openvpn_server_redirect_gateway_container">
		<label class='leftcolumn' for='openvpn_server_redirect_gateway' id='openvpn_server_redirect_gateway_label'>Clients Use VPN For:</label>
		<select class='rightcolumn' id='openvpn_server_redirect_gateway'>
			<option value='true'>All Client Traffic</option>
			<option value='false'>Only Traffic Destined for Hosts Behind VPN</option>
		</select>
	</div>
	



</fieldset>

<fieldset id="openvpn_allowed_client_fieldset">
	<legend class="sectionheader">OpenVPN Server: Allowed Clients</legend>
	

	<span id="openvpn_allowed_client_table_label"><p>Currently Configured Clients:</p></span>
	
	<div id="openvpn_allowed_client_table_container"></div>
	
	<div><em>After generating client configuration, click download to obtain zip file containg necessary credentials, and place in your client&lsquo;s OpenVPN configuration folder</em></div>
	
	<div class='internal_divider'></div>
	
	<span id="openvpn_allowed_client_add_label"><p>Configure A New Client / Set of Credentials :</p></span>
		
	<div class="indent">
		<? cat /www/templates/openvpn_allowed_client_template ?>
		<div>
			<input type='button' id='openvpn_allowed_client_add' value='Add' class='default_button' onclick='addAc()' />
		</div>
	</div>


</fieldset>


<fieldset id="openvpn_client_fieldset">
	<legend class="sectionheader">OpenVPN Client</legend>

	<form id='openvpn_client_form' enctype="multipart/form-data" method="post" action="utility/openvpn_upload_client.sh" target="client_add_target">
		<div>
			<input type="radio" id="openvpn_client_config_upload" name="client_config_mode" onclick="setClientVisibility(document)" /> Upload Client Configuration File(s)
			<br/>
			<input type="radio" id="openvpn_client_config_manual" name="client_config_mode" onclick="setClientVisibility(document)" /> Configure Client Manually
		</div>
	
	
		<div id="openvpn_client_file_controls" class="indent">
			
			<div id="openvpn_client_file_type_container">
				<label id="openvpn_client_file_type_label" class="leftcolumn">Upload Format:</label>
				<select id="openvpn_client_file_type" class="rightcolumn" onchange="setClientVisibility(document)">
					<option value="zip">Single Zip File</option>
					<option value="multi" >Individual Configuration Files</option>
				</select>
			</div>
			<div id="openvpn_client_zip_file_container">
				<label id="openvpn_client_zip_file" class='leftcolumn' for="openvpn_client_zip_file">Zip File:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_zip_file" name="openvpn_client_zip_file" />
			</div>
			
			<div id="openvpn_client_conf_file_container">
				<label id="openvpn_client_conf_file" class='leftcolumn' for="openvpn_client_conf_file">OpenVPN Config File:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_conf_file" name="openvpn_client_conf_file" />
			</div>
			<div id="openvpn_client_ca_file_container">
				<label id="openvpn_client_ca_file" class='leftcolumn' for="openvpn_client_ca_file">CA Certificate File:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_ca_file" name="openvpn_client_ca_file" />
			</div>
			<div id="openvpn_client_cert_file_container">
				<label id="openvpn_client_cert_file" class='leftcolumn' for="openvpn_client_cert_file">Client Certificate File:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_cert_file" name="openvpn_client_cert_file" />
			</div>
			<div id="openvpn_client_key_file_container">
				<label id="openvpn_client_key_file" class='leftcolumn' for="openvpn_client_key_file">Client Key File:</label>
				<input class='rightcolumn' type="file" id="openvpn_client_key_file" name="openvpn_client_key_file" />
			</div>
		</div>
	
	
		<div id="openvpn_client_manual_controls" class="indent">

			<div id='openvpn_client_remote_container'>
				<label class='leftcolumn' for='openvpn_client_remote' id='openvpn_client_remote_label'>OpenVPN Server Address:</label>
				<input type='text' class='rightcolumn' name='openvpn_client_remote' onkeyup="updateClientConfigTextFromControls()" id='openvpn_client_remote' size='30' />
			</div>
			
			<div id='openvpn_client_port_container'>
				<label class='leftcolumn' for='openvpn_client_port' id='openvpn_client_port_label'>OpenVPN Server Port:</label>
				<input type='text' class='rightcolumn' name='openvpn_client_port' onkeyup="updateClientConfigTextFromControls();proofreadPort(this);" id='openvpn_client_port' size='30' />
			</div>

			<div id='openvpn_client_protocol_container'>
				<label class='leftcolumn' for='openvpn_client_protocol' id='openvpn_client_protocol_label'>OpenVPN Protocol:</label>
				<select class='rightcolumn' onchange="updateClientConfigTextFromControls()" id='openvpn_client_protocol'>
					<option value='udp'>UDP</option>
					<option value='tcp'>TCP</option>
				</select>
			</div>
	
			<div id='openvpn_client_cipher_container'>
				<label class='leftcolumn' for='openvpn_client_cipher' id='openvpn_client_cipher_label'>OpenVPN Cipher:</label>
				<select class='rightcolumn' id='openvpn_client_cipher' onchange="setClientVisibility(document);updateClientConfigTextFromControls();" >
					<option value='BF-CBC:128'>Blowfish-CBC 128bit</option>
					<option value='BF-CBC:256'>Blowfish-CBC 256bit</option>
					<option value='AES-128-CBC'>AES-CBC 128bit</option>
					<option value='AES-256-CBC'>AES-CBC 256bit</option>
					<option value='other'>Other</option>
	
				</select>
			</div>
			<div id='openvpn_client_cipher_other_container'>
				<span class="rightcolumnonly"><input type='text' onkeyup="updateClientConfigTextFromControls()" id="openvpn_client_cipher_other" />&nbsp;<em>Cipher</em></span>
				<span class="rightcolumnonly"><input type='text' onkeyup="updateClientConfigTextFromControls()" id="openvpn_client_key_other" />&nbsp;<em>Key Size (optional)</em></span>
			</div>
			
			<div id="openvpn_client_conf_text_container">
				<br/>
				<label class="leftcolumn" for='openvpn_client_conf_text' id='openvpn_client_conf_text_label'>OpenVPN Configuration:</label>
				<br/>
				<span class="leftcolumnonly" style="margin-left:5px;"><em>Configuration below is updated automatically from parameters specified above</em></span>
				<br/>
				<textarea id='openvpn_client_conf_text' name='openvpn_client_conf_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>

			</div>
		
			<div id="openvpn_client_ca_text_container">
				<label class="leftcolumn" for='openvpn_client_ca_text'  id='openvpn_client_ca_text_label'>CA Certificate:</label>
				<br/>
				<textarea id='openvpn_client_ca_text' name='openvpn_client_ca_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>
			<div id="openvpn_client_cert_text_container">
				<label class="leftcolumn" for='openvpn_client_cert_text' id='openvpn_client_cert_text_label'>Client Certificate:</label>
				<br/>
				<textarea id='openvpn_client_cert_text' name='openvpn_client_cert_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>
			<div id="openvpn_client_key_text_container">
				<label class="leftcolumn" for='openvpn_client_key_text' id='openvpn_client_key_text_label'>Client Key:</label>
				<br/>
				<textarea id='openvpn_client_key_text' name='openvpn_client_key_text' onkeyup='updateClientControlsFromConfigText()' style="margin-left:5px;width:95%;height:200px;"></textarea>
			</div>
		</div>
		<input style="display:none" type="hidden" id="openvpn_client_commands" name="commands"></input>
		<input style="display:none" type="hidden" id="openvpn_client_hash" name="hash"></input>
	</form>

	<iframe id="client_add_target" name="client_add_target" src="#" style="display:none"></iframe> 


</fieldset>



<div id="bottom_button_container">
	<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>

</div>

<script>
	resetData()
</script>



<?
	gargoyle_header_footer -f -s "connection" -p "openvpn"
?>
