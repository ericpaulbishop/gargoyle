/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);

	}
	else
	{
		document.body.style.cursor="wait";
		document.getElementById("save_button").style.display="none";
		document.getElementById("reset_button").style.display="none";
		document.getElementById("update_container").style.display="block";

		var uci = uciOriginal.clone();
		var uciCompare = uciOriginal.clone();




		var preCommands = "";	
		var allWirelessSections = uci.getAllSections("wireless");
		var allWifiDeviceSections = uci.getAllSectionsOfType("wireless", "wifi-device");
		var firstWirelessDevice = allWifiDeviceSections[0];
		
		if(firstWirelessDevice == null)
		{
			while(allWirelessSections.length > 0)
			{
				var sectionName = allWirelessSections.shift();
				preCommands = preCommands + "uci del wireless." + sectionName + "\n";
				uci.removeSection("wireless", sectionName);
				uciCompare.removeSection("wireless", sectionName);
			}
			preCommands = preCommands + "uci commit\n";

			firstWirelessDevice = wirelessDriver == "broadcom" ? "wl0" : "wifi0";
			preCommands = preCommands + "uci set wireless." + firstWirelessDevice + "=wifi-device\n";
			preCommands = preCommands + "uci set wireless." + firstWirelessDevice + ".type=" + wirelessDriver + "\n";
			preCommands = preCommands + "uci commit\n";
			uci.set("wireless", firstWirelessDevice, "", "wifi-device");
			uci.set("wireless", firstWirelessDevice, "type", wirelessDriver);

		}


		if(document.getElementById('wan_protocol') == 'none')
		{
			uci.remove("network", "wan", "ifname");
		}
		else
		{
			preCommands = preCommands + "uci set network.wan=interface\n";
			if(trueAndVisible('wan_via_wifi', 'wan_via_wifi_container'))
			{
				if(wirelessDriver == 'atheros')
				{
					wirelessClientIf = document.getElementById("wifi_ssid1_container").style.display != 'none' && document.getElementById("wifi_ssid1_container").style.display != 'none' ? 'ath1' : 'ath0';
					uci.set('network', 'wan', 'ifname', wirelessClientIf);
				}
				else
				{
					uci.set('network', 'wan', 'ifname', wirelessIf);
				}
			}
			else if(trueAndVisible('wan_via_single_port', 'wan_via_single_port_container'))
			{
				uci.set('network', 'wan', 'ifname', defaultLanIf);
			}
			else
			{
				uci.set('network', 'wan', 'ifname', defaultWanIf);
			}
		}
		

		
		if(trueAndVisible('wan_port_to_lan', 'wan_port_to_lan_container'))
		{
			uci.set('network', 'lan', 'ifname', defaultLanIf + " " + defaultWanIf);
		}
		else if(trueAndVisible('wan_via_single_port', 'wan_via_single_port_container'))
		{
			//just in case wirelessIf doesn not exist, remove variable first
			uci.remove('network', 'lan', 'ifname');
			uci.set('network', 'lan', 'ifname', wirelessIf);

		}
		else 
		{
			uci.set('network', 'lan', 'ifname', defaultLanIf);
		}

		

		//clear old wifi sections
		wifiCfg2="";
		wifiCfg3="";
		if(allWirelessSections.length >= 2)
		{
			wifiCfg2 = allWirelessSections[1];
			preCommands = preCommands + "uci del wireless." + wifiCfg2 + "\n";
		}
		if(allWirelessSections.length >= 3)
		{
			wifiCfg3 = allWirelessSections[2];
			preCommands = preCommands + "uci del wireless." + wifiCfg3 + "\n";
		}
		uci.removeSection("wireless", wifiCfg2);
		uci.removeSection("wireless", wifiCfg3);
		uciCompare.removeSection("wireless", wifiCfg2);
		uciCompare.removeSection("wireless", wifiCfg3);
		preCommands = preCommands + "uci commit \n";

		//define new sections, now that we have cleared old ones
		//cfg2 should be AP if we have an AP section, otherwise cfg2 is whatever single mode we are in
		currentModes= document.getElementById('wifi_mode').value;
		var section1 = '';
		var section2 = '';
		if(currentModes.match(/ap/))
		{
			section1 = 'cfg2';
			uci.set("wireless", "cfg2", "", "wifi-iface");
			uci.set("wireless", "cfg2", "device", firstWirelessDevice);
			preCommands = preCommands + "uci set wireless.cfg2='wifi-iface' \n"

			if(currentModes.match(/\+/))
			{
				section2 = 'cfg3';
				uci.set("wireless", "cfg3", "", "wifi-iface");
				uci.set("wireless", "cfg3", "device", firstWirelessDevice);
				preCommands = preCommands + "uci set wireless.cfg3='wifi-iface' \n"
			}
		}
		else if(currentModes != 'disabled')
		{
			section2 = 'cfg2';
			uci.set("wireless", "cfg2", "", "wifi-iface");
			uci.set("wireless", "cfg2", "device", firstWirelessDevice);
			preCommands = preCommands + "uci set wireless.cfg2='wifi-iface' \n"
		}

		
		if(section1 != '')
		{
			uci.set('wireless', section1, 'mode', 'ap');
			uci.set('wireless', section1, 'network', 'lan');
		}
		if(section2 != '')
		{
			mode2=currentModes.replace(/\+?ap\+?/, '');
			uci.set('wireless', section2, 'mode', mode2);
			if(!trueAndVisible('wan_via_wifi', 'wan_via_wifi_container'))
			{
				uci.set('wireless', section2, 'network', 'lan');
			}
		}
		//always remove this option, if wireless is set to disabled merely delete all interface sections
		uci.remove('wireless', firstWirelessDevice, 'disabled'); 



		//set mac filter variables
		macFilterCheck = document.getElementById("mac_filter_enabled");
		var macTable = document.getElementById('mac_table_container').firstChild;
		var macList = getTableDataArray(macTable, true, false);
		var policy = getSelectedValue("mac_filter_policy");			
		var macListStr = macList.join(" ");
		if(wirelessDriver == "broadcom")
		{
			if( (!macFilterCheck.checked) || macListStr == '')
			{
				uci.remove("wireless", firstWirelessDevice, policyOption);
				uci.remove("wireless", firstWirelessDevice, "maclist");
			}
			else
			{
				uci.set("wireless", firstWirelessDevice, policyOption, policy);
				uci.set("wireless", firstWirelessDevice, "maclist", macListStr);
			}
		}
		else if(wirelessDriver == "atheros")
		{
			for(wsecIndex=0; wsecIndex < allWirelessSections.length; wsecIndex++)
			{
				var sectionType = uci.get("wireless", allWirelessSections[wsecIndex], "");
				if( (!macFilterCheck.checked) || macListStr == '' || sectionType != "wifi-iface")
				{
					uci.remove("wireless", allWirelessSections[wsecIndex], policyOption);
					uci.remove("wireless", allWirelessSections[wsecIndex], "maclist");
				}
				else
				{
					uci.set("wireless", allWirelessSections[wsecIndex], policyOption, policy);
					uci.set("wireless", allWirelessSections[wsecIndex], "maclist", macListStr);
				}
			}	
		}


		//if current dhcp range is not in new subnet, or current dhcp range contains new router ip adjust it
		var dhcpSection = getDhcpSection(uciOriginal);
		var newMask = document.getElementById("lan_mask").value;
		var newIp = document.getElementById("lan_ip").value;
		var routerIpEnd = parseInt( (newIp.split("."))[3] );
		var oldStart = parseInt( uciOriginal.get("dhcp", dhcpSection, "start") );
		var oldEnd = oldStart + parseInt( uciOriginal.get("dhcp", dhcpSection, "limit") ) - 1;
		if(!rangeInSubnet(newMask, newIp, oldStart, oldEnd) || (routerIpEnd >= oldStart && routerIpEnd <= oldEnd))
		{
			//compute new dhcp range, note this cannot include router's static ip
			var newRange =getSubnetRange(newMask, newIp);
			var newStart;
			var newEnd;
			if(routerIpEnd - newRange[0] > newRange[1] - routerIpEnd)
			{
				newStart = newRange[0];
				newEnd = routerIpEnd-1;
			}
			else
			{
				newStart = routerIpEnd+1;
				newEnd = newRange[1];
			}
			uci.set("dhcp", dhcpSection, "start", newStart);
			uci.set("dhcp", dhcpSection, "limit", (newEnd+1-newStart) );
		}


		ppoeReconnectIds = ['wan_pppoe_reconnect_pings', 'wan_pppoe_interval'];
		dnsIds = ['lan_dns1', 'lan_dns2', 'lan_dns3'];
		inputIds = ['wan_protocol', 'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', ppoeReconnectIds, 'wan_static_ip', 'wan_static_mask', 'wan_mac', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', dnsIds, 'wifi_channel', 'wifi_ssid1', 'wifi_hidden', 'wifi_isolate', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1' , 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_encryption2', 'wifi_pass2', 'wifi_wep2'];
		
		options = ['proto', 'username', 'password', 'demand', 'keepalive', 'ipaddr', 'netmask', 'macaddr', 'mtu', 'ipaddr', 'netmask', 'gateway', 'dns', 'channel', 'ssid', 'hidden', 'isolate', 'encryption', 'key', 'key', 'server', 'port', 'ssid', 'encryption', 'key', 'key'];
		
		var sv=  setVariableFromValue;
		var svm= setVariableFromModifiedValue;
		var svcat= setVariableFromConcatenation;
		var svcond= setVariableConditionally;
		setFunctions = [sv,sv,sv,svm,svcat,sv,sv,svcond,svcond,sv,sv,sv,svcat,sv,sv,svcond,svcond,sv,sv,sv,sv,sv,sv,sv,sv,sv];
		
		var f=false;
		var t=true;
		var minutesToSeconds = function(value){return value*60;};
		var lowerCase = function(value) { return value.toLowerCase(); }
		var ifCustomMac = function(value){ return (document.getElementById('wan_use_mac').checked == true); };
		var ifCustomMtu = function(value){ return (document.getElementById('wan_use_mtu').checked == true &&  document.getElementById('wan_mtu').value != 1500);};
		var ifHiddenChecked =  function(value) { return document.getElementById('wifi_hidden').checked;};
		var ifIsolateChecked = function(value) { return document.getElementById('wifi_isolate').checked;};
		var demandParams = [f,minutesToSeconds];
		var macParams = [ifCustomMac,f,  document.getElementById('wan_mac').value.toLowerCase()];
		var mtuParams = [ifCustomMtu,t,''];
		var hiddenParams = [ifHiddenChecked,f,'1'];
		var isolateParams = [ifIsolateChecked,f,'1'];
		
		additionalParams = [f,f,f, demandParams,f,f,f,macParams,mtuParams,f,f,f,f,f,f,hiddenParams,isolateParams,f,f,f,f,f,f,f,f,f];

		
		
		dnsVisibilityIds = ['lan_dns_container', 'lan_dns_container', 'lan_dns_container'];
		pppoeReconnectVisibilityIds = ['wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container'];
		multipleVisibilityIds= [pppoeReconnectVisibilityIds, dnsVisibilityIds];
		wirelessSections=[firstWirelessDevice, section1, section1, section1, section1, section1, section1, section1, section1, section2, section2, section2, section2, section2];
		visibilityIds = [];
		pkgs = [];
		sections = [];
		var idIndex;
		for (idIndex=0; idIndex < inputIds.length; idIndex++)
		{
			if(isArray(inputIds[idIndex]))
			{
				visibilityIds.push(multipleVisibilityIds.shift());
			}
			else
			{
				visibilityIds.push(inputIds[idIndex]+ "_container");
			}
			
			
			if(idIndex < 9)
			{
				pkgs.push('network');
				sections.push('wan');
				uci.remove('network', 'wan', options[idIndex]);
			}
			else if(idIndex < 13)
			{
				pkgs.push('network');
				sections.push('lan')
				uci.remove('network', 'lan', options[idIndex]);
			}
			else
			{
				pkgs.push('wireless');
				sections.push(wirelessSections.shift());
			}
		}

		setVariables(inputIds, visibilityIds, uci, pkgs, sections, options, setFunctions, additionalParams);

		
		//if wan protocol is 'none' do not set it
		if(uci.get('network', 'wan', 'proto') == 'none')
		{
			uci.remove('network', 'wan', 'proto');
		}
		if(uci.get('network', 'lan', 'proto') === '')
		{
			uci.set('network', 'lan', 'proto', 'static');
		}
			

		//preserve wan mac definition even if wan is disabled if this is a bcm94704
		if(isBcm94704 && (uci.get("network", "wan", "ifname") != wirelessIf))
		{
			if(uci.get("network", "wan", "macaddr") == "")
			{
				uci.set("network", "wan", "macaddr", defaultWanMac);
			}
		}
		
		//update lan dns to include router ip
		var setDns = uci.get("network", "lan", "dns");
		var currentLanIp = uci.get("network", "lan", "ipaddr");
		if( !setDns.match(currentLanIp))
		{
			uci.set("network", "lan", "dns", currentLanIp + " " + setDns);
		}


		//In X-Wrt option defaultroute = 1 is set for wan section when pppoe is active
		//I have no idea how this solves the issue, but people report this makes pppoe work
		//So, let's try it...
		if(getSelectedValue("wan_protocol", document) == "pppoe")
		{
			uci.set("network", "wan", "defaultroute", "1");
		}
		else
		{
			uci.remove("network", "wan", "defaultroute");
		}



		var oldLanIp = uciOriginal.get("network", "lan", "ipaddr");
		var adjustIpCommands = ""
		if(oldLanIp != currentLanIp && oldLanIp != "" && currentLanIp != "")
		{
			adjustIpCommands = "\nsh " + gargoyleBinRoot + "/utility/update_router_ip.sh " + oldLanIp + "  " + currentLanIp;
		}

		
		var commands = uci.getScriptCommands(uciCompare);
		var restartNetworkCommand = "\nsh " + gargoyleBinRoot + "/utility/restart_network.sh ;\n";
		commands = preCommands + commands + adjustIpCommands + restartNetworkCommand;

		
		//document.getElementById("output").value = commands;

		var param = getParameterDefinition("commands", commands);
		

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				document.getElementById("update_container").style.display="none";		
				document.getElementById("save_button").style.display="inline";
				document.getElementById("reset_button").style.display="inline";
				document.body.style.cursor='auto';
			
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	
		
		if(oldLanIp != currentLanIp)
		{
			doRedirect= function()
			{
				currentProtocol = location.href.match(/^https:/) ? "https" : "http";
				window.location = currentProtocol + "://" + currentLanIp + ":" + window.location.port + window.location.pathname;
			}
			setTimeout( "doRedirect()", 30000);
		}
	}
}


