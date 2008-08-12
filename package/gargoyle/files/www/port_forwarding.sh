#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -c "internal.css" -j "port_forwarding.js parse_firewall.js table.js" -i network dropbear upnpd
?>

<script>
<!--
<?

	echo "var firewallLines = new Array();"
	if [ -e /etc/firewall.user ] ; then
		cat /etc/firewall.user | awk '{print "firewallLines.push(\""$0"\");"}'
	fi
	echo "var firewallData = parseFirewallLines(firewallLines,currentLanIp);"
	echo "";


	if [ -h /etc/rc.d/S95miniupnpd ] ; then
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
				<table>
					<tr class='table_row_add_header'>
						<th><label id='add_app_label' for='add_host'>Application Name</label><br><em>(optional)</em></th>
						<th><label id='add_prot_label' for='add_prot'>Protocol</label></th>
						<th><label id='add_fp_label' for='add_fp'>From Port</label></th>
						<th><label id='add_ip_label' for='add_ip'>To IP</label></th>
						<th><label id='add_tp_label' for='add_dp'>To Port<br><em>(optional)</em></label></th>
						<th></th>
					</tr>
					<tr class='table_row_add'>
						<td><input type='text' id='add_app'  size='10' maxLength='10'/></td>
						<td><select id='add_prot'><option value='Both'>Both</both><option value='TCP'>TCP</option><option value='UDP'>UDP</option></select></td>
						<td><input type='text' id='add_fp' size='5' onkeyup='proofreadNumericRange(this,1,65535)' maxLength='5'/></td>
						<td><input type='text' id='add_ip' size='15' onkeyup='proofreadIp(this)' maxLength='15'/></td>
						<td><input type='text' id='add_dp' size='5' onkeyup='proofreadNumeric(this,1,65535)' maxLength='5'/></td>
							
						<td><input type='button' id='add_button' value='Add' class='default_button' onclick='addPortfRule()'/></td>
					</tr>
				</table>
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
				<table>
					<tr class='table_row_add_header'>
						<th><label id='addr_app_label' for='addr_host'>Application Name</label><br><em>(optional)</em></th>
						<th><label id='addr_prot_label' for='addr_prot'>Protocol</label></th>
						<th><label id='addr_sp_label' for='addr_sp'>Start Port</label></th>
						<th><label id='addr_ep_label' for='addr_ep'>End Port</label></th>
						<th><label id='addr_ip_label' for='addr_ip'>To IP</label></th>
						<th></th>
					</tr>
					<tr class='table_row_add'>
						<td><input type='text' id='addr_app'  size='10' maxLength='10' /></td>
						<td><select id='addr_prot'><option value='Both'>Both</both><option value='TCP'>TCP</option><option value='UDP'>UDP</option></select></td>
						<td><input type='text' id='addr_sp' size='5' onkeyup='proofreadNumericRange(this,1,65535)'  maxLength='5'/></td>
						<td><input type='text' id='addr_ep' size='5' onkeyup='proofreadNumericRange(this,1,65535)'  maxLength='5'/></td>
						<td><input type='text' id='addr_ip' size='15' onkeyup='proofreadIp(this)' maxLength='15'/></td>
							
						<td><input type='button' id='addr_button' value='Add' class='default_button' onclick='addPortfRangeRule()'/></td>
					</tr>
				</table>
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
			<input type='text' class='rightcolumn' name='dmz_ip' id='dmz_ip' onkeyup='proofreadIp(this)' size='17' maxlength='15' />
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
			It is recommended that you enable UpnP only if you are sure you need it.</em>
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
