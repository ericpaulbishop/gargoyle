/*
 * This program is copyright © 2022 Michael Gray and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var advancedStr=new Object(); //part of i18n
var basicS=new Object(); //part of i18n

function resetData()
{
	// Wireless
	setWirelessCountryVisibility();
	if(geocode == '' || geocode == '(null)')
	{
		// async country detection, no point waiting for this
		detectCountry();
	}
	// LAN
	setUSteerCommsVisibility();
	// WAN
	setModemNetworkVisibility();
	// NetworkOpts
	setPktSteeringVisibility();

	setGlobalVisibility();
}

function saveChanges()
{
	var uci = uciOriginal.clone();
	var uciCompare = uciOriginal.clone();

	setControlsEnabled(false, true, UI.WaitSettings);

	var shouldRestartNetwork = false;
	var shouldRegenCachedVars = false;

	// Wireless
	var selWirelessCountry = getSelectedValue('wireless_country');
	if(selWirelessCountry != '00')
	{
		for(x = 0; x < uciWirelessDevs.length; x++)
		{
			uci.set("wireless",uciWirelessDevs[x],"country",selWirelessCountry);
		}
	}
	else
	{
		for(x = 0; x < uciWirelessDevs.length; x++)
		{
			uci.remove("wireless",uciWirelessDevs[x],"country");
		}
	}

	// LAN
	var usteerSec = uci.getAllSectionsOfType('usteer','usteer');
	if(usteerSec.length > 0)
	{
		var selUSteerComms = getSelectedValue('usteer_comms');
		uci.set('usteer', usteerSec[0], 'enabled', selUSteerComms);
	}

	// WAN
	if(byId('use_modem_network').checked)
	{
		var pkg = 'network';
		var sec = 'modem';
		uci.set(pkg,sec,'','interface');
		uci.set(pkg,sec,'ifname',currentWanIf);
		uci.set(pkg,sec,'proto','static');
		uci.set(pkg,sec,'ipaddr',byId('modem_network_ip').value);
		uci.set(pkg,sec,'netmask',byId('modem_network_mask').value);
	}
	else
	{
		uci.removeSection('network','modem');
	}

	// NetworkOpts
	if(num_cpus > 1)
	{
		var selPktSteerOpt = getSelectedValue('pktsteer_opt');
		uci.set('network', 'globals', 'packet_steering', selPktSteerOpt);
	}

	var restartNetworkCommand = "\nsh /usr/lib/gargoyle/restart_network.sh;\n";
	var regenerateCacheCommand = "\nrm -rf /tmp/cached_basic_vars ;\n/usr/lib/gargoyle/cache_basic_vars.sh >/dev/null 2>/dev/null\n";
	var commands = uci.getScriptCommands(uciCompare);
	var postcommands = "";

	if(commands.match(/wireless\.radio[0-9]+\.country/))
	{
		// Wireless country is changing
		shouldRestartNetwork = true;
		shouldRegenCachedVars = true;
	}
	if(commands.match(/usteer/))
	{
		// USteer is changing
		postcommands = postcommands + "/etc/init.d/usteer restart\n";
	}
	if(commands.match(/network\.modem/))
	{
		// Modem network is being created/destroyed
		shouldRestartNetwork = true;
	}
	if(commands.match(/packet_steering/))
	{
		// Packet steering option is being changed
		shouldRestartNetwork = true;
	}

	commands = commands + (shouldRestartNetwork ? restartNetworkCommand : '\n') + (shouldRegenCachedVars ? regenerateCacheCommand : '\n') + postcommands;

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			resetData();
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function arrayMoveIdx(arr, fromIdx, toIdx)
{
	var el = arr[fromIdx];
	arr.splice(fromIdx, 1);
	arr.splice(toIdx, 0, el);
}

function setGlobalVisibility()
{
	var anyVis = 0;
	anyVis += setNetOptContainerVisibility();
	anyVis += setWirelessContainerVisibility();
	anyVis += setLANContainerVisibility();
	anyVis += setWANContainerVisibility();
	byId('no_settings').style.display = anyVis > 0 ? 'none' : 'block';
}

function setWANContainerVisibility()
{
	var wanSec = uciOriginal.get('network','wan');
	var retVal = 0;
	var vis = 'none';
	if(wanSec == 'interface')
	{
		retVal = 1;
		vis = 'block';
	}
	byId('wan_container').style.display = vis;
	return retVal;
}

function setModemNetworkVisibility()
{
	var wandhcp = uciOriginal.get('network','wan','proto');
	if(wandhcp == 'dhcp')
	{
		loadChecked(['use_modem_network',uciOriginal,'network','modem','ipaddr',function(ip){return ip != ""}]);
		loadValueFromVariable(['modem_network_ip',uciOriginal,'network','modem','ipaddr','192.168.0.2']);
		loadValueFromVariable(['modem_network_mask',uciOriginal,'network','modem','netmask','255.255.255.0']);
		enableAssociatedField(byId('use_modem_network'), 'modem_network_ip', '192.168.0.2');
		enableAssociatedField(byId('use_modem_network'), 'modem_network_mask', '255.255.255.0');
	}
}

function setLANContainerVisibility()
{
	var usteerpresent = uciOriginal.getAllSectionsOfType('usteer','usteer');
	var retVal = 0;
	var vis = 'none';
	if(usteerpresent.length > 0)
	{
		retVal = 1;
		vis = 'block';
	}
	byId('lan_container').style.display = vis;
	return retVal;
}

function setUSteerCommsVisibility()
{
	var usteerpresent = uciOriginal.getAllSectionsOfType('usteer','usteer');
	var usteerEn = 0;
	var usteerSec = usteerpresent.length > 0 ? usteerpresent[0] : '';
	loadSelectedValueFromVariable(['usteer_comms',uciOriginal,'usteer',usteerSec,'enabled',1]);
}

function setWirelessContainerVisibility()
{
	var wifiifaces = uciOriginal.getAllSectionsOfType('wireless','wifi-iface');
	var retVal = 0;
	var vis = 'none';
	if(wifiifaces.length > 0)
	{
		retVal = 1;
		vis = 'block';
	}
	byId('wireless_container').style.display = vis;
	return retVal;
}

function setWirelessCountryVisibility()
{
	var currentSel = getSelectedValue('wireless_country');
	var countryData = parseCountry(countryLines);
	// If we have detected a country, put this at the top (under the default)
	if(geocode != '')
	{
		var foundIdx = countryData[1].indexOf(geocode);
		if(foundIdx > -1)
		{
			arrayMoveIdx(countryData[1], foundIdx, 1);
			arrayMoveIdx(countryData[0], foundIdx, 1);
		}
	}
	setAllowableSelections('wireless_country', countryData[1], countryData[0]);
	// Set it to the selected value in wireless config, otherwise Default 00 World
	selCountry = uciOriginal.get('wireless','radio0','country');
	selCountry = selCountry == "" ? uciOriginal.get('wireless','radio1','country') : selCountry;
	selCountry = selCountry == "" ? "00" : selCountry;
	// If we already had a selection, set it again
	selCountry = currentSel == "" ? selCountry : currentSel;
	setSelectedValue('wireless_country',selCountry);
}

function setNetOptContainerVisibility()
{
	var retVal = 0;
	var vis = 'none';

	if(num_cpus > 1)
	{
		retVal = 1;
		vis = 'block';
	}
	byId('netopt_container').style.display = vis;
	return retVal;
}

function setPktSteeringVisibility()
{
	loadSelectedValueFromVariable(['pktsteer_opt',uciOriginal,'network','globals','packet_steering','0']);
}

function parseCountry(countryLines)
{
	countryData = [[],[]];

	for(lineIndex = 0; lineIndex < countryLines.length; lineIndex++)
	{
		line = countryLines[lineIndex];
		if(!line.match(/^[\t]*#/) && line.length > 0)
		{
			splitLine = line.split(/[\t]+/);
			name = stripQuotes(splitLine.pop());
			code = stripQuotes(splitLine.pop());

			countryData[0].push(name);
			countryData[1].push(code);
		}
	}

	return countryData;
}

function parseCountryDetection(detected)
{
	var lines = detected.split('\n');
	lines.forEach(function(line) {
		if(line.match(/ip: /))
		{
			geoip = line.replace(/ip: /,'');
		}
		else if(line.match(/country_code: /))
		{
			geocode = line.replace(/country_code: /,'');
		}
	});
}

function detectCountry()
{
	commands = "gipquery -g 2>/dev/null > /tmp/cached_detected_country;cat /tmp/cached_detected_country;";

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			parseCountryDetection(req.responseText.replace(/Success/,''));
			setWirelessCountryVisibility();
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