function generateWepKey(length)
{
	var keyIndex = 0;
	var key = "";
	var hex = ['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'];
	while(keyIndex < length)
	{
		next=hex[Math.floor(Math.random()*16)];
		key=key+next;
		keyIndex++;
	}
	return key;
}

function setToWepKey(id, length)
{
	document.getElementById(id).value = generateWepKey(length);
	proofreadWep(document.getElementById(id));
}


function proofreadAll()
{
	inputIds = ['wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', 'wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_mac', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', 'wifi_ssid1', 'wifi_pass1', 'wifi_wep1', 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_pass2', 'wifi_wep2'];
	

	vlr1 = function(text){return validateLengthRange(text,1,999);};
	vlr8 = function(text){return validateLengthRange(text,8,999);};
	vip = validateIP;
	vnm = validateNetMask;
	vm = validateMac;
	vn = validateNumeric;
	vw = validateWep;
	
		
	
	functions= [vlr1, vlr1, vn, vn, vn, vip, vnm, vm, vn, vip, vnm, vip, vlr1, vlr8, vw, vip, vn, vlr1, vlr8, vw];
	
	returnCodes= new Array();
	visibilityIds = new Array();
	for (idIndex in inputIds)
	{
		returnCodes.push(0);
		visibilityIds.push( inputIds[idIndex] + "_container" );
	}
	
	
	for(dnsNum=1; dnsNum<=3; dnsNum++)
	{
		dnsId="lan_dns" + dnsNum;
		dnsValue=document.getElementById(dnsId).value;
		if(dnsValue != '')
		{
			inputIds.push(dnsId);
			functions.push(vip);
			returnCodes.push(0);
			visibilityIds.push( 'lan_dns_container' );
		}
	}
	
	
	
	
	
	labelIds = new Array();
	for (idIndex in inputIds)
	{
		labelIds.push( inputIds[idIndex] + "_label");
	}
	return proofreadFields(inputIds, labelIds, functions, returnCodes, visibilityIds);
}


