/*
 * This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

function resetData()
{

	var uptimeSeconds = uptime.split(/[\t ]+/)[0];
	var uptimeDays = Math.floor(uptimeSeconds/(60*60*24));
	uptimeSeconds = uptimeSeconds - (uptimeDays*60*60*24);
	var uptimeHours = Math.floor(uptimeSeconds/(60*60));
	uptimeSeconds = uptimeSeconds - (uptimeHours*60*60);
	var uptimeMinutes = Math.floor(uptimeSeconds/60);

	var percentRamUsed = Math.floor((totalMemory-freeMemory)*100*10/(totalMemory))/10;
	var ramMemory=Math.floor((totalMemory)*10/1024)/10;
	var ramUsed=Math.floor((totalMemory-freeMemory)*10/1024)/10;

	var percentSwapUsed = Math.floor((totalSwap-freeSwap)*100*10/(totalSwap))/10;
	var swapMemory=Math.floor((totalSwap)*10/1024)/10;
	var swapUsed=Math.floor((totalSwap-freeSwap)*10/1024)/10;


	wirelessModes= [];
	wirelessModes["ap"] = "Access Point (AP)"
	wirelessModes["sta"] = "Client"
	wirelessModes["ap+sta"] = "AP+Client"
	wirelessModes["ap+wds"] = "AP+WDS"
	wirelessModes["adhoc"]  = "Ad Hoc";
	wirelessModes["disabled"] = "Disabled";
	var wirelessModeId = getWirelessMode(uciOriginal);
	var wirelessMode   = wirelessModes[ wirelessModeId ];
	
	qosUploadStatus   = qosEnabled && uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") != "" ? "Enabled" : "Disabled";
	qosDownloadStatus = qosEnabled && uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") != "" ? "Enabled" : "Disabled";


	


	var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	setChildText("device_name", uciOriginal.get("system", systemSections[0], "hostname" ));
	setChildText("gargoyle_version", gargoyleVersion);
	setChildText("memory", "" + ramUsed + "MB / " + ramMemory + "MB (" + percentRamUsed + "%)" );
	if(swapMemory > 0)
	{
		document.getElementById("swap_container").style.display = "block";
		setChildText("swap", "" + swapUsed + "MB / " + swapMemory + "MB (" + percentSwapUsed + "%)" );
	}
	else
	{
		document.getElementById("swap_container").style.display = "none";
	}
	setChildText("load_avg", loadAvg );
	setChildText("connections", curConn + "/" + maxConn );

	setChildText("uptime", uptimeDays + " days, " + uptimeHours + " hours, " + uptimeMinutes + " minutes");
	setChildText("current_time", currentTime);


	var bridgeSection = getBridgeSection(uciOriginal);
	setChildText("device_config", bridgeSection == "" ? "Gateway" : "Wireless Bridge/Repeater");
	if(bridgeSection == "")
	{
		document.getElementById("bridge_container").style.display = "none";

		setChildText("lan_ip", currentLanIp);
		setChildText("lan_mask", currentLanMask);
		setChildText("lan_mac", currentLanMac );

	
		if(uciOriginal.get("network", "wan", "") == "")
		{
			document.getElementById("wan_container").style.display = "none";
		}
		setChildText("wan_ip", currentWanIp);
		setChildText("wan_mask", currentWanMask);
		setChildText("wan_mac", currentWanMac );
		setChildText("wan_gateway", currentWanGateway );
		
		var wanDnsList = wanDns.split(/[\t ]+/);
		if(wanDnsList.length > 0)
		{
			setChildText("wan_dns", wanDnsList.shift() );
		}
		while(wanDnsList.length > 0)
		{
			var brk = document.createElement("br");
			var leftSpan = document.createElement("span");
			var rightSpan = document.createElement("span");
			leftSpan.className="leftcolumn";
			leftSpan.appendChild( document.createTextNode("invisible") );
			leftSpan.style.visibility="hidden";
			rightSpan.className="rightcolumn";	
			rightSpan.appendChild( document.createTextNode(wanDnsList.shift()) );
			
			document.getElementById("wan_dns_container").appendChild(brk);
			document.getElementById("wan_dns_container").appendChild(leftSpan);
			document.getElementById("wan_dns_container").appendChild(rightSpan);
		}	

		if(uciOriginal.get("network", "wan", "proto") != "3g")
		{
			document.getElementById("wan_3g_container").style.display = "none";
		}

		setChildText("wireless_mode", wirelessMode);
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
						if(uciOriginal.get("wireless", dev, "hwmode") == "11na")
						{
							devBand = "A";
						}
					}
					apSsids[devBand] = secSsid
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
				setChildText("wireless_apssid_label", "2.4 GHz Access Point SSID:");
				setChildText("wireless_apssid", apSsids["G"])
				setChildText("wireless_apssid_5ghz", apSsids["A"])
	
			}
			else
			{
				document.getElementById("wireless_apssid_div").style.display="block";
				document.getElementById("wireless_apssid_5ghz_div").style.display="none";
				setChildText("wireless_apssid_label", "Access Point SSID:");
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
				setChildText("wireless_otherssid_label", (otherIsSta ? "SSID:" : "SSID Joined by Client:"))
				if(currentWirelessMacs.length > 0 && otherIsSta){ setChildText("wan_mac", currentWirelessMacs[0]); }
			}
			setChildText("wireless_mac", currentWirelessMacs.length > 0 ? currentWirelessMacs[0] : "" );

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
		document.getElementById("lan_container").style.display = "none";
		document.getElementById("wifi_container").style.display = "none";
		
		setChildText("bridge_ip", currentLanIp);
		setChildText("bridge_mask", currentLanMask);
		setChildText("bridge_mac", currentLanMac );
		setChildText("bridge_gateway", uciOriginal.get("network", "lan", "gateway") );
		setChildText("bridge_mode", uciOriginal.get("wireless", bridgeSection, "client_bridge") == "1" ? "Client Bridge" : "WDS");
		setChildText("bridge_ssid", uciOriginal.get("wireless", bridgeSection, "ssid") );
	}

	setChildText("qos_upload", qosUploadStatus);
	setChildText("qos_download", qosDownloadStatus);
}


function getSsids()
{
		
}
