#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "basic" -c "internal.css" -j "basic.js table.js" -z "basic.js" -i gargoyle network wireless dhcp firewall
%>

<script>
<!--
<%
	echo "var apns = new Array();"
	if [ -e ./data/apn.csv ] ; then
		sort ./data/apn.csv | awk -F[\;] '{print "apns.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\"]);"}'
	fi
	#script dies if cache file exists and wifi driver defined
	/usr/lib/gargoyle/cache_basic_vars.sh >/dev/null 2>/dev/null
	cat "/var/cached_basic_vars" 2>/dev/null

	cur_date_seconds=$(date +%s)
	uptime=$(cat /proc/uptime | sed 's/\..*$//g' | sed 's/ .*$//g')
	lease_start=$(uci -P /var/state get network.wan.lease_acquired 2>/dev/null)
	lease_lifetime=$(uci -P /var/state get network.wan.lease_lifetime 2>/dev/null)
	echo "var currentDateSeconds = \"$cur_date_seconds\";"
	echo "var uptime = \"$uptime\";"
	echo "var leaseStart = \"$lease_start\";"
	echo "var leaseLifetime = \"$lease_lifetime\";"
	echo "var timezoneOffStr = \""$(date +%z)"\";"
	timezone_is_utc=$(uci get system.@system[0].timezone | grep "^UTC" | sed 's/UTC//g')
	if [ -n "$timezone_is_utc" ] ; then
		echo "var timezoneName = \""$( echo "UTC-$timezone_is_utc" | sed 's/\-\-/+/g'  )"\";"

	else
		echo "var timezoneName = \""$(date | sed 's/^.*:...//' | sed 's/ .*$//' )"\";"
	fi

	has_usb_tty=$( ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null )
	if [ -z "$has_usb_tty" ] ; then
		echo "var hasUSB = false;"
	else
		echo "var hasUSB = true;"

	fi

	has_qmi=$( grep qmi_wwan /sys/kernel/debug/usb/devices 2>/dev/null)
	if [ -z "$has_qmi" ] ; then
		echo "hasQMI = false;"
	else
		echo "hasQMI = true;"
	fi

	has_ncm=$( grep cdc_ncm /sys/kernel/debug/usb/devices 2>/dev/null)
	if [ -z "$has_ncm" ] ; then
		echo "hasNCM = false;"
	else
		echo "hasNCM = true;"
	fi

	cdcif=$(egrep -Hi "(cdc ethernet control|rndis communications control)" /sys/class/net/*/device/interface 2>/dev/null | cut -f5 -d/)
	if [ -z "$cdcif" ]; then
		echo "cdcif = \"\";"
	else
		echo "cdcif = \"$cdcif\";"
	fi
%>
var timezoneOffset = (parseInt(timezoneOffStr.substr(0,3),10)*60+parseInt(timezoneOffStr.substr(3,2),10))*60;

var policyOption="";
if(wirelessDriver == "broadcom" || wirelessDriver == "mac80211")
{
	policyOption="macfilter";
}
else
{
	policyOption="macpolicy";
}

var txPowerMax= wirelessDriver == "broadcom" ? 31 : (wirelessDriver == "mac80211" ? 20 : 18);

//not perfect, but it this will do for now since only mac80211 drivers are ath9k and b43
var isb43 = wirelessDriver == "mac80211" && (!GwifiN) ? true : false ;

//-->
</script>

<h1 class="page-header">Basic</h1>
<div class="row">

	<div id="config_fieldset" class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ basic.DCfgSect %></h3>
			</div>
			<div class="panel-body">
		<label class='leftcolumn' style="text-decoration:underline"><%~ CfgDev %>:</label>
		<div class='indent'>
			<input type="radio" id="global_gateway" name="global_configuration" value="gateway" onclick="setBridgeVisibility()" />
			<label for="global_gateway"><%~ DvGtwy %></label>
			<br/>
			<input type="radio" id="global_bridge" name="global_configuration" value="bridge" onclick="setBridgeVisibility()" />
			<label for="global_bridge"><%~ DvWBrg %></label>
		</div>
	</div>
</div>
</div>


	<div id="bridge_fieldset" class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DvWBrg %></h3>
			</div>
			<div class="panel-body">
		<div id='bridge_ip_container'>
			<label class='leftcolumn' for='bridge_ip' id='bridge_ip_label'><%~ BrIP %>:</label>
			<input type='text' class='form-control' name='bridge_ip' id='bridge_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
			<em><%~ BrNote %></em>
		</div>
		<div id='bridge_mask_container'>
			<label class='leftcolumn' for='bridge_mask' id='bridge_mask_label'><%~ SMsk %>:</label>
			<input type='text' class='form-control' name='bridge_mask' id='bridge_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
			<em><%~ SMNote %></em>

		</div>
		<div id='bridge_gateway_container'>
			<label class='leftcolumn' for='bridge_gateway' id='bridge_gateway_label'><%~ GwIP %>:</label>
			<input type='text' class='form-control' name='bridge_gateway' id='bridge_gateway' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>

		<div id= "bridge_wan_port_to_lan_container">
			<label class='leftcolumn' for='bridge_wan_port_to_lan' id='bridge_wan_port_to_lan_label'><%~ WanEP %>:</label>
			<select class='form-control' id='bridge_wan_port_to_lan' onchange='setBridgeVisibility()'>
				<option value='disable'><%~ Dsbl %></option>
				<option value='bridge'><%~ BrLAN %></option>
			</select>
		</div>

		<div id="bridge_dns_source_container">
			<span class="leftcolumn">
				<label id='bridge_dns_source_label' for='bridge_dns_source'><%~ DnsSvs %>:</label>
			</span>
			<select class='form-control' id="bridge_dns_source" onchange="setDnsSource(this)">
				<option value="gateway"><%~ GwDNS %></option>
				<option value="custom"><%~ CustDNS %></option>
			</select>
		</div>
		<div id="bridge_dns_custom_container">
			<span class='rightcolumnonly'>
				<input type='text' id='add_bridge_dns' onkeyup='proofreadIp(this)' class='form-control' size='20' maxlength='17' />
				<button class="btn btn-default" id="add_bridge_dns_button" onclick='addDns("bridge")'><%~ Add %></button>
			</span>
			<span style="clear:both"></span>
			<div class="rightcolumnonly"><div id="bridge_dns_table_container"></div></div>
		</div>

		<div class='internal_divider'></div>

		<div id='bridge_mode_container'>
			<select class='nocolumn' id='bridge_mode' onchange='setBridgeVisibility()'>
				<option value='client_bridge'><%~ BrClient %></option>
				<option value='wds'><%~ BrWDS %></option>
			</select>
		</div>
		<div class="indent">
			<div id='bridge_repeater_container'>
				<label class='leftcolumn' for='bridge_repeater' id='bridge_repeater_label'><%~ Rptr %>:</label>
				<select class='form-control' id='bridge_repeater' onchange='setBridgeVisibility()'>
					<option value='enabled'><%~ RptrE %></option>
					<option value='disabled'><%~ RptrD %></option>
				</select>
			</div>

			<div id='bridge_hwmode_container'>
				<label class='leftcolumn' for='bridge_hwmode' id='bridge_hwmode_label'><%~ BrOpr %>:</label>
				<span class='form-control'>
					<select id='bridge_hwmode' onchange='setHwMode(this)'>
						<option value='11gn'>B+G+N</option>
						<option value='11g'>B+G</option>
						<option value='11anac'>A+N+AC</option>
						<option value='11an'>A+N</option>
						<option value='11a'>A</option>
						<option value='auto'><%~ auto %></option>
					</select>
				</span>
			</div>
			<div id='bridge_channel_width_container'>
				<label class='leftcolumn' for='bridge_channel_width' id='bridge_channel_width_label'><%~ ChWdth %>:</label>
				<span class='form-control'>
				<select id='bridge_channel_width' onchange='setChannelWidth(this, "G")'>
						<option value='HT20'>20MHz</option>
						<option value='HT40+'>40MHz (<%~ ChAbv %>)</option>
						<option value='HT40-'>40Mhz (<%~ ChBlw %>)</option>
					</select>
				</span>
			</div>

			<div id='bridge_txpower_container'>
				<label class='leftcolumn' for='bridge_max_txpower' id='bridge_txpower_label'><%~ TrPwr %>:</label>
				<span class='form-control'>
					<select id='bridge_max_txpower' onchange='updateTxPower("bridge_max_txpower","bridge_txpower", "G")'>
						<option value='max'><%~ Max %></option>
						<option value='custom'><%~ Cstm %></option>
					</select>
					&nbsp;
					<input type='text' id='bridge_txpower'  onkeyup='proofreadNumericRange(this,0,getMaxTxPower("G"))' size='10' />
					<em><span id="bridge_dbm">dBm</span></em>
				</span>
			</div>

			<div id='bridge_channel_width_5ghz_container'>
				<label class='leftcolumn' for='bridge_channel_width_5ghz' id='bridge_channel_width_5ghz_label'><%~ ChWdth %>:</label>
				<span>
					<select id='bridge_channel_width_5ghz' class='form-control' onchange='setChannelWidth(this, "A")'>
						<option value='HT20'>20MHz</option>
						<option value='HT40+'>40MHz (<%~ ChAbv %>)</option>
						<option value='HT40-'>40MHz (<%~ ChBlw %>)</option>
						<option value='VHT20'>20MHz</option>
						<option value='VHT40'>40MHz</option>
						<option value='VHT80'>80MHz</option>
					</select>
				</span>
			</div>
			<div id='bridge_txpower_5ghz_container'>
				<label class='leftcolumn' for='bridge_max_txpower_5ghz' id='bridge_txpower_5ghz_label'><%~ TrPwr %>:</label>
				<span>
					<select id='bridge_max_txpower_5ghz' class='form-control' onchange='updateTxPower("bridge_max_txpower_5ghz","bridge_txpower_5ghz", "A")'>
						<option value='max'><%~ Max %></option>
						<option value='custom'><%~ Cstm %></option>
					</select>
					&nbsp;
					<input type='text' id='bridge_txpower_5ghz' onkeyup='proofreadNumericRange(this,0,getMaxTxPower("A"));' size='10' />
					<em><span id="bridge_dbm_5ghz">dBm</span></em>
				</span>
			</div>

			<div id='bridge_list_ssid_container'>
				<label for='bridge_list_ssid' id='bridge_list_ssid_label'><%~ Join %>:</label>
				<span class="rightcolumn">
					<select id="bridge_list_ssid" style='width:180px;max-width:180px' onchange='setSsidVisibility(this.id)' ><option value="custom"><%~ Other %></option></select>
					<button class='btn btn-default' id='bridge_rescan_button' onclick='scanWifi("bridge_custom_ssid")'><%~ RScn %></button>
				</span>
			</div>
			<div id='bridge_custom_ssid_container'>
				<input type='text' class='form-control' id='bridge_custom_ssid' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
			</div>
			<div id='bridge_ssid_container'>
				<label for='bridge_ssid' id='bridge_ssid_label'><%~ Join %>:</label>
				<span>
					<input style="float:left;" type='text' id='bridge_ssid' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
					<button style="float:left;" class='btn btn-default' id='bridge_scan_button' onclick='scanWifi("bridge_ssid")'><%~ Scan %></button>
				</span>
			</div>
			<div id='bridge_broadcast_ssid_container'>
				<label for='bridge_broadcast_ssid' id='bridge_broadcast_ssid_label'><%~ Bcst %>:</label>
				<input class='form-control' type='text' id='bridge_broadcast_ssid' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
			</div>

			<div id='bridge_channel_container'>
				<label class='leftcolumn' for='bridge_channel' id='bridge_channel_label'><%~ WChn %>:</label>
					<select class='form-control' id='bridge_channel' onchange='setChannel(this)'>
					<option value='auto'><%~ auto %></option>
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

			<div id='bridge_channel_5ghz_container'>
				<label class='leftcolumn' for='bridge_channel_5ghz' id='bridge_channel_5ghz_label'><%~ WChn %>:</label>
				<select class='form-control' id='bridge_channel_5ghz' onchange='setChannel(this)' ></select>
			</div>

			<div id='bridge_fixed_channel_container'>
				<label class='leftcolumn' for='bridge_fixed_channel' id='bridge_fixed_channel_label'><%~ WChn %>:</label>
				<span id='bridge_fixed_channel'>&nbsp;</span>
			</div>

			<div id='bridge_encryption_container'>
				<label class='leftcolumn' for='bridge_encryption' id='bridge_encryption_label'><%~ Encr %>:</label>
				<select class='form-control' id='bridge_encryption' onchange='setBridgeVisibility()'>
					<option value='none'><%~ None %></option>
					<option value='psk2'>WPA2 PSK</option>
					<option value='psk'>WPA PSK</option>
					<option value='wep'>WEP</option>
				</select>
			</div>
			<div id='bridge_fixed_encryption_container' >
				<label class='leftcolumn' for='bridge_fixed_encryption' id='bridge_fixed_encryption_label'><%~ Encr %>:</label>
				<span id='bridge_fixed_encryption'>&nbsp;</span>
			</div>

			<div id='bridge_pass_container'>
				<label class='leftcolumn' for='bridge_pass' id='bridge_pass_label'><%~ Pswd %>:</label>
				<input type='password' id='bridge_pass' size='20' class='form-control' onkeyup='proofreadLengthRange(this,8,999)'/><br/>
			</div>
			<div id='bridge_wep_container'>
				<div style="display:block;">
					<label class='leftcolumn' for='bridge_wep' id='bridge_wep_label' ><%~ HexK %>:</label>
					<input type='text' id='bridge_wep' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
				</div>
			</div>

			<div id='bridge_wifi_mac_container' >
				<label class='leftcolumn' id='bridge_wifi_mac_label'><%~ DevMAC %>:</label>
				<span id='bridge_wifi_mac'> </span>
			</div>
			<div id="bridge_wds_container" >
				<label class='leftcolumn' for='bridge_wds_label' id='bridge_wds_label'><%~ OWDS %>:</label>
				<span>
					<input type='text' id='add_bridge_wds_mac' onkeyup='proofreadMac(this)' class='form-control' size='20' maxlength='17' />
					<button class="btn btn-default" id="add_bridge_wds_mac_button" onclick='addMacToWds("bridge")'><%~ Add %></button>
				</span>
				<div class="rightcolumnonly"><div id="bridge_wds_mac_table_container"></div></div>
			</div>

		</div>
	</div>
</div>
</div>


	<div id="wan_fieldset" class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ WANSec %></h3>
			</div>
			<div class="panel-body">

		<div id='wan_protocol_container'>
			<label class='leftcolumn' for='wan_protocol'><%~ Cnct %>:</label>
			<select class='form-control' id='wan_protocol' onchange='setGlobalVisibility()'>
				<option value='dhcp_wired'>DHCP (<%~ Wird %>)</option>
				<option value='pppoe_wired'>PPPoE (<%~ Wird %>)</option>
				<option value='static_wired'><%~ StIP %> (<%~ Wird %>)</option>
				<option value='dhcp_wireless'>DHCP (<%~ Wrlss %>)</option>
				<option value='static_wireless'><%~ StIP %> (<%~ Wrlss %>)</option>
				<option value='3g'><%~ Mo3g %></option>
				<option value='qmi'><%~ Mo3gQMI %></option>
				<option value='ncm'><%~ Mo3gNCM %></option>
				<option value='dhcp_cdc'><%~ Mo3gHiLink %></option>
				<option value='none'><%~ Disabled %></option>
			</select>
		</div>


		<div id='wan_dhcp_ip_container'>
			<label class='leftcolumn'><%~ CurrIP %>:</label>
			<span id='dhcp_ip'></span>
		</div>
		<div id='wan_dhcp_expires_container'>
			<label class='leftcolumn'><%~ CLsExp %>:</label>
			<span>
				<span id="dhcp_expires"></span>
			</span>

			<div class='rightcolumnonly' style="margin-bottom:15px">
				<button id="dhcp_renew_button" class="btn btn-default" onclick="renewDhcpLease()"><%~ Renew %></button>
				<button id="dhcp_release_button" class="btn btn-default" onclick="releaseDhcpLease()"><%~ Rleas %></button>
			</div>
		</div>

		<div id='wan_pppoe_user_container'>
			<label class='leftcolumn' for='wan_pppoe_user' id='wan_pppoe_user_label'><%~ UNam %>:</label>
			<input type='text' class='form-control' id='wan_pppoe_user' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wan_pppoe_pass_container'>
			<label class='leftcolumn' for='wan_pppoe_pass' id='wan_pppoe_pass_label'><%~ Pswd %>:</label>
			<input type='password' class='form-control' id='wan_pppoe_pass' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wan_pppoe_reconnect_mode_container'>
			<label class='leftcolumn' for='wan_pppoe_reconnect_mode'><%~ RMod %>:</label>
			<select class='form-control' id='wan_pppoe_reconnect_mode' onchange='setWanVisibility()'>
				<option value='demand'><%~ CDmd %></option>
				<option value='keepalive'><%~ KAlv %></option>
			</select>
		</div>
		<div id='wan_pppoe_max_idle_container' >
			<label class='leftcolumn' for='wan_pppoe_max_idle' id='wan_pppoe_max_idle_label'><%~ MIdl %>:</label>
			<div>
				<input type='text' class='form-control' id='wan_pppoe_max_idle' onkeyup='proofreadNumeric(this)' size='20' maxlength='4' />
				<em>(<%~ minutes %>)</em>
			</div>
		</div>

		<div id='wan_pppoe_reconnect_pings_container'>
			<label class='leftcolumn' for='wan_pppoe_reconnect_pings' id='wan_pppoe_reconnect_pings_label'><%~ FPngs %>:</label>
			<div>
				<input type='text' id='wan_pppoe_reconnect_pings' onkeyup='proofreadNumeric(this)' class='form-control' size='20' maxlength='4' />
			</div>
		</div>

		<div id='wan_pppoe_interval_container'>
			<label class='leftcolumn' for='wan_pppoe_interval' id='wan_pppoe_interval_label'><%~ PngI %>:</label>
			<div>
				<input type='text' id='wan_pppoe_interval' onkeyup='proofreadNumeric(this)' class='form-control' size='20' maxlength='4' />
				<em>(<%~ seconds %>)</em>
			</div>
		</div>

		<div id='wan_static_ip_container'>
			<label class='leftcolumn' for='wan_static_ip' id='wan_static_ip_label'><%~ StIP %>:</label>
			<input type='text' class='form-control' name='wan_static_ip' id='wan_static_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>
		<div id='wan_static_mask_container'>
			<label class='leftcolumn' for='wan_static_mask' id='wan_static_mask_label'><%~ SMsk %>:</label>
			<input type='text' class='form-control' name='wan_static_mask' id='wan_static_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
		</div>
		<div id='wan_static_gateway_container'>
			<label class='leftcolumn' for='wan_static_gateway' id='wan_static_gateway_label'><%~ Gtwy %>:</label>
			<input type='text' class='form-control' name='wan_static_gateway' id='wan_static_gateway' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>

		<div id='wan_3g_service_container'>
			<label class='leftcolumn' for='wan_3g_service'><%~ Srvc %>:</label>
			<select class='form-control' id='wan_3g_service' onchange="updateService()">
				<option value='cdma'>CDMA/EV-DO</option>
				<option value='umts'><%~ S4G3G2G %></option>
				<option value='umts_pref'><%~ S3GPrfr %></option>
				<option value='gprs_pref'><%~ S2GPrfr %></option>
				<option value='umts_only'><%~ S3GOnly %></option>
				<option value='gprs_only'><%~ S2GOnly %></option>
			</select>
		</div>
		<div id='wan_3g_device_container'>
			<label class='leftcolumn' for='wan_3g_device' id='wan_3g_device_label'><%~ Dvic %>:</label>
			<span>
				<select id="wan_3g_list_device" onchange='set3GDevice(this.value)'></select>
				<input style="float:left;" type='text' class='form-control' id='wan_3g_device' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
				<button style="float:left;" class="btn btn-default" id='wan_3g_scan_button' onclick='scan3GDevice("wan_3g_list_device")'><%~ Scan %></button>
			</span>
		</div>
		<div id='wan_3g_pincode_container'>
			<label class='leftcolumn' for='wan_3g_pincode' id='wan_3g_pincode_label'><%~ Pncd %>:</label>
			<input type='text' class='form-control' id='wan_3g_pincode'  size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
			<em>(<%~ optl %>)</em>
		</div>

		<div id='wan_3g_isp_container'>
			<label class='leftcolumn' for='wan_3g_isp'><%~ MISP %>:</label>
			<select class='form-control' id='wan_3g_isp' onchange='updateApnDetails()'>
			<option value='custom'><%~ Cstm %></option>
			</select>
		</div>

		<div id='wan_3g_apn_container'>
			<label class='leftcolumn' for='wan_3g_apn' id='wan_3g_apn_label'>APN:</label>
			<input type='text' class='form-control' id='wan_3g_apn' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wan_3g_user_container' >
			<label class='leftcolumn' for='wan_3g_user' id='wan_3g_user_label'><%~ UNam %>:</label>
			<input type='text' class='form-control' id='wan_3g_user' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
			<em>(<%~ optl %>)</em>
		</div>
		<div id='wan_3g_pass_container'>
			<label for='wan_3g_pass' id='wan_3g_pass_label'><%~ Pswd %>:</label>
			<input type='text' class='form-control' id='wan_3g_pass' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
			<em>(<%~ optl %>)</em>
		</div>

		<div id='wan_port_to_lan_container'>
			<label for='wan_port_to_lan' id='wan_port_to_lan_label'><%~ WanEP %>:</label>
			<select class='form-control' id='wan_port_to_lan'>
				<option value='disable'><%~ Dsbl %></option>
				<option value='bridge'><%~ BrLAN %></option>
			</select>
		</div>

		<div id='wan_mac_container'>
			<span class='leftcolumn'>
				<label for='wan_use_mac' id='wan_mac_label'><%~ CustMAC %>:</label>
			</span>
			<span class='form-control'>
				<input type='checkbox' id='wan_use_mac' onclick='enableAssociatedField(this, "wan_mac", defaultWanMac)'/>&nbsp;&nbsp;
				<input type='text' name='wan_mac' id='wan_mac' class='form-control' onkeyup='proofreadMac(this)' size='20' maxlength='17'/>
			</span>
		</div>
		<div id='wan_mtu_container'>
			<span class='leftcolumn'>
				<label for='wan_use_mtu' id='wan_mtu_label'><%~ CustMTU %>:</label>
			</span>
			<span>
				<input type='checkbox' id='wan_use_mtu' onclick='enableAssociatedField(this, "wan_mtu", 1500)'/>
				<input type='text' name='wan_mtu' id='wan_mtu' class='form-control' onkeyup='proofreadNumeric(this)' size='20' maxlength='4'/>
			</span>
		</div>
		<div id='wan_ping_container'>
			<span class='rightcolumnonly'>
				<input type='checkbox' id='drop_wan_ping'/>
				<label for='drop_wan_ping' id='wan_ping_label'>&nbsp;&nbsp;<%~ DPing %></label>
			</span>
		</div>

	</div>
</div>
</div>
</div>

<div class="row">

	<div id="lan_fieldset" class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ LANSec %></h3>
			</div>
			<div class="panel-body">

		<div id='lan_ip_container'>
			<label for='lan_ip' id='lan_ip_label'><%~ RtrIP %>:</label>
			<input type='text' class='form-control' name='lan_ip' id='lan_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>
		<div id='lan_mask_container'>
			<label for='lan_mask' id='lan_mask_label'><%~ SMsk %>:</label>
			<input type='text' class='form-control' name='lan_mask' id='lan_mask' onkeyup='proofreadMask(this)' size='20' maxlength='15' />
		</div>
		<div id='lan_gateway_container'>
			<label for='lan_gateway' id='lan_gateway_label'><%~ Gtwy %>:</label>
			<input type='text' class='form-control' name='lan_gateway' id='lan_gateway' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>

		<div id="lan_dns_source_container">
			<span>
				<label id='lan_dns_source_label' for='lan_dns_source'><%~ DnsSvs %>:</label>
			</span>
			<select class='form-control' id="lan_dns_source" onchange="setDnsSource(this)">
				<option value="isp"><%~ DfltDNS %></option>
				<option value="opendns"><%~ OpnSrvs %></option>
				<option value="google"><%~ GooSrvs %></option>
				<option value="custom"><%~ CstDSrv %></option>
			</select>
		</div>
		<div id="lan_dns_custom_container">

			<span>
				<input type='text' id='add_lan_dns' class='form-control' onkeyup='proofreadIp(this)' size='20' maxlength='17' />
				<button class="btn btn-default" id="add_lan_dns_button" onclick='addDns("lan")'><%~ Add %></button>
			</span>
			<span style="clear:both"></span>
			<div id="lan_dns_table_container"></div>
		</div>
		<div id="lan_dns_options_container">
			<span>
				<input type='checkbox' id='lan_dns_altroot' style="padding:0;margin:0px;vertical-align:middle;overflow:hidden;"/>
				<label for="lan_dns_altroot" id="lan_dns_altroot_label" style="vertical-align:middle">&nbsp;&nbsp;<%~ Allow %> <a href="https://bit.namecoin.info">NameCoin</a>/<a href="http://www.opennicproject.org">OpenNIC</a> <%~ Rsln %></label>

			</span>
			<span style="clear:both;"></span>
			<span>
				<input type='checkbox' id='lan_dns_force' style="padding:0;margin:0px;vertical-align:middle;overflow:hidden;"/>
				<label for="lan_dns_force" id="lan_dns_force_label" style="vertical-align:middle">&nbsp;&nbsp;<%~ RtrDNS %></label>
			</span>
		</div>

	</div>
</div>
</div>
</div>

<div id="wifi_fieldset" class="row">

	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Wrlss %></h3>
			</div>
			<div class="panel-body">
		<div id='wifi_mode_container'>
			<label for='wifi_mode' id='wifi_mode_label'><%~ WlMod %>:</label>
			<select id='wifi_mode' class='form-control' onchange='setWifiVisibility()'>
				<option value='ap'><%~ AcPt %> (AP)</option>
				<option value='ap+wds'>AP+WDS</option>
				<option value='sta'><%~ Clnt %></option>
				<option value='ap+sta'><%~ Clnt %>+AP</option>
				<option value='adhoc'>Ad Hoc</option>
				<option value='disabled'><%~ Disabled %></option>
			</select>
		</div>

		<div id='wifi_hwmode_container'>
			<label for='wifi_hwmode' id='wifi_hwmode_label'>2.4GHz <%~ OpMod %>:</label>
			<span>
				<select id='wifi_hwmode' class='form-control' onchange='setHwMode(this)'>
					<option value='disabled'>Disabled</option>
					<option value='11gn'>B+G+N</option>
					<option value='11g'>B+G</option>
				</select>
			</span>
		</div>

		<div id='wifi_channel_width_container'>
			<label for='wifi_channel_width' id='wifi_channel_width_label'>2.4GHz <%~ ChWdth %>:</label>
			<span>
				<select id='wifi_channel_width' class='form-control' onchange='setChannelWidth(this, "G")'>
					<option value='HT20'>20MHz</option>
					<option value='HT40+'>40MHz (<%~ ChAbv %>)</option>
					<option value='HT40-'>40MHz (<%~ ChBlw %>)</option>
				</select>
			</span>
		</div>

		<div id='wifi_txpower_container'>
			<label for='wifi_max_txpower' id='wifi_txpower_label'>2.4GHz <%~ TrPwr %>:</label>
			<span>
				<select id='wifi_max_txpower' class='form-control' onchange='updateTxPower("wifi_max_txpower","wifi_txpower", "G")'>
					<option value='max'><%~ Max %></option>
					<option value='custom'><%~ Cstm %></option>
				</select>
				&nbsp;
				<input type='text' id='wifi_txpower' class='form-control' onkeyup='proofreadNumericRange(this,0,getMaxTxPower("G"))' size='10' />
				<em><span id="wifi_dbm">dBm</span></em>
			</span>
		</div>

		<div id='wifi_hwmode_5ghz_container'>
			<labelfor='wifi_hwmode_5ghz' id='wifi_hwmode_5ghz_label'>5GHz <%~ OpMod %>:</label>
			<span>
				<select id='wifi_hwmode_5ghz' class='form-control' onchange='setHwMode(this)'>
					<option value='disabled'>Disabled</option>
					<option value='11a'>A</option>
					<option value='11an'>A+N</option>
					<option value='11anac'>A+N+AC</option>
				</select>
			</span>
		</div>

		<div id='wifi_channel_width_5ghz_container'>
			<label for='wifi_channel_width_5ghz' id='wifi_channel_width_5ghz_label'>5GHz <%~ ChWdth %>:</label>
			<span>
				<select id='wifi_channel_width_5ghz' class='form-control' onchange='setChannelWidth(this, "A")'>
					<option value='HT20'>20MHz</option>
					<option value='HT40+'>40MHz (<%~ ChAbv %>)</option>
					<option value='HT40-'>40MHz (<%~ ChBlw %>)</option>
					<option value='VHT20'>20MHz</option>
					<option value='VHT40'>40MHz</option>
					<option value='VHT80'>80MHz</option>
				</select>
			</span>
		</div>

		<div id='wifi_txpower_5ghz_container'>
			<label for='wifi_max_txpower_5ghz' id='wifi_txpower_5ghz_label'>5GHz <%~ TrPwr %>:</label>
			<span>
				<select id='wifi_max_txpower_5ghz' class='form-control' onchange='updateTxPower("wifi_max_txpower_5ghz","wifi_txpower_5ghz", "A")'>
					<option value='max'><%~ Max %></option>
					<option value='custom'><%~ Cstm %></option>
				</select>
				&nbsp;
				<input type='text' id='wifi_txpower_5ghz' class='form-control' onkeyup='proofreadNumericRange(this,0,getMaxTxPower("A"));' size='10' />
				<em><span id="wifi_dbm_5ghz">dBm</span></em>
			</span>
		</div>

		<div id="mac_enabled_container">
			<label for='mac_filter_enabled'><%~ WlFltr %>:</label>
			<select class='form-control' id='mac_filter_enabled' onchange='setWifiVisibility()' >
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
		<div id="mac_filter_container">
			<div class="rightcolumnonly">
				<em><%~ FltrInfo %></em>
			</div>
			<div>
				<label class='leftcolumn' for='mac_filter_policy'><%~ MACFiPo %>:</label>
				<select class='form-control' id='mac_filter_policy'>
					<option value='allow'><%~ AllwMAC %></option>
					<option value='deny' ><%~ DnyMAC %></option>
				</select>
			</div>
			<div class='rightcolumnonly'>
				<div>
					<input type='text' id='add_mac' class='form-control' onkeyup='proofreadMac(this)' size='20' maxlength='17' />
					<button class="btn btn-default" id="add_mac_button" onclick="addMacToFilter()"><%~ Add %></button>
				</div>
			</div>
			<div id="mac_table_container"></div>
		</div>

		<div id='internal_divider1' class='internal_divider'></div>

		<div id='wifi_list_ssid2_container'>
			<label for='wifi_list_ssid2' id='wifi_list_ssid2_label'><%~ Join %>:</label>
			<span class="rightcolumn">
				<select id="wifi_list_ssid2" onchange='setSsidVisibility(this.id)'><option value="custom"><%~ Other %></option></select>
				<button class='btn btn-default' id='wifi_rescan_button' onclick='scanWifi("wifi_custom_ssid2")'><%~ RScn %></button>
			</span>
		</div>
		<div id='wifi_custom_ssid2_container'>
			<input type='text' class='form-control' id='wifi_custom_ssid2' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
		</div>
		<div id='wifi_ssid2_container'>
			<label for='wifi_ssid2' id='wifi_ssid2_label'>SSID:</label>
			<span>
				<input type='text' id='wifi_ssid2' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/>
				<button class="btn btn-default" id='wifi_scan_button' onclick='scanWifi("wifi_ssid2")'><%~ Scan %></button>
			</span>
		</div>

		<div id='wifi_client_band_container' class='indent'>
			<label for='wifi_client_band' id='wifi_client_band_label'><%~ WlBnd %>:</label>
			<select class='form-control' id='wifi_client_band' onchange='setHwMode(document.getElementById("wifi_hwmode"))'>
				<option value="2.4">2.4 GHz</option>
				<option value="5">5 GHz</option>
			</select>
		</div>

		<div id='wifi_channel2_container' class='indent'>
			<label for='wifi_channel2' id='wifi_channel2_label'><%~ WChn %>:</label>
			<select class='form-control' id='wifi_channel2' onchange='setChannel(this)' >
				<option value='auto'><%~ auto %></option>
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
		<div id='wifi_fixed_channel2_container' class='indent'>
			<label for='wifi_fixed_channel2' id='wifi_fixed_channel2_label'><%~ WChn %>:</label>
			<span id='wifi_fixed_channel2'>&nbsp;</span>
		</div>
		<div id='wifi_channel2_5ghz_container' class='indent'>
			<label for='wifi_channel2_5ghz' id='wifi_channel2_5ghz_label'><%~ WChn %>:</label>
			<select class='form-control' id='wifi_channel2_5ghz' onchange='setChannel(this)' ></select>
		</div>

		<div id='wifi_encryption2_container' class='indent'>
			<label for='wifi_encryption2' id='wifi_encryption2_label'><%~ Encr %>:</label>
			<select class='form-control' id='wifi_encryption2' onchange='setWifiVisibility()'>
				<option value='none'><%~ None %></option>
				<option value='psk2'>WPA2 PSK</option>
				<option value='psk'>WPA PSK</option>
				<option value='wep'>WEP</option>
			</select>
		</div>
		<div id='wifi_fixed_encryption2_container' class='indent'>
			<label for='wifi_fixed_encryption2' id='wifi_fixed_encryption2_label'><%~ Encr %>:</label>
			<span id='wifi_fixed_encryption2'>&nbsp;</span>
		</div>
		<div id='wifi_pass2_container' class='indent'>
			<label for='wifi_pass2' id='wifi_pass2_label'><%~ Pswd %>:</label>
			<input type='password' id='wifi_pass2' class='form-control' size='20' onkeyup='proofreadLengthRange(this,8,999)'/>&nbsp;&nbsp;
			<input type='checkbox' id='show_pass2' onclick='togglePass("wifi_pass2")'/>
			<label for="show_pass2" id="show_pass2_label"><%~ rvel %></label><br/>
		</div>
		<div id='wifi_wep2_container' class='indent'>
			<div>
				<label for='wifi_wep2' id='wifi_wep2_label'><%~ HexK %>:</label>
				<input type='text' id='wifi_wep2' class='form-control' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
			</div>
		</div>

		<div id='internal_divider2' class='internal_divider'></div>

		<div id='wifi_ssid1_container'>
			<label for='wifi_ssid1' id='wifi_ssid1_label'><%~ AcPtID %>:</label>
			<input type='text' id='wifi_ssid1' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/><br/>
		</div>

		<div id='wifi_ssid1a_container'>
			<label for='wifi_ssid1a' id='wifi_ssid1a_label'>AP 5GHz SSID:</label>
			<input type='text' id='wifi_ssid1a' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/><br/>
		</div>

		<div id='wifi_channel1_container' class='indent'>
			<label for='wifi_channel1' id='wifi_channel1_label'><%~ WChn %>:</label>
			<select class='form-control' id='wifi_channel1' onchange='setChannel(this)' >
				<option value='auto'><%~ auto %></option>
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
		<div id='wifi_fixed_channel1_container' class='indent'>
			<label for='wifi_fixed_channel1' id='wifi_fixed_channel1_label'><%~ WChn %>:</label>
			<span id='wifi_fixed_channel1'>&nbsp;</span>
		</div>

		<div id='wifi_channel1_5ghz_container' class='indent'>
			<label for='wifi_channel1_5ghz' id='wifi_channel1_5ghz_label'><%~ WChn %> (5GHz):</label>
			<select class='form-control' id='wifi_channel1_5ghz' onchange='setChannel(this)' ></select>
		</div>

		<div id='wifi_encryption1_container' class='indent'>
			<label for='wifi_encryption1' id='wifi_encryption1_label'><%~ Encr %>:</label>
			<select class='form-control' id='wifi_encryption1' onchange='setWifiVisibility()'>
				<option value='none'><%~ None %></option>
				<option value='psk2'>WPA2 PSK</option>
				<option value='psk'>WPA PSK</option>
				<option value='wep'>WEP</option>
				<option value='wpa'>WPA RADIUS</option>
				<option value='wpa2'>WPA2 RADIUS</option>
			</select>
		</div>

		<div id='wifi_pass1_container' class='indent'>
			<label for='wifi_pass1' id='wifi_pass1_label'><%~ Pswd %>:</label>
			<input type='password' id='wifi_pass1' class='form-control' size='20' onkeyup='proofreadLengthRange(this,8,999)'/>&nbsp;&nbsp;
			<input type='checkbox' id='show_pass1' onclick='togglePass("wifi_pass1")'/>
			<label for="show_pass1" id="show_pass1_label"><%~ rvel %></label><br/>
		</div>
		<div id='wifi_wep1_container' class='indent'>
			<div style="display:block;">
				<label for='wifi_wep1' id='wifi_wep1_label'><%~ HexK %>:</label>
				<input type='text' id='wifi_wep1' class='form-control' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
			</div>
			<div>
				<button class='btn btn-default' id='wep1gen40' onclick='setToWepKey("wifi_wep1",10)'><%~ Rndm %> 40/64 Bit WEP Key</button>
			</div>
			<div>
				<button class='btn btn-default' id='wep1gen104' onclick='setToWepKey("wifi_wep1",26)'><%~ Rndm %> 104/128 Bit WEP Key</button>
			</div>
		</div>

		<div id='wifi_server1_container' class='indent'>
			<label for='wifi_server1' id='wifi_server1_label'>RADIUS <%~ Srvr %> IP:</label>
			<input type='text' id='wifi_server1' class='form-control' size='20' onkeyup='proofreadIp(this)'/><br/>
		</div>
		<div id='wifi_port1_container' class='indent'>
			<label for='wifi_port1' id='wifi_port1_label'>RADIUS <%~ SrvPt %>:</label>
			<input type='text' id='wifi_port1' class='form-control' size='20' maxlength='5' onkeyup='proofreadNumeric(this)'/><br/>
		</div>

		<div id='wifi_hidden_container' class='indent'>
			<label id='wifi_hidden_label' for='wifi_hidden'><%~ BcsID %>:</label>
			<select id='wifi_hidden' class='form-control'>
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
		<div id='wifi_isolate_container' class='indent'>
			<label id='wifi_isolate_label' for='wifi_isolate'><%~ WlIso %>:</label>
			<select id='wifi_isolate' class='form-control'>
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
		<div id='wifi_mac_container' class="indent">
			<label id='wifi_mac_label'><%~ DevMAC %>:</label>
			<span id='wifi_mac'></span>
		</div>
		<div id="wifi_wds_container" class="indent">
			<label for='wifi_wds_label' id='wifi_wds_label'><%~ OWDS %>:</label>
			<span>
				<input type='text' id='add_wifi_wds_mac' class='form-control' onkeyup='proofreadMac(this)' size='20' maxlength='17' />
				<button class="btn btn-default" id="add_wifi_wds_mac_button" onclick='addMacToWds("wifi")'><%~ Add %></button>
			</span>
			<div id="wifi_wds_mac_table_container"></div>
		</div>

		<div id='internal_divider3' class='internal_divider'></div>

		<div id='wifi_guest_mode_container'>
			<label for='wifi_guest_mode'><%~ GNet %>:</label>
			<select id='wifi_guest_mode' class='form-control' onchange='setWifiVisibility()' >
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
                <div id='wifi_guest_container'>

		<div id='wifi_guest_ssid1_container'>
			<label for='wifi_guest_ssid1' id='wifi_guest_ssid1_label'><%~ GNetID %>:</label>
			<input type='text' id='wifi_guest_ssid1' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/><br/>
		        <input type='text' id='wifi_guest_mac_g' class='form-control' style='display: none'/>
		</div>

		<div id='wifi_guest_ssid1a_container'>
			<label for='wifi_guest_ssid1a' id='wifi_guest_ssid1a_label'><%~ GNet5ID %></label>
			<input type='text' id='wifi_guest_ssid1a' class='form-control' size='20' onkeyup='proofreadLengthRange(this,1,999)'/><br/>
		        <input type='text' id='wifi_guest_mac_a' class='form-control' style='display: none'/>
		</div>

		<div id='wifi_guest_encryption1_container' class='indent'>
			<label for='wifi_guest_encryption1' id='wifi_guest_encryption1_label'><%~ Encr %>:</label>
			<select class='form-control' id='wifi_guest_encryption1' onchange='setWifiVisibility()'>
				<option value='none'><%~ None %></option>
				<option value='psk2'>WPA2 PSK</option>
				<option value='psk'>WPA PSK</option>
				<option value='wep'>WEP</option>
			</select>
		</div>

		<div id='wifi_guest_pass1_container' class='indent'>
			<label for='wifi_guest_pass1' id='wifi_guest_pass1_label'><%~ Pswd %>:</label>
			<input type='password' id='wifi_guest_pass1' class='form-control' size='20' onkeyup='proofreadLengthRange(this,8,999)'/>&nbsp;&nbsp;
			<input type='checkbox' id='show_guest_pass1' onclick='togglePass("wifi_guest_pass1")'/>
			<label for="show_guest_pass1" id="show_guest_pass1_label"><%~ rvel %></label><br/>
		</div>
		<div id='wifi_guest_wep1_container' class='indent'>
			<div class='form-group form-inline'>
				<label for='wifi_guest_wep1' id='wifi_guest_wep1_label'><%~ HexK %>:</label>
				<input type='text' id='wifi_guest_wep1' class='form-control' size='30' maxLength='26' onkeyup='proofreadWep(this)'/>
			</div>
			<div>
				<button class='btn btn-default' id='guestwep1gen40' onclick='setToWepKey("wifi_guest_wep1",10)'><%~ Rndm %> 40/64 Bit WEP Key</button>
			</div>
			<div>
				<button class='btn btn-default' id='guestwep1gen104' onclick='setToWepKey("wifi_guest_wep1",26)'><%~ Rndm %> 104/128 Bit WEP Key</button>
			</div>
		</div>

		<div id='wifi_guest_hidden_container' class='indent'>
			<label id='wifi_guest_hidden_label' for='wifi_guest_hidden'><%~ BcsID %>:</label>
			<select class='form-control' id='wifi_guest_hidden' >
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
		<div id='wifi_guest_isolate_container' class='indent'>
			<label id='wifi_guest_isolate_label' for='wifi_guest_isolate'><%~ WlIso %>:</label>
			<select class='form-control' id='wifi_guest_isolate' >
				<option value='disabled'><%~ Disabled %></option>
				<option value='enabled'><%~ Enabled %></option>
			</select>
		</div>
		</div>
	</div>
</div>
</div>
</div>

	<div id="bottom_button_container">
		<button id="save_button" class="btn btn-info" onclick='saveChanges()'><%~ SaveChanges %></button>
		<button id="reset_button" class="btn btn-warning" onclick='resetData()'/><%~ Reset %></button>
	</div>
	<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>


<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "basic"
%>