function setGlobalVisibility()
{
	//deal with possibility of wireless routed WAN 
	globalIds=['wan_via_wifi_container', 'wan_via_single_port_container', 'wan_port_to_lan_container'];
	wirelessWanVisibility       = defaultWanIf != '' ? [1,0,1,0] : [1,0,0,0];
	defaultVisibility           = defaultWanIf != '' ? [1,0,0,1] : [1,1,0,1];



	selectedVisibility=defaultVisibility;
	if( document.getElementById('wan_via_wifi').checked == true)
	{
		selectedVisibility=wirelessWanVisibility;

		setAllowableSelections('wan_protocol', ['dhcp', 'static'], ['DHCP', 'PPPoE', 'Static']);


		currentMode=getSelectedValue('wifi_mode');
		setAllowableSelections('wifi_mode', ['sta', 'ap+sta'], ['Client', 'AP+Client']);
	       	if(currentMode == 'ap')
		{
			setSelectedValue("wifi_mode", 'ap+sta');
		}
		else if(currentMode == 'disabled')
		{
			setSelectedValue("wifi_mode", 'sta');
		}
	}
	else
	{
		setAllowableSelections('wan_protocol', ['dhcp', 'pppoe', 'static', 'none'], ['DHCP', 'PPPoE', 'Static', 'Disabled']);
		currentMode=getSelectedValue('wifi_mode');
		setAllowableSelections('wifi_mode', ['ap', 'adhoc', 'disabled'], ['Access Point (AP)', 'Ad Hoc', 'Disabled']);
		if(currentMode == 'ap+sta' || currentMode == 'sta')
		{
			setSelectedValue('wifi_mode', 'ap');
		}
		else
		{
			setSelectedValue('wifi_mode', currentMode);
		}
	}


	if(defaultWanIf == '')
	{
		if(document.getElementById('wan_via_wifi').checked == true || document.getElementById('wan_via_single_port').checked == true)
		{
			setAllowableSelections('wan_protocol', ['dhcp', 'static'], ['DHCP', 'Static']);
		}
		else
		{
			setSelectedValue("wan_protocol", 'none' );
			setAllowableSelections('wan_protocol', ['none'], ['Disabled']);
		}
	
		if(document.getElementById('wan_via_single_port').checked == true)
		{
			selectedVisibility[0] = 0;
		}
	}
	
	setVisibility(globalIds, selectedVisibility);
	


	
	setWanVisibility();
	setWifiVisibility();
}




