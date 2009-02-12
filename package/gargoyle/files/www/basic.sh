#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "connection" -p "basic" -c "internal.css" -j "basic.js table.js" -i network wireless dhcp
?>

<script>
<!--
<?
	# determine if this board is a bcm94704, for which the uci wan macaddr variable must ALWAYS be set
	PART="$(grep "nvram" /proc/mtd | awk -F: '{print $1}')"
	if [ -n "$PART" ]
	then
		PREFIX=/dev/mtdblock
		PART="${PART##mtd}"
		[ -d /dev/mtdblock ] && PREFIX=/dev/mtdblock/ 
		nvrampath="${PART:+$PREFIX$PART}"
		boardtype=$(strings $nvrampath | grep boardtype | awk 'BEGIN {FS="="}; {print $2}')
		boardnum=$(strings $nvrampath | grep boardnum | awk 'BEGIN {FS="="}; {print $2}')
		#echo "boardnum = $boardnum, boardtype = $boardtype"
		isbcm94704='false'
		if [ "$boardtype" = "0x0472" ] || [ "$boardtype" = "0x042f" ]
		then
			if [ "$boardnum" != "45" ]
			then
				isbcm94704='true'
			fi
		fi
	else
		isbcm94704='false'
	fi
	echo "var isBcm94704 = $isbcm94704;"

	echo "var allWifi = new Array();"
	all_wif=$(iwconfig 2>/dev/null | egrep  "^[^\t ]" | awk ' { print $1 } ')
	for wif in $all_wif ; do
		echo "allWifi.push(\"$wif\");"
	done

	if [ -e /lib/wifi/broadcom.sh ] ; then
		echo "var wirelessDriver=\"broadcom\";"
	else
		if [ -e /lib/wifi/madwifi.sh ] ; then
			echo "var wirelessDriver=\"atheros\";"
		else
			echo "var wirelessDriver=\"\";"
		fi
	fi
