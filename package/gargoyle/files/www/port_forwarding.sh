#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -c "internal.css" -j "port_forwarding.js  table.js" -i firewall network dropbear upnpd
?>

<script>
<!--
<?
	upnp_config_enabled=$(uci get upnpd.config.enabled)
	if [ -h /etc/rc.d/S95miniupnpd ] && [ "$upnp_config_enabled" != "0" ] ; then
		echo "var upnpdEnabled = true;"
	else
		echo "var upnpdEnabled = false;"
	fi


?>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader">Individual Port Forwarding</legend>

		<div id='portf_add_heading_container'>
			<label class='nocolumn' id='portf_add_heading_label'>Forward Individual Ports From WAN to LAN:</label>
		</div>
		<div class='bottom_gap'>
			<div id='portf_add_container'>
				<? cat templates/single_forward_template ?>				
			</div>
		</div>
		
		<div id='portf_table_container' class="bottom_gap"></div>	
	</fieldset>


	<fieldset>
		<legend class="sectionheader">Port Range Forwarding</legend>
		
		<div id='portfrange_add_heading_container'>
			<label class='nocolumn' id='portf_add_heading_label'>Forward Port Range From WAN to LAN:</label>
		</div>
		
		<div class='bottom_gap'>
			<div id='portfrange_add_container'>
				<? cat templates/multi_forward_template ?>	
			</div>
		</div>
		
		<div id='portfrange_table_container' class="bottom_gap"></div>		
	</fieldset>

	<fieldset>
		<legend class="sectionheader">DMZ</legend>
		<div id='dmz_enabled_container'>
			<input type='checkbox' id='dmz_enabled' onclick="setDmzEnabled()" />
			<label id='dmz_enabled_label' for='dmz_enabled'>Use DMZ (De-Militarized Zone)</label>
		</div>
		<div id="dmz_ip_container" class="indent">
			<label class='leftcolumn' for='dmz_ip' id='dmz_ip_label'>DMZ IP:</label>
			<input type='text' class='rightcolumn' name='dmz_ip' id='dmz_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>
	</fieldset>

	
	<fieldset>
		<legend class="sectionheader">UPnP</legend>
		<div id='upnp_enabled_container'>
			<input type='checkbox' id='upnp_enabled' onclick="setUpnpEnabled()" />
			<label id='upnp_enabled_label' for='upnp_enabled'>UPnP Enabled</label>
		</div>
		<div class='indent'>
			<em>WARNING: The use of UPnP makes your system vulnerable to certain forms of attack, and is therefore
			a security risk.  However, UPnP is (unfortunately) necessary for some Microsoft applications including MSN messenger and
			the risk is relatively small.  Nevertheless, because of this security issue, UPnP is disabled by default.
			It is recommended that you enable UPnP only if you are sure you need it.</em>
		</div>


		<div id='upnp_up_container'>
			<label class='leftcolumn' for='upnp_up' id='upnp_up_label'>Max Up Speed on UPnP Ports:</label>
			<span class = 'rightcolumn'>
				<input type='text' class='rightcolumn' id='upnp_up' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em>Kbit / second</em>
			</span>
		</div>

		<div id='upnp_down_container'>
			<label class='leftcolumn' for='upnp_down' id='upnp_down_label'>Max Down Speed on UPnP Ports:</label>
			<span class='rightcolumn'>
				<input type='text' id='upnp_down' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em>Kbit / second</em>
			</span>
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
	gargoyle_header_footer -f -s "firewall" -p "portforwarding"
?>