function setWanVisibility()
{
	wanIds=['wan_pppoe_user_container', 'wan_pppoe_pass_container', 'wan_pppoe_reconnect_mode_container', 'wan_pppoe_max_idle_container', 'wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container', 'wan_static_ip_container', 'wan_static_mask_container', 'wan_mac_container', 'wan_mtu_container', 'lan_gateway_container'];

	notWifi= document.getElementById('wan_via_wifi').checked == true ? 0 : 1;

	dhcpVisability     = [0,0,0,0,0,0,  0,0,  notWifi,notWifi, 0];
	pppoeVisability    = [1,1,1,1,1,1,  0,0,  notWifi,notWifi, 0];
	staticVisability   = [0,0,0,0,0,0,  1,1,  notWifi,notWifi, 0];
	disabledVisability = [0,0,0,0,0,0,  0,0,  0,0,             1];
	
	wanVisibilities= new Array();
	wanVisibilities['dhcp'] = dhcpVisability;
	wanVisibilities['pppoe'] = pppoeVisability;
	wanVisibilities['static'] = staticVisability;
	wanVisibilities['none'] = disabledVisability;
	
	selectedVisibility= wanVisibilities[document.getElementById('wan_protocol').value];
	selectedVisibility[3] = selectedVisibility[3] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'demand' ? 1 : 0;
	selectedVisibility[4] = selectedVisibility[4] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;
	selectedVisibility[5] = selectedVisibility[5] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;

	setVisibility(wanIds, selectedVisibility);
}

