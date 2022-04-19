/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var hostsStr=new Object(); //part of i18n
var basicS=new Object();

var TSort_Data = new Array ('lease_table', 's', 'p', 's', 's');
tsRegister();
TSort_Data = new Array ('wifi_table', 's', 'p', 's', 's', 's');
tsRegister();
TSort_Data = new Array ('active_table', 's', 'p', 's', 'i', 'i', 'i');
tsRegister();

var updateInProgress = false;
var timeSinceUpdate = -5000;

function resetData()
{
	setSelectedValue("refresh_rate", "10000");
	resetVariables();
	setInterval(checkForRefresh, 500);
}

function checkForRefresh()
{
	timeSinceUpdate = timeSinceUpdate + 500;

	var refreshRate = getSelectedValue("refresh_rate");
	var refreshRate = refreshRate == "never" ? timeSinceUpdate+500 : refreshRate;
	if(timeSinceUpdate < 0 || timeSinceUpdate >= refreshRate)
	{
		timeSinceUpdate = 0;
		reloadVariables();
	}
}


function reloadVariables()
{
	if(!updateInProgress)
	{
		updateInProgress = true;
		var param = getParameterDefinition("commands", "sh /usr/lib/gargoyle/define_host_vars.sh") + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var jsHostVars = req.responseText.replace(/Success/, "");
				eval(jsHostVars);
				resetVariables();
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}
function resetVariables()
{
	if(uciOriginal.get("dhcp", "lan", "ignore") != "1")
	{
		document.getElementById("dhcp_data").style.display="block";
		var columnNames=[UI.HsNm, hostsStr.HostIP, hostsStr.HostMAC, hostsStr.LeaseExp];
		var table = createTable(columnNames, parseDhcp(dhcpLeaseLines), "lease_table", false, false);
		var tableContainer = document.getElementById('lease_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
        	reregisterTableSort('lease_table', 's', 'p', 's', 's');

		document.getElementById("dhcp6_data").style.display="block";
		var columnNames=[UI.HsNm, hostsStr.HostIP, hostsStr.HostDUID, hostsStr.LeaseExp];
		var table = createTable(columnNames, parseDhcp6(dhcp6LeaseLines), "lease6_table", false, false);
		var tableContainer = document.getElementById('lease6_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
        	reregisterTableSort('lease6_table', 's', 'p', 's', 's');
	}
	else
	{
		document.getElementById("dhcp_data").style.display="none";
		document.getElementById("dhcp6_data").style.display="none";
	}

	var arpHash = parseArp(arpLines, dhcpLeaseLines);

	var apFound = false;
	var staFound = false;
	var wifiIfs = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");
	var ifIndex = 0;
	for(ifIndex = 0; ifIndex < wifiIfs.length; ifIndex++)
	{
		apFound = uciOriginal.get("wireless", wifiIfs[ifIndex], "mode") == "ap" ? true : apFound;
		staFound = uciOriginal.get("wireless", wifiIfs[ifIndex], "mode") == "sta" ? true : staFound;
	}
	var wifiDevs = uciOriginal.getAllSectionsOfType("wireless", "wifi-device");
	apFound = apFound && (uciOriginal.get("wireless", wifiDevs[0], "disabled") != "1");
	staFound = staFound && (uciOriginal.get("wireless", wifiDevs[0], "disabled") != "1");

	if(apFound)
	{
		document.getElementById("wifi_data").style.display="block";
		var columnNames=[UI.HsNm, hostsStr.HostIP, hostsStr.HostMAC, hostsStr.Band, "TX "+hostsStr.Bitrate, "RX "+hostsStr.Bitrate, hostsStr.Signal ];
		var table = createTable(columnNames, parseWifi(arpHash, wirelessDriver, wifiLines, "AP"), "wifi_table", false, false);
		var tableContainer = document.getElementById('wifi_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
        	reregisterTableSort('wifi_table', 's', 'p', 's', 's');
	}
	else
	{
		document.getElementById("wifi_data").style.display="none";
	}

	if(staFound)
	{
		document.getElementById("client_wifi_data").style.display="block";
		var columnNames=["AP SSID", "AP IP", "AP MAC", hostsStr.Band, "TX "+hostsStr.Bitrate, "RX "+hostsStr.Bitrate, hostsStr.Signal ];
		var table = createTable(columnNames, parseWifi(arpHash, wirelessDriver, wifiClientLines, "STA"), "client_wifi_table", false, false);
		var tableContainer = document.getElementById('client_wifi_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
        	reregisterTableSort('client_wifi_table', 's', 'p', 's', 's');
	}
	else
	{
		document.getElementById("client_wifi_data").style.display="none";
	}

	remoteAPs = [];
	if(usteerRemotes)
	{
		var usteerRemotesJSON = JSON.parse(usteerRemotes);
		Object.keys(usteerRemotesJSON).forEach(function(remotevar) {
			parseusteerRemote(remotevar,usteerRemotesJSON);
		});

		var usteerClientsJSON = JSON.parse(usteerClients);
		Object.keys(usteerClientsJSON).forEach(function(client) {
			parseusteerClient(client,usteerClientsJSON,arpHash);
		});

		var wifiremotedataContainer = byId('wifi_remote_data');
		while(wifiremotedataContainer.firstChild != null)
		{
			wifiremotedataContainer.removeChild(wifiremotedataContainer.firstChild);
		}
		Object.keys(remoteAPs).forEach(function(remoteAP) {
			var tablediv = document.createElement('div');
			tablediv.id = 'wifi_remote_' + remoteAP + '_data';
			tablediv.class = 'table-responsive';
			var columnNames=[UI.HsNm, hostsStr.HostIP, hostsStr.HostMAC, hostsStr.Band, hostsStr.Signal ];
			var tableformatdata = remoteAPs[remoteAP]['clients'].map(function(client) {
				return remoteAPClientsToTableFormat(client,remoteAPs[remoteAP]['band']);
			});
			var table = createTable(columnNames, tableformatdata, 'wifi_remote_' + remoteAP + '_data_table', false, false);
			tablediv.appendChild(table);
			var labelEl = document.createElement('label');
			labelEl.class = 'col-xs-12';
			labelEl.style = 'text-decoration:underline';
			labelEl.for = 'wifi_remote_' + remoteAP + '_data_table';
			labelEl.innerText = hostsStr.Remote + ' AP: ' + remoteAPs[remoteAP]['ssid'] + ' (' + remoteAPs[remoteAP]['ip'] + ')';
			wifiremotedataContainer.appendChild(labelEl);
			wifiremotedataContainer.appendChild(tablediv);
		});
	}

	var columnNames=[UI.HsNm, hostsStr.HostIP, hostsStr.HostMAC, hostsStr.ActiveConx, hostsStr.RecentConx, hostsStr.UDPConx];
	var table = createTable(columnNames, parseConntrack(arpHash, currentWanIp, conntrackLines), "active_table", false, false);
	var tableContainer = document.getElementById('active_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(table);
	reregisterTableSort('active_table', 's', 'p', 's', 'i', 'i', 'i');
}

function parseusteerRemote(remotevar,usteerRemotesJSON)
{
	var tmpRemoteAP = [];
	tmpRemoteAP['ip'] = remotevar.split("#")[0];
	tmpRemoteAP['ap'] = remotevar.split("#")[1];
	tmpRemoteAP['band'] = usteerRemotesJSON[remotevar]['freq'].toString().substring(0,1) == 2 ? '2.4GHz' : '5GHz';
	tmpRemoteAP['ssid'] = usteerRemotesJSON[remotevar]['rrm_nr'][1];
	tmpRemoteAP['clients'] = [];
	remoteAPs[remotevar] = tmpRemoteAP;
}

function parseusteerVisitedAP(visitedAP,visitedAPs,client,arpHash)
{
	if(visitedAP.match(/#/))
	{
		// # would indicate a remote and not local, so lets do it
		if(visitedAPs[visitedAP]['connected'])
		{
			// Only report clients that are connected, this isn't a hearing map
			var tmpClient = [];
			tmpClient['signal'] = visitedAPs[visitedAP]['signal'];
			tmpClient['ip'] = arpHash[client.toUpperCase()] == null ? UI.unk : arpHash[client.toUpperCase()];
			tmpClient['hostname'] = getHostname(tmpClient['ip']);
			tmpClient['mac'] = client.toUpperCase();
			remoteAPs[visitedAP]['clients'].push(tmpClient);
		}
	}
}

function parseusteerClient(client,usteerClientsJSON,arpHash)
{
	var visitedAPs = usteerClientsJSON[client];
	Object.keys(visitedAPs).forEach(function(visitedAP) {
		parseusteerVisitedAP(visitedAP,visitedAPs,client,arpHash);
	});
}

function signalToSpan(sig)
{
	// Some drivers pass through quality rather than signal level. Convert this back. Also make sure it fits within the standard max/min range
	sig = sig > 0 ? sig - 110 : sig;
	sig = sig > -40 ? -40 : sig;
	sig = sig < -110 ? -110 : sig;
	var toHexTwo = function(num) { var ret = parseInt(num).toString(16).toUpperCase(); ret= ret.length < 2 ? "0" + ret : ret.substr(0,2); return ret; }
	var color = sig < -80  ? "#AA0000" : "";
	color = sig >= -80 && sig < -70 ? "#AA" + toHexTwo(170*((sig+80)/10.0)) + "00" : color;
	color = sig >= -70 && sig < -60 ? "#" + toHexTwo(170-(170*(sig+70)/10.0)) + "AA00" : color;
	color = sig >= -60 ? "#00AA00" : color;
	var sigSpan = document.createElement("span");
	sigSpan.appendChild(document.createTextNode(sig + " dBm"));
	sigSpan.style.color = color;
	return sigSpan;
}

function remoteAPClientsToTableFormat(client,band)
{
	var sig = client['signal'];
	var sigSpan = signalToSpan(sig);
	return [client['hostname'],client['ip'],client['mac'],band,sigSpan];
}

function getHostname(ip)
{
	ip = validateIP6(ip) == 0 ? ip6_canonical(ip) : ip;
	var hostname = ipToHostname[ip] == null ? "("+UI.unk+")" : ipToHostname[ip];
	hostname = hostname.length < 25 ? hostname : hostname.substr(0,22)+"...";
	return hostname;
}

function parseDhcp(leases)
{
	//HostName, Host IP, Host MAC, Time Before Expiration
	var dhcpTableData = [];
	var lineIndex=0;
	for(lineIndex=0; lineIndex < leases.length; lineIndex++)
	{
		var leaseLine = leases[lineIndex];
		var splitLease = leaseLine.split(/[\t ]+/);
		var expTime = splitLease[0];
		var mac = splitLease[1].toUpperCase();
		var ip = splitLease[2];

		var hostname = getHostname(ip);

		var seconds = expTime - currentTime;
		var expHours = Math.floor(seconds/(60*60));
		var expMinutes = Math.floor((seconds-(expHours*60*60))/(60));
		if(expMinutes < 10)
		{
			expMinutes = "0" + expMinutes;
		}
		var exp = expHours + "h " + expMinutes + "m";

		dhcpTableData.push( [hostname, ip, mac, exp ] );
	}
	sort2dStrArr(dhcpTableData, 1);
	return dhcpTableData;
}

function parseDhcp6(leases)
{
	//# br-lan DUID ?? HostName Expiry Suffix NetMask Address1 [, Address2, Address3...]
	var dhcpTableData = [];
	var lineIndex=0;
	for(lineIndex=0; lineIndex < leases.length; lineIndex++)
	{
		var leaseLine = leases[lineIndex];
		var splitLease = leaseLine.split(/[\t ]+/);
		var ip = [];
		for(var x = 8; x < splitLease.length; x++)
		{
			ip.push(ip6_splitmask(splitLease[x]).address);
		}
		var ipTxt = ip.join("\n");
		var hostname = getHostname(ip[0]);
		var duid = splitLease[2];

		var expTime = splitLease[5];
		var exp = "";
		if(expTime == "-1")
		{
			exp = "N/A";
		}
		else
		{
			var seconds = expTime - currentTime;
			var expHours = Math.floor(seconds/(60*60));
			var expMinutes = Math.floor((seconds-(expHours*60*60))/(60));
			if(expMinutes < 10)
			{
				expMinutes = "0" + expMinutes;
			}
			exp = expHours + "h " + expMinutes + "m";
		}

		dhcpTableData.push( [hostname, ipTxt, duid, exp ] );
	}
	sort2dStrArr(dhcpTableData, 1);
	return dhcpTableData;
}

function parseArp(arpLines, leaseLines)
{
	var arpHash = [];

	var lineIndex = 0;
	for(lineIndex=0; lineIndex < arpLines.length; lineIndex++)
	{
		var nextLine = arpLines[lineIndex];
		var splitLine = nextLine.split(/[\t ]+/);
		if(splitLine.length >= 5)
		{
			var mac = splitLine[4].toUpperCase();
			var ip = splitLine[0];
			arpHash[ mac ] = ip;
			arpHash[ ip  ] = mac;
		}
	}


	for(lineIndex=0; lineIndex < leaseLines.length; lineIndex++)
	{
		var leaseLine = leaseLines[lineIndex];
		var splitLease = leaseLine.split(/[\t ]+/);
		var mac = splitLease[1].toUpperCase();
		var ip = splitLease[2];
		arpHash[ mac ] = ip;
		arpHash[ ip  ] = mac;
	}

	return arpHash;
}

function sort2dStrArr(arr, testIndex)
{
	var str2dSort = function(a,b){  return a[testIndex] == b[testIndex] ? 0 : (a[testIndex] < b[testIndex] ? -1 : 1);  }
	arr.sort(str2dSort);
}

function parseWifi(arpHash, wirelessDriver, lines, apsta)
{
	if(wirelessDriver == "" || lines.length == 0) { return []; }

	//Mapping of WLANs to ESSIDs and MACs
	var wifLines = wlanLines.slice(0);
	for(x = 0; x < wifLines.length; x++)
	{
		wifLines[x] = wifLines[x].split(/[\t ]+/);
		var allWifiIfaceSections = uciOriginal.getAllSectionsOfType("wireless","wifi-iface");
		for(wsecIndex = 0; wsecIndex < allWifiIfaceSections.length; wsecIndex++)
		{
			var sec = allWifiIfaceSections[wsecIndex];
			if((uciOriginal.get("wireless",sec,"is_guest_network") == "1") && (uciOriginal.get("wireless",sec,"macaddr").toLowerCase() == wifLines[x][2].toLowerCase()))
			{
				wifLines[x].push("guest");
				break;
			}
			else if(wsecIndex == allWifiIfaceSections.length-1)
			{
				wifLines[x].push("not guest");
			}
		}
	}

	//Host IP, Host MAC
	var wifiTableData = [];
	var lineIndex = 0;
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];
		var whost = nextLine.split(/[\t ]+/);

		//bcm=1, mac80211=2
		var macBitSig =	[
				[whost[1], "0", "0"],
				[whost[0], whost[2], whost[1], whost[3], whost[4], whost[5]]
				];
		var mbs = wirelessDriver == "broadcom" ? macBitSig[0] : macBitSig[1];
		mbs[0] = (mbs[0]).toUpperCase();
		mbs[1] = mbs[1] + " Mbps";

		var sig = parseInt(mbs[2]);
		var sigSpan = signalToSpan(sig);
		mbs[2] = sigSpan;

		if(apsta == "AP")
		{
			var ip = arpHash[ mbs[0] ] == null ? UI.unk : arpHash[ mbs[0] ] ;
			var hostname = getHostname(ip);
		}
		else if(apsta == "STA")
		{
			var ip = UI.unk;
			if(mbs.length > 3)
			{
				for(x = 0; x < wifLines.length; x++)
				{
					if(wifLines[x][0] == mbs[5])
					{
						var hostname = wifLines[x][1];
					}
				}
			}
			else
			{
				var hostname = getHostname(ip);
			}
		}
		if(mbs.length > 3)
		{
			mbs[3] = mbs[3] + " Mbps";
			for(x = 0; x < wifLines.length; x++)
			{
				if(mbs[5] == wifLines[x][0] && wifLines[x][3] == "guest")
				{
					mbs[4] = mbs[4] + " (" + basicS.GNet + ")";
				}
			}
			wifiTableData.push( [ hostname, ip, mbs[0], mbs[4], mbs[1], mbs[3], mbs[2] ] );
		}
		else
		{
			wifiTableData.push( [ hostname, ip, mbs[0], "-", mbs[1], "-", mbs[2] ] );
		}
	}
	sort2dStrArr(wifiTableData, 1);
	return wifiTableData;
}

function parseConntrack(arpHash, currentWanIp, lines)
{
	//Host IP, Host MAC, current TCP cnxns, recently closed TCP cnxns, UDP cnxns
	var activeTableData = [];
	var ipHash = [];
	var protoHash = [];
	var ipList = [];
	var lineIndex = 0;
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];

		var splitLine = nextLine.split(/src=/); //we want FIRST src definition
		var srcIpPart = splitLine[1];
		var splitSrcIp = srcIpPart.split(/[\t ]+/);
		var srcIp = splitSrcIp[0];
		if(isIPv6(srcIp))
		{
			srcIp = ip6_canonical(srcIp);
		}

		splitLine = nextLine.split(/dst=/); //we want FIRST dst definition
		var dstIpPart = splitLine[1];
		var splitDstIp = dstIpPart.split(/[\t ]+/);
		var dstIp = splitDstIp[0];
		if(isIPv6(dstIp))
		{
			dstIp = ip6_canonical(dstIp);
		}


		splitLine=nextLine.split(/[\t ]+/);
		var proto = splitLine[0].toLowerCase();
		if(proto == "tcp")
		{
			var state = splitLine[3].toUpperCase();
			var stateStr = state == "TIME_WAIT" || state == "CLOSE" ? "closed" : "open";
			proto = proto + "-" + stateStr;
		}
		protoHash[ srcIp + "-" + proto ] =  protoHash[ srcIp + "-" + proto ] == null ? 1 : protoHash[ srcIp + "-" + proto ] + 1;
		if(proto == "udp")
		{
			var num = protoHash[ srcIp + "-" + proto ];
		}

		//for some reason I'm seeing src ips of 0.0.0.0 -- WTF???
		//exclude anything starting at router, or ending at external wan ip, since this is probably a connection to router from outside
		if(ipHash[srcIp] == null && srcIp != currentWanIp && currentWanIp6.indexOf(ip6_canonical(srcIp)) < 0 && srcIp != currentLanIp && currentLanIp6.indexOf(ip6_canonical(srcIp)) < 0 && dstIp != currentWanIp && currentWanIp6.indexOf(ip6_canonical(dstIp)) < 0 && srcIp != "0.0.0.0" && srcIp.match(/^fe80:/) == null && dstIp.match(/^fe80:/) == null)
		{
			ipList.push(srcIp);
			ipHash[srcIp] = 1;
		}
	}


	var ipIndex = 0;
	for(ipIndex = 0; ipIndex < ipList.length; ipIndex++)
	{
		var ip        = ipList[ipIndex];
		var mac       = arpHash[ip] == null ? UI.unk : arpHash[ip];
		var tcpOpen   = protoHash[ ip + "-tcp-open" ] == null   ? 0 : protoHash[ ip + "-tcp-open" ];
		var tcpClosed = protoHash[ ip + "-tcp-closed" ] == null  ? 0 : protoHash[ ip + "-tcp-closed" ];
		var udp       = protoHash[ ip + "-udp" ] == null ? 0 : protoHash[ ip + "-udp" ];
		var hostname  = getHostname(ip);
		activeTableData.push( [ hostname, ip, mac, ""+tcpOpen, ""+tcpClosed, ""+udp ] );
	}
	sort2dStrArr(activeTableData, 1);
	return activeTableData;
}
