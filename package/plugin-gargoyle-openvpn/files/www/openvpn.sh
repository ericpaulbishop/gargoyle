#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "openvpn" -c "internal.css" -j "openvpn.js" openvpn httpd_gargoyle dropbear firewall tor
?>

<fieldset id="openvpn_config_fieldset">
	<legend class="sectionheader">OpenVPN Configuration</legend>
	<div id= "openvpn_config_container">
		<label class='leftcolumn' for='openvpn_config' id='openvpn_config_label'>OpenVPN Configuration:</label>
		<select class='rightcolumn' id='openvpn_config' onchange='setOpenVpnVisibility()'>
			<option value='disabled'>OpenVPN Disabled</option>
			<option value='client'>OpenVPN Client</option>
			<option value='server'>OpenVPN Server</option>
		</select>
	</div>
</fielset>

<fieldset id="openvpn_server_fieldset">
	<legend class="sectionheader">OpenVPN Server</legend>
	
	<!--
	# server internal vpn ip
	# openvpn netmask
	# protocol (tcp/udp)
	# port
	# cipher
	# force client traffic through vpn [move to client config section]
	# allow communication between clients
	# allow clients to access router subnet
	-->
	
	<div id='openvpn_server_ip_container'>
		<label class='leftcolumn' for='openvpn_server_ip' id='openvpn_server_ip_label'>OpenVPN Internal IP:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_ip' id='openvpn_server_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
	</div>

	<div id='openvpn_server_mask_container'>
		<label class='leftcolumn' for='openvpn_server_mask' id='openvpn_server_mask_label'>OpenVPN Internal Subnet Mask:</label>
		<input type='text' class='rightcolumn' name='openvpn_server_mask' id='openvpn_server_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
	</div>
	
	<div id= "openvpn_server_protocol_container">
		<label class='leftcolumn' for='openvpn_server_protocol' id='openvpn_server_protocol_label'>OpenVPN Protocol:</label>
		<select class='rightcolumn' id='openvpn_server_protocol'>
			<option value='a128'>UDP</option>
			<option value='a256'>TCP</option>
		</select>
	</div>	
	<div id='openvpn_server_port_container' class='indent'>
		<label class='leftcolumn' for='openvpn_server_port' id='openvpn_port_label'>OpenVPN Port:</label>
		<input type='text' id='openvpn_server_port'  size='20' maxlength='5' onkeyup='proofreadNumeric(this)'/><br/>
	</div>

		<div id= "openvpn_server_cipher_container">
		<label class='leftcolumn' for='openvpn_server_cipher' id='openvpn_server_cipher_label'>OpenVPN Cipher:</label>
		<select class='rightcolumn' id='openvpn_server_cipher'>
			<option value='b128'>Blowfish 128bit</option>
			<option value='b256'>Blowfish 256bit</option>
			<option value='a128'>AES 128bit</option>
			<option value='a256'>AES 256bit</option>
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
	
	<div id= "openvpn_server_subnet_access_container">
		<label class='leftcolumn' for='openvpn_server_subnet_access' id='openvpn_server_subnet_access_label'>Credential Re-Use:</label>
		<select class='rightcolumn' id='openvpn_server_subnet_access'>
			<option value='false'>Credentials Are Specific to Each Client</option>
			<option value='true'>Credentials Can Be Used By Multiple Clients</option>
		</select>
	</div>




</fielset>

<fieldset id="openvpn_client_fieldset">
	<legend class="sectionheader">OpenVPN Client</legend>
	
</fielset>








<?
	gargoyle_header_footer -f -s "connection" -p "openvpn"
?>