function setWifiVisibility()
{
	wifiIds=['wifi_channel_container', 'mac_enabled_container', 'mac_filter_container', 'internal_divider1', 'wifi_ssid1_container', 'wifi_hidden_container', 'wifi_isolate_container', 'wifi_encryption1_container', 'wifi_pass1_container', 'wifi_wep1_container', 'wifi_server1_container', 'wifi_port1_container', 'internal_divider2', 'wifi_ssid2_container', 'wifi_encryption2_container', 'wifi_pass2_container', 'wifi_wep2_container'];

	mf = document.getElementById("mac_filter_enabled").checked ? 1 : 0;
	d1 = mf == 1 ? 0 : 1;
	e1 = document.getElementById('wifi_encryption1').value;
	p1 = (e1 != 'none' && e1 != 'wep') ? 1 : 0;
	w1 = (e1 == 'wep') ? 1 : 0;
	r1 = (e1 == 'wpa' || e1 == 'wpa2') ? 1 : 0;
	e2 = document.getElementById('wifi_encryption2').value;
	p2 = (e2 != 'none' && e2 != 'wep') ? 1 : 0;
	w2 = (e2 == 'wep') ? 1 : 0;

	wifiVisibilities = new Array();
	wifiVisibilities['ap']       = [1,  1,mf,   0,1,1,1,1,p1,w1,r1,r1,  0,0,0,0,0 ];
	wifiVisibilities['sta']      = [1,  1,mf,   0,0,0,0,0,0,0,0,0,      0,1,1,p2,w2];
	wifiVisibilities['ap+sta']   = [1,  1,mf,  d1,1,1,1,1,p1,w1,r1,r1,  1,1,1,p2,w2];
	wifiVisibilities['adhoc']    = [1,  1,mf,   0,0,0,0,0,0,0,0,0,      0,1,1,p2,w2];
	wifiVisibilities['disabled'] = [0,  0,0,    0,0,0,0,0,0,0,0,0,      0,0,0,0,0 ];
	
	wifiMode=document.getElementById("wifi_mode").value;
	wifiVisibility = wifiVisibilities[ wifiMode ];
	
	if(wifiMode == 'adhoc')
	{
		setAllowableSelections('wifi_encryption2', ['none', 'wep'], null);
		document.getElementById("wifi_ssid2_label").firstChild.data = "SSID:";
	}
	else
	{
		document.getElementById("wifi_ssid2_label").firstChild.data = "SSID to Join:";
		if( (wifiMode == 'ap+sta' || wifiMode == 'sta') && wirelessDriver == 'atheros')
		{
			//for some reason atheros in client (sta) mode doesn't like connecting to WPA2 but, does fine with WPA
			setAllowableSelections('wifi_encryption2', ['none', 'psk', 'wep'], ['None', 'WPA PSK', 'WEP']);
		}
		else
		{
			setAllowableSelections('wifi_encryption2', ['none', 'psk2', 'psk', 'wep'], ['None', 'WPA2 PSK', 'WPA PSK', 'WEP']);

		}
	}
	
	setVisibility(wifiIds, wifiVisibility);

}

