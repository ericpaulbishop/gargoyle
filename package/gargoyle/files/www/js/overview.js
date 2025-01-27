/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var ovwS=new Object(); //part of i18n

function secondsToString(seconds)
{
	var numDays = Math.floor(seconds / 86400);
	var numHours = Math.floor((seconds % 86400) / 3600);
	var numMinutes = Math.floor(((seconds % 86400) % 3600) / 60);

	return numDays + " "+UI.days+", " + numHours + " "+UI.hours+", " + numMinutes + " "+UI.minutes;
}

function resetData()
{
	var uptimeSeconds = uptime.split(/[\t ]+/)[0];

	var percentRamUsed = Math.floor((totalMemory-freeMemory)*100*10/(totalMemory))/10;
	var ramMemory=Math.floor((totalMemory)*10/1024)/10;
	var ramUsed=Math.floor((totalMemory-freeMemory)*10/1024)/10;

	var percentSwapUsed = Math.floor((totalSwap-freeSwap)*100*10/(totalSwap))/10;
	var swapMemory=Math.floor((totalSwap)*10/1024)/10;
	var swapUsed=Math.floor((totalSwap-freeSwap)*10/1024)/10;

	wirelessModes= [];
	wirelessModes["ap"] = ovwS.AcPt+" (AP)"
	wirelessModes["sta"] = ovwS.Clnt
	wirelessModes["ap+sta"] = "AP+"+ovwS.Clnt
	wirelessModes["ap+wds"] = "AP+WDS"
	wirelessModes["adhoc"]  = "Ad Hoc";
	wirelessModes["disabled"] = UI.Disabled;
	var wirelessModeId = getWirelessMode(uciOriginal);
	var wirelessMode = wirelessModes[ wirelessModeId ];

	qosUploadStatus = qosEnabled && uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") != "" ? UI.Enabled : UI.Disabled;
	qosDownloadStatus = qosEnabled && uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") != "" ? UI.Enabled : UI.Disabled;

	var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	setChildText("device_model", model);
	setChildText("device_name", uciOriginal.get("system", systemSections[0], "hostname" ));
	setChildText("gargoyle_version", gargoyleVersion);
	setChildText("memory", "" + ramUsed + UI.MB +" / " + ramMemory + UI.MB +" (" + percentRamUsed + "%)" );
	if(swapMemory > 0)
	{
		document.getElementById("swap_container").style.display = "block";
		setChildText("swap", "" + swapUsed + UI.MB + " / " + swapMemory + UI.MB + " (" + percentSwapUsed + "%)" );
	}
	else
	{
		document.getElementById("swap_container").style.display = "none";
	}

	setChildText("load_avg", loadAvg);

	if(temps[0] == 1)
	{
		document.getElementById("temp_container").style.display = "block";
		setChildText("temp_cpu", temps[1]);
		setChildText("temp_mem", temps[2]);
		setChildText("temp_wifi", temps[3]);
	}
	else
	{
		document.getElementById("temp_container").style.display = "none";
	}

	setChildText("connections", curConn + "/" + maxConn);

	setChildText("uptime", secondsToString(uptimeSeconds));
	setChildText("current_time", cnv_LocaleTime(currentTime));

	var bridgeSection = getBridgeSection(uciOriginal);
	setChildText("device_config", bridgeSection == "" ? ovwS.Gtwy : ovwS.WBrgR);
	if(bridgeSection == "")
	{
		document.getElementById("bridge_container").style.display = "none";

		var childTxt = currentLanIp;
		for(var x = 0; x < currentLanIp6.length; x++)
		{
			if(ip6_scope(currentLanIp6[x])[0] == "Global")
			{
				childTxt = childTxt + "\n" + currentLanIp6[x];
			}
		}
		setChildText("lan_ip", childTxt);
		childTxt = currentLanMask;
		for(var x = 0; x < currentLanIp6.length; x++)
		{
			if(ip6_scope(currentLanIp6[x])[0] == "Global")
			{
				childTxt = childTxt + "\n" + currentLanIp6[x] + "/" + currentLanMask6[x];
			}
		}
		setChildText("lan_mask", childTxt);
		setChildText("lan_mac", currentLanMac);

		if(uciOriginal.get("network", "wan", "") == "")
		{
			document.getElementById("wan_container").style.display = "none";
		}
		childTxt = (wan_port == "?" ? "-" : wan_port);
		setChildText("wan_port_speed", childTxt);
		childTxt = (currentWanIp == "" ? "-" : currentWanIp);
		for(var x = 0; x < currentWanIp6.length; x++)
		{
			if(ip6_scope(currentWanIp6[x])[0] == "Global")
			{
				childTxt = childTxt + "\n" + currentWanIp6[x];
			}
		}
		setChildText("wan_ip", childTxt);
		childTxt = (currentWanMask == "" ? "-" : currentWanMask);
		for(var x = 0; x < currentWanIp6.length; x++)
		{
			if(ip6_scope(currentWanIp6[x])[0] == "Global")
			{
				childTxt = childTxt + "\n" + currentWanIp6[x] + "/" + currentWanMask6[x];
			}
		}
		setChildText("wan_mask", childTxt);
		setChildText("wan_mac", currentWanMac==""?"-":currentWanMac);
		childTxt = (currentWanGateway == "" ? "-" : currentWanGateway) + "\n" + (currentWanGateway6 == "" ? "-" : ip6_canonical(currentWanGateway6));
		setChildText("wan_gateway", childTxt);

		var wanDnsList = wanDns.split(/[\t ]+/);
		childTxt = "-";
		if(wanDnsList.length > 0)
		{
			childTxt = wanDnsList.shift();
		}
		while(wanDnsList.length > 0)
		{
			childTxt = childTxt + "\n" + wanDnsList.shift();
		}
		setChildText("wan_dns", childTxt);

		if(uciOriginal.get("network", "wan", "proto") != "pppoe")
		{
			document.getElementById("wan_pppoe_container").style.display = "none";
		}
		else
		{
			setChildText("wan_pppoe_uptime", (typeof pppoeUptime != "undefined") ? secondsToString(pppoeUptime) : ovwS.Discon);
		}

		if(uciOriginal.get("network", "wan", "proto") != "3g")
		{
			document.getElementById("wan_3g_container").style.display = "none";
		}

		if(wifi_status.toString().length == 0 && wirelessModeId != "disabled")
		{
			setChildText("wireless_mode", wirelessMode+" ("+UI.disabled+")");
		}
		else
		{
			setChildText("wireless_mode", wirelessMode);
		}

		if(wirelessModeId != "disabled")
		{
			var allWifiIfaceSections  = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");

			var apSsids = []
			var otherSsid = null
			var otherIsSta = false;
			var wsecIndex;
			for(wsecIndex=0; wsecIndex < allWifiIfaceSections.length ; wsecIndex++)
			{
				var sec = allWifiIfaceSections[wsecIndex]
				var secSsid = uciOriginal.get("wireless", sec, "ssid")
				var secMode = uciOriginal.get("wireless", sec, "mode")
				if(secMode == "ap")
				{
					var dev=uciOriginal.get("wireless", sec, "device");
					var devBand = "G";
					if(dev != "")
					{
						if(uciOriginal.get("wireless", dev, "band") == "5g")
						{
							devBand = "A";
						}
					}
					if (apSsids[devBand] == null)
					{
						apSsids[devBand] = secSsid
					}
				}
				else
				{
					otherIsSsid = secMode == "sta"
					otherSsid   = secSsid;
				}
			}

			// AP SSIDs
			if(apSsids["G"] == null && apSsids["A"] == null)
			{
				document.getElementById("wireless_apssid_div").style.display="none";
				document.getElementById("wireless_apssid_5ghz_div").style.display="none";
			}
			else if(apSsids["G"] != null && apSsids["A"] != null)
			{
				document.getElementById("wireless_apssid_div").style.display="block";
				document.getElementById("wireless_apssid_5ghz_div").style.display="block";
				setChildText("wireless_apssid_label", ovwS.T2p4GID+":");
				setChildText("wireless_apssid", apSsids["G"])
				setChildText("wireless_apssid_5ghz", apSsids["A"])
			}
			else
			{
				document.getElementById("wireless_apssid_div").style.display="block";
				document.getElementById("wireless_apssid_5ghz_div").style.display="none";
				setChildText("wireless_apssid_label", ovwS.APID+":");
				setChildText("wireless_apssid", (apSsids["G"] == null ? apSsids["A"] : apSsids["G"]))
			}

			// Wireless Client / Other SSID
			if(otherSsid == null)
			{
				document.getElementById("wireless_otherssid_div").style.display="none";
			}
			else
			{
				setChildText("wireless_otherssid", otherSsid);
				setChildText("wireless_otherssid_label", (otherIsSta ? "SSID:" : ovwS.IDJoin+":"))
				if(currentWirelessMacs.length > 0 && otherIsSta){ setChildText("wan_mac", currentWirelessMacs[0]); }
			}
			setChildText("wireless_mac", currentWirelessMacs.length > 0 ? currentWirelessMacs[0] : "-" );

		}
		else
		{
			document.getElementById("wireless_mac_div").style.display="none";
			document.getElementById("wireless_apssid_div").style.display="none";
			document.getElementById("wireless_apssid_5ghz_div").style.display="none";
			document.getElementById("wireless_otherssid_div").style.display="none";
		}
	}
	else
	{
		document.getElementById("wan_container").style.display = "none";
		//Show the LAN Port status, but nothing else, in bridge mode
		document.getElementById("lan_list_group").style.display = "none";
		document.getElementById("wifi_container").style.display = "none";

		setChildText("bridge_ip", currentLanIp);
		setChildText("bridge_mask", currentLanMask);
		setChildText("bridge_mac", currentLanMac );
		setChildText("bridge_gateway", uciOriginal.get("network", "lan", "gateway") );
		setChildText("bridge_mode", uciOriginal.get("wireless", bridgeSection, "client_bridge") == "1" ? ovwS.ClBr : "WDS");
		setChildText("bridge_ssid", uciOriginal.get("wireless", bridgeSection, "ssid") );
		setChildText("bridge_relay_ip", uciOriginal.get("network", "bridgecfg", "ipaddr"));
	}

	setChildText("qos_upload", qosUploadStatus);
	setChildText("qos_download", qosDownloadStatus);

	var portsColumns=[ovwS.Port, ovwS.Sts];
	var idx;
	var portsTableData = [];
	if (ports.length > 0)
	{
		for(idx=0; idx < ports.length; idx++)
		{
			portsTableData.push( [ ports[idx][0], ports[idx][1] ] );
		}
		var tableSortFun = function(a,b){ return a[0] == b[0] ? 0 : (a[0] < b[0] ? -1 : 1); }
		portsTableData.sort(tableSortFun);

		var portsTable=createTable(portsColumns, portsTableData, 'ports_table', false, false);
		var tableContainer = document.getElementById('ports_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(portsTable);
	}

	setChildText("wan_3g", csq);
}
