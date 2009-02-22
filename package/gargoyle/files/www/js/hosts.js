/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function resetData()
{
	if(uciOriginal.get("dhcp", "lan", "ignore") != "1")
	{
		document.getElementById("dhcp_data").style.display="block";
		var columnNames=["Host IP", "Host MAC", "Time Before Lease Expiration"];
		var table = createTable(columnNames, parseDhcp(dhcpLeaseLines), "lease_table", false, false);
		var tableContainer = document.getElementById('lease_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
	}
	else
	{
		document.getElementById("dhcp_data").style.display="none";
	}

	var arpHash = parseArp(arpLines);
	
	var apFound = false;
	var wifiIfs = uciOriginal.getAllSectionsOfType("wireless", "wifi-iface");
	var ifIndex = 0;
	for(ifIndex = 0; ifIndex < wifiIfs.length; ifIndex++)
	{
		apFound = uciOriginal.get("wireless", wifiIfs[ifIndex], "mode") == "ap" ? true : apFound;
	}
	var wifiDevs = uciOriginal.getAllSectionsOfType("wireless", "wifi-device");
	apFound = apFound && (uciOriginal.get("wireless", wifiDevs[0], "disabled") != "1");
	
	if(apFound)
	{
		document.getElementById("wifi_data").style.display="block";
		var columnNames=["Host IP", "Host MAC"];
		var table = createTable(columnNames, parseWifi(arpHash, isBrcm, wifiLines), "wifi_table", false, false);
		var tableContainer = document.getElementById('wifi_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(table);
	}
	else
	{
		document.getElementById("wifi_data").style.display="none";
	}
	
	var columnNames=["Host IP", "Host MAC", "Active TCP Cxns", "Recent TCP Cxns", "UDP Cxns"];
	var table = createTable(columnNames, parseConntrack(arpHash, currentWanIp, conntrackLines), "active_table", false, false);
	var tableContainer = document.getElementById('active_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(table);

	
}

function parseDhcp(leases)
{
	//Host IP, Host MAC, Time Before Expiration	
	var dhcpTableData = [];
	var lineIndex=0;
	for(lineIndex=0; lineIndex < leases.length; lineIndex++)
	{
		var leaseLine = leases[lineIndex];
		var splitLease = leaseLine.split(/[\t ]+/);
		var expTime = splitLease[0];
		var mac = splitLease[1].toUpperCase();
		var ip = splitLease[2];

		var seconds = expTime - currentTime;
		var expHours = Math.floor(seconds/(60*60));
		var expMinutes = Math.floor((seconds-(expHours*60*60))/(60));
		if(expMinutes < 10)
		{
			expMinutes = "0" + expMinutes;
		}
		var exp = expHours + "h " + expMinutes + "m";

		dhcpTableData.push( [ip, mac, exp ] );
	}
	return dhcpTableData;
}

function parseArp(lines)
{
	var arpHash = [];
	lines.shift(); //skip header
	var lineIndex = 0;
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];
		var splitLine = nextLine.split(/[\t ]+/);
		var mac = splitLine[3].toUpperCase();
		var ip = splitLine[0];
		arpHash[ mac ] = ip;
		arpHash[ ip  ] = mac;
	}
	return arpHash;
}

function parseWifi(arpHash, isBrcm, lines)
{
	//Host IP, Host MAC
	var wifiTableData = [];
	var lineIndex = 0;
	if(!isBrcm)
	{
		lines.shift();
	}
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];
		var splitLine = nextLine.split(/[\t ]+/);
		var mac = isBrcm ? splitLine[1].toUpperCase() : splitLine[0].toUpperCase();
		var ip = arpHash[ mac ] == null ? "unknown" : arpHash[ mac ] ;
		wifiTableData.push( [ ip, mac ] );
	}
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
		
		var ipPart = splitLine[1];
		var splitIp = ipPart.split(/[\t ]+/);
		var ip = splitIp[0];
		
		splitLine=nextLine.split(/[\t ]+/);
		var proto = splitLine[0].toLowerCase();
		if(proto == "tcp")
		{
			var state = splitLine[3].toUpperCase() == "TIME_WAIT" ? "closed" : "open";
			proto = proto + "-" + state;
		}
		protoHash[ ip + "-" + proto ] =  protoHash[ ip + "-" + proto ] == null ? 1 : protoHash[ ip + "-" + proto ] + 1;
		if(proto == "udp")
		{
			var num = protoHash[ ip + "-" + proto ];
		}	
			
		//for some reason I'm seeing src ips of 0.0.0.0 -- WTF???
		if(ipHash[ip] == null && ip != currentWanIp && ip != currentLanIp && ip != "0.0.0.0")
		{
			ipList.push(ip);
			ipHash[ip] = 1;
		}
	}


	var ipIndex = 0;
	for(ipIndex = 0; ipIndex < ipList.length; ipIndex++)
	{
		var ip        = ipList[ipIndex];
		var mac       = arpHash[ip] == null ? "unknown" : arpHash[ip];
		var tcpOpen   = protoHash[ ip + "-tcp-open" ] == null   ? 0 : protoHash[ ip + "-tcp-open" ];
		var tcpClosed = protoHash[ ip + "-tcp-closed" ] == null  ? 0 : protoHash[ ip + "-tcp-closed" ];
		var udp       = protoHash[ ip + "-udp" ] == null ? 0 : protoHash[ ip + "-udp" ];
		activeTableData.push( [ ip, mac, ""+tcpOpen, ""+tcpClosed, ""+udp ] );
	
	}
	return activeTableData;
}


