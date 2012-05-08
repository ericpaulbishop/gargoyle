#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "openvpn" -c "internal.css" -j "openvpn.js" openvpn ddns_gargoyle httpd_gargoyle dropbear firewall tor
?>

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
</fieldset>

<fieldset id="openvpn_server_fieldset">
	<legend class="sectionheader">OpenVPN Server: Configuration</legend>
	
	<!--
	# server internal vpn ip
	# openvpn netmask
	# protocol (tcp/udp)
	# port
	# cipher
	# allow communication between clients
	# allow clients to access router subnet
	# allow one set of credentials for multiple hosts
	# force client traffic through vpn 
	-->
	
	<div id='openvpn_server_ip_container'>
		<label class='leftcolumn' for='openvpn_server_ip' id='openvpn_server_ip_label'>OpenVPN Internal IP:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_ip' id='openvpn_server_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
	</div>

	<div id='openvpn_server_mask_container'>
		<label class='leftcolumn' for='openvpn_server_mask' id='openvpn_server_mask_label'>OpenVPN Internal Subnet Mask:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_mask' id='openvpn_server_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
	</div>
	
	<div id='openvpn_server_port_container'>
		<label class='leftcolumn' for='openvpn_server_port' id='openvpn_port_label'>OpenVPN Port:</label>
		<input type='text' id='openvpn_server_port'  size='20' maxlength='5' onkeyup='proofreadNumeric(this)'/><br/>
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
		<select class='rightcolumn' id='openvpn_server_duplicate_cn'>
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
	
	<div class="internal_divider"></div>

	<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>

</fieldset>

<fieldset id="openvpn_allowed_client_fieldset">
	<legend class="sectionheader">OpenVPN Server: Allowed Clients</legend>
	

	<span id="openvpn_allowed_client_table_label"><p>Currently Configured Clients:</p></span>
	
	<div id="openvpn_allowed_client_table_container"></div>
	
	<div><em>After generating client configuration, click download to obtain zip file containg necessary credentials, and place in your client&lsquo;s OpenVPN configuration folder</em></div>
	
	<div class='internal_divider'></div>
	
	<span id="add_ddns_label"><p>Configure A New Client / Set of Credentials :</p></span>
		
	<div class="indent">
		<? cat /www/templates/openvpn_allowed_client_template ?>
	
	</div>

	<input type="button" id="openvpn_allowed_client_add_button" class="bottom_button" value="Add Client" onclick="addClient()" />



</fieldset>


<fieldset id="openvpn_client_fieldset">
	<legend class="sectionheader">OpenVPN Client</legend>
	
</fieldset>








<?
	gargoyle_header_footer -f -s "connection" -p "openvpn"
?>
