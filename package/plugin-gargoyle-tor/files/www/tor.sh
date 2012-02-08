#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "tor" -c "internal.css" -j "tor.js" tor
?>


	<fieldset>
		<legend class="sectionheader">Tor Anonymization Client</legend>
		
		<div id='tor_client_mode_container'>
			<label  class='wideleftcolumn' for='tor_client_mode' id='tor_client_mode_label' >Tor Client:</label>
			<select class='rightcolumn' id="tor_client_mode" onchange='setTorVisibility()' >
				<option value="2">Enabled, Toggled By Each Host</option>
				<option value="1">Enabled For All Hosts</option>
				<option value="3">Hidden Service Access Only</option>
				<option value="0">Disabled</option>
			</select>
			<br/>
			<em><span class="farrightcolumnonly" id="mode_description"></span></em>
			<br/>
		</div>

		<div id='tor_other_proto_container'>
			<label  class='wideleftcolumn' for='tor_other_proto' id='tor_other_proto_label'>Protocols Not Handled By Tor:</label>
			<select class='rightcolumn' id="tor_other_proto">
				<option value="0">Ignore</option>
				<option value="1">Block</option>
			</select>
		</div>

		<div id='tor_hidden_subnet_container'>
			<label  class='wideleftcolumn' for='tor_hidden_subnet' id='tor_hidden_subnet_label'>Tor Hidden Service Subnet:</label>
			<input type="text" class="rightcolumn" id='tor_hidden_subnet' onkeyup='proofreadIp(this)' />
		</div>
		<div id='tor_hidden_mask_container'>
			<label  class='wideleftcolumn' for='tor_hidden_mask' id='tor_hidden_mask_label'>Tor Hidden Service Subnet Mask:</label>
			<input type="text" class="rightcolumn" id='tor_hidden_mask' onkeyup='proofreadMask(this)'/>
		</div>
	
	</fieldset>

	<fieldset>
		<legend class="sectionheader">Tor Relay</legend>
		
		<div id='tor_client_mode_container'>
			<label  class='wideleftcolumn' for='tor_relay_mode' id='tor_relay_mode_label' >Tor Relay:</label>
			<select class='rightcolumn' id="tor_client_mode" onchange='setTorVisibility()' >
				<option value="2">Enabled, Configured As A Bridge</option>
				<option value="1">Enabled, Configured As a Relay</option>
				<option value="0">Disabled</option>
			</select>
			<br/>
			<em><span class="farrightcolumnonly" id="mode_description"></span></em>
			<br/>
		</div>
		

		<div id='tor_relay_max_bw_container'>
			<label  class='wideleftcolumn' for='tor_relay_max_bw' id='tor_relay_max_bw_label'>Max Relay Banwidth:</label>
			<input type="text" class="rightcolumn" id='tor_relay_max_bw' onkeyup='proofreadNumeric(this)' />
			<em>&nbsp;&nbsp;KBytes/s</em>
		</div>

		<div id='tor_relay_nickname_container'>
			<label  class='wideleftcolumn' for='tor_relay_nickname' id='tor_relay_nickname_label'>Node Nickname:</label>
			<input type="text" class="rightcolumn" id='tor_relay_nickname' onkeyup='proofreadLength(this,1,9999)' />
		</div>

		<div id='tor_relay_contact_container'>
			<label  class='wideleftcolumn' for='tor_relay_contact' id='tor_relay_contact_label'>Contact email / PGP PubKey (optional):</label>
			<textarea class="rightcolumn" id='tor_relay_contact' ></textarea>
		</div>


	
	</fieldset>




	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button"  onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button"  onclick='resetData()'/>
	</div>



<script>
	resetData();
</script>



<?
	gargoyle_header_footer -f -s "connection" -p "tor"
?>
