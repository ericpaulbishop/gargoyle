#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" )
	gargoyle_header_footer -h -s "system" -p "access" -c "internal.css" -j "access.js" -i httpd_gargoyle dropbear gargoyle firewall network wireless 
?>


<form>
	<fieldset>
		<legend class='sectionheader'>Web Access</legend>
		


		<div>
			<label class='leftcolumn' id='local_web_protocol_label' for='local_web_protocol'>Web Admin Protocol:</label>
			<select class='rightcolumn' id='local_web_protocol' onchange='updateVisibility()'>
				<option value='https'>HTTPS</option>
				<option value='http'>HTTP</option>
				<option value='both'>HTTP & HTTPS</option>
			</select>
		</div>
		
		<div class='indent'>
			<div id='local_http_port_container'>
				<label class='leftcolumn' for='local_http_port' id='local_http_port_label'>Local HTTP Port:</label>
				<input type='text' class='rightcolumn' id='local_http_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
			</div>
			<div id='local_https_port_container'>
				<label class='leftcolumn' for='local_https_port' id='local_https_port_label'>Local HTTPS Port:</label>
				<input type='text' class='rightcolumn' id='local_https_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
			</div>
		</div>
		<div id='remote_web_protocol_container'>
			<label class='leftcolumn' id='remote_web_protocol_label' for='remote_web_protocol'>Remote Web Admin Access:</label>
			<select class='rightcolumn' id='remote_web_protocol' onchange='updateVisibility()'>
				<option value='disabled'>Disabled</option>
				<option value='https'>HTTPS</option>
				<option value='http'>HTTP</option>
				<option value='both'>HTTP & HTTPS</option>
			</select>
		</div>
		<div class='indent' id='remote_web_ports_container'>
			<div id='remote_http_port_container'>
				<label class='leftcolumn' for='remote_http_port' id='remote_http_port_label'>Remote HTTP Port:</label>
				<input type='text' class='rightcolumn' id='remote_http_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
			</div>
			<div id='remote_https_port_container'>
				<label class='leftcolumn' for='remote_https_port' id='remote_https_port_label'>Remote HTTPS Port:</label>
				<input type='text' class='rightcolumn' id='remote_https_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
			</div>
		</div>
		<div class="nocolumn">
			<input type='checkbox' id='disable_web_password' />
			<label id='disable_web_password_label' for='disable_web_password'>Disable password protection of web interface</label> <em>(Not Recommended!)</em>
		</div>

	</fieldset>

	<fieldset>
		<legend class='sectionheader'>SSH Access</legend>

		<div>
			<label class='leftcolumn' for='local_ssh_port' id='local_ssh_port_label'>Local SSH Port:</label>
			<input type='text' class='rightcolumn' id='local_ssh_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
		</div>

		<div class='nocolumn' id='remote_ssh_enabled_container'>
			<input type='checkbox' id='remote_ssh_enabled' onclick="updateVisibility()" />
			<label id='remote_ssh_enabled_label' for='remote_ssh_enabled'>Enable Remote SSH Access</label>
		</div>
		<div class='indent' id='remote_ssh_port_container'>
			<label class='leftcolumn' for='remote_ssh_port' id='remote_ssh_port_label'>Remote SSH Port:</label>
			<input type='text' class='rightcolumn' id='remote_ssh_port'  size='7' maxlength='5' onkeyup='proofreadNumericRange(this,1,65535)'/>
		</div>

	</fieldset>

	<fieldset>
		<legend class="sectionheader">Change Administrator Password</legend>
		<div>
			<label class='leftcolumn' for='password1' id='password1_label'>New Password:</label>
			<input type='password' class='rightcolumn' id='password1'  size='25' />
		</div>
		<div>
			<label class='leftcolumn' for='password2' id='password2_label'>Confirm Password:</label>
			<input type='password' class='rightcolumn' id='password2'  size='25' />
		</div>

	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>


	<span id="update_container" >Please wait while new settings are applied. . .</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>
<?
	gargoyle_header_footer -f -s "system" -p "access"
?>