function resetData()
{
	//first load basic variables for wan & lan sections
	networkIds = ['wan_protocol', 'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle','wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_use_mac', 'wan_mac', 'wan_use_mtu', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', 'lan_dns1', 'lan_dns2', 'lan_dns3'];
	networkPkgs = new Array();
	for (idIndex in networkIds)
	{
		networkPkgs.push('network');
	}

	networkSections = ['wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'lan', 'lan', 'lan', 'lan', 'lan', 'lan'];
	networkOptions  = ['proto', 'username', 'password', 'demand', 'keepalive', 'keepalive', 'ipaddr', 'netmask', 'macaddr','macaddr', 'mtu', 'mtu', 'ipaddr', 'netmask', 'gateway', 'dns', 'dns', 'dns'];


	
	dnsList = uciOriginal.get('network', 'lan', 'dns');
	dnsSplit = dnsList.split(/[\t ]+/);
	dnsStartIndex = dnsSplit[0] == uciOriginal.get('network', 'lan', 'ipaddr') ? 1 : 0;

	pppoeDemandParams = [5*60,1/60];
	pppoeReconnectParams = [3,0];
	pppoeIntervalParams = [5,1];
	dns1Params = ['', dnsStartIndex];
	dns2Params = ['', dnsStartIndex+1];
	dns3Params = ['', dnsStartIndex+2];
	useMtuTest = function(v){return (v=='' || v==null || v==1500 ? false : true);}
	useMacTest = function(v){v = (v== null ? '' : v);  return (v=='' || v.toLowerCase()==defaultWanMac.toLowerCase() ? false : true);}

	networkParams = ['dhcp', '', '', pppoeDemandParams, pppoeReconnectParams, pppoeIntervalParams, '10.1.1.10', '255.255.255.0', useMacTest, defaultWanMac, useMtuTest, 1500, '192.168.1.1', '255.255.255.0', '192.168.1.1', dns1Params,dns2Params,dns3Params];

	

	lv=loadValueFromVariable;
	lsv=loadSelectedValueFromVariable;
	lvm=loadValueFromVariableMultiple;
	lvi=loadValueFromVariableAtIndex;
	lc=loadChecked;
	networkFunctions = [lsv,lv,lv,lvm,lvi,lvi,lv,lv,lc,lv,lc,lv,lv,lv,lv,lvi,lvi,lvi];
	
	loadVariables(uciOriginal, networkIds, networkPkgs, networkSections, networkOptions, networkParams, networkFunctions);

	if(uciOriginal.get('network', 'wan', 'proto') == '')
	{
		document.getElementById('wan_protocol').value='none';
	}	
	
	enableAssociatedField(document.getElementById('wan_use_mac'), 'wan_mac', defaultWanMac);
	enableAssociatedField(document.getElementById('wan_use_mtu'), 'wan_mtu', 1500);


	//note: we have to set pppoe_reconnect_mode in a custom manner, it is a bit non-standard
	keepalive=uciOriginal.get("network", "wan", "keepalive");
	demand=uciOriginal.get("network", "wan", "demand");
	reconnect_mode=(keepalive != '' || demand == '') ? 'keepalive' : 'demand';
	document.getElementById("wan_pppoe_reconnect_mode").value = reconnect_mode;
	
	
	//now load wireless variables
	var allWirelessSections = uciOriginal.getAllSections("wireless");
	var allWifiDeviceSections = uciOriginal.getAllSectionsOfType("wireless", "wifi-device");
	var firstWirelessDevice = allWifiDeviceSections[0];

	wifiCfg2="";
	wifiCfg3="";
	if(allWirelessSections.length >= 2)
	{
		wifiCfg2 = allWirelessSections[1];
	}
	if(allWirelessSections.length >= 3)
	{
		wifiCfg3 = allWirelessSections[2];
	}
	cfg2mode=uciOriginal.get("wireless", wifiCfg2, "mode");
	cfg3mode=uciOriginal.get("wireless", wifiCfg3, "mode");
	apcfg=  cfg2mode== 'ap' ? wifiCfg2 : (cfg3mode=='ap' ? wifiCfg3 : '' );
	othercfg= apcfg== wifiCfg3 || apcfg== '' ? wifiCfg2 : wifiCfg3;

	if( ( cfg2mode == 'sta' || cfg3mode == 'sta') && wirelessDriver == 'atheros' && uciOriginal.get("wireless", othercfg, "encryption") == "psk2")
	{
		uciOriginal.set("wireless", othercfg, "encryption", "psk");
	}



	wirelessIds=['wifi_channel', 'wifi_ssid1', 'wifi_hidden', 'wifi_isolate', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1', 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_encryption2', 'wifi_pass2', 'wifi_wep2'];
	wirelessPkgs= new Array();
	for( wIndex in wirelessIds)
	{
		wirelessPkgs.push('wireless');
	}
	wirelessSections=[firstWirelessDevice, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, othercfg, othercfg, othercfg, othercfg];
	wirelessOptions=['channel', 'ssid', 'hidden', 'isolate', 'encryption', 'key', 'key', 'server', 'port', 'ssid', 'encryption', 'key','key'];
	ifOne = function(p) { return p == 1 ? true : false; };
	wirelessParams=['5', 'Gargoyle', ifOne, ifOne, 'none', '', '', '', '', 'OpenWrt', 'none', '',''];
	wirelessFunctions=[lsv,lv,lc,lc,lsv,lv,lv,lv,lv,lv,lsv,lv,lv];
	
	resetWirelessMode();
	loadVariables(uciOriginal, wirelessIds, wirelessPkgs, wirelessSections, wirelessOptions, wirelessParams, wirelessFunctions);	


	setSelectedValue('wifi_channel', uciOriginal.get("wireless", firstWirelessDevice, "channel"));




	
	var macFilterCheck = document.getElementById("mac_filter_enabled");
	macFilterCheck.checked = false;
	var macListStr = '';
	var policy = '';
	if(wirelessDriver == "broadcom")
	{
		policy = uciOriginal.get("wireless", firstWirelessDevice, policyOption);
		macListStr = uciOriginal.get("wireless", firstWirelessDevice, "maclist");
		macFilterCheck.checked = ( (policy == "allow" || policy == "deny" || policy == "1" || policy == "2" ) && macListStr != "");
	}
	else if(wirelessDriver == "atheros")
	{
		/*
		Atheros has definitions in interface sections, broadcom in wifi-device section.
		To keep consistency we apply first atheros mac filter defined (if any) to all sections
		
		Granted, this means you can not use the enhanced atheros functionality of specifying mac
		filters on a per-interface basis.  However, I believe consistency is more important than
		flexibility in this case.  The interface will seem to the user to be identical on all platforms
		and that is my highest priority.
		
		Here, if mac filter is active on any interface we apply to all of them
		I am not sure this is the best policy, but the alternative is ditching the filter
		which someone may want and went to some trouble to configure.  I do it this way
		because the majority of the time users will only have one AP interface
		*/
		for(wsecIndex=0; wsecIndex < allWirelessSections.length && macFilterCheck.checked == false; wsecIndex++)
		{
			macListStr = uciOriginal.get("wireless", allWirelessSections[wsecIndex], "maclist");
			if(macListStr != '')
			{
				policy = uciOriginal.get("wireless", allWirelessSections[wsecIndex], policyOption);
				macFilterCheck.checked = true;
			}
		}
	}
	if(policy == '1' || policy == 'deny')
	{
		setSelectedValue("mac_filter_policy", "deny");
	}
	else
	{
		setSelectedValue("mac_filter_policy", "allow");
	}

	macList = macListStr.split(/[;,\t ]+/);
	macTableData=[];
	if(macListStr.match(/:/))
	{
		for(macIndex=0; macIndex < macList.length; macIndex++)
		{
			macTableData.push([ macList[macIndex] ]);
		}
	}

	macTable=createTable([""], macTableData, "mac_table", true, false);
	tableContainer = document.getElementById('mac_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(macTable);





	encryptNum = 1;
	while (encryptNum <= 2)
	{
		encMode = document.getElementById('wifi_encryption' + encryptNum).value;
		if(encMode == 'wep')
		{
			document.getElementById('wifi_pass' + encryptNum).value = '';
		}
		else if(encMode != 'none')
		{
			document.getElementById('wifi_wep' + encryptNum).value = '';
		}
		else
		{
			document.getElementById('wifi_pass' + encryptNum).value = '';
			document.getElementById('wifi_wep' + encryptNum).value = '';
		}
		encryptNum++;
	}
	//finally deal with wifi routing/bridging/single port variables
	wanUciIf= uciOriginal.get('network', 'wan', 'ifname');
	lanUciIf= uciOriginal.get('network', 'lan', 'ifname');


	wanIsWifi = wanUciIf != '' && (wanUciIf == wirelessIf || wanUciIf == firstWirelessDevice || wanUciIf.match(/^ath/));

	document.getElementById('wan_via_single_port').checked = (wanUciIf == defaultLanIf && defaultWanIf == '');
	document.getElementById('wan_via_wifi').checked = wanIsWifi;
	document.getElementById('wan_port_disabled').checked = (lanUciIf.indexOf(defaultWanIf) < 0);
	document.getElementById('wan_port_to_lan').checked = (lanUciIf.indexOf(defaultWanIf) >= 0);

	proofreadAll();	
	setGlobalVisibility();
}



function resetWirelessMode()
{
	setSelectedValue('wifi_mode', getWirelessMode(uciOriginal));
}

function validateWep(text)
{
	return (validateHex(text) == 0 && (text.length == 10 || text.length == 26)) ? 0 : 1;
}

function proofreadWep(input)
{
	proofreadText(input, validateWep, 0);
}


function addMacToFilter()
{
	macStr=document.getElementById("add_mac").value;
	if(validateMac(macStr) != 0)
	{
		alert("ERROR: Specified MAC is not valid.");
	}
	else
	{
		macStr = macStr.toUpperCase();
		macTable = document.getElementById('mac_table_container').firstChild;
		macData = getTableDataArray(macTable, true, false);
		inTable = false;
		for(macIndex = 0; macIndex < macData.length; macIndex++)
		{
			var testMac = macData[macIndex];
			inTable = inTable || (macStr == testMac);
		}
		if(inTable)
		{
			alert("ERROR: Duplicate MAC.");
		}
		else
		{
			addTableRow(macTable, [macStr], true, false, null, null );
			document.getElementById("add_mac").value = "";
		}
	}
}

