/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
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





	var systemSections = uciOriginal.getAllSections("system");
	setChildText("device_name", uciOriginal.get("system", systemSections[0], "hostname" ));
	setChildText("gargoyle_version", gargoyleVersion);
	setChildText("memory", "" + ramUsed + "MB / " + ramMemory + "MB (" + percentRamUsed + "%)" );

	setChildText("uptime", uptimeDays + " days, " + uptimeHours + " hours, " + uptimeMinutes + " minutes");
	setChildText("current_time", currentTime);


	setChildText("lan_ip", currentLanIp);
	setChildText("lan_mask", currentLanMask);
	setChildText("lan_mac", currentLanMac );
	
	setChildText("wan_ip", currentWanIp);
	setChildText("wan_mask", currentWanMask);
	setChildText("wan_mac", currentWanMac );

	setChildText("wireless_mode", wirelessMode);
	if(wirelessModeId != "disabled")
	{
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
		apssid=uciOriginal.get("wireless", apcfg, "ssid");
		otherssid=uciOriginal.get("wireless", othercfg, "ssid");


		setChildText("wireless_mac", currentWirelessMac);
		if(apssid != '')
		{
			setChildText("wireless_apssid", apssid);
		}
		else
		{
			document.getElementById("wireless_apssid_div").style.display="none";
		}
		if(othercfg != '')
		{
			if(cfg2mode != 'sta' && cfg3mode !='sta')
			{
				setChildText("wireless_otherssid_label", "SSID:");
			}
			else
			{
				setChildText("wireless_otherssid_label", "SSID Joined By Client:");
			}
			setChildText("wireless_otherssid", otherssid);
		}
		else
		{
			document.getElementById("wireless_otherssid_div").style.display="none";
		}
	}
	else
	{
		document.getElementById("wireless_mac_div").style.display="none";
		document.getElementById("wireless_apssid_div").style.display="none";
		document.getElementById("wireless_otherssid_div").style.display="none";
	}

	setChildText("qos_upload", qosUploadStatus);
	setChildText("qos_download", qosDownloadStatus);
}