?>
var policyOption="";
if(wirelessDriver == "broadcom")
{
	policyOption="macfilter";
}
else
{
	policyOption="macpolicy";
}
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader">Internet / WAN</legend>

		
		<div id='wan_protocol_container'>
			<label class='leftcolumn' for='wan_protocol'>Connection Protocol:</label>
			<select class='rightcolumn' id='wan_protocol' onchange='setGlobalVisibility()'>
				<option value='dhcp'>DHCP</option>
				<option value='pppoe'>PPPoE</option>
				<option value='static'>Static</option>
				<option value='none'>Disabled</option>
			</select>
		</div>

		
		<div id='wan_pppoe_user_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_user' id='wan_pppoe_user_label'>User Name:</label>
			<input type='text' class='rightcolumn' id='wan_pppoe_user'  size='17' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wan_pppoe_pass_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_pass' id='wan_pppoe_pass_label'>Password:</label>
			<input type='password' class='rightcolumn' id='wan_pppoe_pass'  size='17'  onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wan_pppoe_reconnect_mode_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_reconnect_mode'>Reconnect Mode:</label>
			<select class='rightcolumn' id='wan_pppoe_reconnect_mode' onchange='setWanVisibility()'>
				<option value='demand'>Connect On Demand</option>
				<option value='keepalive'>Keep Alive</option>
			</select>
		</div>
		<div id='wan_pppoe_max_idle_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_max_idle' id='wan_pppoe_max_idle_label'>Max Idle Time:</label>
			<div class='rightcolumn'>
				<input type='text' class='rightcolumn' id='wan_pppoe_max_idle' onkeyup='proofreadNumeric(this)' size='17' maxlength='4' />
				<em>(minutes)</em>
			</div>
		</div>

		<div id='wan_pppoe_reconnect_pings_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_reconnect_pings' id='wan_pppoe_reconnect_pings_label'>Failed Pings Before Reconnect:</label>
			<div class='rightcolumn'>
				<input type='text' id='wan_pppoe_reconnect_pings' onkeyup='proofreadNumeric(this)'  size='17' maxlength='4' />
			</div>
		</div>

		<div id='wan_pppoe_interval_container' class='indent'>
			<label class='leftcolumn' for='wan_pppoe_interval' id='wan_pppoe_interval_label'>Ping Interval:</label>
			<div class='rightcolumn'>
				<input type='text' id='wan_pppoe_interval' onkeyup='proofreadNumeric(this)'  size='17' maxlength='4' />
				<em>(seconds)</em>
			</div>
		</div>
	
		
		<div id='wan_static_ip_container' class='indent'>
			<label class='leftcolumn' for='wan_static_ip' id='wan_static_ip_label'>Static IP:</label>
			<input type='text' class='rightcolumn' name='wan_static_ip' id='wan_static_ip' onkeyup='proofreadIp(this)' size='17' maxlength='15' />
		</div>
		<div id='wan_static_mask_container' class='indent'>
			<label class='leftcolumn' for='wan_static_mask' id='wan_static_ip_label'>Subnet Mask:</label>
			<input type='text' class='rightcolumn' name='wan_static_mask' id='wan_static_mask' onkeyup='proofreadMask(this)' size='17' maxlength='15' />
		</div>
		<div id='wan_static_gateway_container' class='indent'>
			<label class='leftcolumn' for='wan_static_gateway' id='wan_static_gateway_label'>Gateway:</label>
			<input type='text' class='rightcolumn' name='wan_static_gateway' id='wan_static_gateway' onkeyup='proofreadIp(this)' size='17' maxlength='15' />
		</div>





		<div id='wan_via_wifi_container'>
			<div class='nocolumn'>
				<input type='checkbox' id='wan_via_wifi' onclick='setGlobalVisibility()'/>
				<label id='wan_via_wifi_label' for='wan_via_wifi'>Connect via Wireless Client (Routed)</label>
			</div>
		</div>

		<div id='wan_via_single_port_container'>
			<div class='nocolumn'>
				<input type='checkbox' id='wan_via_single_port' onclick='setGlobalVisibility()'/>
				<label id='wan_via_single_port_label' for='wan_via_single_port'>Ethernet Port Connects to WAN (not LAN)</label>
			</div>
		</div>

		<div id='wan_port_to_lan_container' class='indent'>
			<div>
				<input type='radio' id='wan_port_disabled' name='wan_port_to_lan' value='disable'/>
				<label id='wan_port_to_lan_label' for='wan_port_disabled'>Disable WAN ethernet port</label>
			</div>
			<div>
				<input type='radio' id='wan_port_to_lan' name='wan_port_to_lan' value='lan'/>
				<label id='wan_port_to_lan_label' for='wan_port_to_lan'>Bridge WAN ethernet port to LAN</label>
			</div>
		</div>
		
		<div id='wan_mac_container'>
			<span class='leftcolumn'>
				<input type='checkbox' id='wan_use_mac' onclick='enableAssociatedField(this, "wan_mac", defaultWanMac)'/>
				<label for='wan_mac' id='wan_mac_label'>Use Custom MAC Address:</label>
			</span>
			<input type='text' class='rightcolumn' name='wan_mac' id='wan_mac' onkeyup='proofreadMac(this)' size='17' maxlength='17' />
		</div>
		<div id='wan_mtu_container'>
			<span class='leftcolumn'>
				<input type='checkbox' id='wan_use_mtu' onclick='enableAssociatedField(this, "wan_mtu", 1500)'/>
				<label for='wan_mtu' id='wan_mtu_label'>Use Custom MTU:</label>
			</span>
			<input type='text' class='rightcolumn' name='wan_mtu' id='wan_mtu' onkeyup='proofreadNumeric(this)'  size='17' maxlength='4' /> 
		</div>
	</fieldset>
	
	<fieldset>
		<legend class="sectionheader">Local Network / LAN</legend>
	
		<div id='lan_ip_container'>
			<label class='leftcolumn' for='lan_ip' id='lan_ip_label'>Router IP:</label>
			<input type='text' class='rightcolumn' name='lan_ip' id='lan_ip' onkeyup='proofreadIp(this)' size='15' maxlength='15' />
		</div>
		<div id='lan_mask_container'>
			<label class='leftcolumn' for='lan_mask' id='lan_mask_label'>Subnet Mask:</label>
			<input type='text' class='rightcolumn' name='lan_mask' id='lan_mask' onkeyup='proofreadMask(this)' size='15' maxlength='15' />
		</div>
		<div id='lan_gateway_container'>
			<label class='leftcolumn' for='lan_gateway' id='lan_gateway_label'>Gateway:</label>
			<input type='text' class='rightcolumn' name='lan_gateway' id='lan_gateway' onkeyup='proofreadIp(this)' size='15' maxlength='15' />
		</div>
		<div id='lan_dns_container'>
			<div>
				<label class='leftcolumn' >DNS Servers:</label>
				<label for='lan_dns1' id='lan_dns1_label' style="display:none">DNS Server 1:</label>
				<input class='rightcolumn' type='text' id='lan_dns1' onkeyup='proofreadIp(this)' size='15' maxlength='15' />
			</div>
			<div>
				<label class='leftcolumn' for='lan_dns2' id='lan_dns2_label' style="visibility:hidden">DNS Server 2:</label>
				<input class='rightcolumn' type='text' id='lan_dns2' onkeyup='proofreadIp(this)' size='15' maxlength='15' />
			</div>
			<div>
				<label class='leftcolumn' for='lan_dns3' id='lan_dns3_label'style="visibility:hidden">DNS Server 3:</label>
				<input class='rightcolumn' type='text' id='lan_dns3' onkeyup='proofreadIp(this)' size='15' maxlength='15' />
			</div>
		</div>
	</fieldset>

	<fieldset>
		
		<legend class="sectionheader">Wireless</legend>

		<div id='wifi_mode_container'>
			<label class='leftcolumn' for='wifi_mode' id='wifi_mode_label'>Wireless Mode:</label>
			<select class='rightcolumn' id='wifi_mode'  onchange='setWifiVisibility()'>
				<option value='ap'>Access Point (AP)</option>
				<option value='sta'>Client</option>
				<option value='ap+sta'>AP+Client</option>
				<option value='adhoc'>Ad Hoc</option>
				<option value='disabled'>Disabled</option>
			</select>
		</div>

		<div id='wifi_channel_container'>
			<label class='leftcolumn' for='wifi_channel' id='wifi_channel_label'>Wireless Channel:</label>
			<select class='rightcolumn' id='wifi_channel'>
				<option value='1'>1</option>
				<option value='2'>2</option>
				<option value='3'>3</option>
				<option value='4'>4</option>
				<option value='5'>5</option>
				<option value='6'>6</option>
				<option value='7'>7</option>
				<option value='8'>8</option>
				<option value='9'>9</option>
				<option value='10'>10</option>
				<option value='11'>11</option>
				<option value='12'>12</option>
				<option value='13'>13</option>
				<option value='14'>14</option>
			</select>
		</div>
	

		<div id="mac_enabled_container" class="nocolumn">
			<input type='checkbox' id='mac_filter_enabled' onclick='setWifiVisibility()' />
			<label for='mac_filter_enabled'>Enable Wireless MAC Filter</label>
		</div>
		<div id="mac_filter_container" class="indent">
			<div>
				<p>Be aware that MAC filtering applies to all wireless interfaces, 
				including those in client mode.  If you are using a MAC filter
				and are in client mode either set the policy to Deny only 
				listed MACs, or include the MAC of the Access Point you are
				connecting to in the MAC list below.</p>
			</div>
			<div class="internal_divider"></div>
			<div>
				<label class='leftcolumn' for='mac_filter_policy'>MAC Filter Policy:</label>
				<select class='rightcolumn' id='mac_filter_policy'>
					<option value='allow'>Allow Only MACs Listed Below</option>
					<option value='deny' >Deny Only MACs Listed Below</option>
				</select>
			</div>
			<div class='rightcolumnonly'>
				<div class="indent">	
					<input type='text' id='add_mac' class='rightcolumn' onkeyup='proofreadMac(this)' size='17' maxlength='17' />
					<input type="button" class="default_button" id="add_mac_button" value="Add" onclick="addMacToFilter()" />
				</div>
			</div>
			<div class="rightcolumnonly"><div class="indent" id="mac_table_container"></div></div>
			<div class="internal_divider"></div>	

		</div>

		
		<div id='internal_divider1' class='internal_divider'></div>
		
		<div id='wifi_ssid1_container'>
			<label class='leftcolumn' for='wifi_ssid1' id='wifi_ssid1_label'>Access Point SSID:</label>
			<input type='text' id='wifi_ssid1'  size='17' onkeyup='proofreadLengthRange(this,1,999)'/><br/>
		</div>

		<div id='wifi_hidden_container' class='indent'>
			<div class='leftcolumn'>
				<input type='checkbox' id='wifi_hidden' />
				<label id='wifi_hidden_label' for='wifi_hidden'>Do Not Broadcast SSID</label>
			</div>
		</div>
		<div id='wifi_isolate_container' class='indent'>
			<div class='leftcolumn'>
				<input type='checkbox' id='wifi_isolate' />
				<label id='wifi_isolate_label' for='wifi_isolate'>Isolate Wireless Clients</label>
			</div>
		</div>

		<div id='wifi_encryption1_container'>
			<label class='leftcolumn' for='wifi_encryption1' id='wifi_encryption1_label'>Encryption:</label>
			<select class='rightcolumn' id='wifi_encryption1' onchange='setWifiVisibility()'>
				<option value='none'>None</option>
				<option value='psk2'>WPA2 PSK</option>
				<option value='psk'>WPA PSK</option>
				<option value='wep'>WEP</option>
				<option value='wpa'>WPA RADIUS</option>
				<option value='wpa2'>WPA2 RADIUS</option>
			</select>
		</div>

		<div id='wifi_pass1_container' class='indent'>
			<label class='leftcolumn' for='wifi_pass1' id='wifi_pass1_label'>Password:</label>
			<input type='password' id='wifi_pass1'  size='17' onkeyup='proofreadLengthRange(this,8,999)'/><br/>
		</div>
		<div id='wifi_wep1_container' class='indent'>
			<div style="display:block;">
				<label class='leftcolumn' for='wifi_wep1' id='wifi_wep1_label'>WEP Hex Key:</label>
				<input type='text' id='wifi_wep1' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
			</div>
			<div>
				<input class='rightcolumnonly' type='button' value='Random 40/64 Bit WEP Key' id='wep1gen40' onclick='setToWepKey("wifi_wep1",10)'>
			</div>
			<div>
				<input class='rightcolumnonly' type='button' value='Random 104/128 Bit WEP Key' id='wep1gen104' onclick='setToWepKey("wifi_wep1",26)'>
			</div>
		</div>


		<div id='wifi_server1_container' class='indent'>
			<label class='leftcolumn' for='wifi_server1' id='wifi_server1_label'>RADIUS Server IP:</label>
			<input type='text' id='wifi_server1'  size='17' onkeyup='proofreadIP(this)'/><br/>
		</div>
		<div id='wifi_port1_container' class='indent'>
			<label class='leftcolumn' for='wifi_port1' id='wifi_port1_label'>RADIUS Server Port:</label>
			<input type='text' id='wifi_port1'  size='17' maxlength='5' onkeyup='proofreadNumeric(this)'/><br/>
		</div>

		<div id='internal_divider2' class='internal_divider'></div>
		
		<div id='wifi_ssid2_container'>
			<label class='leftcolumn' for='wifi_ssid2' id='wifi_ssid2_label'>SSID:</label>
			<input type='text' id='wifi_ssid2'  size='17' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		
		<div id='wifi_encryption2_container'>
			<label class='leftcolumn' for='wifi_encryption2' id='wifi_encryption2_label'>Encryption:</label>
			<select class='rightcolumn' id='wifi_encryption2' onchange='setWifiVisibility()'>
				<option value='none'>None</option>
				<option value='psk2'>WPA2 PSK</option>
				<option value='psk'>WPA PSK</option>
				<option value='wep'>WEP</option>
			</select>
		</div>
		<div id='wifi_pass2_container' class='indent'>
			<label class='leftcolumn' for='wifi_pass2' id='wifi_pass2_label'>Password:</label>
			<input type='password' id='wifi_pass2' size='17' onkeyup='proofreadLengthRange(this,8,999)'/><br/>
		</div>
		<div id='wifi_wep2_container' class='indent'>
			<div style="display:block;">
				<label class='leftcolumn' for='wifi_wep2' id='wifi_wep2_label'>WEP Hex Key:</label>
				<input type='text' id='wifi_wep2' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
			</div>
			<div>
				<input class='rightcolumnonly' type='button' value='Random 40/64 Bit WEP Key' id='wep2gen40' onclick='setToWepKey("wifi_wep2",10)'>
			</div>
			<div>
				<input class='rightcolumnonly' type='button' value='Random 104/128 Bit WEP Key' id='wep2gen104' onclick='setToWepKey("wifi_wep2",26)'>
			</div>
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
	gargoyle_header_footer -f -s "connection" -p "basic"
?>
