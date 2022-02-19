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

var googleDns = [["8.8.8.8", "8.8.4.4"],["2001:4860:4860::8888","2001:4860:4860::8844"]];
var openDns = [["208.67.222.222", "208.67.220.220"],["2620:119:35::35","2620:119:53::53"]];
var openDnsFS = [["208.67.222.123", "208.67.220.123"],[]];
var quad9DNS = [["9.9.9.9", "149.112.112.112"],["2620:fe::fe","2620:fe::9"]];
var cloudflareDns = [["1.1.1.1","1.0.0.1"],["2606:4700:4700::1111","2606:4700:4700::1001"]];

var ncDns  = [["178.32.31.41", "176.58.118.172"],["2001:41d0:2:f391::401"]]
var onDns  = [["66.244.95.20", "95.211.32.162", "95.142.171.235"],[]]
var ncTlds = [ ".bit" ];
var onTlds = [ ".glue", ".parody", ".dyn", ".bbs", ".free", ".fur", ".geek", ".gopher", ".indy", ".ing", ".null", ".oss", ".micro" ];

var wanMacLoc = "wan";

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

		var wasBridge = isBridge(uciCompare);

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
			wifiDevG = wirelessDriver == "broadcom" ? "wl0" : "radio0";
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
				uci.set("wireless", wifiDevG, "htmode",  getSelectedValue("wifi_channel_width") );						//always set htmode, even if it is "NONE"
			}
			txPowerSet("wifi_max_txpower", "wifi_txpower", wifiDevG)
		}
		if(channels["A"] != "")
		{
			uci.set("wireless", wifiDevA, "channel", channels["A"]);
			uci.set("wireless", wifiDevA, "htmode",  getSelectedValue("wifi_channel_width_5ghz") );
			if(getSelectedValue("wifi_channel_width_5ghz") == "VHT80P80" && channels["A2"] != "")
			{
				uci.set("wireless", wifiDevA, "channel2", channels["A2"]);
			}
			else
			{
				uci.remove("wireless",wifiDevA, "channel2");
			}
			txPowerSet("wifi_max_txpower_5ghz", "wifi_txpower_5ghz", wifiDevA)
		}

		currentLanIp = "";
		var adjustIpCommands = ""
		var bridgeEnabledCommands = "";

		//always remove bridge configs
		preCommands = preCommands + "\nuci -q del network.bridgecfg\nuci commit\n";
		uci.removeSection("network", "bridgecfg");
		uciCompare.removeSection("network", "bridgecfg");
		preCommands = preCommands + "\nuci -q del network.wwan\nuci commit\n";
		uci.removeSection("network", "wwan");
		uciCompare.removeSection("network", "wwan");
		var lanzone = uci.getAllSectionsOfType("firewall","zone");
		for(x = 0; x < lanzone.length; x++)
		{
			var name = uci.get("firewall",lanzone[x],"name");
			if(name == "lan")
			{
				uci.remove("firewall", lanzone[x], "network")
				uci.createListOption("firewall", lanzone[x], "network", true)
				uci.set("firewall", lanzone[x], "network", ["lan"]);
			}
		}

		if( document.getElementById("global_gateway").checked )
		{
			//If we were previously bridged, and are now going to a gateway, we re-enable DHCP for the user
			//Under all other circumstances, DHCP will remain as it was previously set
			if(wasBridge)
			{
				uci.remove("dhcp","lan","ignore");
			}
			var wifiGSelected = true;
			var wifiASelected = false;
			var dualBandSelected = false;
			if(document.getElementById("wifi_hwmode_container").style.display == "block")
			{
				uci.set("wireless",  wifiDevG, "hwmode", "11g");
				wifiASelected = (getSelectedValue("wifi_hwmode_5ghz") == "disabled") ? false : true;
				wifiGSelected = (getSelectedValue("wifi_hwmode") == "disabled") ? false : true;
				dualBandSelected = ((wifiGSelected == true) && (wifiASelected == true));
			}

			currentLanIp = document.getElementById("lan_ip").value;
			if(getSelectedValue('wan_protocol') == 'none')
			{
				preCommands = preCommands + "\nuci -q del network.wan\nuci commit\n";
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
				}
				else if(getSelectedValue("wan_protocol").match(/qmi/))
				{
					uci.remove('network', 'wan', 'ifname');
				}
				else if(getSelectedValue("wan_protocol").match(/ncm/))
				{
					uci.remove('network', 'wan', 'ifname');
				}
				else if(getSelectedValue("wan_protocol").match(/mbim/))
				{
					uci.remove('network', 'wan', 'ifname');
				}
				else if(getSelectedValue("wan_protocol").match(/cdc/))
				{
					uci.set('network', 'wan', 'ifname', cdcif);
				}
				else if(getSelectedValue("wan_protocol").match(/iph/))
				{
					uci.set('network', 'wan', 'ifname', iphif);
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
			currentModes = getSelectedValue('wifi_mode');

			var guestGSelected = wifiGSelected && (getSelectedValue('wifi_guest_mode') == 'dual' || getSelectedValue('wifi_guest_mode') == '24ghz');
			var guestASelected = wifiASelected && (getSelectedValue('wifi_guest_mode') == 'dual' || getSelectedValue('wifi_guest_mode') == '5ghz');

			var apcfg = '';
			var ap2cfg = '';
			var apgncfg = '';
			var apgn2cfg = '';
			var othercfg = '';

			if(currentModes.match(/ap/))
			{
				var macConflict = [];
				macConflict.push(currentLanMac,defaultWanMac,currentWanMac,document.getElementById("wan_mac").value);
				macConflict = macConflict.concat(currentWirelessMacs);
				if(wifiGSelected)
				{
					apcfg = 'ap_g';
					uci.set("wireless", apcfg, "", "wifi-iface");
					uci.set("wireless", apcfg, "device", wifiDevG);
					uci.set('wireless', apcfg, 'mode', 'ap');
					uci.set('wireless', apcfg, 'network', 'lan');
					uci.set('wireless', apcfg, 'disassoc_low_ack', '0');
					preCommands = preCommands + "uci set wireless." + apcfg + "='wifi-iface' \n";

					if(guestGSelected)
					{
						apgncfg = 'ap_gn_g';
						uci.set("wireless", apgncfg, "", "wifi-iface");
						uci.set("wireless", apgncfg, "device", wifiDevG);
						uci.set('wireless', apgncfg, 'mode', 'ap');
						uci.set('wireless', apgncfg, 'network', 'lan');
						uci.set('wireless', apgncfg, 'disassoc_low_ack', '0');
						uci.set('wireless', apgncfg, 'is_guest_network', '1');

						if (!distribTarget.match(/ramips/))
						{
							var mac = document.getElementById("wifi_guest_mac_g").value;
							if(mac == "")
							{
								do
								{
									if(distribTarget.match(/mvebu/))
									{
										wmacIdx = wirelessIfs.indexOf(wifiDevG.replace("radio","wlan"));
										ref = currentWirelessMacs[wmacIdx];
										mac = getRandomMacWithMask(true,true,ref,"fd:ff:ff:ff:ff:f0");
									}
									else
									{
										mac = getRandomMac(true,true);
									}
								} while(macConflict.join(",").toLowerCase().split(",").indexOf(mac.toLowerCase()) != -1);
							}
							macConflict.push(mac);
							uci.set("wireless", apgncfg, 'macaddr', mac);
						}


						preCommands = preCommands + "uci set wireless." + apgncfg + "='wifi-iface' \n";
					}
				}
				if(wifiASelected)
				{
					apacfg='ap_a';
					uci.set("wireless", apacfg, "", "wifi-iface");
					uci.set("wireless", apacfg, "device", wifiDevA);
					uci.set('wireless', apacfg, 'mode', 'ap');
					uci.set('wireless', apacfg, 'network', 'lan');
					uci.set('wireless', apacfg, 'disassoc_low_ack', '0');
					preCommands = preCommands + "uci set wireless." + apacfg + "='wifi-iface' \n";

					if(guestASelected)
					{
						apgnacfg='ap_gn_a';
						uci.set("wireless", apgnacfg, "", "wifi-iface");
						uci.set("wireless", apgnacfg, "device", wifiDevA);
						uci.set('wireless', apgnacfg, 'mode', 'ap');
						uci.set('wireless', apgnacfg, 'network', 'lan');
						uci.set('wireless', apgnacfg, 'disassoc_low_ack', '0');
						uci.set('wireless', apgnacfg, 'is_guest_network', '1');

					   	var mac = document.getElementById("wifi_guest_mac_a").value ;
						if(mac == "")
						{
							do
							{
								if(distribTarget.match(/mvebu/))
								{
									wmacIdx = wirelessIfs.indexOf(wifiDevA.replace("radio","wlan"));
									ref = currentWirelessMacs[wmacIdx];
									mac = getRandomMacWithMask(true,true,ref,"fd:ff:ff:ff:ff:f0");
								}
								else
								{
									mac = getRandomMac(true,true);
								}
							} while(macConflict.join(",").toLowerCase().split(",").indexOf(mac.toLowerCase()) != -1);
						}
						macConflict.push(mac);
						uci.set("wireless", apgnacfg, 'macaddr', mac);
						preCommands = preCommands + "uci set wireless." + apgnacfg + "='wifi-iface' \n";
					}

					if(dualBandSelected)
					{
						ap2cfg = apacfg;
						if(guestASelected)
						{
							apgn2cfg = apgnacfg;
						}
					}
					else
					{
						apcfg = apacfg;
						if(guestASelected)
						{
							apgncfg = apgnacfg;
						}
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
						if(dualBandSelected)
						{
							wdsCfgs.push(ap2cfg);
						}

						var wci;
						for(wci=0; wci < wdsCfgs.length ; wci++)
						{
							wcfg = wdsCfgs[wci];
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
				if (wirelessDriver == "mac80211" && document.getElementById('wan_use_mac').checked && wimode.match(/sta/) && getSelectedValue("wan_protocol").match(/wireless/))
				{
					uci.set("wireless", othercfg, "macaddr", document.getElementById('wan_mac').value);
				}
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
			var dnsmasqSection = uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq").shift();
			var currentAltServers = uci.remove("dhcp", dnsmasqSection, "server");
			var currentRebindServers = uci.remove("dhcp", dnsmasqSection, "rebind_domain");
			var altServerDefs     = getAltServerDefs(currentAltServers, currentRebindServers, useAltRoot)
			if(altServerDefs[0].length > 0)
			{
				uci.createListOption("dhcp", dnsmasqSection, "server", true);
				uci.createListOption("dhcp", dnsmasqSection, "rebind_domain", true);
				uci.set("dhcp", dnsmasqSection, "server", altServerDefs[0]);
				uci.set("dhcp", dnsmasqSection, "rebind_domain", altServerDefs[1]);

			}

			//force clients to use router DNS?
			var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");
			var forceDNS = document.getElementById("lan_dns_force").checked ? "1" : "";
			uciOriginal.set("firewall", firewallDefaultSections[0], "force_router_dns", forceDNS);
			uci.set("firewall", firewallDefaultSections[0], "force_router_dns", forceDNS);
			var fdCommand = forceDNS == "1" ?  "\nuci set firewall.@defaults[0].force_router_dns=1 \n" : "\nuci -q del firewall.@defaults[0].force_router_dns \n";
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
			wifiGuestSsidId = wifiGSelected ? "wifi_guest_ssid1" : "wifi_guest_ssid1a";
			inputIds = [
					'wan_protocol', 'wan6_protocol', 'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', ppoeReconnectIds, 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_static_ip6', 'wan_static_gateway6', 'wan_mac', 'wan_mtu', 
					'lan_ip', 'lan_mask', 'lan_gateway', 'lan_ip6assign', 'lan_ip6hint', 'lan_ip6ifaceid', 'lan_ip6gw',
					wifiSsidId, 'wifi_ft', 'wifi_hidden', 'wifi_isolate', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1', wifiGuestSsidId, 'wifi_guest_ft', 'wifi_guest_hidden', 'wifi_guest_isolate', 'wifi_guest_encryption1', 'wifi_guest_pass1', 'wifi_guest_wep1', 'wifi_server1', 'wifi_port1', 'wifi_pass2', 'wifi_wep2', 
					'wan_3g_device', 'wan_3g_user', 'wan_3g_pass', 'wan_3g_apn', 'wan_3g_pincode', 'wan_3g_service', 'wan_3g_isp'
					];

			options = [
					'proto', 'proto', 'username', 'password', 'demand', 'keepalive', 'ipaddr', 'netmask', 'gateway', 'ip6addr', 'ip6gw', 'macaddr', 'mtu', 
					'ipaddr', 'netmask', 'gateway', 'ip6assign', 'ip6hint', 'ip6ifaceid', 'ip6gw',
					'ssid', 'ieee80211r', 'hidden', 'isolate', 'encryption', 'key', 'key', 'ssid', 'ieee80211r', 'hidden', 'isolate', 'encryption', 'key', 'key', 'auth_server', 'auth_port', 'key', 'key',
					'device', 'username', 'password', 'apn', 'pincode', 'service', 'mobile_isp'
					];

			var sv=  setVariableFromValue;
			var svm= setVariableFromModifiedValue;
			var svcat= setVariableFromConcatenation;
			var svcond= setVariableConditionally;
			setFunctions = [
					sv,sv,sv,sv,svm,svcat,sv,sv,sv,svm,svm,svcond,svcond,
					sv,sv,sv,sv,sv,sv,svm,
					sv,svcond,svcond,svcond,sv,sv,sv,sv,svcond,svcond,svcond,sv,sv,sv,sv,sv,sv,sv,
					sv,sv,sv,sv,sv,sv,sv
					];
			var f=false;
			var t=true;
			var minutesToSeconds = function(value){return value*60;};
			var lowerCase = function(value) { return value.toLowerCase(); }
			var ifCustomMac = function(value){ return (document.getElementById('wan_use_mac').checked == true) || (defaultWanMac != currentWanMac); };
			var ifCustomMtu = function(value){ return (document.getElementById('wan_use_mtu').checked == true &&  document.getElementById('wan_mtu').value != 1500);};
			var ifFtChecked =  function(value) { return getSelectedValue('wifi_ft') == "enabled" && document.getElementById('wifi_encryption1_container').style.display != "none"; };
			var ifHiddenChecked =  function(value) { return getSelectedValue('wifi_hidden') == "disabled" ? 1 : 0;}; //the label is for "broadcast", so disabled means it is hidden
			var ifIsolateChecked = function(value) { return getSelectedValue('wifi_isolate') == "enabled" ? 1 : 0;};
			var ifGuestFtChecked =  function(value) { return getSelectedValue('wifi_guest_ft') == "enabled" && document.getElementById('wifi_guest_encryption1_container').style.display != "none"; };
			var ifGuestHiddenChecked =  function(value) { return getSelectedValue('wifi_guest_hidden') == "disabled" ? 1 : 0;}; //the label is for "broadcast", so disabled means it is hidden
			var ifGuestIsolateChecked = function(value) { return getSelectedValue('wifi_guest_isolate') == "enabled" ? 1 : 0;};
			var demandParams = [f,minutesToSeconds];
			var macParams = [ifCustomMac,f,  document.getElementById('wan_mac').value.toLowerCase()];
			var mtuParams = [ifCustomMtu,t,''];
			var ftParams = [ifFtChecked,f,'1'];
			var hiddenParams = [ifHiddenChecked,f,'1'];
			var isolateParams = [ifIsolateChecked,f,'1'];
			var guestFtParams = [ifGuestFtChecked,f,'1'];
			var guestHiddenParams = [ifGuestHiddenChecked,f,'1'];
			var guestIsolateParams = [ifGuestIsolateChecked,f,'1'];
			var ip6Params = [f,ip6_canonical];

			additionalParams = [
						f,f,f,f, demandParams,f,f,f,f,ip6Params,ip6Params,macParams,mtuParams,
						f,f,f,f,f,f,ip6Params,
						f,ftParams,hiddenParams,isolateParams,f,f,f,f,guestFtParams,guestHiddenParams,guestIsolateParams,f,f,f,f,f,f,f,
						f,f,f,f,f,f,f
					];

			pppoeReconnectVisibilityIds = ['wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container'];
			multipleVisibilityIds= [pppoeReconnectVisibilityIds];
			wirelessSections=[apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apcfg, apgncfg, apgncfg, apgncfg, apgncfg, apgncfg, apgncfg, apgncfg, apcfg, apcfg, othercfg, othercfg];
			visibilityIds = [];
			pkgs = [];
			sections = [];
			var idIndex;
inputIds = [
					'wan_protocol', 'wan6_protocol', 'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', ppoeReconnectIds, 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_static_ip6', 'wan_static_gateway6', 'wan_mac', 'wan_mtu', 
					'lan_ip', 'lan_mask', 'lan_gateway', 'lan_ip6assign', 'lan_ip6hint', 'lan_ip6ifaceid', 'lan_ip6gw',
					wifiSsidId, 'wifi_ft', 'wifi_hidden', 'wifi_isolate', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1', wifiGuestSsidId, 'wifi_guest_ft', 'wifi_guest_hidden', 'wifi_guest_isolate', 'wifi_guest_encryption1', 'wifi_guest_pass1', 'wifi_guest_wep1', 'wifi_server1', 'wifi_port1', 'wifi_pass2', 'wifi_wep2', 
					'wan_3g_device', 'wan_3g_user', 'wan_3g_pass', 'wan_3g_apn', 'wan_3g_pincode', 'wan_3g_service', 'wan_3g_isp'
					];
			for(idIndex=0; idIndex < inputIds.length; idIndex++)
			{
				if(isArray(inputIds[idIndex]))
				{
					visibilityIds.push(multipleVisibilityIds.shift());
				}
				else
				{
					visibilityIds.push(inputIds[idIndex]+ "_container");
				}

				if(idIndex == 1)
				{
					pkgs.push('network');
					sections.push('wan6');
					uci.remove('network', 'wan6', options[idIndex]);
				}
				else if(idIndex == 11)
				{
					pkgs.push('network');
					sections.push(wanMacLoc);
					uci.remove('network', wanMacLoc, options[idIndex]);
				}
				else if(idIndex < 13 || idIndex > 37)
				{
					pkgs.push('network');
					sections.push('wan');
					uci.remove('network', 'wan', options[idIndex]);
				}
				else if(idIndex < 20)
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

			//set lan_ip6addr if requested
			if(document.getElementById("lan_ip6assign_option").value == "disabled")
			{
				uci.createListOption("network","lan","ip6addr",true);
				ip6addrdata = getTableDataArray(document.getElementById("lan_ip6_table"), true, false);
				uci.set("network","lan","ip6addr",ip6addrdata.map(function(row) { return row[0]; }));
			}
			else
			{
				uci.remove("network","lan","ip6addr");
			}

			//correct G networks as needed (though it would be better not to write them)
			if(apgncfg && uci.get("wireless", apgncfg, "ssid") == "")
			{
				uci.set("wireless", apgncfg, "disabled", "1");
			}

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

				if(document.getElementById("wifi_ft_key_container").style.display != "none")
				{
					uci.set("wireless", apcfg, "ft_psk_generate_local", "0");
					uci.set("wireless", apcfg, "pmk_r1_push", "1");
					var ft_key = document.getElementById("wifi_ft_key").value;
					uci.createListOption("wireless", apcfg, "r0kh", true)
					uci.createListOption("wireless", apcfg, "r1kh", true)
					var r0kh = ["ff:ff:ff:ff:ff:ff,*," + ft_key];
					var r1kh = ["00:00:00:00:00:00,00:00:00:00:00:00," + ft_key];
					uci.set("wireless", apcfg, "r0kh", r0kh, false)
					uci.set("wireless", apcfg, "r1kh", r1kh, false)
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
					var ssid = document.getElementById("wifi_ssid1a").value;
					uci.set("wireless", ap2cfg, "ssid", ssid );
					ssid || uci.set("wireless", ap2cfg, "disabled", "1");
					dup_sec_options("wireless", apcfg, ap2cfg, ['ieee80211r', 'ft_psk_generate_local', 'pmk_r1_push', 'r0kh', 'r1kh', 'hidden', 'isolate', 'encryption', 'key', 'auth_server', 'auth_port'])
				}
				if(apgn2cfg != "")
				{
					var dup_sec_options = function(pkg, fromcfg, tocfg, optlist)
					{
						var opti;
						for(opti=0; opti < optlist.length; opti++)
						{
							uci.set(pkg, tocfg, optlist[opti], uci.get("wireless", fromcfg, optlist[opti]));
						}
					}
					var ssid = document.getElementById("wifi_guest_ssid1a").value;
					uci.set("wireless", apgn2cfg, "ssid", ssid );
					ssid || uci.set("wireless", apgn2cfg, "disabled", "1");

					dup_sec_options("wireless", apgncfg, apgn2cfg, ['ieee80211r', 'hidden', 'isolate', 'encryption', 'key'])
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

				w6prot = getSelectedValue('wan6_protocol');
				ip6en = "0";
				if(getSelectedValue('wan_protocol') == 'pppoe_wired')
				{
					ip6en = "auto";
				}
				else if(w6prot != 'none')
				{
					ip6en = "1"
				}
				uci.set("network", "wan", "ipv6", ip6en);
				uci.set("network", "wan6", "proto", w6prot);
			}
			if(uci.get('network', 'lan', 'proto') === '')
			{
				uci.set('network', 'lan', 'proto', 'static');
			}

			//preserve wan mac definition even if wan is disabled if this is a bcm94704
			if((isBcm94704 || isRamips) && (uci.get("network", "wan", "type") != "bridge"))
			{
				if(uci.get("network", "wan", "macaddr") == "")
				{
					uci.set("network", "wan", "macaddr", defaultWanMac.toLowerCase());
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
				var hwMode = getSelectedValue("bridge_hwmode"); //true or false, are we using 5ghz for bridge?
				var hwMatchMode = !(String(hwMode).match(/11g/))
				bridgeDev = hwMatchMode == true ? wifiDevA : wifiDevG
				if(wifiDevG == bridgeDev)
				{
					uci.set("wireless",  wifiDevG, "hwmode", "11g");
				}
				else if(wifiDevA == bridgeDev)
				{
					uci.set("wireless",  wifiDevA, "hwmode", "11a");
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
			preCommands = preCommands + "\nuci -q del network.wan\nuci commit\n";
			uci.removeSection("network", "wan");
			uciCompare.removeSection("network", "wan");

			uci.set("network", "lan", "ipaddr",  document.getElementById("bridge_ip").value);
			uci.set("network", "lan", "netmask", document.getElementById("bridge_mask").value);
			uci.set("network", "lan", "gateway", document.getElementById("bridge_gateway").value);
			uci.set("network", "lan", "dns",     document.getElementById("bridge_gateway").value);

			//setup new bridge network
			if( getSelectedValue("bridge_mode") == "client_bridge")
			{
				preCommands = preCommands + "\nuci set network.bridgecfg=interface\nuci set network.wwan=interface\n";
				uci.set("network", "bridgecfg", "proto", "relay");
				uci.set("network", "bridgecfg", "network", "lan wwan");

				uci.set("network", "wwan", "proto", "dhcp");

				uci.set("dhcp", "lan", "ignore", "1");

				var lanzone = uci.getAllSectionsOfType("firewall","zone");
				for(x = 0; x < lanzone.length; x++)
				{
					var name = uci.get("firewall",lanzone[x],"name");
					if(name == "lan")
					{
						uci.remove("firewall", lanzone[x], "network")
						uci.createListOption("firewall", lanzone[x], "network", true)
						uci.set("firewall", lanzone[x], "network", ["lan", "wwan"]);
					}
				}
			}

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
				uci.set("wireless", "cfg2", "network", "wwan");
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
				else //mac80211 driver
				{
					var cfg = "cfg2";
					if(getSelectedValue("bridge_repeater") =="enabled")
					{
						uci.set("wireless", cfg, "", "wifi-iface");
						uci.set("wireless", cfg, "device", bridgeDev);
						uci.set("wireless", cfg, "network", "lan");
						uci.set("wireless", cfg, "mode", "ap");
						uci.set("wireless", cfg, "wds", "1");
						uci.set("wireless", cfg, "encryption", encryption);
						if(encryption != "none") { uci.set("wireless", cfg, "key", key); }

						uci.set("wireless", cfg, "ssid", document.getElementById("bridge_broadcast_ssid").value);
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
					if(encryption != "none")       {  uci.set("wireless", cfg, "key", key); }
					preCommands = preCommands + "\nuci set wireless." + cfg + "=wifi-iface\n";
				}

			}
			preCommands = preCommands + "\nuci commit\n";

			var bridgeCommandList = [];
			bridgeCommandList.push("/etc/init.d/dnsmasq disable");
			bridgeCommandList.push("/etc/init.d/miniupnpd disable");
			bridgeCommandList.push("uci -q del gargoyle.connection.dhcp");
			bridgeCommandList.push("uci -q del gargoyle.firewall.portforwarding");
			bridgeCommandList.push("uci -q del gargoyle.firewall.restriction");
			bridgeCommandList.push("uci -q del gargoyle.firewall.quotas");
			bridgeCommandList.push("uci -q del gargoyle.firewall.portforwarding");
			bridgeCommandList.push("uci set qos_gargoyle.global.network=lan");

			bridgeCommandList.push("uci commit");
			bridgeEnabledCommands = "\n" + bridgeCommandList.join("\n") + "\n";

		}

		//set wifi country
		if(document.getElementById("wireless_country_container").style.display == "block")
		{
			for(x = 0; x < uciWirelessDevs.length; x++)
			{
				uci.set("wireless",uciWirelessDevs[x],"country",document.getElementById("wireless_country").value);
			}
		}

		//set lan dns from table
		//this code is the same for both router & bridge
		//we set from lan table, but we keep bridge & lan dns tables synchronized
		//so they should be identical
		uci.remove('network', 'wan', 'dns');
		uci.remove('network', 'wan6', 'dns');
		var lanGateway = uci.get("network", "lan", "gateway");
		lanGateway = lanGateway == "" ? uci.get("network", "lan", "ipaddr") : lanGateway;
		var dns = lanGateway;
		var dns6 = "";
		var dnsSource = getSelectedValue("lan_dns_source");
		var notBridge = document.getElementById("global_gateway").checked
		if(dnsSource != "isp")
		{
			var dnsList = [];
			var dnsList6 = [];
			if(dnsSource == "google" && notBridge )
			{
				dnsList = googleDns[0];
				dnsList6 = googleDns[1];
			}
			else if(dnsSource == "opendns" && notBridge )
			{
				dnsList = openDns[0];
				dnsList6 = openDns[1];
			}
			else if(dnsSource == "opendnsfs" && notBridge )
			{
				dnsList = openDnsFS[0];
				dnsList6 = openDnsFS[1];
			}
			else if(dnsSource == "quad9" && notBridge )
			{
				dnsList = quad9DNS[0];
				dnsList6 = quad9DNS[1];
			}
			else if(dnsSource == "cloudflare" && notBridge )
			{
				dnsList = cloudflareDns[0];
				dnsList6 = cloudflareDns[1];
			}
			else //custom
			{
				var dnsData = getTableDataArray(document.getElementById("lan_dns_table_container").firstChild);
				var dnsIndex=0;
				for(dnsIndex=0; dnsIndex < dnsData.length; dnsIndex++)
				{
					if(isIPv4(dnsData[dnsIndex][0]))
					{
						dnsList.push(dnsData[dnsIndex][0]);
					}
					else if(isIPv6(dnsData[dnsIndex][0]))
					{
						dnsList6.push(dnsData[dnsIndex][0]);
					}
				}
			}
			dns = dnsList.length > 0 ? dnsList.join(" ") : dns;
			dns6 = dnsList6.length > 0 ? dnsList6.join(" ") : dns6;

			//if a wan is active and we have custom DNS settings, propagate to the wan too
			if( notBridge && uci.get("network", "wan", "") != "" && dns != lanGateway)
			{
				uci.set("network", "wan", "dns", dns);
				uci.set("network", "wan", "peerdns", "0");
				if(dns6 != "")
				{
					uci.set("network", "wan6", "dns", dns6);
					uci.set("network", "wan6", "peerdns", "0");
				}
			}
			dns = [dns,dns6].join(" ");
		}
		else if( notBridge && uci.get("network", "wan", "") != "")
		{
			uci.remove("network", "wan", "peerdns");
			uci.remove("network", "wan6", "peerdns");
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

function generateHexKey(length)
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
	document.getElementById(id).value = generateHexKey(length);
	proofreadWep(document.getElementById(id));
}

function setToFtKey(id)
{
	document.getElementById(id).value = generateHexKey(64);
	proofreadFtKey(document.getElementById(id));
}

function proofreadAll()
{
	var vlr1 = function(text){return validateLengthRange(text,1,999);};
	var vip = validateIP;
	var vnp = validatePort;
	var vnm = validateNetMask;
	var vm = validateMac;
	var vn = validateNumeric;
	var vw = validateWep;
	var vid = validateSsid;
	var vp = function(text){ return validatePass(text, 'wifi_encryption1'); }
	var vpg = function(text){ return validatePass(text, 'wifi_guest_encryption1'); }
	var vp2 = function(text){ return validatePass(text, 'wifi_encryption2'); }
	var vpb = function(text){ return validatePass(text, 'bridge_encryption'); }
	var vfk = validateFtKey;
	var vtp = function(text){ return validateNumericRange(text, 0, getMaxTxPower("G")); };
	var vtpa = function(text){ return validateNumericRange(text, 0, getMaxTxPower("A")); };
	var vip6fr = validateIP6ForceRange;
	var vip6 = validateIP6;
	var vip6m = function(text) { return validateNumericRange(text, 1, 128); };
	var vip6h = function(text) { return validateLengthRange(text,0,4) || (text.match(/[^0-9a-f]/) == null ? 0 : 1); };
	var vip6if = function(text) { return validateIP6(text) || ((ip6_canonical(text) == ip6_mask(text,-32)) ? 0 : 1); };

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
		var inputIds = [
				'wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', 'wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_mac', 'wan_mtu', 'wan_static_ip6', 'wan_static_gateway6',
				'lan_ip', 'lan_mask', 'lan_gateway', 'lan_ip6assign', 'lan_ip6hint', 'lan_ip6ifaceid', 'lan_ip6gw',
				'wifi_txpower', 'wifi_txpower_5ghz', 'wifi_ssid1', 'wifi_pass1', 'wifi_wep1', 'wifi_ft_key', 'wifi_guest_pass1', 'wifi_guest_wep1', 'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_pass2', 'wifi_wep2', 
				'wan_3g_device', 'wan_3g_apn'
		];

		var functions= [
				vlr1, vlr1, vn, vn, vn, vip, vnm, vip, vm, vn, vip6fr, vip6,
				vip, vnm, vip, vip6m, vip6h, vip6if, vip6,
				vtp, vtpa, vid, vp, vw, vfk, vpg, vw, vip, vnp, vid, vp2, vw,
				vlr1, vlr1
		];

		var optInputIds = ['wifi_ssid1a', 'wifi_guest_ssid1', 'wifi_guest_ssid1a']
		for(index in optInputIds)
		{
			var input = optInputIds[index]
			if(document.getElementById(input + "_container").style.display == "block")
			{
				inputIds.push(input);
				functions.push(vid);
			}
		}

		var returnCodes= new Array();
		var visibilityIds = new Array();
		var idIndex;
		for(idIndex= 0; idIndex < inputIds.length; idIndex++)
		{
			returnCodes.push(0);
			var id = inputIds[idIndex];
			var container = id + "_container";
			visibilityIds.push( container );
		}

		var labelIds = new Array();
		for(idIndex= 0; idIndex < inputIds.length; idIndex++)
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
		var functions = [vip, vnm, vip, vtp, vid, vpb, vw, vid];
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
	var channelError = testChannels();
	if(channelError != null){ errors.push(channelError); }

	return errors;
}

function setGlobalVisibility()
{
	if(getSelectedValue("wan_protocol") == "none")
	{
		setSelectedValue("wan6_protocol", "none");
		document.getElementById("wan6_protocol").disabled = true;
	}
	else
	{
		document.getElementById("wan6_protocol").disabled = false;
	}

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
		if(wirelessDriver == "")
		{
			removeOptionFromSelectElementByValue("wifi_mode", "ap", document);
			removeOptionFromSelectElementByValue("wifi_mode", "ap+wds", document);
			removeOptionFromSelectElementByValue("wifi_mode", "adhoc", document);
		}
		if(currentMode == 'ap+sta' || currentMode == 'sta')
		{
			setSelectedValue('wifi_mode', 'ap');
		}
		else
		{
			setSelectedValue('wifi_mode', currentMode);
		}
	}

	var proto1 = ['dhcp_wired', 'pppoe_wired', 'static_wired'];
	var proto2 = ['DHCP ('+basicS.Wird+')', 'PPPoE ('+basicS.Wird+')', basicS.StIP+' ('+basicS.Wird+')'];

	if(wirelessDriver)
	{
		proto1.push('dhcp_wireless','static_wireless');
		proto2.push('DHCP ('+basicS.Wrlss+')',basicS.StIP+' ('+basicS.Wrlss+')');
	}
	if(hasUSB)
	{
		proto1.push('3g');
		proto2.push(basicS.Mo3g);
	}

	if(hasQMI)
	{
		proto1.push('qmi');
		proto2.push(basicS.Mo3gQMI);
	}

	if(hasNCM)
	{
		proto1.push('ncm');
		proto2.push(basicS.Mo3gNCM);
	}

	if(hasMBIM)
	{
		proto1.push('mbim');
		proto2.push(basicS.Mo3gMBIM);
	}

	if(cdcif != "")
	{
		proto1.push('dhcp_cdc');
		proto2.push(basicS.Mo3gHiLink);
	}

	if(iphif != "")
	{
		proto1.push('dhcp_iph');
		proto2.push(basicS.Mo3gIPH);
	}

	proto1.push('none');
	proto2.push(UI.Disabled);
	setAllowableSelections('wan_protocol', proto1, proto2);

	setVisibility( [ 'wan_port_to_lan_container' ], ((getSelectedValue("wan_protocol") == 'none' || getSelectedValue("wan_protocol").match(/wireless/) || getSelectedValue("wan_protocol").match(/3g/) || getSelectedValue("wan_protocol").match(/ncm/) || getSelectedValue("wan_protocol").match(/qmi/) || getSelectedValue("wan_protocol").match(/mbim/) || getSelectedValue("wan_protocol").match(/iph/))  && (!singleEthernetPort())) ? [1] : [0] )

	setWanVisibility();
	setWifiVisibility();
}

function setWanVisibility()
{
	var wanIds=['wan_dhcp_ip_container', 'wan_dhcp_expires_container', 'wan_pppoe_user_container', 'wan_pppoe_pass_container', 'wan_pppoe_reconnect_mode_container', 'wan_pppoe_max_idle_container', 'wan_pppoe_reconnect_pings_container', 'wan_pppoe_interval_container', 'wan_static_ip_container', 'wan_static_mask_container', 'wan_static_gateway_container', 'wan_mac_container', 'wan_mtu_container', 'wan_ping_container', 'lan_gateway_container', 'wan_3g_device_container', 'wan_3g_user_container', 'wan_3g_pass_container', 'wan_3g_apn_container', 'wan_3g_pincode_container', 'wan_3g_service_container', 'wan_3g_isp_container', 'wan_dhcp6_ip_container', 'wan_static_ip6_container', 'wan_static_gateway6_container', 'lan_ip6gw_container', 'lan_ip6addr_container', 'lan_ip6hint_container', 'lan_ip6ifaceid_container', 'lan_ip6assign_container'];

	var maxIdleIndex = 5;
	var notWifi= getSelectedValue('wan_protocol').match(/wireless/) ? 0 : 1;
	var w6p = getSelectedValue('wan6_protocol') == 'static' ? 1 : 0;
	var w6en = getSelectedValue('wan6_protocol') == 'none' ? 0 : w6p == 0 ? 1 : 0;
	var l6a = getSelectedValue('lan_ip6assign_option') == "enabled" ? 1 : 0;
	var nl6a = !l6a;

	var dhcpVisability     = [1,1,  0,0,0,0,0,0,  0,0,0,  1,notWifi,1,       0, 0,0,0,0,0,0,0,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];
	var pppoeVisability    = [0,0,  1,1,1,1,1,1,  0,0,0,  notWifi,notWifi,1, 0, 0,0,0,0,0,0,0,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];
	var staticVisability   = [0,0,  0,0,0,0,0,0,  1,1,1,  1,notWifi,1,       0, 0,0,0,0,0,0,0,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];
	var disabledVisability = [0,0,  0,0,0,0,0,0,  0,0,0,  0,0,0,             1, 0,0,0,0,0,0,0,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];
	var tgVisability       = [0,0,  0,0,0,0,0,0,  0,0,0,  0,0,1,             0, 1,1,1,1,1,1,1,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];
	var qmiVisability      = [0,0,  0,0,0,0,0,0,  0,0,0,  1,0,1,             0, 1,1,1,1,1,0,1,  w6en,w6p,w6p,  nl6a,nl6a,l6a,l6a,l6a];

	var wanVisibilities= new Array();
	wanVisibilities['dhcp'] = dhcpVisability;
	wanVisibilities['pppoe'] = pppoeVisability;
	wanVisibilities['static'] = staticVisability;
	wanVisibilities['none'] = disabledVisability;
	wanVisibilities['3g'] = tgVisability;
	wanVisibilities['qmi'] = qmiVisability;
	wanVisibilities['ncm'] = qmiVisability;
	wanVisibilities['mbim'] = qmiVisability;
	wanVisibilities['iph'] = qmiVisability;

	var selectedVisibility= wanVisibilities[ getSelectedValue("wan_protocol").replace(/_.*$/g, "") ];

	selectedVisibility[maxIdleIndex] = selectedVisibility[maxIdleIndex] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'demand' ? 1 : 0;
	selectedVisibility[maxIdleIndex+1] = selectedVisibility[maxIdleIndex+1] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;
	selectedVisibility[maxIdleIndex+2] = selectedVisibility[maxIdleIndex+2] ==1 && document.getElementById('wan_pppoe_reconnect_mode').value == 'keepalive' ? 1 : 0;

	setVisibility(wanIds, selectedVisibility);
	showApn();
}

function setWifiVisibility()
{
	var wifiMode=getSelectedValue("wifi_mode");
	if(wifiMode == "ap+wds")
	{
		var values = ['none', 'psk2', 'psk', 'wep'];
		var names = [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP'];
		if(wpad_sae)
		{
			values.splice(values.indexOf('psk2'),0,'sae-mixed','sae');
			names.splice(names.indexOf('WPA2 PSK'),0,'WPA3/WPA2 SAE/PSK','WPA3 SAE');
		}
		if(wpad_owe)
		{
			values.push('owe');
			names.push('OWE');
		}
		setAllowableSelections('wifi_encryption1', values, names);
	}
	else
	{
		var values = ['none', 'psk2', 'psk', 'wep'];
		var names = [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP'];
		if(wpad_sae)
		{
			values.splice(values.indexOf('psk2'),0,'sae-mixed','sae');
			names.splice(names.indexOf('WPA2 PSK'),0,'WPA3/WPA2 SAE/PSK','WPA3 SAE');
		}
		if(wpad_owe)
		{
			values.push('owe');
			names.push('OWE');
		}
		if(wpad_eap)
		{
			values.push('wpa2');
			names.push('WPA2 RADIUS');
			values.push('wpa');
			names.push('WPA RADIUS');
			if(wpad_sb192)
			{
				//values.splice(values.indexOf('wpa2'),0,'wpa3');
				//names.splice(names.indexOf('WPA2 RADIUS'),0,'WPA3 RADIUS');
			}
		}
		setAllowableSelections('wifi_encryption1', values, names);
	}

	if(wifiMode == 'adhoc')
	{
		setAllowableSelections('wifi_encryption2', ['none', 'wep'], [basicS.None, 'WEP']);
		document.getElementById("wifi_ssid2_label").firstChild.data = "SSID:";
	}
	else
	{
		document.getElementById("wifi_ssid2_label").firstChild.data = basicS.Join+":";
		var values = ['none', 'psk2', 'psk', 'wep'];
		var names = [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP'];
		if(wpad_sae)
		{
			values.splice(values.indexOf('psk2'),0,'sae-mixed','sae');
			names.splice(names.indexOf('WPA2 PSK'),0,'WPA3/WPA2 SAE/PSK','WPA3 SAE');
		}
		if(wpad_owe)
		{
			values.push('owe');
			names.push('OWE');
		}
		setAllowableSelections('wifi_encryption2', values, names);
	}

	if(GwifiN == false)
	{
		setAllowableSelections('wifi_hwmode', ['disabled', '11g'], [UI.Disabled, 'B+G']);
	}
	else
	{
		setAllowableSelections('wifi_hwmode', ['disabled', '11gn', '11g'], [UI.Disabled, 'B+G+N', 'B+G']);
	}

	if((AwifiN == false) && ((uciOriginal.get("wireless", wifiDevA, "hwmode"))==""))
	{
		setAllowableSelections('wifi_hwmode_5ghz', ['disabled'], [UI.Disabled]);
	}
	else if(AwifiN == false)
	{
		setAllowableSelections('wifi_hwmode_5ghz', ['disabled', '11a'], [UI.Disabled, 'A']);
	}
	else if(AwifiAC == true)
	{
		setAllowableSelections('wifi_hwmode_5ghz', ['disabled', '11anac', '11an', '11a'], [UI.Disabled, 'A+N+AC', 'A+N', 'A']);
	}
	else
	{
		setAllowableSelections('wifi_hwmode_5ghz', ['disabled', '11an', '11a'], [UI.Disabled, 'A+N', 'A']);
	}


	var wifiIds=[	'internal_divider1',
			'wifi_hwmode_container',
			'wifi_channel_width_container',
			'wifi_txpower_container',
			'wifi_hwmode_5ghz_container',
			'wifi_channel_width_5ghz_container',
			'wifi_txpower_5ghz_container',
			'mac_enabled_container',
			'mac_filter_container',
			'wireless_country_container',


			'wifi_ssid1_container',
			'wifi_ssid1a_container',
			'wifi_channel1_container',
			'wifi_fixed_channel1_container',
			'wifi_channel1_5ghz_container',
			'wifi_channel1_seg2_5ghz_container',
			'wifi_ft_container',
			'wifi_ft_key_container',
			'wifi_hidden_container',
			'wifi_isolate_container',
			'wifi_encryption1_container',
			'wifi_pass1_container',
			'wifi_wep1_container',
			'wifi_server1_container',
			'wifi_port1_container',


			'internal_divider3',
			'wifi_guest_ssid1_container',
			'wifi_guest_ssid1a_container',
			'wifi_guest_ft_container',
			'wifi_guest_hidden_container',
			'wifi_guest_isolate_container',
			'wifi_guest_encryption1_container',
			'wifi_guest_pass1_container',
			'wifi_guest_wep1_container',
			'wifi_guest_mode_container',


			'wifi_mac_container',
			'wifi_wds_container',


			'internal_divider2',
			'wifi_list_ssid2_container',
			'wifi_custom_ssid2_container',
			'wifi_ssid2_container',
			'wifi_channel2_container',
			'wifi_fixed_channel2_container',
			'wifi_channel2_5ghz_container',
			'wifi_client_band_container',
			'wifi_encryption2_container',
			'wifi_fixed_encryption2_container',
			'wifi_pass2_container',
			'wifi_wep2_container'
			];

	var ae = wifiDevA != "" ? 1 : 0; //A band exists
	var g  = (getSelectedValue("wifi_hwmode") == "disabled") ? 0: 1; //2.4 disabled?
	var a  = (getSelectedValue("wifi_hwmode_5ghz") == "disabled") ? 0: 1; //5 disabled?
	var gw = g && (getSelectedValue("wifi_hwmode") != "11g"); //do we need 2.4 ch width? g& a check for N
	var aw = a && (getSelectedValue("wifi_hwmode_5ghz") != "11a"); //do we need 5 ch width? a& a check for N or AC
	var as2 = a && (getSelectedValue("wifi_channel_width_5ghz") == "VHT80P80");

	var mf = getSelectedValue("mac_filter_enabled") == "enabled" ? 1 : 0;
	var e1 = document.getElementById('wifi_encryption1').value;
	var p1 = (e1 != 'none' && e1 != 'wep' && e1 != 'owe') ? 1 : 0;
	var w1 = (e1 == 'wep') ? 1 : 0;
	var r1 = (e1 == 'wpa' || e1 == 'wpa2') ? 1 : 0;
	var k1 = r1 && (getSelectedValue('wifi_ft') == "enabled" ? 1 : 0);
	var gns = (wirelessDriver == "mac80211" && !isb43); //drivers that support guest networks
	var gn = getSelectedValue("wifi_guest_mode") != "disabled" ? 1 : 0;
	var gng = getSelectedValue("wifi_guest_mode") == "24ghz" || getSelectedValue("wifi_guest_mode") == 'dual'  ? 1 : 0;
	var gna = getSelectedValue("wifi_guest_mode") == "5ghz" || getSelectedValue("wifi_guest_mode") == 'dual' ? 1 : 0;
	var ge1 = document.getElementById('wifi_guest_encryption1').value;
	var gp1 = gn && (ge1 != 'none' && ge1 != 'wep') ? 1 : 0;
	var gw1 = gn && (ge1 == 'wep') ? 1 : 0;
	var gr1 = gn && (ge1 == 'wpa' || ge1 == 'wpa2') ? 1 : 0;
	var e2 = document.getElementById('wifi_fixed_encryption2').style.display != 'none' ? document.getElementById('wifi_fixed_encryption2').firstChild.data : getSelectedValue('wifi_encryption2');
	var b = (GwifiN ? 0 : 1) && (AwifiN ? 0 : 1); //we shouldnt have to look for AC here

	var p2 = e2.match(/sae/) || e2.match(/psk/) || e2.match(/WPA/) ? 1 : 0;
	var w2 = e2.match(/wep/) || e2.match(/WEP/) ? 1 : 0;

	var wc = checkWifiCountryVisibility();

	var wifiVisibilities = new Array();
	wifiVisibilities['ap']       = [1,1,gw,g,ae,aw,a,1,mf,wc,   1,a,1,0,a,as2,p1,k1,1,1,1,p1,w1,r1,r1,   gns,gng,gna,gp1,gn,gn,gn,gp1,gw1,gns,   0,0,  0,0,0,0,0,0,0,0,0,0,0,0 ];
	wifiVisibilities['ap+wds']   = [1,1,gw,g,ae,aw,a,1,mf,wc,   1,0,1,0,0,0,p1,k1,1,1,1,p1,w1,r1,r1,   gns,gng,gna,gp1,gn,gn,gn,gp1,gw1,gns,   b,b,  0,0,0,0,0,0,0,0,0,0,0,0 ];
	wifiVisibilities['sta']      = [1,1,gw,g,ae,aw,a,1,mf,wc,   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,                  0,0,  0,0,0,1,g,0,a,0,1,0,p2,w2];
	wifiVisibilities['ap+sta']   = [1,1,gw,g,ae,aw,a,1,mf,wc,   1,a,1,0,a,as2,p1,k1,1,1,1,p1,w1,r1,r1,   gns,gng,gna,gp1,gn,gn,gn,gp1,gw1,gns,   0,0,  1,0,0,1,g,0,a,a,1,0,p2,w2];
	wifiVisibilities['adhoc']    = [1,1,gw,g,ae,aw,a,1,mf,wc,   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,                  0,0,  0,0,0,1,g,0,a,0,1,0,p2,w2];
	wifiVisibilities['disabled'] = [0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,                  0,0,  0,0,0,0,0,0,0,0,0,0,0,0 ];

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
		var values = ['none', 'psk2', 'psk', 'wep'];
		var names = [basicS.None, 'WPA2 PSK', 'WPA PSK', 'WEP'];
		if(wpad_sae)
		{
			values.splice(values.indexOf('psk2'),0,'sae-mixed','sae');
			names.splice(names.indexOf('WPA2 PSK'),0,'WPA3/WPA2 SAE/PSK','WPA3 SAE');
		}
		if(wpad_owe)
		{
			values.push('owe');
			names.push('OWE');
		}
		setAllowableSelections('bridge_encryption', values, names);
		var brenc = document.getElementById("bridge_fixed_encryption_container").style.display == "none" ? getSelectedValue("bridge_encryption") : document.getElementById("bridge_fixed_encryption").firstChild.data;
		document.getElementById("bridge_pass_container").style.display = brenc.match(/sae/) || brenc.match(/psk/) || brenc.match(/WPA/) ? "block" : "none";
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

		document.getElementById("bridge_wireless_country_container").style.display = checkWifiCountryVisibility() ? "block" : "none";

		document.getElementById("bridge_note").innerHTML = bridgeMode == "wds" ? basicS.BrNoteWDS : basicS.BrNoteClient;
	}

	var allowedbridgemodes = [];
	var allowedbridgemodes2 = [];
	if(dualBandWireless)
	{
		if(GwifiN)
		{
			allowedbridgemodes.push('B+G+N')
			allowedbridgemodes2.push('11gn');
		}
		allowedbridgemodes.push('B+G');
		allowedbridgemodes2.push('11g');
		if(AwifiAC)
		{
			allowedbridgemodes.push('A+N+AC')
			allowedbridgemodes2.push('11anac');
		}
		if(AwifiN)
		{
			allowedbridgemodes.push('A+N')
			allowedbridgemodes2.push('11an');
		}

		allowedbridgemodes.push('A');
		allowedbridgemodes2.push('11a');
	}
	else
	{
		if(GwifiN)
		{
			allowedbridgemodes.push('B+G+N')
			allowedbridgemodes2.push('11gn');
		}
		allowedbridgemodes.push('B+G');
		allowedbridgemodes2.push('11g');

	}


	//If no wireless, disable bridge/repeater setup
	if(wirelessDriver == "")
	{
		document.getElementById("global_bridge").disabled = true;
	}
	setAllowableSelections("bridge_hwmode",allowedbridgemodes2,allowedbridgemodes)

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
	else if(systemDateFormat == "hungary")
	{
		ldateStr = y4 + "." + m + "." + d + h;
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
	wanMacAltLoc = defaultWanIf != "" ? ("wan_" + defaultWanIf.replace(".","_") + "_dev"): "wan_dev";
	wanMacLoc = uciOriginal.get("network",wanMacAltLoc) != "" ? wanMacAltLoc : wanMacLoc;
	var removeChannels = [];
	var hwAmode = "disabled";
	var hwGmode = "disabled";
	if(wirelessDriver == "broadcom")
	{
		//no auto for brcm
		removeChannels.push("auto");
	}
	else if(wirelessDriver == "mac80211")
	{
		setAllowableSelections("bridge_channel", mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel1",  mac80211Channels["G"], mac80211Channels["G"], document);
		setAllowableSelections("wifi_channel2",  mac80211Channels["G"], mac80211Channels["G"], document);
		if(mac80211Channels["A"] != null)
		{
			setAllowableSelections("wifi_channel1_5ghz",   mac80211Channels["A"], mac80211Channels["A"], document);
			setAllowableSelections("wifi_channel1_seg2_5ghz",   mac80211Channels["A"], mac80211Channels["A"], document);
			setAllowableSelections("bridge_channel_5ghz",  mac80211Channels["A"], mac80211Channels["A"], document);
			setAllowableSelections("bridge_channel_seg2_5ghz",  mac80211Channels["A"], mac80211Channels["A"], document);
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

	var ip6txt = "-";
	if(currentWanIp6.length > 0)
	{
		ip6txt = "";
		for(var x = 0; x < currentWanIp6.length; x++)
		{
			if(ip6_scope(currentWanIp6[x])[0] == "Global" && currentWanMask6[x] == "128")
			{
				ip6txt = ip6txt + (x == 0 ? "" : "\n") + currentWanIp6[x];
			}
		}
				
	}
	setChildText("dhcp6_ip", ip6txt);


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
		}
	}
	else
	{
		setSelectedValue("bridge_mode", "client_bridge");
		setSelectedValue("bridge_repeater", "enabled");
		document.getElementById("bridge_ssid").value = "Gargoyle";
		setSelectedValue("bridge_channel", "5");
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

	if(isBcm94704 && (!wanIsWireless) && uciOriginal.get("network", wanMacLoc, "macaddr") != "")
	{
		var currentMac = uciOriginal.get("network", wanMacLoc, "macaddr").toUpperCase();
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
	if(wp != "none" && wp != "3g" && wp != "qmi" && wp != "ncm" && wp != "mbim" && wp != "iph") { wp = wanIsWifi ? wp + "_wireless" : wp + "_wired"; }
	if(wp == "dhcp_wired")
	{
		if(wanUciIf == cdcif)
		{
			wp = "dhcp_cdc";
		}
		else if(wanUciIf == iphif)
		{
			wp = "dhcp_iph";
		}
	}
	setSelectedValue("wan_protocol", wp);

	var wp6 = uciOriginal.get("network", "wan6", "proto");
	var wp6en = uciOriginal.get("network", "wan", "ipv6");
	if(wp6en == "" || wp6en == "0")
	{
		wp6 = "none";
	}
	setSelectedValue("wan6_protocol", wp6);

	var wanToLanStatus = lanUciIf.indexOf(defaultWanIf) < 0 ? 'disable' : 'bridge' ;
	setSelectedValue('bridge_wan_port_to_lan', wanToLanStatus);
	setSelectedValue('wan_port_to_lan', wanToLanStatus);

	for(idIndex=0; idIndex < apns.length; idIndex++)
	{
		addOptionToSelectElement('wan_3g_isp', apns[idIndex][0], apns[idIndex][0]);
	}


	//first load basic variables for wan & lan sections
	networkIds = ['wan_pppoe_user', 'wan_pppoe_pass', 'wan_pppoe_max_idle', 'wan_pppoe_reconnect_pings', 'wan_pppoe_interval', 'wan_static_ip', 'wan_static_mask', 'wan_static_gateway', 'wan_static_ip6', 'wan_static_gateway6', 'wan_use_mac', 'wan_mac', 'wan_use_mtu', 'wan_mtu', 'lan_ip', 'lan_mask', 'lan_gateway', 'lan_ip6assign', 'lan_ip6assign_option', 'lan_ip6hint', 'lan_ip6ifaceid', 'wan_3g_device', 'wan_3g_user', 'wan_3g_pass', 'wan_3g_apn', 'wan_3g_pincode', 'wan_3g_service', 'wan_3g_isp'];
	networkPkgs = new Array();
	for(idIndex in networkIds)
	{
		networkPkgs.push('network');
	}

	networkSections = ['wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', wanMacLoc, wanMacLoc, 'wan', 'wan', 'lan', 'lan', 'lan', 'lan', 'lan', 'lan', 'lan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan', 'wan'];
	networkOptions  = ['username', 'password', 'demand', 'keepalive', 'keepalive', 'ipaddr', 'netmask', 'gateway', 'ip6addr', 'ip6gw', 'macaddr','macaddr', 'mtu', 'mtu', 'ipaddr', 'netmask', 'gateway', 'ip6assign', 'ip6assign', 'ip6hint', 'ip6ifaceid', 'device', 'username', 'password', 'apn', 'pincode', 'service', 'mobile_isp'];

	pppoeDemandParams = [5*60,1/60];
	pppoeReconnectParams = [3,0];
	pppoeIntervalParams = [5,1];
	useMtuTest = function(v){return (v=='' || v==null || v==1500 ? false : true);}
	useMacTest = function(v){v = (v== null ? '' : v);  return (v=='' || v.toLowerCase()==defaultWanMac.toLowerCase() ? false : true);}
	ip6AssignTest = function(v){return (v == "" ? "disabled" : "enabled");}

	networkParams = ['', '', pppoeDemandParams, pppoeReconnectParams, pppoeIntervalParams, '10.1.1.10', '255.255.255.0', '127.0.0.1', '2001:db80::2/64', '2001:db80::1', useMacTest, defaultWanMac, useMtuTest, 1500, '192.168.1.1', '255.255.255.0', '192.168.1.1', '60', ['60', ip6AssignTest], '', '::1', '/dev/ttyUSB0', '', '', 'internet', '', 'umts', 'custom'];

	var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");

	lv=loadValueFromVariable;
	lsv=loadSelectedValueFromVariable;
	lvm=loadValueFromVariableMultiple;
	lvi=loadValueFromVariableAtIndex;
	lc=loadChecked;
	lvmod=loadValueFromModifiedVariable;
	networkFunctions = [lv,lv,lvm,lvi,lvi,lv,lv,lv,lv,lv,lc,lv,lc,lv,lv,lv,lv,lv,lvmod,lv,lv,lv,lv,lv,lv,lv,lv,lv];

	loadVariables(uciOriginal, networkIds, networkPkgs, networkSections, networkOptions, networkParams, networkFunctions);

	//load additional ipv6
	lan_ip6addrdata = [];
	lan_ip6addr = uciOriginal.get("network","lan","ip6addr");
	if(lan_ip6addr != "")
	{
		if(typeof(lan_ip6addr) == "object")
		{
			for(var x = 0; x < lan_ip6addr.length; x++)
			{
				scope = ip6_scope(lan_ip6addr[x]);
				if(scope[0] == "Global")
				{
					if(scope[1] == "Global Unicast Address")
					{
						lan_ip6addrgw = lan_ip6addr[x];
					}
					lan_ip6addrdata.push([lan_ip6addr[x]]);
				}
			}
		}
		else
		{
			lan_ip6addrdata.push([lan_ip6addr]);
		}
	}
	else if(currentLanIp6.length > 0)
	{
		lan_ip6addrgw = "";
		for(var x = 0; x < currentLanIp6.length; x++)
		{
			scope = ip6_scope(currentLanIp6[x]);
			if(scope[0] == "Global")
			{
				if(scope[1] == "Global Unicast Address")
				{
					lan_ip6addrgw = currentLanIp6[x] + "/" + currentLanMask6[x];
				}
				lan_ip6addrdata.push([currentLanIp6[x] + "/" + currentLanMask6[x]]);
			}
		}
	}
	else
	{
		//load default
		lan_ip6addrdata.push([currentULAPrefix + "1/60"]);
	}

	var lanIp6Table=createTable([""], lan_ip6addrdata, "lan_ip6_table", true, false);
	var lanIp6TableContainer = document.getElementById('lan_ip6_table_container');
	if(lanIp6TableContainer.firstChild != null)
	{
		lanIp6TableContainer.removeChild(lanIp6TableContainer.firstChild);
	}
	lanIp6TableContainer.appendChild(lanIp6Table);

	lan_ip6gw = uciOriginal.get("network","lan","ip6gw");
	document.getElementById("lan_ip6gw").value = lan_ip6gw == "" ? ip6_splitmask(lan_ip6addrgw).address : lan_ip6gw;

	var ip6txt = "";
	for(var x = 0; x < currentLanIp6.length; x++)
	{
		if(ip6_scope(currentLanIp6[x])[0] == "Global")
		{
			ip6txt = ip6txt + (x == 0 ? "" : "\n") + currentLanIp6[x] + "/" + currentLanMask6[x];
		}
	}
	setChildText("lan_ip6", ip6txt);

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
		if(  (!isRouterIp) && (!isBridgeGw) && (validateIP(dip) == 0 || validateIP6(dip) == 0))
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
	else if( dnsTableData.join(",") == openDnsFS.join(",") || dnsTableData.join(",") == openDnsFS.reverse().join(",") )
	{
		dnsType = "opendnsfs";
	}
	else if( dnsTableData.join(",") == googleDns.join(",") || dnsTableData.join(",") == googleDns.reverse().join(",") )
	{
		dnsType = "google";
	}
	else if( dnsTableData.join(",") == quad9DNS.join(",") || dnsTableData.join(",") == quad9DNS.reverse().join(",") )
	{
		dnsType = "quad9";
	}
	else if( dnsTableData.join(",") == cloudflareDns.join(",") || dnsTableData.join(",") == cloudflareDns.reverse().join(",") )
	{
		dnsType = "cloudflare";
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
	var apgncfg = "";
	var apgn2cfg = "";
	var othercfg = "";
	var otherdev = "";
	var seci=0;
	for(seci=0;seci < allWirelessSections.length; seci++)
	{
		var sec = allWirelessSections[seci];
		var secmode = uciOriginal.get("wireless", sec, "mode");
		var secdev  = uciOriginal.get("wireless", sec, "device")
		var isguest = uciOriginal.get("wireless", sec, "is_guest_network")

		if(isguest == "1")
		{
			apgncfg     = secmode == "ap" && secdev != wifiDevA && apgncfg  == ""  ? sec : apgncfg;
			apgn2cfg    = secmode == "ap" && secdev == wifiDevA && apgn2cfg == ""  ? sec : apgn2cfg;
		}
		else
		{
			apcfg     = secmode == "ap" && secdev != wifiDevA && apcfg    == ""  ? sec : apcfg;
			ap2cfg    = secmode == "ap" && secdev == wifiDevA && ap2cfg   == ""  ? sec : ap2cfg;
			othercfg  = secmode != "ap" && secmode != "wds"   && othercfg == ""  ? sec : othercfg
			otherdev  = secmode != "ap" && secmode != "wds"   && otherdev == ""  ? secdev : otherdev
		}
	}
	var apgcfg = apcfg;
	var apacfg = ap2cfg;


	var apgngcfg = apgncfg ;
	var apgnacfg = apgn2cfg ;

	apgncfg = apgncfg != "" || (apgncfg == "" && apgn2cfg == "") ? apgncfg : apgn2cfg;

	var apcfgBand = apcfg != "" || ap2cfg != "" ? "G" : ""
	if(apgcfg == "" && apacfg != "")
	{
		apcfg = ap2cfg
		ap2cfg = ""
		apcfgBand = "A"
	}

	//wireless N + AC variables
	if( GwifiN || AwifiN || AwifiAC )
	{
		var hwGmode = uciOriginal.get("wireless", wifiDevG, "hwmode");
		hwGmode = hwGmode == "" ? "11g" : hwGmode;
		setAllowableSelections( "wifi_hwmode", [ 'disabled', '11gn', '11g' ], [UI.Disabled, 'B+G+N', 'B+G' ] );
		setSelectedValue("wifi_hwmode", hwGmode);
		if(dualBandWireless)
		{
			if(AwifiAC)
			{
				setAllowableSelections( "wifi_hwmode_5ghz", [ 'disabled',  '11anac', '11an', '11a' ], [UI.Disabled,  'A+N+AC', 'A+N', 'A' ] );
			}
			else if(AwifiN)
			{
				setAllowableSelections( "wifi_hwmode_5ghz", [ 'disabled', '11an', '11a' ], [UI.Disabled, 'A+N', 'A' ] );
			}
			else
			{
				setAllowableSelections( "wifi_hwmode_5ghz", [ 'disabled', '11a' ], [UI.Disabled, 'A' ] );
			}
			hwAmode = uciOriginal.get("wireless", wifiDevA, "hwmode");
			setSelectedValue("wifi_hwmode_5ghz", hwAmode);
		}

		dualMode = ((hwGmode != "disabled") && (hwAmode != "disabled"))
		if(hwGmode == "disabled")
		{
			hwdualMode=hwAmode;
			if(dualMode == false)
			{
				setAllowableSelections( "wifi_hwmode", [ 'disabled' ], [ UI.Disabled ] );
			}
		}
		else
		{
			hwdualMode=hwGmode;
			if(dualMode == false)
			{
				setAllowableSelections( "wifi_hwmode_5ghz", [ 'disabled' ], [ UI.Disabled ] );
			}
		}
		setSelectedValue("bridge_hwmode", (dualMode == true ? hwGmode : hwdualMode));
		if(dualBandWireless && otherdev != "" )
		{
			setSelectedValue("wifi_client_band", (otherdev == wifiDevA ? "5" : "2.4") )
		}

		var htGMode = uciOriginal.get("wireless", wifiDevG, "htmode");
		var htAMode = uciOriginal.get("wireless", wifiDevA, "htmode");

		if((htGMode != "NONE") && (htGMode != "") && (otherdev == "" || otherdev == wifiDevG))
		{
			setSelectedValue("wifi_hwmode", "11gn");
			setSelectedValue("bridge_hwmode", "11gn");
		}
		else if(((htGMode == "") || (htGMode == "NONE")) && (otherdev == "" || otherdev == wifiDevG))
		{
			setSelectedValue("wifi_hwmode", "11g");
			setSelectedValue("bridge_hwmode", "11g");
		}
		else if((htGMode != "") && (htGMode != "NONE") && (otherdev != wifiDevG))
		{
			setSelectedValue("wifi_hwmode", "11gn");
		}
		else if((htGMode == "") || (htGMode == "NONE") && (otherdev != wifiDevG))
		{
			setSelectedValue("wifi_hwmode", "11g");
		}

		if((htAMode != "NONE") && (htAMode != "") && (otherdev == "" || otherdev == wifiDevA))
		{
			setSelectedValue("wifi_hwmode_5ghz", "11an");
			setSelectedValue("bridge_hwmode", "11an");
			if(/VHT/i.test(htAMode))
			{
				setSelectedValue("wifi_hwmode_5ghz", "11anac");
				setSelectedValue("bridge_hwmode", "11anac");
			}
		}
		else if(((htAMode == "") || (htAMode == "NONE")) && (otherdev == "" || otherdev == wifiDevA))
		{
			setSelectedValue("wifi_hwmode_5ghz", "11a");
			setSelectedValue("bridge_hwmode", "11a");
		}
		else if((htAMode != "") && (htAMode != "NONE") && (otherdev != wifiDevA))
		{
			setSelectedValue("wifi_hwmode_5ghz", "11an");
			if(/VHT/i.test(htAMode))
			{
				setSelectedValue("wifi_hwmode_5ghz", "11anac");
			}
		}
		else if((htAMode == "") || (htAMode == "NONE") && (otherdev != wifiDevA))
		{
			setSelectedValue("wifi_hwmode_5ghz", "11a");
		}

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
		hwGmode = uciOriginal.get("wireless", wifiDevG, "hwmode");
		hwAmode = uciOriginal.get("wireless", wifiDevA, "hwmode");
		setSelectedValue("wifi_hwmode", hwGmode);
		setSelectedValue("wifi_hwmode_5ghz", hwAmode);
	}



	var wirelessIds=['wifi_channel1', 'wifi_channel2', 'wifi_channel1_5ghz', 'wifi_channel1_seg2_5ghz', 'wifi_channel2_5ghz', 'wifi_ssid1', 'wifi_ssid1a', 'wifi_encryption1', 'wifi_pass1', 'wifi_wep1',      'wifi_guest_ssid1', 'wifi_guest_mac_g', 'wifi_guest_ssid1a', 'wifi_guest_mac_a', 'wifi_guest_encryption1', 'wifi_guest_pass1', 'wifi_guest_wep1',        'wifi_server1', 'wifi_port1', 'wifi_ssid2', 'wifi_encryption2', 'wifi_pass2', 'wifi_wep2'];
	var wirelessPkgs= new Array();
	var wIndex;
	for(wIndex=0; wIndex < wirelessIds.length; wIndex++)
	{
		wirelessPkgs.push('wireless');
	}
	var default5ID = uciOriginal.get("wireless", apcfg, "ssid") ? uciOriginal.get("wireless", apcfg, "ssid") + " 5GHz" : "Gargoyle 5GHz";
	var defaultGuest5ID = apgncfg && uciOriginal.get("wireless", apgncfg, "ssid") ? uciOriginal.get("wireless", apgncfg, "ssid") + " 5GHz" : "Guests 5GHz";



	var wirelessSections=[wifiDevG, wifiDevG, wifiDevA, wifiDevA, wifiDevA, apgcfg, apacfg, apcfg, apcfg, apcfg,                                                                apgngcfg, apgngcfg, apgnacfg, apgnacfg, apgncfg, apgncfg, apgncfg,               apcfg, apcfg, othercfg, othercfg, othercfg, othercfg];
	var wirelessOptions=['channel', 'channel', 'channel', 'channel2', 'channel', 'ssid', 'ssid', 'encryption', 'key', 'key',                                                    'ssid', 'macaddr', 'ssid', 'macaddr', 'encryption', 'key', 'key',                   'auth_server', 'auth_port', 'ssid', 'encryption', 'key','key'];
	var wirelessParams=["5", "5", "36","36","36", 'Gargoyle', default5ID, 'none','','',        'Guests', '', defaultGuest5ID, '', 'none', '', '',                                  '', '', 'ExistingWireless', 'none', '',''];
	var wirelessFunctions=[lsv, lsv, lsv,lsv, lsv, lv, lv, lsv, lv, lv,                                                                                                 lv, lv, lv, lv, lsv, lv, lv,                                                        lv, lv, lv, lsv, lv, lv];



	loadVariables(uciOriginal, wirelessIds, wirelessPkgs, wirelessSections, wirelessOptions, wirelessParams, wirelessFunctions);






	var r0kh = uciOriginal.get("wireless", apcfg, "r0kh");
	var r1kh = uciOriginal.get("wireless", apcfg, "r1kh");
	r0kh = r0kh == null || r0kh == "" ? [] : r0kh;
	r1kh = r1kh == null || r1kh == "" ? [] : r1kh;
	if(r0kh.length == 1 && r1kh.length == 1)
	{
		r0kh = r0kh[0].split(",");
		r1kh = r1kh[0].split(",");
		if(r0kh.length == 3 && r1kh.length == 3 && r0kh[1] == "*" && r0kh[2] == r1kh[2])
		{
			document.getElementById("wifi_ft_key").value = r0kh[2];
		}
	}
	setSelectedValue('wifi_ft', uciOriginal.get("wireless", apcfg, "ieee80211r")==1 ? "enabled" : "disabled")
	setSelectedValue('wifi_hidden', uciOriginal.get("wireless", apcfg, "hidden")==1 ? "disabled" : "enabled")
	setSelectedValue('wifi_isolate', uciOriginal.get("wireless", apcfg, "isolate")==1 ? "enabled" : "disabled")

	var gmvalues = [ 'disabled' ]
	var gmnames  = [ UI.Disabled ]
	if((hwGmode != "disabled") && (hwAmode != "disabled"))
	{
		gmvalues.push('dual');
		gmnames.push(UI.Enabled);

		gmvalues.push('24ghz');
		gmnames.push(basicS.F24GHzOnly);

		gmvalues.push('5ghz');
		gmnames.push(basicS.F5GHzOnly);

	}
	else if(hwGmode == "disabled")
	{
		gmvalues.push('5ghz');
		gmnames.push(UI.Enabled);
	}
	else
	{
		gmvalues.push('24ghz');
		gmnames.push(UI.Enabled);
	}
	setAllowableSelections( "wifi_guest_mode", gmvalues, gmnames )


	if(apgncfg != "")
	{
		if(dualMode == true && apgngcfg == "")
		{
			setSelectedValue('wifi_guest_mode', '5ghz')
		}
		else if(dualMode == true && apgnacfg == "")
		{
			setSelectedValue('wifi_guest_mode', '24ghz')
		}
		else
		{
			setSelectedText('wifi_guest_mode', UI.Enabled);
		}
	}
	else
	{
		setSelectedValue('wifi_guest_mode', 'disabled');
	}

	setSelectedValue('wifi_guest_ft', uciOriginal.get("wireless", apgncfg, "ieee80211r")==1 ? "enabled" : "disabled")
	setSelectedValue('wifi_guest_hidden', uciOriginal.get("wireless", apgncfg, "hidden")==1 ? "disabled" : "enabled")
	setSelectedValue('wifi_guest_isolate', !apgncfg || uciOriginal.get("wireless", apgncfg, "isolate")==1 ? "enabled" : "disabled")

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
		MAC80211 has definitions in interface sections, broadcom in wifi-device section.
		To keep consistency we apply first MAC80211 mac filter defined (if any) to all sections
		Granted, this means you can not use the enhanced MAC80211 functionality of specifying mac
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
		for(sectionIndex=0; sectionIndex < allWirelessSections.length; sectionIndex++)
		{
			if(wirelessDriver == "broadcom")
			{
				if(uciOriginal.get("wireless", allWirelessSections[sectionIndex], "mode") == "wds")
				{
					wifiWdsData.push( [ uciOriginal.get("wireless", allWirelessSections[sectionIndex], "bssid").toUpperCase()  ] );
					setSelectedValue("wifi_mode", "ap+wds");
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


	// should they be enabled?
	var originalGmode = uciOriginal.get("wireless","ap_g");
	var originalAmode = uciOriginal.get("wireless","ap_a");
	if(originalGmode == "")
	{
		//hwGmode=originalGmode;
		setSelectedValue("wifi_hwmode", "disabled");
	}
	if(originalAmode == "")
	{
		//hwAmode=originalAmode;
		setSelectedValue("wifi_hwmode_5ghz", "disabled");
	}

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
			if(mobile_isp == apns[apnIndex][0])
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
		if(selectElement.id.match("seg2"))
		{
			setSelectedValue("wifi_channel1_seg2_5ghz",  selectedValue);
			setSelectedValue("bridge_channel_seg2_5ghz",  selectedValue);
		}
		else
		{
			setSelectedValue("wifi_channel1_5ghz",  selectedValue);
			setSelectedValue("wifi_channel2_5ghz",  selectedValue);
			setSelectedValue("bridge_channel_5ghz", selectedValue);
		}
		
		updateTxPower("wifi_max_txpower_5ghz","wifi_txpower_5ghz", "A")
		dfsChan = dfsChanTest();
		document.getElementById("wifi_channel1_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
		document.getElementById("wifi_channel2_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
		document.getElementById("bridge_channel_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
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

function validatePass(text, encryption)
{
	if(getSelectedValue(encryption).match(/psk/))
	{
		return validateLengthRange(text, 8, 63) == 0 || (text.length == 64 && validateHex(text) == 0) ? 0 : 1;
	}
	else
	{
		return validateLengthRange(text, 8, 999);
	}
}

function proofreadPass(input, encryption)
{
	input = typeof input === 'string' ? document.getElementById(input) : input;
	encryption = typeof encryption === 'string' ? encryption : encryption.id;
	proofreadText(input, function(text){return validatePass(text, encryption);}, 0);
}

function validateFtKey(text)
{
	return (text.length == 64 && validateHex(text) == 0) ? 0 : 1;
}

function proofreadFtKey(input)
{
	proofreadText(input, validateFtKey, 0);
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
	addTextToSingleColumnTable(textId, "lan_dns_table_container", isIPv4(addIp) ? validateIP : validateIP6, function(str){ return str; }, 0, false, "IP");
	if(addIp != "" && document.getElementById(textId).value == "")
	{
		document.getElementById(textId).value = addIp;
		addTextToSingleColumnTable("add_" + section + "_dns", "bridge_dns_table_container", isIPv4(addIp) ? validateIP : validateIP6, function(str){ return str; }, 0, false, "IP");
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
					if((GwifiN || AwifiN || AwifiAC) && dualBandWireless)
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

		document.getElementById("wifi_hwmode").disabled = 0;
		document.getElementById("wifi_hwmode_5ghz").disabled = 0;

		var ic = scannedIndex == "custom" ? 1 : 0;
		var inc = ic == 0 ? 1 : 0;
		if(inc)
		{
			scannedIndex = parseInt(scannedIndex)
			var enc  = scannedSsids[1][scannedIndex];
			var chan = scannedSsids[2][scannedIndex];
			var band = scannedSsids[4][scannedIndex];

			if(GwifiN)
			{
				var modes = ['11gn','11g'];
				var mnames = ['B+G+N','B+G'];
			}
			else
			{
				var modes = ['11g'];
				var mnames = ['B+G'];
			}
			if(band == "A")
			{
				modes.unshift('disabled');
				mnames.unshift(UI.Disabled);
				document.getElementById("wifi_hwmode").disabled = 1;
			}
			else
			{
				setAllowableSelections("bridge_hwmode", modes, mnames);
			}
			setAllowableSelections("wifi_hwmode", modes, mnames);
			document.getElementById("wifi_hwmode").selectedIndex = 0;
			if(dualBandWireless)
			{
				if(AwifiAC)
				{
					var modes = ['11anac','11an','11a'];
					var mnames = ['A+N+AC','A+N','A'];
				}
				else if(AwifiN)
				{
					var modes = ['11an','11a'];
					var mnames = ['A+N','A'];
				}
				else
				{
					var modes = ['11a'];
					var mnames = ['A'];
				}
				if(band == "G")
				{
					modes.unshift('disabled');
					mnames.unshift(UI.Disabled);
					document.getElementById("wifi_hwmode_5ghz").disabled = 1;
				}
				else
				{
					setAllowableSelections("bridge_hwmode", modes, mnames);
				}
				setAllowableSelections("wifi_hwmode_5ghz", modes, mnames);
				document.getElementById("wifi_hwmode_5ghz").selectedIndex = 0;
			}
			/*if((GwifiN || AwifiN || AwifiAC) && dualBandWireless && isAp )
			{
				modes.unshift("dual")
				mnames.unshift(basicS.DBand)
			}*/

			//var curBand = getSelectedValue("wifi_hwmode")
			//setAllowableSelections( "wifi_hwmode", modes, mnames );
			//setAllowableSelections( "bridge_hwmode", modes, mnames ); not sure about this

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
		var bp = be.match(/sae/) || be.match(/psk/) || be.match(/WPA/) ? 1 : 0;
		var bw = be.match(/wep/) || be.match(/WEP/) ? 1 : 0;
		var wp = we.match(/sae/) || we.match(/psk/) || we.match(/WPA/) ? 1 : 0;
		var ww = we.match(/wep/) || we.match(/WEP/) ? 1 : 0;
		setVisibility(visIds , [1,ic,0,ic,inc,ic,inc,  1,ic,0,ic,inc,ic,inc,  ic*isAp,inc*isAp,  wp,ww,bp,bw] );
	}
	else
	{
		var be = getSelectedValue('bridge_encryption');
		var we = getSelectedValue('wifi_encryption2');
		var bp = be.match(/sae/) || be.match(/psk/) || be.match(/WPA/) ? 1 : 0;
		var bw = be.match(/wep/) || be.match(/WEP/) ? 1 : 0;
		var wp = we.match(/sae/) || we.match(/psk/) || we.match(/WPA/) ? 1 : 0;
		var ww = we.match(/wep/) || we.match(/WEP/) ? 1 : 0;
		setVisibility(visIds, [0,0,1,1,0,1,0,          0,0,1,1,0,1,0,         isAp,0,    wp,ww,bp,bw] );
	}

	if(ic)
	{
		modes = [];
		mnames = [];
		for(x = 0; x < document.getElementById("wifi_hwmode").length; x++)
		{
			selection = document.getElementById("wifi_hwmode");
			modes[x] = selection.options[x].value;
			mnames[x] = selection.options[x].text;
		}
		if(modes[0] != "disabled")
		{
			modes.unshift("disabled");
			mnames.unshift(UI.Disabled);
		}
		if(!dualBandWireless)	//if G is theo nly option, don't let it be disabled
		{
			modes.shift();
			mnames.shift();
		}
		setAllowableSelections("wifi_hwmode", modes, mnames);
		document.getElementById("wifi_hwmode").selectedIndex = 1;

		if(dualBandWireless)
		{
			modes = [];
			mnames = [];
			for(x = 0; x < document.getElementById("wifi_hwmode_5ghz").length; x++)
			{
				selection = document.getElementById("wifi_hwmode_5ghz");
				modes[x] = selection.options[x].value;
				mnames[x] = selection.options[x].text;
			}
			if(modes[0] != "disabled")
			{
				modes.unshift("disabled");
				mnames.unshift(UI.Disabled);
			}
			setAllowableSelections("wifi_hwmode_5ghz", modes, mnames);
			document.getElementById("wifi_hwmode_5ghz").selectedIndex = 1;
		}
		var allowedbridgemodes = [];
		var allowedbridgemodes2 = [];
		if(dualBandWireless)
		{
			if(GwifiN)
			{
				allowedbridgemodes.push('B+G+N')
				allowedbridgemodes2.push('11gn');
			}
			allowedbridgemodes.push('B+G');
			allowedbridgemodes2.push('11g');
			if(AwifiAC)
			{
				allowedbridgemodes.push('A+N+AC')
				allowedbridgemodes2.push('11anac');
			}
			if(AwifiN)
			{
				allowedbridgemodes.push('A+N')
				allowedbridgemodes2.push('11an');
			}

			allowedbridgemodes.push('A');
			allowedbridgemodes2.push('11a');
		}
		else
		{
			if(GwifiN)
			{
				allowedbridgemodes.push('B+G+N')
				allowedbridgemodes2.push('11gn');
			}
			allowedbridgemodes.push('B+G');
			allowedbridgemodes2.push('11g');
		}
		setAllowableSelections("bridge_hwmode",allowedbridgemodes2,allowedbridgemodes)
	}
	setHwMode(document.getElementById("wifi_hwmode"))
	if(getSelectedValue("wifi_mode") == "ap+sta")
	{
		document.getElementById("wifi_hwmode").disabled = 0;
		document.getElementById("wifi_hwmode_5ghz").disabled = 0;
	}
}

function parseWifiScan(rawScanOutput)
{
	adjScanOutput = rawScanOutput.replace(/Quality/g, "\n          Quality");
	adjScanOutput = adjScanOutput.replace(/Channel/g, "\n          Channel");

	var parsed = [ [],[],[],[],[] ];
	var cells = adjScanOutput.split(/Cell/);
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
		var cellData  = cells.shift();
		var cellLines = cellData.split(/[\r\n]+/);

		var ssid    = getCellValues("ESSID", cellLines).shift();
		var channel = getCellValues("Channel", cellLines).shift();
		var qualStr = getCellValues("Quality", cellLines).shift();
		var encStr  = getCellValues("Encryption", cellLines).shift();



		if(ssid != null && channel != null && qualStr != null && encStr != null )
		{
			var enc = "psk2"
			if((encStr.match(/WPA3 SAE/) || encStr.match(/WPA2\/WPA3 PSK\/SAE/)) && wpad_sae)
			{
				enc = "sae"
			}
			else if(encStr.match(/WPA2 PSK/))
			{
				enc = "psk2"
			}
			else if(encStr.match(/WPA PSK/))
			{
				enc = "psk"
			}
			else if(encStr.match(/WEP/))
			{
				enc = "wep"
			}
			else if(encStr.match(/none/))
			{
				enc = "none"
			}

			var splitQual =qualStr.replace(/[\t ]+Sig.*$/g, "").split(/\//);
			var quality = Math.round( (parseInt(splitQual[0])*100)/parseInt(splitQual[1]) );
			quality = quality > 100 ? 100 : quality;


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
	if(wirelessDriver == "")
	{
		return;
	}
	var bridgematch = typeof(selectCtl) == "object" ? selectCtl.id : selectCtl;
	var chw =  getSelectedValue(selectCtl.id)
	var hplus = chw =='HT40+';
	var h40 = (chw == 'HT40+' || chw == 'HT40-');
	var vht = (chw == 'VHT20' || chw == 'VHT40' || chw == 'VHT80' || chw == 'VHT160' || chw == 'VHT80P80');
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
			var validAPlus  = [36, 44, 52, 60, 100, 108, 116, 124, 132, 140, 149, 157]
			var validAMinus = [40, 48, 56, 64, 104, 112, 120, 128, 136, 144, 153, 161]
			validAPlus = channelBondCheck(origAChan,band,validAPlus,2,1);
			validAMinus = channelBondCheck(origAChan,band,validAMinus,2,-1);
			var validTest  = hplus ? arrToHash(validAPlus) : arrToHash(validAMinus)
			for(var chanIndex=0; chanIndex < origAChan.length; chanIndex++)
			{
				var ch = origAChan[chanIndex]
				if(validTest[ch] == 1) { aChannels.push(ch); }
			}
		}
		else if(vht && (chw != 'VHT20'))
		{
			aChannels = []
			var valid40 = [36, 44, 52, 60, 100, 108, 116, 124, 132, 140, 149, 157]
			var valid80 = [36, 52, 100, 116, 132, 149]
			var valid160 = [36, 100]
			valid40 = channelBondCheck(origAChan,band,valid40,2,1);
			valid80 = channelBondCheck(origAChan,band,valid80,4,1);
			valid160 = channelBondCheck(origAChan,band,valid160,8,1);
			if(chw == 'VHT40')
			{
				var validTest  = arrToHash(valid40)
				for(var chanIndex=0; chanIndex < origAChan.length; chanIndex++)
				{
					var ch = origAChan[chanIndex]
					if(validTest[ch] == 1) { aChannels.push(ch); }
				}
			}
			else if(chw == 'VHT80' || chw == 'VHT80P80')
			{
				var validTest  = arrToHash(valid80)
				for(var chanIndex=0; chanIndex < origAChan.length; chanIndex++)
				{
					var ch = origAChan[chanIndex]
					if(validTest[ch] == 1) { aChannels.push(ch); }
				}
			}
			else if(chw == 'VHT160')
			{
				var validTest  = arrToHash(valid160)
				for(var chanIndex=0; chanIndex < origAChan.length; chanIndex++)
				{
					var ch = origAChan[chanIndex]
					if(validTest[ch] == 1) { aChannels.push(ch); }
				}
			}
		}
		setAllowableSelections("wifi_channel1_5ghz",  aChannels, aChannels, document);
		setAllowableSelections("wifi_channel1_seg2_5ghz",  aChannels, aChannels, document);
		setAllowableSelections("wifi_channel2_5ghz",  aChannels, aChannels, document);
		setAllowableSelections("bridge_channel_5ghz", aChannels, aChannels, document);
		setAllowableSelections("bridge_channel_seg2_5ghz", aChannels, aChannels, document);
		updateTxPower("wifi_max_txpower_5ghz","wifi_txpower_5ghz", "A")
		if(!bridgematch.match(/bridge/))
		{
			document.getElementById("wifi_channel1_seg2_5ghz_container").style.display = ((chw == 'VHT80P80') && !(bridgematch.match(/bridge/))) ? "block" : "none";
		}
		else
		{
			document.getElementById("bridge_channel_seg2_5ghz_container").style.display = ((chw == 'VHT80P80') && (getSelectedValue("bridge_repeater") == "disabled")) ? "block" : "none";
		}
		//fire the DFS check again
		dfsChan = dfsChanTest();
		document.getElementById("wifi_channel1_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
		document.getElementById("wifi_channel2_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
		document.getElementById("bridge_channel_5ghz_dfs").style.display = dfsChan == true ? "block" : "none";
	}
}

function channelBondCheck(channels, band, bonded, num_bond, dir_bond)
{
	retVal = [];
	gap = band == "A" ? 4 : 4;
	bonded.forEach(function(primary) {
		valid = false;
		if(channels.indexOf(primary) > -1)
		{
			valid = true;
			x = 1;
			while(x < num_bond)
			{
				if(channels.indexOf(primary+(x*gap*dir_bond)) == -1)
				{
					valid = false;
					break;
				}
				x++;
			}
		}
		if(valid)
		{
			retVal.push(primary);
		}
	});
	return retVal;
}

function getSelectedWifiChannels()
{
	var channels = []
	channels["A"] = ""
	channels["A2"] = ""
	channels["G"] = ""
	if(wirelessDriver == "")
	{
		return channels;
	}
	if(document.getElementById("global_gateway").checked)
	{
		var wimode = getSelectedValue("wifi_mode")
		var wifiGSelected = true;
		var wifiASelected = false;
		var dualBandSelected = false;
		if(document.getElementById("wifi_hwmode_container").style.display == "block")
		{
			wifiASelected = (getSelectedValue("wifi_hwmode_5ghz") == "disabled") ? "false" : "true";
			wifiGSelected = (getSelectedValue("wifi_hwmode") == "disabled") ? "false" : "true";
			dualBandSelected = ((wifiGSelected == true) && (wifiASelected == true));
		}
		var fixedChannels = scannedSsids[0].length > 0 && wimode.match(/sta/);
		var ssidIndex = getSelectedValue("wifi_list_ssid2")
		fixedChannels = fixedChannels && (ssidIndex != "custom")
		if(fixedChannels)
		{
			var fixedChannel = scannedSsids[2][ parseInt(ssidIndex) ].trim()
			var fixedBand    = scannedSsids[4][ parseInt(ssidIndex) ]
			channels[ fixedBand ] = fixedChannel
			if(dualBandSelected)
			{
				var otherBand = fixedBand == "G" ? "A" : "G"
				var otherSelectId = otherBand == "G" ? "wifi_channel1" : "wifi_channel1_5ghz"
				channels[ otherBand ] = getSelectedValue(otherSelectId).trim()
				if(otherBand == "A")
				{
					channels["A2"] = getSelectedValue("wifi_channel1_seg2_5ghz").trim()
				}
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
				channels[ "A" ] = getSelectedValue("wifi_channel1_5ghz");
				channels[ "A2" ] = getSelectedValue("wifi_channel1_seg2_5ghz");
			}
		}
	}
	else
	{
		var bridgeModeA = getSelectedValue("bridge_hwmode"); //true or false, are we using 5ghz for bridge?
		var bridgeModeA = !(String(bridgeModeA).match(/11g/))
		if(bridgeModeA)
		{
			if(document.getElementById("bridge_fixed_channel_container").style.display != "none")
			{
				channels["A"] = document.getElementById("bridge_fixed_channel").firstChild.data.trim();
			}
			else
			{
				channels["A"] = getSelectedValue("bridge_channel_5ghz").trim();
				channels["A2"] = getSelectedValue("bridge_channel_seg2_5ghz").trim();
			}
		}
		else
		{
			channels["G"] = document.getElementById("bridge_fixed_channel_container").style.display != "none" ?  document.getElementById("bridge_fixed_channel").firstChild.data.trim() : getSelectedValue("bridge_channel").trim();
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


	hwGmode=getSelectedValue("wifi_hwmode");
	hwAmode=getSelectedValue("wifi_hwmode_5ghz");

	//check that we aren't disabling both if we are in a STA config
	if(getSelectedValue("wifi_mode").match(/sta/))
	{
		if(hwGmode == "disabled" && hwAmode == "disabled")
		{
			if(dualBandWireless && selectCtl.id.match(/5ghz/g))
			{
				document.getElementById("wifi_hwmode").selectedIndex = 1;
				hwGmode = getSelectedValue("wifi_hwmode");
			}
			else if(dualBandWireless)
			{
				document.getElementById("wifi_hwmode_5ghz").selectedIndex = 1;
				hwAmode = getSelectedValue("wifi_hwmode_5ghz");
			}
			else	//dont let them disable their only option
			{
				document.getElementById("wifi_hwmode").selectedIndex = 1;
				hwGmode = getSelectedValue("wifi_hwmode");
			}
		}
		if(selectCtl.id.match(/bridge/))
		{
			//if we came from the bridge controller we must be resetting the data
			var allWirelessSections = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");

			// generic variables
			var seci=0;
			for(seci=0;seci < allWirelessSections.length; seci++)
			{
				var sec = allWirelessSections[seci];
				var secmode = uciOriginal.get("wireless", sec, "mode");
				var secdev  = uciOriginal.get("wireless", sec, "device")

				if(secmode == "sta")
				{
					HWMODE = uciOriginal.get("wireless", secdev, "hwmode");
					if(HWMODE == "11g")
					{
						hwGmode = uciOriginal.get("wireless", secdev, "htmode").match(/HT/) ? "11gn" : "11g";
						for(x = 0; x<document.getElementById("wifi_hwmode").length;x++)
						{
							if(hwGmode == document.getElementById("wifi_hwmode")[x].value)
							{
								document.getElementById("wifi_hwmode").selectedIndex = x;
							}
						}
					}
					else
					{
						hwAmode = uciOriginal.get("wireless", secdev, "htmode").match(/HT/) ? "11an" : "11a";
						hwAmode = uciOriginal.get("wireless", secdev, "htmode").match(/VHT/) ? "11anac" : hwAmode;
						for(x = 0; x<document.getElementById("wifi_hwmode_5ghz").length;x++)
						{
							if(hwAmode == document.getElementById("wifi_hwmode_5ghz")[x].value)
							{
								document.getElementById("wifi_hwmode_5ghz").selectedIndex = x;
							}
						}
					}
				}
			}
		}
	}

	//also check that only one is enabled if we are in STA (but not STA+AP)
	if(getSelectedValue("wifi_mode").match(/^sta$/))
	{
		if(hwGmode != "disabled" && hwAmode != "disabled")
		{
			if(dualBandWireless && selectCtl.id.match(/5ghz/g))
			{
				document.getElementById("wifi_hwmode").selectedIndex = 0;
				hwGmode = getSelectedValue("wifi_hwmode");
			}
			else
			{
				document.getElementById("wifi_hwmode_5ghz").selectedIndex = 0;
				hwAmode = getSelectedValue("wifi_hwmode_5ghz");
			}
		}
	}

	dualMode = ((hwGmode != "disabled") && (hwAmode != "disabled"))
	if(hwGmode == "disabled")
	{
		hwdualMode=hwAmode;
	}
	else
	{
		hwdualMode=hwGmode;
	}

	hwmode = dualMode == true && document.getElementById("global_bridge").checked ? hwGmode : hwdualMode;
	if(selectCtl.id == "bridge_hwmode" && dualMode == false)
	{
		setSelectedValue("wifi_hwmode", hwGmode);
	}
	bridgehwmode = getSelectedValue("bridge_hwmode");
	if(bridgehwmode.match(/11g/) && document.getElementById("global_bridge").checked == true)
	{
		hwGmode = bridgehwmode;
		hwAmode = "disabled";
	}
	else if(document.getElementById("global_bridge").checked == true)
	{
		hwGmode = "disabled";
		hwAmode = bridgehwmode;
	}





	document.getElementById("wifi_txpower_container").style.marginBottom    = "20px";
	document.getElementById("wifi_txpower_5ghz_container").style.marginBottom    = "20px";
	if(getSelectedValue("wifi_hwmode")=="disabled")
	{
		document.getElementById("wifi_hwmode_container").style.marginBottom    = "20px";
	}
	else
	{
		document.getElementById("wifi_hwmode_container").style.marginBottom    = "5px";
	}

	if(getSelectedValue("wifi_hwmode_5ghz")=="disabled")
	{
		document.getElementById("wifi_hwmode_5ghz_container").style.marginBottom    = "20px";
	}
	else
	{
		document.getElementById("wifi_hwmode_5ghz_container").style.marginBottom    = "5px";
	}

	//need to check if the allowable selections for channel width has changed
	if(hwGmode == "disabled")
	{
		setAllowableSelections('wifi_channel_width', ['NONE']);
	}
	else if(hwGmode == "11g")
	{
		setAllowableSelections('wifi_channel_width', ['NONE'], ['20MHz']);
	}
	else
	{
		setAllowableSelections('wifi_channel_width', ['HT20', 'HT40+', 'HT40-'], ['20MHz', '40MHz ' + basicS.ChAbv, '40MHz ' + basicS.ChBlw]);
	}

	if(hwAmode == "disabled")
	{
		setAllowableSelections('wifi_channel_width_5ghz', ['NONE']);
	}
	else if(hwAmode == "11a")
	{
		setAllowableSelections('wifi_channel_width_5ghz', ['NONE'], ['20MHz']);
	}
	else if(hwAmode == "11an")
	{
		setAllowableSelections('wifi_channel_width_5ghz', ['HT20', 'HT40+', 'HT40-'], ['20MHz', '40MHz ' + basicS.ChAbv, '40MHz ' + basicS.ChBlw]);
	}
	else
	{
		if(maxACwidth == "80")
		{
			setAllowableSelections('wifi_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80'], ['20MHz', '40MHz', '80MHz']);
		}
		else
		{
			if(AC80P80)
			{
				setAllowableSelections('wifi_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80', 'VHT160', 'VHT80P80'], ['20MHz', '40MHz', '80MHz', '160MHz', '80+80MHz']);
			}
			else
			{
				setAllowableSelections('wifi_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80', 'VHT160'], ['20MHz', '40MHz', '80MHz', '160MHz']);
			}
		}
	}

	//now check for bridge also.
	if(bridgehwmode == "11g")
	{
		setAllowableSelections('bridge_channel_width', ['NONE'], ['20MHz']);
	}
	else if(bridgehwmode == "11gn")
	{
		setAllowableSelections('bridge_channel_width', ['HT20', 'HT40+', 'HT40-'], ['20MHz', '40MHz ' + basicS.ChAbv, '40MHz ' + basicS.ChBlw]);
	}
	else if(bridgehwmode == "11a")
	{
		setAllowableSelections('bridge_channel_width_5ghz', ['NONE'], ['20MHz']);
	}
	else if(bridgehwmode == "11an")
	{
		setAllowableSelections('bridge_channel_width_5ghz', ['HT20', 'HT40+', 'HT40-'], ['20MHz', '40MHz ' + basicS.ChAbv, '40MHz ' + basicS.ChBlw]);
	}
	else
	{
		if(maxACwidth == "80")
		{
			setAllowableSelections('bridge_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80'], ['20MHz', '40MHz', '80MHz']);
		}
		else
		{
			if(AC80P80)
			{
				setAllowableSelections('bridge_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80', 'VHT160', 'VHT80P80'], ['20MHz', '40MHz', '80MHz', '160MHz', '80+80MHz']);
			}
			else
			{
				setAllowableSelections('bridge_channel_width_5ghz', ['VHT20', 'VHT40', 'VHT80', 'VHT160'], ['20MHz', '40MHz', '80MHz', '160MHz']);
			}
		}
	}


	setChildText("wifi_ssid1a_label", (dualMode == false ? basicS.AcPt+" SSID:" : "AP 5GHz SSID:"));
	setChildText("wifi_channel1_5ghz_label", (dualMode == false ? basicS.WChn+":" : basicS.WChn+" (5GHz):"));
	setChildText("wifi_channel1_seg2_5ghz_label", (dualMode == false ? basicS.WChn+" 2:" : basicS.WChn+" 2 (5GHz):"));

	setChildText("wifi_ssid1_label", (dualMode == true ?  "AP 2.4GHz SSID:" : basicS.AcPt+" SSID:"));
	setChildText("wifi_channel1_label", (dualMode == true ? basicS.WChn+" (2.4GHz):" : basicS.WChn+":"));


	if(wirelessDriver == "mac80211")
	{
		setChannel(document.getElementById("wifi_channel1"), "G")
		if(dualMode == true || hwAmode != "disabled")
		{
			setChannel(document.getElementById("wifi_channel1_5ghz"), "A")
			setChannel(document.getElementById("wifi_channel1_seg2_5ghz"), "A")
		}
	}
	var displayWidth = GwifiN;
	if(!displayWidth)
	{
		setSelectedValue("wifi_channel_width", "NONE");
		setChannelWidth(document.getElementById("wifi_channel_width"), "G")
	}

	displayWidth = (AwifiN || AwifiAC);
	if(!displayWidth)
	{
		setSelectedValue("wifi_channel_width_5ghz", "NONE");
		setChannelWidth(document.getElementById("wifi_channel_width_5ghz"), "A")
	}

	displayWidth = (GwifiN || AwifiN || AwifiAC);

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
				"wifi_channel1_seg1_5ghz_container",
				"wifi_channel2_5ghz_container",

				"bridge_channel_width_container",
				"bridge_channel_width_5ghz_container",
				"bridge_txpower_container",
				"bridge_txpower_5ghz_container",
				"bridge_channel_container",
				"bridge_channel_5ghz_container",
				"bridge_channel_seg1_5ghz_container",

				"wifi_guest_ssid1a_container",
				"wifi_guest_ssid1_container",




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

	if(selectCtl.id == "wifi_hwmode")
	{
		var gname = getSelectedText('wifi_guest_mode');
		var gval  = getSelectedValue('wifi_guest_mode');

		var mvalues = ['disabled' ]
		var mnames  = [ UI.Disabled ]

		if(dualMode == true)
		{
			mvalues.push('dual');
			mnames.push(UI.Enabled);

			mvalues.push('24ghz');
			mnames.push(basicS.F24GHzOnly);

			mvalues.push('5ghz');
			mnames.push(basicS.F5GHzOnly);

			gval = gname == UI.Enabled ? 'dual' : gval;

		}
		else if(hwAmode != "disabled")
		{
			mvalues.push('5ghz');
			mnames.push(UI.Enabled);
			gval = gval != 'disabled' ? '5ghz' : gval;

		}
		else
		{
			mvalues.push('24ghz');
			mnames.push(UI.Enabled);
			gval = gval != 'disabled' ? '24ghz' : gval;
		}
		setAllowableSelections( "wifi_guest_mode", mvalues, mnames);
		setSelectedValue('wifi_guest_mode', gval);


	}
	var gmode = getSelectedValue('wifi_guest_mode');
	setChildText("wifi_guest_ssid1a_label", (dualMode == true && gmode == "dual" ? basicS.GNet5ID : basicS.GNetID));
	document.getElementById("wifi_ssid1a").value = document.getElementById("wifi_ssid1a").value == "" ?  document.getElementById("wifi_ssid1").value + " 5GHz" :  document.getElementById("wifi_ssid1a").value;
	setChildText("wifi_guest_ssid1_label", (dualMode == true && gmode == "dual" ?  basicS.GNet24ID : basicS.GNetID));
	document.getElementById("wifi_guest_ssid1a").value = document.getElementById("wifi_guest_ssid1a").style.display == "block" && document.getElementById("wifi_guest_ssid1a").value == "" && document.getElementById("wifi_guest_ssid1").value != "" ?  document.getElementById("wifi_guest_ssid1").value + " 5GHz" :  document.getElementById("wifi_guest_ssid1a").value;
	
	var Awidth = getSelectedValue("wifi_channel_width_5ghz");


	for(ci=0 ; ci < containers.length; ci++)
	{
		var container = document.getElementById( containers[ci] );
		var cid = container.id
		var isA = cid.match("5ghz") || cid.match(/a_/)
		var notA = !isA;
		var cBand = isA ? "A" : "G";
		var ap_only = (cid.match(/1_/) || cid.match(/1a_/)) && !((cid == "wifi_channel1_seg1_5ghz_container") || (cid == "bridge_channel_seg1_5ghz_container"))
		var cli_only = cid.match(/2_/) || cid.match(/2a_/)
		var cli_ap_mismatch = (ap_only && !wimode.match(/ap/)) || (cli_only && !wimode.match(/sta/))
		var hide_in_favor_of_fixed_channel = fixedChannels && (cid.match(/channel/) && (!cid.match(/channel_width/))) && ((!wimode.match(/ap/)) || fixedChannelBand == cBand);
		var fixed_channel_exception = fixedChannels && (cid == "wifi_channel1_5ghz_container" || cid == "wifi_channel1_seg2_5ghz" || cid == "bridge_channel_5ghz_container" || cid == "bridge_channel_seg2_5ghz_container") && (Awidth == "VHT80P80");
		var displayWithoutWidth = cid ==  "wifi_ssid1_container" || cid == "wifi_ssid1a_container" || cid == "wifi_txpower_container" || cid == "wifi_txpower_5ghz_container" || cid == "wifi_channel1_container" || cid == "wifi_channel2_container" || cid == "bridge_txpower_container" || cid == 'bridge_channel_container' || cid == "wifi_guest_ssid1_container" || cid == "wifi_guest_ssid1a_container" || cid == "bridge_channel_5ghz_container" || cid == "bridge_channel_seg1_5ghz_container" || cid == "bridge_txpower_5ghz_container" || cid == "wifi_channel1_5ghz_container" || cid == "wifi_channel1_seg1_5ghz_container" || cid == "wifi_channel2_5ghz_container"
		var isitbridge = 1 && document.getElementById("global_bridge").checked; //are we looking at bridge?
		var bridgeModeA = getSelectedValue("bridge_hwmode"); //true or false, are we using 5ghz for bridge?
		var bridgeModeA = !(String(bridgeModeA).match(/11g/))
		var vis = 	(displayWidth || displayWithoutWidth) &&
				(!cli_ap_mismatch) &&
				(!((hide_in_favor_of_fixed_channel) && (!fixed_channel_exception))) &&
				((isA && (dualMode == true || hwAmode != "disabled")) || (notA && (hwGmode != "disabled"))) &&
				(!(isA && (isitbridge == true) && (bridgeModeA == false))) &&
				(!(notA && (isitbridge == true) && (bridgeModeA == true))) &&
				( (!cid.match(/^wifi_guest_/)) || gmode == "dual" || (gmode == "5ghz" && isA) || (gmode == "24ghz" && (!isA)) );

		container.style.display = vis ? "block" : "none";
	}
	document.getElementById("wifi_client_band_container").style.display = ((wimode == "ap+sta") && (dualMode == true)) ? "block" : "none";
	if(wimode == "ap+sta" && dualMode == true)
	{
		var cband = getSelectedValue("wifi_client_band")
		document.getElementById("wifi_client_band_container").style.display   = (!fixedChannels) ? "block" : "none"
		document.getElementById("wifi_channel2_container").style.display      = cband == "2.4" && (!fixedChannels) ? "block" : "none"
		document.getElementById("wifi_channel2_5ghz_container").style.display = cband == "5"   && (!fixedChannels) ? "block" : "none"
	}

	//at this point we should double check the allowed channels
	//we should also store any existing channel selection and attempt to reapply it (if still valid)
	channelG=getSelectedValue('wifi_channel1');
	channelA=getSelectedValue('wifi_channel1_5ghz');
	channelA2=getSelectedValue('wifi_channel1_seg2_5ghz');
	setChannelWidth(document.getElementById("wifi_channel_width"),"G");
	setChannelWidth(document.getElementById("wifi_channel_width_5ghz"),"A");
	setChannelWidth(document.getElementById("bridge_channel_width"),"G");
	setChannelWidth(document.getElementById("bridge_channel_width_5ghz"),"A");
	setSelectedValue('wifi_channel1',channelG);
	setSelectedValue('wifi_channel1_5ghz',channelA);
	setSelectedValue('wifi_channel1_seg2_5ghz',channelA2);

	//im thinking if they both get disabled, we should just set wireless mode to disabled also, and reset the others so we dont end up in a loop
	if(hwGmode == "disabled" && hwAmode == "disabled")
	{
		setSelectedValue('wifi_mode',"disabled");
		setSelectedValue('wifi_hwmode',"11g");
		setSelectedValue('wifi_hwmode_5ghz',"11a");

		//atempt to set to higher bandwidth values if they are available as these should be the defautls
		setSelectedValue('wifi_hwmode',"11gn");
		setSelectedValue('wifi_hwmode_5ghz',"11an");
		setSelectedValue('wifi_hwmode_5ghz',"11anac");

		setWifiVisibility();
	}
}

function getMaxTxPower(band)
{
	var chMaxPwr = txPowerMax;
	if(wirelessDriver == "mac80211")
	{
		var ch = getSelectedValue( band == "A" ? "wifi_channel1_5ghz" : "wifi_channel1");
		var ch2 = band == "A" ? getSelectedValue("wifi_channel1_seg2_5ghz"): null;
		var b = mac80211ChPwrs[band];
		var p = b == null ? null : b[ch];
		var p2 = ch2 == null ? p : (b == null ? null : b[ch2]);
		p = p == null ? p : Math.min(p,p2);
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
	commands.push(". /lib/functions/network.sh");
	commands.push("while ! network_get_ipaddr wan_ip wan && [ $wait_sec -gt 0 ] ; do");
	commands.push("sleep 1");
	commands.push("wait_sec=$(($wait_sec - 1))");
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
	//Leave the WAN interface semi-configured to bring back up
	//Then just spin for 2 seconds & update page data.
	var commands = [];
	commands.push("killall -SIGUSR2 udhcpc");

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

// if there are server/rebind defs other than alt defs leave them alone
// this is necessary for handling .onion domains if the tor plugin is installed
function getAltServerDefs(currentAltDefs, currentRebindServers, altDefsEnabled)
{

	var defs = []
	var domains = []

	var definedTlds = {}
	var setTlds = {}
	var tldLists = [ncTlds, onTlds]
	var tli
	var cadi
	var crsi
	for(tli=0; tli < tldLists.length; tli++)
	{
		var tlds = tldLists[tli]
		var ti
		for(ti=0; ti< tlds.length; ti++)
		{
			definedTlds[ tlds[ti] ] = 1
		}
	}
	for(cadi=0; cadi < currentAltDefs.length; cadi++)
	{
		var def = currentAltDefs[cadi]
		var defTld = def.replace(/^\//, "").replace(/\/.*$/, "")
		if( definedTlds[defTld] == null )
		{
			defs.push(def)
		}
	}
	for(crsi=0; crsi < currentRebindServers.length; crsi++)
	{
		var defTld = currentRebindServers[crsi]
		if( definedTlds[defTld] == null )
		{
			domains.push(defTld)
		}
	}

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
				if(setTlds[t] == null)
				{
					domains.push(t)
					setTlds[t] = t
				}
			}
		}
	}
	if(altDefsEnabled)
	{
		addDefsForAlt(ncTlds, ncDns[0]);
		addDefsForAlt(onTlds, onDns[0]);
	}
	return [ defs, domains ];
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
	document.getElementById("wan_3g_apn_container").style.display = (getSelectedValue("wan_3g_service") != "cdma" && getSelectedValue("wan_protocol") == "3g") || getSelectedValue("wan_protocol") == "qmi" || getSelectedValue("wan_protocol") == "ncm" || getSelectedValue("wan_protocol") == "mbim" ? "block" : "none";
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

function togglePass(name)
{
	password_field = document.getElementById(name);
	if(password_field.type == 'password')
	{
		password_field.type = 'text';
	}
	else
	{
		password_field.type = 'password';
	}
}


function randHexDigit()
{
	return (Math.floor(Math.random()*16)).toString(16);
}

function getRandomMac(local,unicast)
{
	/*
	 * Universally Administered Addresses (UAA) are defined by the manufacturer. Locally Administered Addresses (LAA) are defined by the network administrator.
	 * In the case of a VAP (like the guest network) we should be using LAA.
	 * This is controlled by the second least significant bit in the first octet: 0 = UAA, 1 = LAA.
	 */
	/*
	 * Unicast and Multicast addresses are controlled by the least significant bit in the first octet: 0 = Unicast, 1 = Multicast
	 * In all cases, we should be using Unicast, and hostapd won't start the interface if we try to use multicast
	 */
	
	var macPairs = []
	while(macPairs.length < 6 )
	{
		if(macPairs.length == 0)
		{
			var secondDigit = randHexDigit();
			secondDigit = setNthBitToX(("0x"+secondDigit), 1, local ? 1 : 0);
			secondDigit = setNthBitToX(("0x"+secondDigit), 0, unicast ? 0 : 1);
			macPairs.push( randHexDigit() + secondDigit )
		}
		else
		{
			macPairs.push( randHexDigit() + randHexDigit() )
		}
	}
	return macPairs.join(":");
}

function getRandomMacWithMask(local,unicast,ref,mask)
{
	/*
	 * Some devices use firmware level filtering for packets entering the interface, and won't work unless all MAC addresses on a particular interface
	 * are within the same mask space.
	 */
	ref = ref.toLowerCase();
	mask = mask.toLowerCase();
	var retVal = ref;
	
	while(retVal == ref)
	{
		var macPairs = [];
		var randMac = getRandomMac(local,unicast);
		macPairs = randMac.split(":");
		
		if(ref == "00:00:00:00:00:00" || ref == "" || ref == null)
		{
			return randMac;
		}
		if(mask == "00:00:00:00:00:00" || mask == "" || mask == null)
		{
			mask = "fc:ff:ff:ff:ff:f0";
		}
		if(mask == "ff:ff:ff:ff:ff:ff")
		{
			return ref;
		}
		 
		//Now force the mask bits onto the random MAC we generated
		var refPairs = [];
		var maskPairs = [];
		refPairs = ref.split(":");
		maskPairs = mask.split(":");
		 
		for(x = 0; x < 6; x++)
		{
			var macPair = "0x"+macPairs[x];
			var refPair = "0x"+refPairs[x];
			var maskPair = "0x"+maskPairs[x];
			
			for(y = 0; y < 8; y++)
			{
				if((maskPair & (1 << y)) != 0)
				{
					macPair = "0x"+setNthBitToX(macPair, y, (refPair >> y) & 1);
				}
			}
			macPairs[x] = ("0" + parseInt(macPair).toString(16)).slice(-2);
		}
		retVal = macPairs.join(":");
	}

	return retVal;
}

function setNthBitToX(val, n, x)
{
	val = val & ~(1 << n) | (x << n);
	return val.toString(16);
}

function parseCountry(countryLines)
{
	countryName = [];

	for(lineIndex = 0; lineIndex < countryLines.length; lineIndex++)
	{
		line = countryLines[lineIndex];
		if(!line.match(/^[\t]*#/) && line.length > 0)
		{
			splitLine = line.split(/[\t]+/);
			name = stripQuotes(splitLine.pop());
			code = stripQuotes(splitLine.pop());

			countryName[code] = name;
		}
	}

	return countryName;
}
function checkWifiCountryVisibility()
{
	if(typeof geo_countrycode === "undefined" || geo_countrycode === null)
	{
		return false;
	}
	else if(uciOriginal.get("wireless",uciWirelessDevs[0], "country") == "")
	{
		var selOpt = countryName[geo_countrycode];
		if(selOpt == "" || selOpt === undefined)
		{
			return false;
		}
		else if(document.getElementById("wireless_country").length == 1)
		{
			removeAllOptionsFromSelectElement(document.getElementById("wireless_country"));
			removeAllOptionsFromSelectElement(document.getElementById("bridge_wireless_country"));
			addOptionToSelectElement("wireless_country", countryName["00"], "00");
			addOptionToSelectElement("bridge_wireless_country", countryName["00"], "00");
			addOptionToSelectElement("wireless_country", selOpt, geo_countrycode);
			addOptionToSelectElement("bridge_wireless_country", selOpt, geo_countrycode);
		}
	}
	if(document.getElementById("wireless_country").length == 1)
	{
		return false;
	}

	return true;
}

function syncWifiCountrySelection(elSelected)
{
	var selIdx = elSelected.selectedIndex;
	if(elSelected.id == "bridge_wireless_country")
	{
		document.getElementById("wireless_country").selectedIndex = selIdx;
	}
	else
	{
		document.getElementById("bridge_wireless_country").selectedIndex = selIdx;
	}
}

function testChannels()
{
	var error = null;
	//VHT80P80 only thing we need to check for now
	if(getSelectedValue("wifi_channel_width_5ghz") == "VHT80P80")
	{
		var seg1 = getSelectedValue("wifi_channel1_5ghz");
		var seg2 = getSelectedValue("wifi_channel1_seg2_5ghz");
		
		if(seg1 == seg2)
		{
			error = basicS.SameChanSeg;
		}
		else if(Math.abs(seg1-seg2) == 16)
		{
			error = basicS.ContigChanSeg;
		}
	}
	return error;
}

function dfsChanTest()
{
	var ch1 = 36;
	var ch2 = 36;
	
	ch1 = getSelectedValue("wifi_channel1_5ghz");
	ch2 = getSelectedValue("wifi_channel_width_5ghz") == "VHT80P80" ? getSelectedValue("wifi_channel1_seg2_5ghz") : ch2;
	
	return ((ch1 >= 52 && ch1 <= 144 ? true : false) || (ch2 >= 52 && ch2 <= 144 ? true : false));
}

function calculateMask6(mask)
{
	retVal = "";
	addr = document.getElementById("lan_ip6").value;
	if(validateIP6(addr) == 0)
	{
		retVal = ip6_mask(addr, mask);
	}
	
	document.getElementById("lan_mask6").innerHTML = "<em>" + retVal + "</em>";
}

function addIp6(section)
{
	var textId = "add_" + section + "_ip6";
	addTextToSingleColumnTable(textId, "lan_ip6_table_container", validateIP6ForceRange, function(str){ return ip6_canonical(str); }, 0, false, "IP");
}

function proofreadDnsIp(input)
{
	isIPv4(input.value) ? proofreadIp(input) : proofreadIp6(input);
}
