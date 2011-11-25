#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "tor" -c "internal.css" -j "tor.js" tor
?>


	<fieldset>
		<legend class="sectionheader">Tor Anonymization Client</legend>
		
		<div id='tor_enabled_container'>
		<label  class='wideleftcolumn' for='tor_enabled' id='tor_enabled_label' >Tor Client:</label>
			<select class='rightcolumn' id="tor_enabled" onchange='setVisibility()' >
				<option value="2">Enabled, Toggled By Each Host</option>
				<option value="1">Enabled For All Hosts</option>
				<option value="0">Disabled</option>
			</select>
		</div>
		<div id='tor_other_proto_container'>
			<label  class='wideleftcolumn' for='tor_other_proto' id='tor_other_proto_label'>Protocols Not Handled By Tor:</label>
			<select class='rightcolumn' id="tor_other_proto">
				<option value="0">Ignore</option>
				<option value="1">Block</option>
			</select>
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
