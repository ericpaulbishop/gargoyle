#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -c "internal.css" -j "port_forwarding.js  table.js" gargoyle -i firewall network dropbear upnpd
?>

<script>
<!--
<?
	upnp_config_enabled=$(uci get upnpd.config.enable_upnp)
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
		<legend class="sectionheader">UPnP / NAT-PMP</legend>
		<div id='upnp_enabled_container'>
			<input type='checkbox' id='upnp_enabled' onclick="setUpnpEnabled()" />
			<label id='upnp_enabled_label' for='upnp_enabled'>Enable UPnP &amp; NAT-PMP service</label>
		</div>


		<div id='upnp_table_heading_container'>
			<span class='nocolumn'>Active port forwards:</span>
		</div>
 
		<br>

		<div class='indent'>
			<div id='upnp_table_container' class="bottom_gap"></div>
		</div>

		<div id='upnp_up_container'>
			<label class='leftcolumn' for='upnp_up' id='upnp_up_label'>Upload speed to report:</label>
			<span class = 'rightcolumn'>
				<input type='text' class='rightcolumn' id='upnp_up' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em>Kbit / second</em>
			</span>
		</div>

		<div id='upnp_down_container'>
			<label class='leftcolumn' for='upnp_down' id='upnp_down_label'>Download speed to report:</label>
			<span class='rightcolumn'>
				<input type='text' id='upnp_down' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em>Kbit / second</em>
			</span>


		<div id="upnp_help" class="indent">
		<span id='upnp_help_txt'>

		<p>UPnP (Universal Plug and Play) and NAT-PMP (NAT Port Mapping Protocol) are both protocols which allows devices and applications on your 
		LAN to automatically configure your router with the port forwards needed for proper operation.  If a device supports either protocol
		it is not necessary to create manual port forward rules (see the top of this page) as they will automatically be created by the device.</p>

		<p>When enabled Gargoyle shows a table of automatically created port forwards so you can see which devices have requested forwards and
		verify that this feature is working properly.  This service may not work correctly in network configurations containing two or more 
		routers (double NAT).  If you see a single row with '***' it means there are no port forwards registered.</p>

		<p>As part of the protocol the LAN device can request the speed of the WAN connection from the router.  Two fields are provided to configure
		the response to such queries. Client applications can use this information to optimize their performance.  But is important to note that 
		the router does not do anything to limit speeds based on this data.  It is only reported to the requester.  If zero is entered for either 
		value the speed of the interface is reported, usually 100MB or 1GB depending on the router’s interface speed.</p>

		<p>There is some controversy about the security of this service and it does require additional RAM to run which may be important on memory
		constrained routers, so by default this feature is off.</p>

		</span>
		<a id="upnp_help_ref" onclick='setDescriptionVisibility("upnp_help")'  href="#upnp_help">Hide Text</a>

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
