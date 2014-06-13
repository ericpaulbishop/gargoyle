/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
 
var basicS=new Object(); //part of i18n

var scannedSsids = [[],[],[],[],[]];
var toggleReload = false;
var currentLanIp;

var googleDns = ["8.8.8.8", "8.8.4.4" ];
var openDns = ["208.67.222.222", "208.67.220.220" ];

var ncDns  = [ "178.32.31.41", "106.187.47.17", "176.58.118.172" ]
var onDns  = [ "66.244.95.20", "95.211.32.162", "95.142.171.235" ]
var ncTlds = [ ".bit" ];
var onTlds = [ ".glue", ".parody", ".dyn", ".bbs", ".free", ".fur", ".geek", ".gopher", ".indy", ".ing", ".null", ".oss", ".micro" ];

function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		var uci = uciOriginal.clone();
		var uciCompare = uciOriginal.clone();

		var doReboot= (!isBridge(uciOriginal)) && document.getElementById("global_bridge").checked;
		if(doReboot)
		{
			setControlsEnabled(false, true, basicS.WaitRb);
		}
		else
		{
			setControlsEnabled(false, true, UI.WaitSettings);
		}

		var preCommands = "";

		if(wifiDevG == "" && wirelessDriver != "")
		{
			wifiDevG = wirelessDriver == "broadcom" ? "wl0" : ( wirelessDriver == "atheros" ? "wifi0" : "radio0");
			preCommands = preCommands + "uci set wireless." + wifiDevG + "=wifi-device\n";
			preCommands = preCommands + "uci set wireless." + wifiDevG + ".type=" + wirelessDriver + "\n";
			preCommands = preCommands + "uci commit\n";
			uci.set("wireless", wifiDevG, "", "wifi-device");
			uci.set("wireless", wifiDevG, "type", wirelessDriver);
			if(wirelessDriver == "mac80211")
			{
				uci.set("wireless", wifiDevG, "hwmode", "11g");
			}
		}

		//clear all old wifi-iface sections
		var wifiDelIndex=0;
		var allWirelessInterfaceSections = uci.getAllSectionsOfType("wireless", "wifi-iface");
		for(wifiDelIndex=0; wifiDelIndex < allWirelessInterfaceSections.length; wifiDelIndex++)
		{
			var delSection = allWirelessInterfaceSections[wifiDelIndex];
			if(uci.get("wireless", delSection, "") == "wifi-iface")
			{
				uci.removeSection("wireless", delSection);
				uciCompare.removeSection("wireless", delSection);
				preCommands = preCommands + "uci del wireless." + delSection + "\n";
			}
		}
		preCommands = preCommands + "uci commit \n";

		//always remove wireless disabled option, if wireless is set to disabled merely delete all interface sections
		if(wifiDevG != "") { uci.remove('wireless', wifiDevG, 'disabled'); }
		if(wifiDevA != "") { uci.remove('wireless', wifiDevA, 'disabled'); }

		var txPowerSet = function(sel_id, txt_id, dev)
		{
			if(getSelectedValue(sel_id) == "max")
			{
				uci.remove("wireless", dev, "txpower")
			}
			else
			{
				uci.set("wireless", dev, "txpower", document.getElementById(txt_id).value);
			}
		}
		var channels = getSelectedWifiChannels()
		if(channels["G"] != "")
		{
			uci.set("wireless", wifiDevG, "channel", channels["G"]);
			if( document.getElementById("wifi_channel_width_container").style.display == "block" || document.getElementById("bridge_channel_width_container").style.display == "block" )
			{
				uci.set("wireless", wifiDevG, "htmode",  getSelectedValue("wifi_channel_width") );
			}
			txPowerSet("wifi_max_txpower", "wifi_txpower", wifiDevG)
		}
		if(channels["A"] != "")
		{
			uci.set("wireless", wifiDevA, "channel", channels["A"]);
			uci.set("wireless", wifiDevA, "htmode",  getSelectedValue("wifi_channel_width_5ghz") );
			txPowerSet("wifi_max_txpower_5ghz", "wifi_txpower_5ghz", wifiDevA)
		}

		currentLanIp = "";
		var adjustIpCommands = ""
		var bridgeEnabledCommands = "";
		if( document.getElementById("global_gateway").checked )
		{
			var wifiGSelected = true;
			var wifiASelected = false;
			var dualBandSelected = false;
			if(document.getElementById("wifi_hwmode_container").style.display == "block")
			{
				var hwMode = getSelectedValue("wifi_hwmode");
				var hwGMode = hwMode == "dual" || hwMode == "11na" ? "11ng" : hwMode;
				uci.set("wireless",  wifiDevG, "hwmode", hwGMode);
				wifiASelected = hwMode == "dual" || hwMode == "11na";
				wifiGSelected = hwMode != "11na";
				dualBandSelected = hwMode == "dual";
			}

			currentLanIp = document.getElementById("lan_ip").value;
			if(getSelectedValue('wan_protocol') == 'none')
			{
				preCommands = preCommands + "\nuci del network.wan\nuci commit\n";
				uci.removeSection("network", "wan");
				uciCompare.removeSection("network", "wan");
			}
			else
			{
				preCommands = preCommands + "\nuci set network.wan=interface\n";
				uci.remove('network', 'wan', 'type');
				if(getSelectedValue("wan_protocol").match(/wireless/))
				{
					uci.remove('network', 'wan', 'ifname');
					uci.set('network', 'wan', 'type', 'bridge');
				}
				else if( singleEthernetIsWan() )
				{
					uci.set('network', 'wan', 'ifname', defaultLanIf);
				}
				else if(getSelectedValue("wan_protocol").match(/3g/))
				{
					uci.remove('network', 'wan', 'ifname');
					uci.set('network', 'wan', 'auto', '1');
				}
				else
				{
					uci.set('network', 'wan', 'ifname', defaultWanIf);
				}
			}

			if( document.getElementById("wan_port_to_lan_container").style.display != "none" && getSelectedValue('wan_port_to_lan') == "bridge" )
			{
				uci.set('network', 'lan', 'ifname', defaultLanIf + " " + defaultWanIf);
			}
			else if( singleEthernetIsWan() )
			{
				//just in case wirelessIf does not exist, remove variable first
				uci.remove('network', 'lan', 'ifname');
				uci.set('network', 'lan', 'ifname', wirelessIfs.length > 0 ? wirelessIfs[0] : "");
			}
			else 
			{
				uci.set('network', 'lan', 'ifname', defaultLanIf);
			}

			//define new sections, now that we have cleared old ones
			//cfg2 should be AP if we have an AP section, otherwise cfg2 is whatever single mode we are in
			currentModes= getSelectedValue('wifi_mode');
			var apcfg = '';
			var ap2cfg = '';
			var othercfg = '';

			if(currentModes.match(/ap/))
			{
				
				if(wifiGSelected)
				{
					apcfg = 'ap_g';
					uci.set("wireless", apcfg, "", "wifi-iface");
					uci.set("wireless", apcfg, "device", wifiDevG);
					uci.set('wireless', apcfg, 'mode', 'ap');
					uci.set('wireless', apcfg, 'network', 'lan');
					uci.set('wireless', apcfg, 'disassoc_low_ack', '0');
					preCommands = preCommands + "uci set wireless." + apcfg + "='wifi-iface' \n"
				}
				if(wifiASelected)
				{
					apacfg='ap_a';
					uci.set("wireless", apacfg, "", "wifi-iface");
					uci.set("wireless", apacfg, "device", wifiDevA);
					uci.set('wireless', apacfg, 'mode', 'ap');
					uci.set('wireless', apacfg, 'network', 'lan');
					uci.set('wireless', apacfg, 'disassoc_low_ack', '0');
					preCommands = preCommands + "uci set wireless." + apacfg + "='wifi-iface' \n"
					if(dualBandSelected)
					{
						ap2cfg = apacfg
					}
					else
					{
						apcfg = apacfg
					}
				}
				if(currentModes == "ap+wds") //non-ap sections for ap+wds
				{
					var wdsData = getTableDataArray(document.getElementById('wifi_wds_mac_table_container').firstChild, 1, 0);
					var wdsList = [];
					var wIndex=0;
					for(wIndex=0; wIndex< wdsData.length; wIndex++)
					{
						wdsList.push( wdsData[wIndex][0] );
					}

					if(wirelessDriver == "broadcom")
					{
						//add one section for each bssid
						var encryption = getSelectedValue("wifi_encryption1");
						var key = encryption == "none" ? "" : ( encryption == "wep" ? document.getElementById("wifi_wep1").value : document.getElementById("wifi_pass1").value );
						var ssid = document.getElementById("wifi_ssid1").value;
						var wIndex=0;
						for(wIndex=0; wIndex < wdsList.length; wIndex++)
						{
							var sectionIndex=wIndex + 3;
							var section = "cfg" + sectionIndex;
							uci.set("wireless", section, "", "wifi-iface");
							uci.set("wireless", section, "device", wifiDevG);
							uci.set("wireless", section, "network", "lan");
							uci.set("wireless", section, "mode", "wds");
							uci.set("wireless", section, "ssid", ssid);
							uci.set("wireless", section, "bssid", wdsList[wIndex].toLowerCase());
							uci.set("wireless", section, "encryption", encryption);
							if(encryption != "none") { uci.set("wireless", section, "key", key); }
							preCommands = preCommands + "\nuci set wireless." + section + "=wifi-iface\n";
						}
					}
					else
					{
						var wdsCfgs = [ apcfg ] 
						//TODO: if dual band look at new control and set wdsCfgs from this if both are selected
						
						var wci;
						for(wci=0; wci < wdsCfgs.length ; wci++)
						{
							wcfg = wdsCfgs[wci];
							if(wirelessDriver == "atheros")
							{
								uci.set("wireless", wcfg, "bssid", wdsList.join(" ").toLowerCase());
							}
							uci.set("wireless", wcfg, "wds", "1");
						}
					}
				}
			}
			if( (currentModes.match(/\+/) || ( ! currentModes.match(/ap/)) ) && currentModes != "disabled" && currentModes != "ap+wds")
			{
				var otherCfgDev = wifiDevG
				var wimode = getSelectedValue("wifi_mode")
				var fixedChannels = scannedSsids[0].length > 0 && wimode.match(/sta/);
				if(fixedChannels)
				{
					var ssidIndex = getSelectedValue("wifi_list_ssid2") 
					fixedChannels = ssidIndex != "custom"
					if(fixedChannels)
					{
						otherCfgDev = scannedSsids[4][ parseInt(ssidIndex) ] == "A" ? wifiDevA : wifiDevG;
					}
				}
				if(!fixedChannels)
				{
					if(dualBandSelected)
					{
						otherCfgDev = getSelectedValue('wifi_client_band') == "5" ? wifiDevA : wifiDevG;
					}
					else
					{
						otherCfgDev = wifiASelected ? wifiDevA : wifiDevG
					}
				}
				otherMode=currentModes.replace(/\+?ap\+?/, '');
				othercfg=otherMode + "cfg";
				uci.set("wireless", othercfg, "", "wifi-iface");
				uci.set("wireless", othercfg, "device", otherCfgDev);
				uci.set("wireless", othercfg, "mode", otherMode);
				uci.set("wireless", othercfg, "network", getSelectedValue("wan_protocol").match(/wireless/) ? "wan" : "lan");
				preCommands = preCommands + "uci set wireless." + othercfg + "='wifi-iface' \n"
			}
			preCommands = preCommands + "uci commit \n";

			//set mac filter variables
			macFilterEnabled = getSelectedValue("mac_filter_enabled") == "enabled";
			var macTable = document.getElementById('mac_table_container').firstChild;
			var macList = getTableDataArray(macTable, true, false);
			var policy = getSelectedValue("mac_filter_policy");
			var macListStr = macList.join(" ");
			if(wirelessDriver == "broadcom")
			{
				if( (!macFilterEnabled) || macListStr == '')
				{
					uci.remove("wireless", wifiDevG, policyOption);
					uci.remove("wireless", wifiDevG, "maclist");
				}
				else
				{
					uci.set("wireless", wifiDevG, policyOption, policy);
					uci.set("wireless", wifiDevG, "maclist", macListStr);
				}
			}
			else 
			{
				for(wsecIndex=0; wsecIndex < allWirelessInterfaceSections.length; wsecIndex++)
				{
					var sectionType = uci.get("wireless", allWirelessInterfaceSections [wsecIndex], "");
					if( (!macFilterEnabled) || macListStr == '' || sectionType != "wifi-iface")
					{
						uci.remove("wireless", allWirelessInterfaceSections[wsecIndex], policyOption);
						uci.remove("wireless", allWirelessInterfaceSections[wsecIndex], "maclist");
					}
					else
					{
						uci.set("wireless", allWirelessInterfaceSections[wsecIndex], policyOption, policy);
						uci.set("wireless", allWirelessInterfaceSections[wsecIndex], "maclist", macListStr);
					}
				}
			}

			//use altroot?
			var useAltRoot = document.getElementById("lan_dns_altroot").checked;
			uci.remove("dhcp", uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq").shift(), "server");
			if(useAltRoot)
			{
				var dnsmasqSection = uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq").shift();
				uci.createListOption("dhcp", dnsmasqSection, "server", true);
				uci.set("dhcp", dnsmasqSection, "server", getAltServerDefs());
			}

			//force clients to use router DNS?
			var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");
			var forceDNS = document.getElementById("lan_dns_force").checked ? "1" : "";
			uciOriginal.set("firewall", firewallDefaultSections[0], "force_router_dns", forceDNS);
			uci.set("firewall", firewallDefaultSections[0], "force_router_dns", forceDNS);
			var fdCommand = forceDNS == "1" ?  "\nuci set firewall.@defaults[0].force_router_dns=1 \n" : "\nuci del firewall.@defaults[0].force_router_dns \n";
			preCommands = preCommands + fdCommand ;

			//is ping drop from WAN side?
			var pingSection = getPingSection();
			var isPingDrop = document.getElementById("drop_wan_ping").checked ? "DROP" : "ACCEPT";
			uciOriginal.set("firewall", pingSection, "target", isPingDrop);
			uci.set("firewall", pingSection, "target", isPingDrop);

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
			wifiSsidId = wifiGSelected ? "wifi_ssid1" : "wifi_ssid1a";
			inputIds = ['wan_protocol', 'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', ppoeReconnectIds, 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_mac', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', wifiSsidId, 'wifi_hidden', 'wifi_isolate', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1' , 'wifi_server1', 'wifi_port1', 'wifi_pass2', 'wifi_wep2', 'wan_3g_device', 'wan_3g_user', 'wan_3g_pass', 'wan_3g_apn', 'wan_3g_pincode', 'wan_3g_service', 'wan_3g_isp'];

			options = ['proto', 'username', 'password', 'demand', 'keepalive', 'ipaddr', 'netmask', 'gateway', 'macaddr', 'mtu', 'ipaddr', 'netmask', 'gateway', 'ssid', 'hidden', 'isolate', 'encryption', 'key', 'key', 'server', 'port', 'key', 'key', 'device', 'username', 'password', 'apn', 'pincode', 'service', 'mobile_isp'];

			var sv=  setVariableFromValue;
			var svm= setVariableFromModifiedValue;
			var svcat= setVariableFromConcatenation;
			var svcond= setVariableConditionally;
			setFunctions = [sv,sv,sv,svm,svcat,sv,sv,sv,svcond,svcond,sv,sv,sv,sv,svcond,svcond,sv,sv,sv,sv,sv,sv,sv,sv,sv,sv,sv,sv,sv,sv];

			var f=false;
			var t=true;
			var minutesToSeconds = function(value){return value*60;};
			var lowerCase = function(value) { return value.toLowerCase(); }
			var ifCustomMac = function(value){ return (document.getElementById('wan_use_mac').checked == true); };
			var ifCustomMtu = function(value){ return (document.getElementById('wan_use_mtu').checked == true &&  document.getElementById('wan_mtu').value != 1500);};
			var ifHiddenChecked =  function(value) { return getSelectedValue('wifi_hidden') == "disabled" ? 1 : 0;}; //the label is for "broadcast", so disabled means it is hidden
			var ifIsolateChecked = function(value) { return getSelectedValue('wifi_isolate') == "enabled" ? 1 : 0;};
			var demandParams = [f,minutesToSeconds];
			var macParams = [ifCustomMac,f,  document.getElementById('wan_mac').value.toLowerCase()];
			var mtuParams = [ifCustomMtu,t,''];
			var hiddenParams = [ifHiddenChecked,f,'1'];
			var isolateParams = [ifIsolateChecked,f,'1'];

			additionalParams = [f,f,f, demandParams,f,f,f,f,macParams,mtuParams,f,f,f,f,hiddenParams,isolateParams,f,f,f,f,f,f,f,f,f,f,f,f,f,f];

			pppoeReconnectVisibilityIds = ['wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container'];
			multipleVisibilityIds= [pppoeReconnectVisibilityIds];
			wirelessSections=[apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, othercfg, othercfg];
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

				if(idIndex < 10 || idIndex > 22)
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

			//set wifi channel(s), othercfg ssid, othercfg encryption
			//this is a bit complex (and we have to do it here) because of options introduced by wireless scanning
			if(getSelectedValue("wifi_mode") != 'disabled')
			{
				var ssid2 ="";
				var enc2 = "";
				if(document.getElementById("wifi_ssid2_container").style.display != "none")
				{
					ssid2 = document.getElementById("wifi_ssid2").value;
					enc2 = getSelectedValue("wifi_encryption2");
				}
				else if(document.getElementById("wifi_custom_ssid2_container").style.display != "none")
				{
					ssid2 = document.getElementById("wifi_custom_ssid2").value;
					enc2 = getSelectedValue("wifi_encryption2");
				}
				else if(document.getElementById("wifi_list_ssid2_container").style.display != "none")
				{
					ssid2 = scannedSsids[0][ parseInt(getSelectedValue("wifi_list_ssid2")) ];
					enc2  = scannedSsids[1][ parseInt(getSelectedValue("wifi_list_ssid2")) ];
				}
				if(ssid2 != "")
				{
					uci.set("wireless", othercfg, "ssid", ssid2);
					uci.set("wireless", othercfg, "encryption", enc2);
				}

				//handle dual band configuration
				if(ap2cfg != "")
				{
					var dup_sec_options = function(pkg, fromcfg, tocfg, optlist)
					{
						var opti;
						for(opti=0; opti < optlist.length; opti++)
						{
							uci.set(pkg, tocfg, optlist[opti], uci.get("wireless", fromcfg, optlist[opti]));
						}
					}
					uci.set("wireless", ap2cfg, "ssid", document.getElementById("wifi_ssid1a").value );
					dup_sec_options("wireless", apcfg, ap2cfg, ['hidden', 'isolate', 'encryption', 'key', 'server', 'port'])
				}
			}

			//if wan protocol is 'none' do not set it
			if(getSelectedValue('wan_protocol') == 'none')
			{
				uci.removeSection("network", "wan");
			}
			else
			{
				uci.set("network", "wan", "proto", getSelectedValue('wan_protocol').replace(/_.*$/g, ""));
			}
			if(uci.get('network', 'lan', 'proto') === '')
			{
				uci.set('network', 'lan', 'proto', 'static');
			}

			//preserve wan mac definition even if wan is disabled if this is a bcm94704
			if(isBcm94704 && (uci.get("network", "wan", "type") != "bridge"))
			{
				if(uci.get("network", "wan", "macaddr") == "")
				{
					uci.set("network", "wan", "macaddr", defaultWanMac);
				}
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

			var bridgeCommandList = [];
			bridgeCommandList.push("/etc/init.d/dnsmasq enable");
			bridgeCommandList.push("uci set gargoyle.connection.dhcp=200");
			bridgeCommandList.push("uci set gargoyle.firewall.portforwarding=100");
			bridgeCommandList.push("uci set gargoyle.firewall.restriction=125");
			bridgeCommandList.push("uci set gargoyle.firewall.quotas=175");
			bridgeCommandList.push("uci set qos_gargoyle.global.network=wan");
			bridgeCommandList.push("uci commit");
			bridgeEnabledCommands = "\n" + bridgeCommandList.join("\n") + "\n";
		}
		else
		{
			var bridgeDev = wifiDevG;
			if(document.getElementById("bridge_hwmode_container").style.display != "none")
			{
				var hwMode = getSelectedValue("bridge_hwmode");
				bridgeDev = hwMode == "11na" ? wifiDevA : wifiDevG
				if(wifiDevG == bridgeDev)
				{
					uci.set("wireless",  wifiDevG, "hwmode", hwMode);
				}
			}

			if( document.getElementById("bridge_wan_port_to_lan_container").style.display != "none" && getSelectedValue('bridge_wan_port_to_lan') == "bridge" )
			{
				uci.set('network', 'lan', 'ifname', defaultLanIf + " " + defaultWanIf);
			}
			else
			{
				uci.set('network', 'lan', 'ifname', defaultLanIf);
			}

			currentLanIp = document.getElementById("bridge_ip").value;
			//compute configuration  for bridge
			preCommands = preCommands + "\nuci del network.wan\nuci commit\n";
			uci.removeSection("network", "wan");
			uciCompare.removeSection("network", "wan");

			uci.set("network", "lan", "ipaddr",  document.getElementById("bridge_ip").value);
			uci.set("network", "lan", "netmask", document.getElementById("bridge_mask").value);
			uci.set("network", "lan", "gateway", document.getElementById("bridge_gateway").value);
			uci.set("network", "lan", "dns",     document.getElementById("bridge_gateway").value);

			var ssid ="";
			var encryption = "";
			if(document.getElementById("bridge_ssid_container").style.display != "none")
			{
				ssid = document.getElementById("bridge_ssid").value;
				encryption = getSelectedValue("bridge_encryption");
			}
			else if(document.getElementById("bridge_custom_ssid_container").style.display != "none")
			{
				ssid = document.getElementById("bridge_custom_ssid").value;
				encryption = getSelectedValue("bridge_encryption");
			}
			else if(document.getElementById("bridge_list_ssid_container").style.display != "none")
			{
				ssid = scannedSsids[0][ parseInt(getSelectedValue("bridge_list_ssid")) ];
				encryption  = scannedSsids[1][ parseInt(getSelectedValue("bridge_list_ssid")) ];
			}
			var key = encryption == "none" ? "" : ( encryption == "wep" ? document.getElementById("bridge_wep").value : document.getElementById("bridge_pass").value );

			if( getSelectedValue("bridge_mode") == "client_bridge")
			{
				//client bridge
				uci.set("wireless", "cfg2", "", "wifi-iface");
				uci.set("wireless", "cfg2", "device", bridgeDev);
				uci.set("wireless", "cfg2", "network", "lan");
				uci.set("wireless", "cfg2", "mode", "sta");
				uci.set("wireless", "cfg2", "client_bridge", "1");
				uci.set("wireless", "cfg2", "ssid", ssid);
				uci.set("wireless", "cfg2", "encryption", encryption);
				if(encryption != "none") { uci.set("wireless", "cfg2", "key", key); }
				preCommands = preCommands + "\nuci set wireless.cfg2=wifi-iface\n";

				if(getSelectedValue("bridge_repeater") == "enabled" && document.getElementById("bridge_repeater_container").style.display != "none")
				{
					var apSsid = document.getElementById("bridge_broadcast_ssid").value;
					uci.set("wireless", "cfg3", "", "wifi-iface");
					uci.set("wireless", "cfg3", "device", bridgeDev);
					uci.set("wireless", "cfg3", "network", "lan");
					uci.set("wireless", "cfg3", "mode", "ap");
					uci.set("wireless", "cfg3", "ssid", apSsid);
					uci.set("wireless", "cfg3", "encryption", encryption);
					if(encryption != "none") { uci.set("wireless", "cfg3", "key", key); }
					preCommands = preCommands + "\nuci set wireless.cfg3=wifi-iface\n";
				}
			}
			else
			{
				//wds

				//get bssids
				var wdsData = getTableDataArray(document.getElementById('bridge_wds_mac_table_container').firstChild);
				var wdsList = [];
				var wIndex=0;
				for(wIndex=0; wIndex< wdsData.length; wIndex++)
				{
					wdsList.push( wdsData[wIndex][0].toLowerCase() );
				}

				if(wirelessDriver == "broadcom")
				{
					uci.set("wireless", "cfg2", "", "wifi-iface");
					uci.set("wireless", "cfg2", "device", bridgeDev);
					uci.set("wireless", "cfg2", "network", "lan");
					uci.set("wireless", "cfg2", "mode", "ap");
					uci.set("wireless", "cfg2", "ssid", ssid);
					uci.set("wireless", "cfg2", "encryption", encryption);
					if(encryption != "none") { uci.set("wireless", "cfg2", "key", key); }
					preCommands = preCommands + "\nuci set wireless.cfg2=wifi-iface\n";

					for(wIndex=0; wIndex < wdsList.length; wIndex++)
					{
						var sectionIndex=wIndex + 3;
						var section = "cfg" + sectionIndex;
						uci.set("wireless", section, "", "wifi-iface");
						uci.set("wireless", section, "device", bridgeDev);
						uci.set("wireless", section, "network", "lan");
						uci.set("wireless", section, "mode", "wds");
						uci.set("wireless", section, "ssid", ssid);
						uci.set("wireless", section, "bssid", wdsList[wIndex].toLowerCase());
						uci.set("wireless", section, "encryption", encryption);
						if(encryption != "none") { uci.set("wireless", section, "key", key); }
						preCommands = preCommands + "\nuci set wireless." + section + "=wifi-iface\n";
					}
				}
				else //atheros & mac80211 driver
				{
					var cfg = "cfg2";
					if(wirelessDriver == "atheros" || (wirelessDriver == "mac80211" && getSelectedValue("bridge_repeater") =="enabled"))
					{
						uci.set("wireless", cfg, "", "wifi-iface");
						uci.set("wireless", cfg, "device", bridgeDev);
						uci.set("wireless", cfg, "network", "lan");
						uci.set("wireless", cfg, "mode", "ap");
						uci.set("wireless", cfg, "wds", "1");
						uci.set("wireless", cfg, "encryption", encryption);
						if(encryption != "none") { uci.set("wireless", cfg, "key", key); }

						if(wirelessDriver == "atheros" )
						{
							uci.set("wireless", cfg, "ssid", ssid);
							uci.set("wireless", cfg, "bssid", wdsList.join(" ").toLowerCase() );
						}
						else
						{
							uci.set("wireless", cfg, "ssid", document.getElementById("bridge_broadcast_ssid").value);
						}
						preCommands = preCommands + "\nuci set wireless." + cfg + "=wifi-iface\n";
						cfg = "cfg3";
					}

					uci.set("wireless", cfg, "", "wifi-iface");
					uci.set("wireless", cfg, "device", bridgeDev);
					uci.set("wireless", cfg, "network", "lan");
					uci.set("wireless", cfg, "mode", "sta");
					uci.set("wireless", cfg, "wds", "1");
					uci.set("wireless", cfg, "ssid", ssid);
					uci.set("wireless", cfg, "encryption", encryption);
					if(wirelessDriver == "atheros"){  uci.set("wireless", cfg, "bssid", wdsList.join(" ").toLowerCase() ); }
					if(encryption != "none")       {  uci.set("wireless", cfg, "key", key); }
					preCommands = preCommands + "\nuci set wireless." + cfg + "=wifi-iface\n";
				}

			}
			preCommands = preCommands + "\nuci commit\n";

			var bridgeCommandList = [];
			bridgeCommandList.push("/etc/init.d/dnsmasq disable");
			bridgeCommandList.push("/etc/init.d/miniupnpd disable");
			bridgeCommandList.push("uci del gargoyle.connection.dhcp");
			bridgeCommandList.push("uci del gargoyle.firewall.portforwarding");
			bridgeCommandList.push("uci del gargoyle.firewall.restriction");
			bridgeCommandList.push("uci del gargoyle.firewall.quotas");
			bridgeCommandList.push("uci del gargoyle.firewall.portforwarding");
			bridgeCommandList.push("uci set qos_gargoyle.global.network=lan");

			bridgeCommandList.push("uci commit");
			bridgeEnabledCommands = "\n" + bridgeCommandList.join("\n") + "\n";

		}

		//set lan dns from table
		//this code is the same for both router & bridge
		//we set from lan table, but we keep bridge & lan dns tables synchronized
		//so they should be identical
		uci.remove('network', 'wan', 'dns'); 
		var lanGateway = uci.get("network", "lan", "gateway");
		lanGateway = lanGateway == "" ? uci.get("network", "lan", "ipaddr") : lanGateway;
		var dns = lanGateway;
		var dnsSource = getSelectedValue("lan_dns_source");
		var notBridge = document.getElementById("global_gateway").checked 
		if(dnsSource != "isp")
		{
			var dnsList = [];
			if(dnsSource == "google" && notBridge )
			{
				dnsList = googleDns;
			}
			else if(dnsSource == "opendns" && notBridge )
			{
				dnsList = openDns;
			}
			else //custom
			{
				var dnsData = getTableDataArray(document.getElementById("lan_dns_table_container").firstChild);
				var dnsIndex=0;
				for(dnsIndex=0; dnsIndex < dnsData.length; dnsIndex++) { dnsList.push(dnsData[dnsIndex][0]); }
			}
			dns = dnsList.length > 0 ? dnsList.join(" ") : dns;
			
			//if a wan is active and we have custom DNS settings, propagate to the wan too
			if( notBridge && uci.get("network", "wan", "") != "" && dns != lanGateway)
			{
				uci.set("network", "wan", "dns", dns);
				uci.set("network", "wan", "peerdns", "0")
			}
		}
		else if( notBridge && uci.get("network", "wan", "") != "")
		{
			uci.remove("network", "wan", "peerdns")
		}
		uci.set("network", "lan", "dns", dns)

		var oldLanIp = uciOriginal.get("network", "lan", "ipaddr");
		if(oldLanIp != currentLanIp && oldLanIp != "" && currentLanIp != "")
		{
			adjustIpCommands = "\nsh /usr/lib/gargoyle/update_router_ip.sh " + oldLanIp + "  " + currentLanIp;
		}

		var commands = uci.getScriptCommands(uciCompare);
		var restartNetworkCommand = (wirelessDriver== "broadcom" ? "\niwconfig wl0 txpower 31\n" : "") + "sh /usr/lib/gargoyle/restart_network.sh ;\n" ;
		if(doReboot)
		{
			restartNetworkCommand = "\nsh /usr/lib/gargoyle/reboot.sh ;\n";
		}
		var regenerateCacheCommand = "\nrm -rf /tmp/cached_basic_vars ;\n/usr/lib/gargoyle/cache_basic_vars.sh >/dev/null 2>/dev/null\n";

		commands = preCommands + commands + adjustIpCommands + bridgeEnabledCommands + restartNetworkCommand + regenerateCacheCommand;

		//document.getElementById("output").value = commands;
		var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				if(oldLanIp == currentLanIp && (!doReboot))
				{
					uciOriginal = uci.clone();
					resetData();
					setControlsEnabled(true);
				}
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

		//if we're rebooting, this tests whether reboot is done, otherwise
		//it tests if router is up at new ip
		if(oldLanIp != currentLanIp || doReboot)
		{
			currentProtocol = location.href.match(/^https:/) ? "https" : "http";
			testLocation = currentProtocol + "://" + currentLanIp + ":" + window.location.port + "/utility/reboot_test.sh";
			testReboot = function()
			{
				toggleReload = true;
				setTimeout( "testReboot()", 5*1000);  //try again after 5 seconds
				document.getElementById("reboot_test").src = testLocation;
			}
			setTimeout( "testReboot()", 25*1000);  //start testing after 25 seconds
			setTimeout( "reloadPage()", 240*1000); //after 4 minutes, try to reload anyway
		}
	}
}

function reloadPage()
{
	if(toggleReload)
	{
		//IE calls onload even when page isn't loaded -- it just times out and calls it anyway
		//We can test if it's loaded for real by looking at the (IE only) readyState property
		//For Browsers NOT designed by dysfunctional cretins whose mothers were a pack of sewer-dwelling, shit-eating rodents,
		//well, for THOSE browsers, readyState (and therefore reloadState) should be null 
		var reloadState = document.getElementById("reboot_test").readyState;
		if( typeof(reloadState) == "undefined" || reloadState == null || reloadState == "complete")
		{
			toggleReload=false;
			document.getElementById("reboot_test").src = "";
			currentProtocol = location.href.match(/^https:/) ? "https" : "http";
			window.location = currentProtocol + "://" + currentLanIp + ":" + window.location.port + window.location.pathname;
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
	var vlr1 = function(text){return validateLengthRange(text,1,999);};
	var vlr8 = function(text){return validateLengthRange(text,8,999);};
	var vip = validateIP;
	var vnm = validateNetMask;
	var vm = validateMac;
	var vn = validateNumeric;
	var vw = validateWep;
	var vtp = function(text){ return validateNumericRange(text, 0, getMaxTxPower("G")); };
	var vtpa = function(text){ return validateNumericRange(text, 0, getMaxTxPower("A")); };

	var testWds = function(tableContainerId, selectId, wdsValue)
	{
		var error = null;
		if( getSelectedValue(selectId) == wdsValue && wirelessDriver != "mac80211" )
		{
			var wdsData = getTableDataArray(document.getElementById(tableContainerId).firstChild);
			error = wdsData.length > 0 ? null : basicS.WDSMAC;
		}
		return error;
	}

	var errors = [];
	if(document.getElementById("global_gateway").checked)
	{
		var inputIds = ['wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', 'wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_mac', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', 'wifi_txpower', 'wifi_txpower_5ghz', 'wifi_ssid1', 'wifi_pass1', 'wifi_wep1', 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_pass2', 'wifi_wep2', 'wan_3g_device', 'wan_3g_apn'];

		var functions= [vlr1, vlr1, vn, vn, vn, vip, vnm, vip, vm, vn, vip, vnm, vip, vtp, vtpa, vlr1, vlr8, vw, vip, vn, vlr1, vlr8, vw, vlr1, vlr1];

		var returnCodes= new Array();
		var visibilityIds = new Array();
		var idIndex;
		for (idIndex= 0; idIndex < inputIds.length; idIndex++)
		{
			returnCodes.push(0);
			var id = inputIds[idIndex];
			var container = id + "_container";
			visibilityIds.push( container );
		}

		var labelIds = new Array();
		for (idIndex= 0; idIndex < inputIds.length; idIndex++)
		{
			labelIds.push( inputIds[idIndex] + "_label");
		}
		errors= proofreadFields(inputIds, labelIds, functions, returnCodes, visibilityIds);

		var wdsError = testWds("wifi_wds_mac_table_container", "wifi_mode", "ap+wds");
		if(wdsError != null){ errors.push(wdsError); }
	}
	else
	{
		var inputIds = ['bridge_ip', 'bridge_mask', 'bridge_gateway', 'bridge_txpower', 'bridge_ssid', 'bridge_pass', 'bridge_wep', 'bridge_broadcast_ssid'];
		var functions = [vip, vnm, vip, vtp, vlr1,vlr8,vw,vlr1];
		var returnCodes=[];
		var visibilityIds=[];
		var labelIds=[];

		var idIndex=0;
		for(idIndex=0; idIndex < inputIds.length; idIndex++)
		{
			returnCodes.push(0);
			visibilityIds.push( inputIds[idIndex] + "_container" );
			labelIds.push( inputIds[idIndex] + "_label" );
		}
		errors= proofreadFields(inputIds, labelIds, functions, returnCodes, visibilityIds);

		var wdsError = testWds("bridge_wds_mac_table_container", "bridge_mode", "wds");
		if(wdsError != null){ errors.push(wdsError); }
	}
	return errors;
}

function setGlobalVisibility()
{
	if( getSelectedValue("wan_protocol").match(/wireless/) )
	{
		currentMode=getSelectedValue('wifi_mode');
		if(!isb43)
		{
			setAllowableSelections('wifi_mode', ['sta', 'ap+sta'], [basicS.Clnt, basicS.Clnt+'+AP']);
			setSelectedValue("wifi_mode", currentMode.match(/ap/) ? "ap+sta" : "sta");
		}
		else
		{
			setAllowableSelections('wifi_mode', ['sta'], [basicS.Clnt]);
			setSelectedValue("wifi_mode", 'sta');
		}
	}
	else
	{
		currentMode=getSelectedValue('wifi_mode');
		setAllowableSelections('wifi_mode', ['ap', 'ap+wds', 'adhoc', 'disabled'], [basicS.AcPt+' (AP)', 'AP+WDS', 'Ad Hoc', UI.Disabled]);
		if(currentMode == 'ap+sta' || currentMode == 'sta')
		{
			setSelectedValue('wifi_mode', 'ap');
		}
		else
		{
			setSelectedValue('wifi_mode', currentMode);
		}
	}

	if (hasUSB == false) {
		setAllowableSelections('wan_protocol', ['dhcp_wired', 'pppoe_wired', 'static_wired', 'dhcp_wireless', 'static_wireless', 'none'], ['DHCP ('+basicS.Wird+')', 'PPPoE ('+basicS.Wird+')', basicS.StIP+' ('+basicS.Wird+')', 'DHCP ('+basicS.Wrlss+')', basicS.StIP+' ('+basicS.Wrlss+')',UI.Disabled]);
	}
	else
	{
		setAllowableSelections('wan_protocol', ['dhcp_wired', 'pppoe_wired', 'static_wired', 'dhcp_wireless', 'static_wireless', '3g', 'none'], ['DHCP ('+basicS.Wird+')', 'PPPoE ('+basicS.Wird+')', basicS.StIP+' ('+basicS.Wird+')', 'DHCP ('+basicS.Wrlss+')', basicS.StIP+' ('+basicS.Wrlss+')', basicS.Mo3g, UI.Disabled]);
	}
	

	setVisibility( [ 'wan_port_to_lan_container' ], ((getSelectedValue("wan_protocol").match(/wireless/) || getSelectedValue("wan_protocol").match(/3g/))  && (!singleEthernetPort())) ? [1] : [0] )

	setWanVisibility();
	setWifiVisibility();
}

function setWanVisibility()
{
	var wanIds=['wan_dhcp_ip_container', 'wan_dhcp_expires_container', 'wan_pppoe_user_container', 'wan_pppoe_pass_container', 'wan_pppoe_reconnect_mode_container', 'wan_pppoe_max_idle_container', 'wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container', 'wan_static_ip_container', 'wan_static_mask_container', 'wan_static_gateway_container', 'wan_mac_container', 'wan_mtu_container', 'wan_ping_container', 'lan_gateway_container', 'wan_3g_device_container', 'wan_3g_user_container', 'wan_3g_pass_container', 'wan_3g_apn_container', 'wan_3g_pincode_container', 'wan_3g_service_container', 'wan_3g_isp_container'];

	var maxIdleIndex = 5;
	var notWifi= getSelectedValue('wan_protocol').match(/wireless/) ? 0 : 1;

	var dhcpVisability     = [1,1,  0,0,0,0,0,0,  0,0,0,  notWifi,notWifi,1, 0, 0,0,0,0,0,0,0];
	var pppoeVisability    = [0,0,  1,1,1,1,1,1,  0,0,0,  notWifi,notWifi,1, 0, 0,0,0,0,0,0,0];
	var staticVisability   = [0,0,  0,0,0,0,0,0,  1,1,1,  notWifi,notWifi,1, 0, 0,0,0,0,0,0,0];
	var disabledVisability = [0,0,  0,0,0,0,0,0,  0,0,0,  0,0,0,             1, 0,0,0,0,0,0,0];
	var tgVisability       = [0,0,  0,0,0,0,0,0,  0,0,0,  0,0,1,             0, 1,1,1,1,1,1,1];

	var wanVisibilities= new Array();
	wanVisibilities['dhcp'] = dhcpVisability;
	wanVisibilities['pppoe'] = pppoeVisability;
	wanVisibilities['static'] = staticVisability;
	wanVisibilities['none'] = disabledVisability;
	wanVisibilities['3g'] = tgVisability;

	var selectedVisibility= wanVisibilities[ getSelectedValue("wan_protocol").replace(/_.*$/g, "") ];

	selectedVisibility[maxIdleIndex] = selectedVisibility[maxIdleIndex] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'demand' ? 1 : 0;
	selectedVisibility[maxIdleIndex+1] = selectedVisibility[maxIdleIndex+1] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;
	selectedVisibility[maxIdleIndex+2] = selectedVisibility[maxIdleIndex+2] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;

	setVisibility(wanIds, selectedVisibility);
}

function setWifiVisibility()
{
	var wifiMode=getSelectedValue("wifi_mode");
	if(wifiMode == "ap+wds")
	{
		setAllowableSelections('wifi_encryption1', ['none', 'psk2', 'psk', 'wep'], [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP']);
	}
	else
	{
		setAllowableSelections('wifi_encryption1', ['none', 'psk2', 'psk', 'wep', 'wpa', 'wpa2'], [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP', 'WPA RADIUS', 'WPA2 RADIUS']);
	}

	if(wifiMode == 'adhoc')
	{
		setAllowableSelections('wifi_encryption2', ['none', 'wep'], [basicS.None, 'WEP']);
		document.getElementById("wifi_ssid2_label").firstChild.data = "SSID:";
	}
	else
	{
		document.getElementById("wifi_ssid2_label").firstChild.data = basicS.Join+":";
		setAllowableSelections('wifi_encryption2', ['none', 'psk2', 'psk', 'wep'], [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP']);
	}

	var modes  = ['11ng',  '11g', '11b']
	var mnames = ['N+G+B', 'G+B', 'B']
	if(wifiN && dualBandWireless )
	{
		modes.splice( 1,0,"11na")
		mnames.splice(1,0,"N+A")
		if(wifiMode.match(/ap/) || wifiMode.match(/disabled/))
		{
			modes.unshift("dual")
			mnames.unshift(basicS.DBand)
		}
	}
	setAllowableSelections( "wifi_hwmode", modes, mnames );

	var wifiIds=[	'internal_divider1',
			'wifi_hwmode_container',
			'wifi_channel_width_container',
			'wifi_txpower_container',
			'wifi_channel_width_5ghz_container',
			'wifi_txpower_5ghz_container',
			'mac_enabled_container',
			'mac_filter_container',


			'wifi_ssid1_container',
			'wifi_ssid1a_container',
			'wifi_channel1_container',
			'wifi_fixed_channel1_container',
			'wifi_channel1_5ghz_container',
			'wifi_hidden_container',
			'wifi_isolate_container',
			'wifi_encryption1_container',
			'wifi_pass1_container',
			'wifi_wep1_container',
			'wifi_server1_container',
			'wifi_port1_container',


			'wifi_mac_container',
			'wifi_wds_container',


			'internal_divider2',
			'wifi_list_ssid2_container',
			'wifi_custom_ssid2_container',
			'wifi_ssid2_container',
			'wifi_scan_button',
			'wifi_channel2_container',
			'wifi_fixed_channel2_container',
			'wifi_channel2_5ghz_container',
			'wifi_client_band_container',
			'wifi_encryption2_container',
			'wifi_fixed_encryption2_container',
			'wifi_pass2_container',
			'wifi_wep2_container'
			];

	var wn = wifiN ? 1 : 0; //N active
	var da = wn && (getSelectedValue("wifi_hwmode") == "11na" || getSelectedValue("wifi_hwmode") == "dual") ? 1 : 0; //A active
	var sa = wn && getSelectedValue("wifi_hwmode")  == "11na" ? 1 : 0; //A only
	var  g = sa == 1 ? 0 : 1; //any G active
	var ng = wn && sa==0  //N+G active

	var mf = getSelectedValue("mac_filter_enabled") == "enabled" ? 1 : 0;
	var e1 = document.getElementById('wifi_encryption1').value;
	var p1 = (e1 != 'none' && e1 != 'wep') ? 1 : 0;
	var w1 = (e1 == 'wep') ? 1 : 0;
	var r1 = (e1 == 'wpa' || e1 == 'wpa2') ? 1 : 0;
	var e2 = document.getElementById('wifi_fixed_encryption2').style.display != 'none' ? document.getElementById('wifi_fixed_encryption2').firstChild.data : getSelectedValue('wifi_encryption2');
	var b = wifiN ? 0 : 1;

	var p2 = e2.match(/psk/) || e2.match(/WPA/) ? 1 : 0;
	var w2 = e2.match(/wep/) || e2.match(/WEP/) ? 1 : 0;

	var wifiVisibilities = new Array();
	wifiVisibilities['ap']       = [1,wn,ng,g,da,da,1,mf,   1,da,1,0,da,1,1,1,p1,w1,r1,r1, 0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0 ];
	wifiVisibilities['ap+wds']   = [1,wn,ng,g,da,da,1,mf,   1,0,1,0,0,1,1,1,p1,w1,r1,r1,   b,b,  0,0,0,0,0,0,0,0,0,0,0,0,0 ];
	wifiVisibilities['sta']      = [1,wn,ng,g,da,da,1,mf,   0,0,0,0,0,0,0,0,0,0,0,0,       0,0,  0,0,0,1,1,g,0,sa,0,1,0,p2,w2];
	wifiVisibilities['ap+sta']   = [1,wn,ng,g,da,da,1,mf,   1,da,1,0,da,1,1,1,p1,w1,r1,r1, 0,0,  1,0,0,1,1,g,0,sa,da,1,0,p2,w2];
	wifiVisibilities['adhoc']    = [1,wn,ng,g,da,da,1,mf,   0,0,0,0,0,0,0,0,0,0,0,0,       0,0,  0,0,0,1,0,g,0,sa,0,1,0,p2,w2];
	wifiVisibilities['disabled'] = [0,0,0,0,0,0,0,0,        0,0,0,0,0,0,0,0,0,0,0,0,       0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0 ];

	var wifiVisibility = wifiVisibilities[ wifiMode ];
	setVisibility(wifiIds, wifiVisibility);

	if(wifiMode.match(/sta/))
	{
		setSsidVisibility("wifi_list_ssid2");
	}
	if(wifiMode != "disabled")
	{
		setHwMode(document.getElementById("wifi_hwmode"));
	}
}

function setBridgeVisibility()
{
	showIds = document.getElementById("global_gateway").checked ? ["wan_fieldset", "lan_fieldset", "wifi_fieldset"] : ["bridge_fieldset"];
	hideIds = document.getElementById("global_gateway").checked ? ["bridge_fieldset"] : ["wan_fieldset", "lan_fieldset", "wifi_fieldset"];
	var allIds = [hideIds, showIds];
	var statIndex;
	for(statIndex=0; statIndex < 2; statIndex++)
	{
		var ids =allIds[statIndex];
		var idIndex;
		for(idIndex=0; idIndex < ids.length; idIndex++)
		{
			document.getElementById(ids[idIndex]).style.display = statIndex==0 ? "none" : "block";
		}
	}

	if(singleEthernetPort())
	{
		document.getElementById("bridge_wan_port_to_lan_container").style.display = "none";
	}
	else
	{
		document.getElementById("bridge_wan_port_to_lan_container").style.display = "block";
	}

	if(document.getElementById("global_bridge").checked)
	{
		var brenc = document.getElementById("bridge_fixed_encryption_container").style.display == "none" ? getSelectedValue("bridge_encryption") : document.getElementById("bridge_fixed_encryption").firstChild.data;
		document.getElementById("bridge_pass_container").style.display = brenc.match(/psk/) || brenc.match(/WPA/) ? "block" : "none";
		document.getElementById("bridge_wep_container").style.display  = brenc.match(/wep/) || brenc.match(/WEP/) ? "block" : "none";

		var bridgeMode = getSelectedValue("bridge_mode") 
		var repeaterPossible = (bridgeMode == "client_bridge" && (!isb43)) || (bridgeMode=="wds" && wirelessDriver == "mac80211") ;
		document.getElementById("bridge_repeater_container").style.display = repeaterPossible  ? "block" : "none";
		document.getElementById("bridge_wifi_mac_container").style.display = bridgeMode == "wds" && wirelessDriver != "mac80211" ? "block" : "none";
		document.getElementById("bridge_wds_container").style.display = bridgeMode == "wds" && wirelessDriver != "mac80211" ? "block" : "none";
		document.getElementById("bridge_fixed_encryption_container").style.display="none";
		document.getElementById("bridge_fixed_channel_container").style.display="none";

		document.getElementById("bridge_broadcast_ssid_container").style.display = repeaterPossible && getSelectedValue("bridge_repeater") =="enabled" ? "block" : "none";
		if(document.getElementById("bridge_broadcast_ssid").value == "")
		{
			var ssid ="";
			if(document.getElementById("bridge_ssid_container").style.display != "none")
			{
				ssid = document.getElementById("bridge_ssid").value;
			}
			else if(document.getElementById("bridge_custom_ssid_container").style.display != "none")
			{
				ssid = document.getElementById("bridge_custom_ssid").value;
			}
			else if(document.getElementById("bridge_list_ssid_container").style.display != "none")
			{
				ssid = scannedSsids[0][ parseInt(getSelectedValue("bridge_list_ssid")) ];
			}
			document.getElementById("bridge_broadcast_ssid").value = ssid;
		}

		setSsidVisibility("bridge_list_ssid");
	}
	setHwMode(document.getElementById("bridge_hwmode"))
	setGlobalVisibility();
}

function localdate(ldate)
{
	var twod = function(num) { var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; return nstr; }
	var ldateStr = "";
	var systemDateFormat = uciOriginal.get("gargoyle",  "global", "dateformat");
	var y2 = twod(ldate.getUTCFullYear()%100)
	var y4 = ldate.getUTCFullYear();
	var m = twod(ldate.getUTCMonth()+1);
	var d = twod(ldate.getUTCDate());
	var h = " " + twod(ldate.getUTCHours()) + ":" + twod(ldate.getUTCMinutes()) + " " + timezoneName;
	if(systemDateFormat == "iso")
	{
		ldateStr = y4 + "/" + m + "/" + d + h;
	}
	else if(systemDateFormat == "iso8601")
	{
		ldateStr = y4 + "-" + m + "-" + d + h;
	}
	else if(systemDateFormat == "australia")
	{
		ldateStr = d + "/" + m + "/" + y2 + h;
	}
	else if(systemDateFormat == "russia")
	{
		ldateStr = d + "." + m + "." + y4 + h;
	}
	else if(systemDateFormat == "argentina")
	{
		ldateStr = d + "/" + m + "/" + y4 + h;
	}
	else
	{
		ldateStr = m + "/" + d + "/" + y2 + h;
	}

	return ldateStr;
}

function resetData()
{
	var removeChannels = [];
	if(wirelessDriver == "broadcom")
	{
		//no auto for brcm
		removeChannels.push("auto");
	}
	else if(wirelessDriver == "atheros")
	{
		//atheros can't handle channels 12-14
		removeChannels.push("12");
		removeChannels.push("13");
		removeChannels.push("14");
	}
	else if(wirelessDriver == "mac80211")
	{
		setAllowableSelections("bridge_channel", mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel1",  mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel2",  mac80211Channels["G"], mac80211Channels["G"], document);
		if(mac80211Channels["A"] != null)
		{
			setAllowableSelections("wifi_channel1_5ghz",   mac80211Channels["A"], mac80211Channels["A"], document);
			setAllowableSelections("bridge_channel_5ghz",  mac80211Channels["A"], mac80211Channels["A"], document);
		}
	}
	while(removeChannels.length > 0)
	{
		var rc = removeChannels.shift();
		removeOptionFromSelectElement("bridge_channel", rc, document);
		removeOptionFromSelectElement("wifi_channel1",  rc, document);
		removeOptionFromSelectElement("wifi_channel2",  rc, document);
	}

	if(leaseStart != "")
	{
		setElementEnabled(document.getElementById("dhcp_renew_button"), true);
		setElementEnabled(document.getElementById("dhcp_release_button"), true);
		var releaseDate = new Date();
		var leaseStartSeconds = (parseInt(currentDateSeconds) - parseInt(uptime)) + parseInt(leaseStart);
		releaseDate.setTime( (leaseStartSeconds*1000) + (parseInt(leaseLifetime)*1000) + (timezoneOffset*1000) );
		
		setChildText("dhcp_expires", localdate(releaseDate));
		setChildText("dhcp_ip", currentWanIp);
	}
	else
	{
		setElementEnabled(document.getElementById("dhcp_renew_button"), true);
		setElementEnabled(document.getElementById("dhcp_release_button"), false);
	}

	setChildText("bridge_wifi_mac",  currentWirelessMacs[0], null, null, null);
	setChildText("wifi_mac", currentWirelessMacs[0], null, null, null);

	var confIsBridge = isBridge(uciOriginal);
	var confIsGateway = !confIsBridge;
	document.getElementById("global_gateway").checked = confIsGateway;
	document.getElementById("global_bridge").checked = confIsBridge;

	//set bridge variables
	document.getElementById("bridge_ip").value      = uciOriginal.get("network", "lan", "ipaddr");
	document.getElementById("bridge_mask").value    = uciOriginal.get("network", "lan", "netmask");
	document.getElementById("bridge_gateway").value = uciOriginal.get("network", "lan", "gateway");
	var bridgeWdsTableData = [];
	if(confIsBridge)
	{
		var bridgeSection = getBridgeSection(uciOriginal);
		var mode = uciOriginal.get("wireless", bridgeSection, "client_bridge") == "1" ? "client_bridge" : "wds";
		setSelectedValue("bridge_mode", mode);
		document.getElementById("bridge_ssid").value = uciOriginal.get("wireless", bridgeSection, "ssid");
	
		var repeaterSection = "";
		var testSections = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");
		var testIndex=0;
		for(testIndex=0; testIndex < testSections.length && ( mode != "wds" || wirelessDriver == "mac80211") && repeaterSection == ""; testIndex++)
		{
			var s = testSections[testIndex];
			repeaterSection = uciOriginal.get("wireless", s, "mode") == "ap" ? s : "";
		}
		setSelectedValue("bridge_repeater", (repeaterSection == "" ? "disabled" : "enabled") );

		var encryption = uciOriginal.get("wireless", bridgeSection, "encryption");
		encryption = encryption == "" ? "none" : encryption;
		setSelectedValue("bridge_encryption", encryption);
		document.getElementById("bridge_wep").value = encryption == "wep" ? uciOriginal.get("wireless", bridgeSection, "key") : "";
		document.getElementById("bridge_pass").value = encryption != "wep" && encryption != "none" ? uciOriginal.get("wireless", bridgeSection, "key") : "";
		document.getElementById("bridge_broadcast_ssid").value = repeaterSection == "" ? uciOriginal.get("wireless", bridgeSection, "ssid") : uciOriginal.get("wireless", repeaterSection, "ssid"); 

		if(mode == "wds")
		{
			if(wirelessDriver == "broadcom")
			{
				var ifaceSections = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");
				var ifIndex;
				for(ifIndex=0; ifIndex< ifaceSections.length; ifIndex++)
				{
					if( uciOriginal.get("wireless", ifaceSections[ifIndex], "mode") == "wds")
					{
						var bssid = uciOriginal.get("wireless", ifaceSections[ifIndex], "bssid");
						bridgeWdsTableData.push( [bssid.toUpperCase()] );
					}
				}
			}
			else if(wirelessDriver == "atheros")
			{
				var bssids = uciOriginal.get("wireless", bridgeSection, "bssid").split(/[\t ]+/);
				var bIndex;
				for(bIndex = 0; bIndex < bssids.length; bIndex++)
				{
					bridgeWdsTableData.push([ bssids[bIndex].toUpperCase() ]);
				}
			}
		}
	}
	else
	{
		setSelectedValue("bridge_mode", "client_bridge");
		setSelectedValue("bridge_repeater", "enabled");
		document.getElementById("bridge_ssid").value = "Gargoyle";
		setSelectedValue("bridge_channel", wirelessDriver=="atheros"  ? "auto" : "5");
		setSelectedValue("bridge_encryption", "none");
	}
	var bridgeWdsMacTable=createTable([""], bridgeWdsTableData, "bridge_wds_mac_table", true, false);
	var bridgeWdsTableContainer = document.getElementById('bridge_wds_mac_table_container');
	if(bridgeWdsTableContainer.firstChild != null)
	{
		bridgeWdsTableContainer.removeChild(bridgeWdsTableContainer.firstChild);
	}
	bridgeWdsTableContainer.appendChild(bridgeWdsMacTable);

	//reset default wan mac if isBcm94704 is true
	var wanIsWireless = false;
	var wifIndex=0;
	for(wifIndex=0; wifIndex < wirelessIfs.length; wifIndex++)
	{
		wanIsWireless = uciOriginal.get("network", "wan", "ifname") == wirelessIfs[wifIndex];
	}

	if(isBcm94704 && (!wanIsWireless) && uciOriginal.get("network", "wan", "macaddr") != "")
	{
		var currentMac = uciOriginal.get("network", "wan", "macaddr").toUpperCase();
		var currentStart = currentMac.substr(0, 15);
		var currentEnd = currentMac.substr(15, 2);
		var lanMacIndex=0;
		for(lanMacIndex=0; lanMacIndex < allLanMacs.length; lanMacIndex++)
		{
			var nextMac = allLanMacs[lanMacIndex].toUpperCase();
			var nextStart = nextMac.substr(0,15);
			var nextEnd = nextMac.substr(15,2);
			if(nextStart == currentStart && Math.abs(parseInt(nextEnd,16) - parseInt(currentEnd,16)) == 1)
			{
				defaultWanMac = currentMac;
			}
		}
	}

	//set wan proto && wan/wifi/bridge variables
	var wp = uciOriginal.get("network", "wan", "proto");
	var wanUciIf= uciOriginal.get('network', 'wan', 'ifname');
	var wanType = uciOriginal.get('network', 'wan', 'type');
	var lanUciIf= uciOriginal.get('network', 'lan', 'ifname');
	var wanIsWifi = (wanUciIf == '' && wanType == 'bridge' ) && ( getWirelessMode(uciOriginal) == "sta" || getWirelessMode(uciOriginal) == "ap+sta");
	wp = wp == "" ? "none" : wp;
	if(wp != "none" && wp != "3g") { wp = wanIsWifi ? wp + "_wireless" : wp + "_wired"; }
	setSelectedValue("wan_protocol", wp);

	var wanToLanStatus = lanUciIf.indexOf(defaultWanIf) < 0 ? 'disable' : 'bridge' ;
	setSelectedValue('bridge_wan_port_to_lan', wanToLanStatus);
	setSelectedValue('wan_port_to_lan', wanToLanStatus);

	for (idIndex=0; idIndex < apns.length; idIndex++)
	{
		addOptionToSelectElement('wan_3g_isp', apns[idIndex][0], apns[idIndex][0]);
	}

	//first load basic variables for wan & lan sections
	networkIds = ['wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', 'wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_use_mac', 'wan_mac', 'wan_use_mtu', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', 'wan_3g_device', 'wan_3g_user', 'wan_3g_pass', 'wan_3g_apn', 'wan_3g_pincode', 'wan_3g_service', 'wan_3g_isp'];
	networkPkgs = new Array();
	for (idIndex in networkIds)
	{
		networkPkgs.push('network');
	}

	networkSections = ['wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'lan', 'lan', 'lan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan'];
	networkOptions  = ['username', 'password', 'demand', 'keepalive', 'keepalive', 'ipaddr', 'netmask', 'gateway', 'macaddr','macaddr', 'mtu', 'mtu', 'ipaddr', 'netmask', 'gateway', 'device', 'username', 'password', 'apn', 'pincode', 'service', 'mobile_isp'];

	pppoeDemandParams = [5*60,1/60];
	pppoeReconnectParams = [3,0];
	pppoeIntervalParams = [5,1];
	useMtuTest = function(v){return (v=='' || v==null || v==1500 ? false : true);}
	useMacTest = function(v){v = (v== null ? '' : v);  return (v=='' || v.toLowerCase()==defaultWanMac.toLowerCase() ? false : true);}

	networkParams = ['', '', pppoeDemandParams, pppoeReconnectParams, pppoeIntervalParams, '10.1.1.10', '255.255.255.0', '127.0.0.1', useMacTest, defaultWanMac, useMtuTest, 1500, '192.168.1.1', '255.255.255.0', '192.168.1.1', '/dev/ttyUSB0', '', '', 'internet', '', 'umts', 'custom'];

	var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");

	lv=loadValueFromVariable;
	lsv=loadSelectedValueFromVariable;
	lvm=loadValueFromVariableMultiple;
	lvi=loadValueFromVariableAtIndex;
	lc=loadChecked;
	networkFunctions = [lv,lv,lvm,lvi,lvi,lv,lv,lv,lc,lv,lc,lv,lv,lv,lv,lv,lv,lv,lv,lv,lv,lv];

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

	//initialize dns
	document.getElementById("lan_dns_force").checked = (uciOriginal.get("firewall", firewallDefaultSections[0], "force_router_dns") == "1");
	document.getElementById("lan_dns_altroot").checked = (uciOriginal.get("dhcp", uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq").shift(), "server") instanceof Array);

	//is ping drop from WAN side?
	document.getElementById("drop_wan_ping").checked = (uciOriginal.get("firewall", getPingSection(), "target").toLowerCase() == "drop");

	var origDns = uciOriginal.get("network", "lan", "dns").split(/[\t ]+/);
	var routerIp = uciOriginal.get("network", "lan", "ipaddr");
	var routerGateway = uciOriginal.get("network", "lan", "gateway");
	var dIndex = 0;
	var dnsTableData = [];
	for(dIndex=0; dIndex < origDns.length; dIndex++)
	{
		var dip = origDns[dIndex] 
		var isRouterIp = !confIsBridge && dip == routerIp
		var isBridgeGw =   confIsBridge && dip == routerGateway
		if(  (!isRouterIp) && (!isBridgeGw) && validateIP(dip) == 0)
		{
			dnsTableData.push([dip]);
		}
	}

	var dnsType="custom";
	if(dnsTableData.length == 0)
	{
		dnsType = "isp";
	}
	else if( dnsTableData.join(",") == openDns.join(",") || dnsTableData.join(",") == openDns.reverse().join(",") )
	{
		dnsType = "opendns";
	}

	else if( dnsTableData.join(",") == googleDns.join(",") || dnsTableData.join(",") == googleDns.reverse().join(",") )
	{
		dnsType = "google";
	}
	setSelectedValue("lan_dns_source", dnsType);
	setDnsSource(document.getElementById("lan_dns_source"))

	var lanDnsTable=createTable([""], (dnsType == "custom" ? dnsTableData : []), "lan_dns_table", true, false);
	var lanDnsTableContainer = document.getElementById('lan_dns_table_container');
	if(lanDnsTableContainer.firstChild != null)
	{
		lanDnsTableContainer.removeChild(lanDnsTableContainer.firstChild);
	}
	lanDnsTableContainer.appendChild(lanDnsTable);

	var bridgeDnsTable = createTable([""], (dnsType == "custom" ? dnsTableData : []), "bridge_dns_table", true, false);
	var bridgeDnsTableContainer = document.getElementById('bridge_dns_table_container');
	if(bridgeDnsTableContainer.firstChild != null)
	{
		bridgeDnsTableContainer.removeChild(bridgeDnsTableContainer.firstChild);
	}
	bridgeDnsTableContainer.appendChild(bridgeDnsTable);

	//now load wireless variables
	var allWirelessSections = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");

	// generic variables
	var apcfg = "";
	var ap2cfg = "";
	var othercfg = "";
	var otherdev = "";
	var seci=0;
	for(seci=0;seci < allWirelessSections.length; seci++)
	{
		var sec = allWirelessSections[seci];
		var secmode = uciOriginal.get("wireless", sec, "mode");
		var secdev  = uciOriginal.get("wireless", sec, "device") 
		apcfg     = secmode == "ap" && secdev != wifiDevA && apcfg    == ""  ? sec : apcfg;
		ap2cfg    = secmode == "ap" && secdev == wifiDevA && ap2cfg   == ""  ? sec : ap2cfg;
		othercfg  = secmode != "ap" && secmode != "wds"   && othercfg == ""  ? sec : othercfg
		otherdev  = secmode != "ap" && secmode != "wds"   && otherdev == ""  ? secdev : otherdev
	}
	var apgcfg = apcfg;
	var apacfg = ap2cfg;
	var apcfgBand = apcfg != "" || ap2cfg != "" ? "G" : ""
	if(apgcfg == "" && apacfg != "")
	{
		apcfg = ap2cfg
		ap2cfg = ""
		apcfgBand = "A"
	}

	//wireless N variables
	if( wifiN )
	{
		var htGMode = uciOriginal.get("wireless", wifiDevG, "htmode");
		var htAMode = uciOriginal.get("wireless", wifiDevA, "htmode");
		
		setSelectedValue("wifi_channel_width", htGMode);
		setSelectedValue("wifi_channel_width_5ghz", htAMode);
	
		setSelectedValue("bridge_channel_width", htGMode);
		setSelectedValue("bridge_channel_width_5ghz", htAMode);
	
		setChannelWidth(document.getElementById("wifi_channel_width"), "G");
		setChannelWidth(document.getElementById("wifi_channel_width_5ghz"), "A");
		document.getElementById("bridge_channel_width_container").style.display="block";
	}
	else
	{
		document.getElementById("bridge_channel_width_container").style.display="none";
	}

	if(wifiN)
	{
		var hwmode = uciOriginal.get("wireless", wifiDevG, "hwmode");
		hwmode = hwmode == "" ? "11g" : hwmode;
		if(dualBandWireless)
		{
			setAllowableSelections( "wifi_hwmode", [ 'dual', '11ng', '11na', '11g', '11b' ], [basicS.DBand, 'N+G+B', 'N+A', 'G+B', 'B' ] );
			setAllowableSelections( "bridge_hwmode", [ '11ng', '11na', '11g', '11b' ], ['N+G+B', 'N+A', 'G+B', 'B' ] );
			hwmode = (ap2cfg != "" && apcfg != "") || (apcfg == "" && ap2cfg == "") || (apcfgBand == "A" && otherdev == wifiDevG) || (apcfgBand == "G" && otherdev == wifiDevA) ? "dual" : hwmode ;
			hwmode = apgcfg == "" && (apacfg != "" || otherdev == wifiDevA) ? "11na" : hwmode ;
		}
		setSelectedValue("wifi_hwmode", hwmode);
		setSelectedValue("bridge_hwmode", (hwmode == "dual" ? "11ng" : hwmode));
		if(hwmode == "dual" && otherdev != "" )
		{
			setSelectedValue("wifi_client_band", (otherdev == wifiDevA ? "5" : "2.4") )
		}
	}
	else
	{
		document.getElementById("bridge_hwmode_container").style.display = "none";
	}

	var wirelessIds=['wifi_channel1', 'wifi_channel2', 'wifi_channel1_5ghz', 'wifi_channel2_5ghz', 'wifi_ssid1', 'wifi_ssid1a', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1', 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_encryption2', 'wifi_pass2', 'wifi_wep2'];
	var wirelessPkgs= new Array();
	var wIndex;
	for(wIndex=0; wIndex < wirelessIds.length; wIndex++)
	{
		wirelessPkgs.push('wireless');
	}
	var wirelessSections=[wifiDevG, wifiDevG, wifiDevA, wifiDevA, apgcfg, apacfg, apcfg, apcfg, apcfg, apcfg, apcfg, othercfg, othercfg, othercfg, othercfg];
	var wirelessOptions=['channel', 'channel', 'channel', 'channel', 'ssid', 'ssid', 'encryption', 'key', 'key', 'server', 'port', 'ssid', 'encryption', 'key','key'];
	var wirelessParams=[wirelessDriver=="atheros" ? 'auto' : "5", wirelessDriver=="atheros" ? 'auto' : "5", "36","36", 'Gargoyle', 'Gargoyle_5GHz', 'none', '', '', '', '', 'ExistingWireless', 'none', '',''];
	var wirelessFunctions=[lsv,lsv,lsv,lsv,lv,lv,lsv,lv,lv,lv,lv,lv,lsv,lv,lv];
	loadVariables(uciOriginal, wirelessIds, wirelessPkgs, wirelessSections, wirelessOptions, wirelessParams, wirelessFunctions);	

	setSelectedValue('wifi_hidden', uciOriginal.get("wireless", apcfg, "hidden")==1 ? "disabled" : "enabled")
	setSelectedValue('wifi_isolate', uciOriginal.get("wireless", apcfg, "isolate")==1 ? "enabled" : "disabled")
	var initTxPwr = function(sel_id, txt_id, dev, band)
	{
		var txpwr = uciOriginal.get("wireless", dev, "txpower");
		var selMax = txpwr == "" ? "max" : "custom"
		setSelectedValue(sel_id, selMax)
		if(selMax == "custom") { document.getElementById(txt_id).value = txpwr; }
		updateTxPower(sel_id, txt_id, band)
	}
	initTxPwr("wifi_max_txpower", "wifi_txpower", wifiDevG, "G")
	if(wifiDevA != "")
	{
		initTxPwr("wifi_max_txpower_5ghz", "wifi_txpower_5ghz", wifiDevA, "A")
	}
	setSelectedValue('bridge_channel', uciOriginal.get("wireless", wifiDevG, "channel"));

	setSelectedValue("mac_filter_enabled", 'disabled');
	var macListStr = '';
	var policy = '';
	if(wirelessDriver == "broadcom")
	{
		policy = uciOriginal.get("wireless", wifiDevG, policyOption);
		macListStr = uciOriginal.get("wireless", wifiDevG, "maclist");
		setSelectedValue("mac_filter_enabled", ( (policy == "allow" || policy == "deny" || policy == "1" || policy == "2" ) && macListStr != "") ? 'enabled' : 'disabled');
	}
	else 
	{
		/*
		Atheros & MAC80211 have definitions in interface sections, broadcom in wifi-device section.
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
		for(wsecIndex=0; wsecIndex < allWirelessSections.length && getSelectedValue("mac_filter_enabled") == "disabled"; wsecIndex++)
		{
			macListStr = uciOriginal.get("wireless", allWirelessSections[wsecIndex], "maclist");
			if(macListStr != '')
			{
				policy = uciOriginal.get("wireless", allWirelessSections[wsecIndex], policyOption);
				setSelectedValue("mac_filter_enabled", "enabled");
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

	//load bssids for wds if necessary
	var wifiWdsData = [];
	if(apcfg != "")
	{
		var sectionIndex=0;
		var atherosFound = false;
		for(sectionIndex=0; sectionIndex < allWirelessSections.length && (!atherosFound); sectionIndex++)
		{
			if(wirelessDriver == "broadcom")
			{
				if(uciOriginal.get("wireless", allWirelessSections[sectionIndex], "mode") == "wds")
				{
					wifiWdsData.push( [ uciOriginal.get("wireless", allWirelessSections[sectionIndex], "bssid").toUpperCase()  ] );
					setSelectedValue("wifi_mode", "ap+wds");
				}
			}
			else if(wirelessDriver == "atheros")
			{
				if(uciOriginal.get("wireless", allWirelessSections[sectionIndex], "wds") == "1")
				{
					atherosFound = true;
					var bSplit = uciOriginal.get("wireless", allWirelessSections[sectionIndex], "bssid").split(/[\t ]+/);;
					var bIndex=0;
					for(bIndex=0; bIndex < bSplit.length; bIndex++)
					{
						wifiWdsData.push( [ bSplit[bIndex].toUpperCase() ]);
						setSelectedValue("wifi_mode", "ap+wds");
					}
				}
			}
		}
	}
	var wifiWdsMacTable=createTable([""], wifiWdsData, "wifi_wds_mac_table", true, false);
	var wifiWdsTableContainer = document.getElementById('wifi_wds_mac_table_container');
	if(wifiWdsTableContainer.firstChild != null)
	{
		wifiWdsTableContainer.removeChild(wifiWdsTableContainer.firstChild);
	}
	wifiWdsTableContainer.appendChild(wifiWdsMacTable);

	resetWirelessMode();
	proofreadAll();
	setBridgeVisibility();

	showApn();
	updateApnDetails();
}

function updateApnDetails()
{
	mobile_isp=getSelectedValue("wan_3g_isp");

	if(mobile_isp == "custom")
	{
		setElementEnabled( document.getElementById("wan_3g_apn"), true, document.getElementById("wan_3g_apn").value);
		setElementEnabled( document.getElementById("wan_3g_user"), true, document.getElementById("wan_3g_user").value);
		setElementEnabled( document.getElementById("wan_3g_pass"), true, document.getElementById("wan_3g_pass").value);
	}
	else
	{
		for(apnIndex=0; apnIndex < apns.length; apnIndex++)
		{
			if (mobile_isp == apns[apnIndex][0])
			{
				setElementEnabled( document.getElementById("wan_3g_apn"),  false, apns[apnIndex][1]);
				setElementEnabled( document.getElementById("wan_3g_user"), false, apns[apnIndex][2]);
				setElementEnabled( document.getElementById("wan_3g_pass"), false, apns[apnIndex][3]);
				// bug?
				document.getElementById("wan_3g_pass").value=apns[apnIndex][3];
				break;
			}
		}
	}
}

function setChannel(selectElement)
{
	var selectedValue = getSelectedValue(selectElement.id);
	if(selectElement.id.match("5ghz"))
	{
		setSelectedValue("wifi_channel1_5ghz",  selectedValue);
		setSelectedValue("wifi_channel2_5ghz",  selectedValue);
		setSelectedValue("bridge_channel_5ghz", selectedValue);
		updateTxPower("wifi_max_txpower_5ghz","wifi_txpower_5ghz", "A")
	}
	else
	{
		setSelectedValue("wifi_channel1",  selectedValue);
		setSelectedValue("wifi_channel2",  selectedValue);
		setSelectedValue("bridge_channel", selectedValue);
		updateTxPower("wifi_max_txpower", "wifi_txpower", "G");
	}
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


function addTextToSingleColumnTable(textId, tableContainerId, validator, preprocessor, validReturn, duplicatesAllowed, columnName)
{
	var val=document.getElementById(textId).value;
	if(validator(val) != validReturn)
	{
		alert(basicS.SpcEr1+" " + columnName + " "+basicS.SpcEr2);
	}
	else
	{
		val = preprocessor(val);
		var singleTable = document.getElementById(tableContainerId).firstChild;
		var singleTableData = getTableDataArray(singleTable, true, false);
		var inTable = false;
		var tabIndex = 0
		for(tabIndex = 0; tabIndex < singleTableData.length; tabIndex++)
		{
			var test = singleTableData[tabIndex];
			inTable = inTable || (val == test[0]);
		}
		if(inTable && !duplicatesAllowed)
		{
			alert(basicS.DplErr+" " + columnName);
		}
		else
		{
			addTableRow(singleTable, [val], true, false, null, null );
			document.getElementById(textId).value = "";
		}
	}
}

function setDnsSource(selectEl)
{
	var dnsSrc = getSelectedValue(selectEl.id);
	var bridgeSrc = dnsSrc == "custom" ? "custom" : "gateway";
	var lanSrc = dnsSrc == "gateway" ? "isp" : dnsSrc;
	setSelectedValue("lan_dns_source", lanSrc);
	setSelectedValue("bridge_dns_source", bridgeSrc);
	document.getElementById("lan_dns_custom_container").style.display    = dnsSrc == "custom" ? "block" : "none";
	document.getElementById("bridge_dns_custom_container").style.display = dnsSrc == "custom" ? "block" : "none";
}

function addDns(section)
{
	var textId = "add_" + section + "_dns";
	addIp = document.getElementById(textId).value;
	addTextToSingleColumnTable(textId, "lan_dns_table_container", validateIP, function(str){ return str; }, 0, false, "IP"); 
	if(addIp != "" && document.getElementById(textId).value == "")
	{
		document.getElementById(textId).value = addIp;
		addTextToSingleColumnTable("add_" + section + "_dns", "bridge_dns_table_container", validateIP, function(str){ return str; }, 0, false, "IP"); 
	}
}

function addMacToWds(section)
{
	var textId = "add_" + section + "_wds_mac";
	var tableContainerId = section + "_wds_mac_table_container";
	addTextToSingleColumnTable(textId, tableContainerId, validateMac, function(str){ return str.toUpperCase(); }, 0, false, "MAC"); 
}


function addMacToFilter()
{
	addTextToSingleColumnTable("add_mac", "mac_table_container", validateMac, function(str){ return str.toUpperCase(); }, 0, false, "MAC"); 
}

function scanWifi(ssidField)
{
	setControlsEnabled(false, true, basicS.ScnWt);
	var param = getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			scannedSsids = parseWifiScan(req.responseText);	
			if(scannedSsids[0].length > 0)
			{
				var oldSsid = document.getElementById(ssidField).value;
				document.getElementById("wifi_custom_ssid2").value = oldSsid;
				document.getElementById("bridge_custom_ssid").value = oldSsid;

				var ssidDisplay = [];
				var ssidValue = [];
				var ssidIndex=0;
				for(ssidIndex=0; ssidIndex < scannedSsids[0].length; ssidIndex++)
				{
					var ssid = scannedSsids[0][ssidIndex];
					var enc  = scannedSsids[1][ssidIndex];
					var qual = scannedSsids[3][ssidIndex];

					enc = enc =="none" ? "Open" :  enc.replace(/psk/g, "wpa").toUpperCase();
					if(wifiN && dualBandWireless)
					{
						var ghz = scannedSsids[4][ssidIndex] == "A" ? "5GHz" : "2.4GHz";
						ssidDisplay.push( ssid + " (" + enc + ", " + qual +"% Signal, " + ghz +  ")");
					}
					else
					{
						ssidDisplay.push( ssid + " (" + enc + ", " + qual +"% "+basicS.Sgnl+")");
					}
					ssidValue.push(ssidIndex + "");
				}
				ssidDisplay.push( basicS.Other );
				ssidValue.push(  "custom" );
				
				setAllowableSelections("wifi_list_ssid2", ssidValue, ssidDisplay);
				setAllowableSelections("bridge_list_ssid", ssidValue, ssidDisplay);

				var matchIndex = -1;
				var testIndex;
				var oldVal = document.getElementById(ssidField).value;
				for(testIndex=0; testIndex < scannedSsids[0].length && matchIndex < 0; testIndex++){ matchIndex = scannedSsids[0][testIndex] == oldVal ? testIndex : matchIndex; }
				if(matchIndex < 0) { matchIndex = oldVal == "Gargoyle" || oldVal == "OpenWrt" || oldVal == "" ? "0" : "custom"; }
				setSelectedValue("wifi_list_ssid2",  matchIndex + "");
				setSelectedValue("bridge_list_ssid", matchIndex + "");
			}
			else
			{
				alert(basicS.NoWLAN);
			}
			setSsidVisibility("wifi_list_ssid2");
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/scan_wifi.sh", param, stateChangeFunction);
}

function setSsidVisibility(selectId)
{
	var visIds =	[
			'bridge_list_ssid_container',
			'bridge_custom_ssid_container',
			'bridge_ssid_container',
			'bridge_channel_container',
			'bridge_fixed_channel_container',
			'bridge_encryption_container',
			'bridge_fixed_encryption_container',


			'wifi_list_ssid2_container',
			'wifi_custom_ssid2_container',
			'wifi_ssid2_container',
			'wifi_channel2_container',
			'wifi_fixed_channel2_container',
			'wifi_encryption2_container',
			'wifi_fixed_encryption2_container',


			'wifi_channel1_container',
			'wifi_fixed_channel1_container',


			'wifi_pass2_container',
			'wifi_wep2_container',
			'bridge_pass_container',
			'bridge_wep_container'
			];

	var isAp = getSelectedValue("wifi_mode").match(/ap/) && document.getElementById("global_gateway").checked ? 1 : 0;
	if(scannedSsids[0].length > 0)
	{
		var scannedIndex = getSelectedValue(selectId);
		setSelectedValue("bridge_list_ssid", scannedIndex)
		setSelectedValue("wifi_list_ssid2", scannedIndex)
		
		var ic = scannedIndex == "custom" ? 1 : 0;
		var inc = ic == 0 ? 1 : 0;
		if(inc)
		{
			scannedIndex = parseInt(scannedIndex)
			var enc  = scannedSsids[1][scannedIndex];
			var chan = scannedSsids[2][scannedIndex];
			var band = scannedSsids[4][scannedIndex];

			var modes = ['11ng',  '11g', '11b']
			var mnames = ['N+G+B', 'G+B', 'B']
			if(band == "A")
			{
				modes  = [ '11na' ];
				mnames = [ 'N+A'  ];
			}
			if(wifiN && dualBandWireless && isAp )
			{
				modes.unshift("dual")
				mnames.unshift(basicS.DBand)
			}
			var curBand = getSelectedValue("wifi_hwmode")
			setAllowableSelections( "wifi_hwmode", modes, mnames );
			setAllowableSelections( "bridge_hwmode", modes, mnames );
			if(band == "A" && curBand != "dual" && curBand != "11na")
			{
				setSelectedValue("wifi_hwmode", "dual");
			}

			setSelectedValue("wifi_encryption2", enc);
			setSelectedValue("bridge_encryption", enc);
			var enc = getSelectedText("wifi_encryption2");
			setChildText("wifi_fixed_encryption2", enc);
			setChildText("bridge_fixed_encryption", enc);

			var chanEl1 = "wifi_channel1"  + (band == "A" ? "_5ghz" : "")
			var chanEl2 = "wifi_channel2"  + (band == "A" ? "_5ghz" : "")
			var chanElB = "bridge_channel" + (band == "A" ? "_5ghz" : "")

			setSelectedValue(chanEl1, chan);
			setSelectedValue(chanEl2, chan);
			setSelectedValue(chanElB, chan);

			setChildText("wifi_fixed_channel1",  chan);
			setChildText("wifi_fixed_channel2",  chan);
			setChildText("bridge_fixed_channel", chan);
		}
		var be = getSelectedValue('bridge_encryption');
		var we = getSelectedValue('wifi_encryption2');
		var bp = be.match(/psk/) || be.match(/WPA/) ? 1 : 0;
		var bw = be.match(/wep/) || be.match(/WEP/) ? 1 : 0;
		var wp = we.match(/psk/) || we.match(/WPA/) ? 1 : 0;
		var ww = we.match(/wep/) || we.match(/WEP/) ? 1 : 0;
		setVisibility(visIds , [1,ic,0,ic,inc,ic,inc,  1,ic,0,ic,inc,ic,inc,  ic*isAp,inc*isAp,  wp,ww,bp,bw] );
	}
	else
	{
		var be = getSelectedValue('bridge_encryption');
		var we = getSelectedValue('wifi_encryption2');
		var bp = be.match(/psk/) || be.match(/WPA/) ? 1 : 0;
		var bw = be.match(/wep/) || be.match(/WEP/) ? 1 : 0;
		var wp = we.match(/psk/) || we.match(/WPA/) ? 1 : 0;
		var ww = we.match(/wep/) || we.match(/WEP/) ? 1 : 0;
		setVisibility(visIds, [0,0,1,1,0,1,0,          0,0,1,1,0,1,0,         isAp,0,    wp,ww,bp,bw] );
	}
	setHwMode(document.getElementById("wifi_hwmode"))
}

function parseWifiScan(rawScanOutput)
{
	var parsed = [ [],[],[],[],[] ];
	var cells = rawScanOutput.split(/Cell/);
	cells.shift(); //get rid of anything before first AP data

	var getCellValues=function(id, cellLines)
	{
		var vals=[];
		var lineIndex;
		for(lineIndex=0; lineIndex < cellLines.length; lineIndex++)
		{
			var line = cellLines[lineIndex];
			var idIndex = line.indexOf(id);
			var cIndex  = line.indexOf(":");
			var eqIndex = line.indexOf("=");
			var splitIndex = cIndex;
			if(splitIndex < 0 || (eqIndex >= 0 && eqIndex < splitIndex))
			{
				splitIndex = eqIndex;
			}
			if(idIndex >= 0 && splitIndex > idIndex)
			{
				var val=line.substr(splitIndex+1);
				val = val.replace(/^[^\"]*\"/g, "");
				val = val.replace(/\".*$/g, "");
				vals.push(val);
			}
		}
		return vals;
	}

	while(cells.length > 0)
	{
		var cellData = cells.shift();
		var cellLines = cellData.split(/[\r\n]+/);
		var ssid = getCellValues("ESSID", cellLines).shift();
		var channel = getCellValues("Channel", cellLines).shift();
		var freq = getCellValues("Frequency", cellLines).shift();
		var encOn = getCellValues("Encryption key", cellLines).shift();
		var ie = getCellValues("IE", cellLines);
		var qualStr = getCellValues("Quality", cellLines).shift();

		if(ssid != null && ssid != "" && encOn != null && qualStr != null)
		{
			var encType = "wep";
			while(ie.length > 0)
			{
				e = ie.shift();
				encType = e.match(/WPA2/) ? "psk2" : encType;
				encType = encType=="wep" && e.match(/WPA/) ? "psk" : encType;
			}
			var enc = encOn == "on" ? encType : "none";

			var splitQual =qualStr.replace(/[\t ]+Sig.*$/g, "").split(/\//);
			var quality = Math.round( (parseInt(splitQual[0])*100)/parseInt(splitQual[1]) );
			quality = quality > 100 ? 100 : quality;

			if(channel == null)
			{
				channel = freq.split(/\(Channel /)[1];
				channel = channel.split(/\)/)[0];
			}	

			parsed[0].push(ssid);
			parsed[1].push(enc);
			parsed[2].push(channel);
			parsed[3].push(quality);
			parsed[4].push( channel > 30 ? "A" : "G")
		}
	}

	var qualityIndices = [];
	var qIndex;
	for(qIndex=0; qIndex < parsed[3].length; qIndex++) { qualityIndices.push( [ qIndex, parsed[3][qIndex] ] ); }
	var sortQuality = function(q1,q2){ return q2[1] - q1[1]; };
	qualityIndices = qualityIndices.sort(sortQuality);
	
	var sortedParsed = [ [],[],[],[],[] ];
	while(qualityIndices.length > 0)
	{
		var i = qualityIndices.shift()[0];
		var pIndex;
		for(pIndex=0; pIndex < 5; pIndex++){ sortedParsed[pIndex].push( parsed[pIndex][i] ); }
	}
	
	return sortedParsed;
}

function setChannelWidth(selectCtl, band)
{
	var chw =  getSelectedValue(selectCtl.id)
	var hplus = chw =='HT40+';
	var h40 = (chw == 'HT40+' || chw == 'HT40-')
	if(band == "G")
	{
		setSelectedValue("wifi_channel_width", getSelectedValue(selectCtl.id));
		setSelectedValue("bridge_channel_width", getSelectedValue(selectCtl.id));

		setAllowableSelections("bridge_channel", mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel1",  mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel2",  mac80211Channels["G"], mac80211Channels["G"], document);
		var removeChannels = [];
		var rIndex = 1;
		for(rIndex=1; rIndex <= 4 && h40; rIndex++)
		{
			removeChannels.push( hplus ? mac80211Channels["G"][ mac80211Channels["G"].length-rIndex] : rIndex)
		}
		while(removeChannels.length > 0)
		{
			var rc = removeChannels.shift();
			removeOptionFromSelectElement("bridge_channel", rc, document);
			removeOptionFromSelectElement("wifi_channel1", rc, document);
			removeOptionFromSelectElement("wifi_channel2", rc, document);
		}
		updateTxPower("wifi_max_txpower", "wifi_txpower", "G");	
	}
	else if(band == "A" && mac80211Channels["A"] != null)
	{
		var origAChan  = mac80211Channels["A"]
		var aChannels  = origAChan
		if(h40)
		{
			aChannels  = []
			var validAPlus  = [36, 44, 52, 60, 149, 157]
			var validAMinus = [40, 48, 56, 64, 153, 161]
			var validTest  = hplus ? arrToHash(validAPlus) : arrToHash(validAMinus)
			for(var chanIndex=0; chanIndex < origAChan.length; chanIndex++)
			{
				var ch = origAChan[chanIndex]
				if (validTest[ch] == 1) { aChannels.push(ch); }
			}
		}
		setAllowableSelections("wifi_channel1_5ghz",  aChannels, aChannels, document);
		setAllowableSelections("wifi_channel2_5ghz",  aChannels, aChannels, document);
		setAllowableSelections("bridge_channel_5ghz", aChannels, aChannels, document);
		updateTxPower("wifi_max_txpower_5ghz","wifi_txpower_5ghz", "A")
	}
}

function getSelectedWifiChannels()
{
	var channels = []
	channels["A"] = ""
	channels["G"] = ""
	if(document.getElementById("global_gateway").checked)
	{
		var wimode = getSelectedValue("wifi_mode")
		var hwmode = "bg";
		var wifiGSelected = true;
		var wifiASelected = false;
		var dualBandSelected = false;
		if(document.getElementById("wifi_hwmode_container").style.display == "block")
		{
			hwMode = getSelectedValue("wifi_hwmode");
			wifiASelected = hwMode == "dual" || hwMode == "11na";
			wifiGSelected = hwMode != "11na";
			dualBandSelected = hwMode == "dual";
		}
		var fixedChannels = scannedSsids[0].length > 0 && wimode.match(/sta/);
		var ssidIndex = getSelectedValue("wifi_list_ssid2") 
		fixedChannels = fixedChannels && (ssidIndex != "custom")
		if(fixedChannels)
		{
			var fixedChannel = scannedSsids[2][ parseInt(ssidIndex) ]
			var fixedBand    = scannedSsids[4][ parseInt(ssidIndex) ]
			channels[ fixedBand ] = fixedChannel
			if(dualBandSelected)
			{
				var otherBand = fixedBand == "G" ? "A" : "G"
				var otherSelectId = otherBand == "G" ? "wifi_channel1" : "wifi_channel1_5ghz"
				channels[ otherBand ] = getSelectedValue(otherSelectId)
			}

		}
		if(!fixedChannels)
		{
			if(wifiGSelected)
			{
				channels[ "G" ] = getSelectedValue("wifi_channel1")
			}
			if(wifiASelected)
			{
				channels[ "A" ] = getSelectedValue("wifi_channel1_5ghz")
			}
		}
	}
	else
	{
		if( document.getElementById("bridge_hwmode_container").style.display == "block")
		{
			var hwmode = getSelectedValue("bridge_hwmode")
			if(hwmode == '11na')
			{
				channels["A"] = document.getElementById("bridge_fixed_channel_container").style.display != "none" ?  document.getElementById("bridge_fixed_channel").firstChild.data : getSelectedValue("bridge_channel_5ghz");
			}
			else
			{
				channels["G"] = document.getElementById("bridge_fixed_channel_container").style.display != "none" ?  document.getElementById("bridge_fixed_channel").firstChild.data : getSelectedValue("bridge_channel");
			}
		}
		else
		{
			channels["G"] = document.getElementById("bridge_fixed_channel_container").style.display != "none" ?  document.getElementById("bridge_fixed_channel").firstChild.data : getSelectedValue("bridge_channel");
		}
	}
	return channels ;
}

function setHwMode(selectCtl)
{
	if(	document.getElementById("wifi_hwmode_container").style.display == "none"  && 
		document.getElementById("bridge_hwmode_container").style.display == "none" 
		)
	{
		//setting of hwmode not allowed, nothing here applies
		return;
	}
	var hwmode = getSelectedValue(selectCtl.id)
	hwmode = hwmode == "dual" && document.getElementById("global_bridge").checked ? "11ng" : hwmode
	if(selectCtl.id == "bridge_hwmode" && getSelectedValue("wifi_hwmode") != "dual")
	{
		setSelectedValue("wifi_hwmode", hwmode);
	}
	setSelectedValue("bridge_hwmode", hwmode == "dual" ? "11ng" : hwmode)

	document.getElementById("wifi_channel_width_container").style.marginTop      = hwmode == "dual" ? "20px" :  "5px";
	document.getElementById("wifi_channel_width_5ghz_container").style.marginTop = hwmode == "11na" ?  "5px" : "20px";
	document.getElementById("wifi_txpower_5ghz_container").style.marginBottom    = hwmode == "11na" ?  "5px" : "20px";

	setChildText("wifi_ssid1a_label", (hwmode == "11na" ? basicS.AcPt+" SSID:" : "AP 5GHz SSID:"));
	setChildText("wifi_channel1_5ghz_label", (hwmode == "11na" ? basicS.WChn+":" : basicS.WChn+" (5GHz):"));
	setChildText("wifi_txpower_5ghz_label", (hwmode == "11na" ? basicS.TrPwr+":" : "5GHz "+basicS.TrPwr+":"));
	setChildText("wifi_channel_width_5ghz_label", (hwmode == "11na" ? basicS.ChWdth+":" : "5GHz "+basicS.ChWdth+":"));
	
	setChildText("wifi_ssid1_label", (hwmode == "dual" ?  "AP 2.4GHz SSID:" : basicS.AcPt+" SSID:"));
	setChildText("wifi_channel1_label", (hwmode == "dual" ? basicS.WChn+" (2.4GHz):" : basicS.WChn+":"));
	setChildText("wifi_txpower_label", (hwmode == "dual" ? "2.4GHz "+basicS.TrPwr+":" : basicS.TrPwr+":"));
	setChildText("wifi_channel_width_label", (hwmode == "dual" ? "2.4GHz "+basicS.ChWdth+":" : basicS.ChWdth+":"));

	document.getElementById("wifi_ssid1a").value = document.getElementById("wifi_ssid1a").value == "" ?  document.getElementById("wifi_ssid1").value + "_5GHz" :  document.getElementById("wifi_ssid1a").value;
	if(wirelessDriver == "mac80211")
	{
		setChannel(document.getElementById("wifi_channel1"), "G")
		if(hwmode == "dual" || hwmode == "11na")
		{
			setChannel(document.getElementById("wifi_channel1_5ghz"), "A")
		}
	}
	var displayWidth = (hwmode == "11ng" || hwmode == "11na" || hwmode == "dual")
	if(!displayWidth)
	{
		setSelectedValue("wifi_channel_width", "HT20");
		setChannelWidth(document.getElementById("wifi_channel_width"), "G")
	}

	var containers = [
				"wifi_channel_width_container",
				"wifi_txpower_container",
				"wifi_channel_width_5ghz_container",
				"wifi_txpower_5ghz_container",
				"wifi_ssid1a_container",
				"wifi_ssid1_container",
				"wifi_channel1_container",
				"wifi_channel2_container",
				"wifi_channel1_5ghz_container",
				"wifi_channel2_5ghz_container",
				
				"bridge_channel_width_container",
				"bridge_channel_width_5ghz_container",
				"bridge_txpower_container",
				"bridge_txpower_5ghz_container",
				"bridge_channel_container",
				"bridge_channel_5ghz_container"

				];

	var ci;
	var wimode = getSelectedValue("wifi_mode")
	var fixedChannels = scannedSsids[0].length > 0 && (wimode.match(/sta/) || document.getElementById("global_bridge").checked)
	var fixedChannelBand = "";
	if(fixedChannels)
	{
		var ssidIndex = getSelectedValue("wifi_list_ssid2") 
		fixedChannels = ssidIndex != "custom"
		if(fixedChannels)
		{
			fixedChannelBand = scannedSsids[4][ parseInt(ssidIndex) ];
		}
	}

	for(ci=0 ; ci < containers.length; ci++)
	{
		var container = document.getElementById( containers[ci] );
		var cid = container.id
		var isA = cid.match("5ghz") || cid.match(/a_/)
		var notA = !isA;
		var cBand = isA ? "A" : "G";
		var ap_only = cid.match(/1_/) || cid.match(/1a_/) 
		var cli_only = cid.match(/2_/) || cid.match(/2a_/) 
		var cli_ap_mismatch = (ap_only && !wimode.match(/ap/)) || (cli_only && !wimode.match(/sta/))
		var hide_in_favor_of_fixed_channel = fixedChannels && (cid.match(/channel/) && (!cid.match(/channel_width/))) && ((!wimode.match(/ap/)) || fixedChannelBand == cBand);
		var displayWithoutWidth = cid ==  "wifi_ssid1_container" || cid == "wifi_txpower_container" || cid == "wifi_channel1_container" || cid == "wifi_channel2_container" || cid == "bridge_txpower_container" || cid == 'bridge_channel_container'
		var vis = (displayWidth || displayWithoutWidth) && (!cli_ap_mismatch) && (!hide_in_favor_of_fixed_channel) && ((isA && (hwmode == "dual" || hwmode == "11na")) || (notA && (hwmode != "11na")))
		container.style.display = vis ? "block" : "none";
	}
	document.getElementById("wifi_client_band_container").style.display = (wimode == "ap+sta" && hwmode == "dual") ? "block" : "none";
	if(wimode == "ap+sta" && hwmode == "dual")
	{
		var cband = getSelectedValue("wifi_client_band")
		document.getElementById("wifi_client_band_container").style.display   = (!fixedChannels) ? "block" : "none"
		document.getElementById("wifi_channel2_container").style.display      = cband == "2.4" && (!fixedChannels) ? "block" : "none"
		document.getElementById("wifi_channel2_5ghz_container").style.display = cband == "5"   && (!fixedChannels) ? "block" : "none"
	}
}

function getMaxTxPower(band)
{
	var chMaxPwr = txPowerMax;
	if(wirelessDriver == "mac80211")
	{
		var ch = getSelectedValue( band == "A" ? "wifi_channel1_5ghz" : "wifi_channel1");
		var b = mac80211ChPwrs[band];
		var p = b == null ? null : b[ch];
		chMaxPwr = p == null ? chMaxPwr : p;
	}
	return chMaxPwr
}

function updateTxPower(selectId, textId, band)
{
	var chMaxPwr = getMaxTxPower(band);

	var max = getSelectedValue(selectId);
	var atMax = max == "max";
	var dbm = document.getElementById(textId).value

	var updateIds = []
	updateIds["G"] = [["wifi_max_txpower", "wifi_txpower", "wifi_dbm"], ["bridge_max_txpower", "bridge_txpower", "bridge_dbm"]]
	updateIds["A"] = [["wifi_max_txpower_5ghz", "wifi_txpower_5ghz", "wifi_dbm_5ghz"], ["bridge_max_txpower_5ghz", "bridge_txpower_5ghz", "bridge_dbm_5ghz"]]
	var bandUpdateIds = updateIds[band];
	var idIndex=0;
	for(idIndex=0; idIndex < bandUpdateIds.length; idIndex++)
	{
		sel = document.getElementById(bandUpdateIds[idIndex][0])
		txt = document.getElementById(bandUpdateIds[idIndex][1])
		lab = document.getElementById(bandUpdateIds[idIndex][2])

		setSelectedValue(sel.id, max)
		setElementEnabled(txt, !atMax, "" + chMaxPwr);
		txt.value = atMax  || dbm > chMaxPwr ? "" + chMaxPwr : "" + dbm;
		lab.firstChild.data = "(0 - " + chMaxPwr + "dBm)";
		lab.style.color = atMax ? "gray" : "black";
	}
}

function renewDhcpLease()
{
    //Send the SUGUSR1 signal to the still running but inactive udhcpc process to obtain an IP. Hopeully, it will be new (not checked).
    //The sleep seems to be necessary for the new IP to propogate through the configuration files used by uci.
    //Such problems with setChildText("dhcp_ip", wi) - wi never got an updated IP - so just reload the page for a quick fix
	var commands = [];
	commands.push("killall -SIGUSR1 udhcpc");
	commands.push("sleep 1");
	commands.push("wait_sec=10");
	commands.push("while [ $(uci -p /tmp/state get network.wan.ipaddr 2>/dev/null) == NULL ] && [ $wait_sec -gt 0 ] ; do");
	commands.push("sleep 1");
	commands.push("done");

	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, basicS.RnwL);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
            window.location.reload(true); //this reload will get the IP
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function releaseDhcpLease()
{
    //To release a DHCP lease we send the SIGUSR2 signal to udhcpc & will go inactive or wait for a SIGUSR1 renew signal.
    //Leave the WAN interface semi-configured to bring back up, just delete the IP for checks & set variables to ""
    //Then just spin for 2 seconds & update page data.
	var commands = [];
	commands.push("killall -SIGUSR2 udhcpc");
	commands.push("uci -P /var/state set network.wan.ipaddr=\'\'");
	commands.push("uci -P /var/state set network.wan.lease_lifetime=\'\'");
	commands.push("uci -P /var/state set network.wan.lease_acquired=\'\'");
	commands.push("uci -P /var/state set network.wan.gateway=\'\'");
	commands.push("uci -P /var/state set network.wan.lease_server=\'\'");
	commands.push("uci -P /var/state set network.wan.up=0");
	commands.push("uci commit");

	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, basicS.RlsL);
    setElementEnabled(document.getElementById("dhcp_release_button"), false);
    var stateChangeFunction = function(req)
    {
        setChildText("dhcp_expires", "");
        setChildText("dhcp_ip", "");
        if(req.readyState == 4)
        {
            setTimeout(setControlsEnabled(true), 2*1000);
        }
    }
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}

function singleEthernetIsWan()
{
	return singleEthernetPort() && document.getElementById("global_gateway").checked && getSelectedValue("wan_protocol").match(/wired/) 
}

function singleEthernetPort()
{
	return defaultWanIf == "" || defaultWanIf == defaultLanIf;
}

function getAltServerDefs()
{
	var defs = [];
	function addDefsForAlt(tlds, dns)
	{
		var ti;
		for(ti=0; ti< tlds.length; ti++)
		{
			var t=tlds[ti];
			var di;
			for(di=0; di< dns.length; di++)
			{
				defs.push( "/" + t + "/" + dns[di] );
			}
		}
	}
	addDefsForAlt(ncTlds, ncDns);
	addDefsForAlt(onTlds, onDns);

	return defs;
}

function set3GDevice(device)
{
	document.getElementById("wan_3g_device").value = device;
}

function scan3GDevice(field)
{
	setControlsEnabled(false, true, basicS.ScnMo);
	var param = getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var scannedDevices = [];
			scannedDevices = req.responseText.split(/\n/);
			scannedDevices.pop();
			if(scannedDevices.length > 0)
			{
				setAllowableSelections(field, scannedDevices, scannedDevices);
				set3GDevice(getSelectedValue("wan_3g_list_device"));
				document.getElementById("wan_3g_device").style.display = "none";
				document.getElementById("wan_3g_list_device").style.display = "block";
			}
			else
			{
				alert(basicS.NoDv);
			}
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/scan_3gdevices.sh", param, stateChangeFunction);
}

function updateService()
{
	setSelectedValue("wan_3g_isp", "custom");
	updateApnDetails();
	document.getElementById("wan_3g_user").value="";
	document.getElementById("wan_3g_pass").value="";

	showApn();
}

function showApn()
{
	document.getElementById("wan_3g_apn_container").style.display = getSelectedValue("wan_3g_service") != "cdma" && getSelectedValue("wan_protocol") == "3g" ? "block" : "none";
}

function getPingSection()
{
	var ruleSections = uciOriginal.getAllSectionsOfType("firewall", "rule");
	var ruleIndex;
	for(ruleIndex=0; ruleIndex < ruleSections.length; ruleIndex++)
	{
		var section   = ruleSections[ruleIndex];
		var icmp_type = uciOriginal.get("firewall", section, "icmp_type").toLowerCase();
		var family    = uciOriginal.get("firewall", section, "family").toLowerCase();
		if(icmp_type == "echo-request" && family == "ipv4")
		{
			return section;
		}
	}
}

function togglePass(radio)
{
	password_field = document.getElementById('wifi_pass' + radio);
	if(password_field.type == 'password')
	{
		password_field.type = 'text';
	} else {
		password_field.type = 'password';
	}
}
