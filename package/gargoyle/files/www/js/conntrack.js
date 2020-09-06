/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var connTS=new Object(); //part of i18n

var TSort_Data = new Array('connection_table', 's', 's', 's', 'm', 's', 's');
var TFilter_Data = [['connection_table', [true, true, true, false, true, true], [], []]];

var updateInProgress;
var timeSinceUpdate;

var httpsPort ="";
var httpPort = "";
var remoteHttpsPort = "";
var remoteHttpPort = "";

var qosUpMask = "";
var qosDownMask = "";
var markToQosClass = [];

var ipToOVPNHostname = [];

function initializeConnectionTable()
{
	httpsPort = getHttpsPort()
	httpPort  = getHttpPort()

	setSelectedValue("host_display", "hostname");

	remoteHttpsPort = "";
	remoteHttpPort = "";
	var remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept");
	var acceptIndex=0;
	for(acceptIndex = 0; acceptIndex < remoteAcceptSections.length; acceptIndex++)
	{
		var section = remoteAcceptSections[acceptIndex];
		var localPort = uciOriginal.get("firewall", section, "local_port");
		var remotePort = uciOriginal.get("firewall", section, "remote_port");
		var proto = uciOriginal.get("firewall", section, "proto").toLowerCase();
		var zone = uciOriginal.get("firewall", section, "zone").toLowerCase();
		if((zone == "wan" || zone == "") && (proto == "tcp" || proto == ""))
		{
			remotePort = remotePort == "" ? localPort : remotePort;
			if(localPort == httpsPort && localPort != "")
			{
				remoteHttpsPort = remotePort;
			}
			else if(localPort == httpPort && localPort != "")
			{
				remoteHttpPort = remotePort;
			}
		}
	}

	var qmIndex=0;
	for(qmIndex=0; qmIndex < qosMarkList.length; qmIndex++)
	{
		var mask=  parseInt((qosMarkList[qmIndex][3]).toLowerCase());
		qosUpMask   = qosMarkList[qmIndex][0] == "upload"   ? mask: qosUpMask;
		qosDownMask = qosMarkList[qmIndex][0] == "download" ? mask : qosDownMask;
		markToQosClass[ parseInt(qosMarkList[qmIndex][2]) ] = qosMarkList[qmIndex][1];
	}

	//Get local VPN IPs
	var allowedClientSections = uciOriginal.getAllSectionsOfType("openvpn_gargoyle", "allowed_client");
	var clientIndex=0;
	for(clientIndex = 0; clientIndex < allowedClientSections.length; clientIndex++)
	{
		var section = allowedClientSections[clientIndex];
		var vpnIP = uciOriginal.get("openvpn_gargoyle", section, "ip");
		var hostName = uciOriginal.get("openvpn_gargoyle", section, "name");
		if(vpnIP != "" && hostName != "")
		{
			ipToOVPNHostname[vpnIP] = "(VPN) " + hostName;
		}
	}

	//Get remote VPN IPs (which could dynamically change, and we won't catch here)
	while(ovpnStatusFileLines.length > 0 && ovpnStatusFileLines[0] != "OpenVPN CLIENT LIST")
	{
		ovpnStatusFileLines.shift() ; 
	}
	for(i=0; i<3; i++)
	{
		ovpnStatusFileLines.shift()
	}
	while(ovpnStatusFileLines.length > 0 && ovpnStatusFileLines[0] != "ROUTING TABLE" &&  ovpnStatusFileLines[0] != "GLOBAL STATS" && ovpnStatusFileLines[0] != "END")
	{
		var lineParts = ovpnStatusFileLines.shift().split(/,/);
		var hostName = uciOriginal.get("openvpn_gargoyle", lineParts[0]) == "allowed_client" ? uciOriginal.get("openvpn_gargoyle", lineParts[0], "name") : lineParts[0];
		vpnIP = lineParts[1].replace(/:.*$/, "");
		if(vpnIP != "" && hostName != "")
		{
			ipToOVPNHostname[vpnIP] = "(VPN) " + hostName;
		}
	}

	updateInProgress = false;
	timeSinceUpdate = -5000;
	setInterval("checkForRefresh()", 500);
}


function checkForRefresh()
{
	timeSinceUpdate = timeSinceUpdate + 500;
	refreshRate = getSelectedValue("refresh_rate");
	refreshRate = refreshRate == "never" ? timeSinceUpdate+500 : refreshRate;
	if(timeSinceUpdate < 0 || timeSinceUpdate >= refreshRate)
	{
		timeSinceUpdate = 0;
		updateConnectionTable();
	}
}

function getHostDisplay(ip)
{
	var hostDisplay = getSelectedValue("host_display");
	var host = ip;
	if(hostDisplay == "hostname" && (ipToHostname[ip] != null || ipToOVPNHostname[ip] != null))
	{
		host = ipToHostname[ip] != null ? ipToHostname[ip] : ipToOVPNHostname[ip];
		host = host.length < 25 ? host : host.substr(0,22)+"...";
	}
	if(host == ip && isIPv6(host))
	{
		host = "[" + host + "]";
	}
	return host;
}

function updateConnectionTable()
{

	if(!updateInProgress)
	{
		updateInProgress = true;
		var commands="cat /proc/net/nf_conntrack"
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var bwUnits = getSelectedValue("bw_units");
				var conntrackLines = req.responseText.split(/[\n\r]+/);
				var tableData = new Array();
				var conntrackIndex;
				for(conntrackIndex=0; conntrackLines[conntrackIndex].match(/^Success/) == null ; conntrackIndex++)
				{
					var line = conntrackLines[conntrackIndex];
					try
					{
						var ipFamily= (line.split(/[\t ]+/))[0] == "ipv4" ? "IPv4" : "IPv6";
						var protocol= (line.split(/[\t ]+/))[2];
						var srcIp   = (line.match(/src=([^ \t]*)[\t ]+/))[1];
						var srcPort = (line.match(/sport=([^ \t]*)[\t ]+/))[1];
						var dstIp   = (line.match(/dst=([^ \t]*)[\t ]+/))[1];
						var dstPort = (line.match(/dport=([^ \t]*)[\t ]+/))[1];
						var bytes = (line.match(/bytes=([^ \t]*)[\t ]+/))[1];
						var connmark    = line.match(/mark=/) ? parseInt((line.match(/mark=([^ \t]*)[\t ]+/))[1]) : "";
						var l7proto = line.match(/l7proto=/) ? (line.match(/l7proto=([^ \t]*)[\t ]+/))[1] : "";
						var srcIp2   = (line.match(/src=([^ \t]*)[\t ]+.*src=([^ \t]*)[\t ]+/))[2];
						var srcPort2 = (line.match(/sport=([^ \t]*)[\t ]+.*sport=([^ \t]*)[\t ]+/))[2];
						var dstIp2   = (line.match(/dst=([^ \t]*)[\t ]+.*dst=([^ \t]*)[\t ]+/))[2];
						var dstPort2 = (line.match(/dport=([^ \t]*)[\t ]+.*dport=([^ \t]*)[\t ]+/))[2];
						var bytes2 = (line.match(/bytes=([^ \t]*)[\t ]+.*bytes=([^ \t]*)[\t ]+/))[2];

						if(ipFamily == "IPv6")
						{
							srcIp = ip6_canonical(srcIp);
							dstIp = ip6_canonical(dstIp);
							srcIp2 = ip6_canonical(srcIp2);
							dstIp2 = ip6_canonical(dstIp2);
						}

						var wan_connection = true;
						var ovpn_match = uciOriginal.get('openvpn_gargoyle','server','internal_ip');
						ovpn_match = ovpn_match == "" ? "10.8.0.1" : ovpn_match;
						ovpn_match = ovpn_match.split('.',3).join('.') + '.';

						//Connections are weird in that they list src/dest while we are interested in upload/download.
						//IPv4: Based on the location of the router WanIP in the connection record we can determine traffic direction
						if(ipFamily == "IPv4")
						{
							if (dstIp2 == currentWanIp) {
								downloadBytes = bytes2;
								uploadBytes = bytes;
								localIp = srcIp;
								localPort = srcPort;
								WanIp = srcIp2;
								WanPort = srcPort2;
							} else if (dstIp == currentWanIp) {
								downloadBytes = bytes;
								uploadBytes = bytes2;
								localIp = srcIp2;
								localPort = srcPort2;
								WanIp = dstIp2;
								WanPort = dstPort2;
							//Openvpn connections. These only appear if Openvpn is active
							} else if ((dstIp2.indexOf(ovpn_match) === 0) && (srcIp.indexOf(ovpn_match) !== 0)) {
								downloadBytes = bytes2;
								uploadBytes = bytes;
								localIp = srcIp;
								localPort = srcPort;
								WanIp = dstIp;
								WanPort = dstPort;
							} else {	// filter out LAN-LAN connections
								wan_connection = false;
							}
						}
						//IPv6: Based on the location of the local machine IP in the connection record we can determine traffic direction. We also need to grab router connections separately.
						else if(ipFamily == "IPv6")
						{
							if((checkIP6Local(srcIp) || currentWanIp6.indexOf(srcIp) > -1) && currentLanIp6.indexOf(srcIp) < 0 && currentLanIp6.indexOf(dstIp) < 0)
							{
								downloadBytes = bytes2;
								uploadBytes = bytes;
								localIp = srcIp;
								localPort = srcPort;
								WanIp = srcIp2;
								WanPort = srcPort2;
							} else if ((checkIP6Local(srcIp2) || currentWanIp6.indexOf(srcIp2) > -1) && currentLanIp6.indexOf(srcIp2) < 0 && currentLanIp6.indexOf(dstIp2) < 0) {
								downloadBytes = bytes;
								uploadBytes = bytes2;
								localIp = srcIp2;
								localPort = srcPort2;
								WanIp = dstIp2;
								WanPort = dstPort2;
							} else {
								wan_connection = false;
							}
						}

						if (wan_connection)
						{
							var tableRow =[	parseInt(uploadBytes) + parseInt(downloadBytes),
								ipFamily,
								protocol,
								textListToSpanElement([ getHostDisplay(localIp) + ":" + localPort, getHostDisplay(WanIp) + ":" + WanPort]),
								textListToSpanElement([parseBytes(uploadBytes, bwUnits),parseBytes(downloadBytes, bwUnits)])
								];
							if(qosEnabled)
							{
								var getQosName = function(mask, mark)
								{
									var section = mask == "" ? "" : markToQosClass[ (mask & mark) ];
									var name = uciOriginal.get("qos_gargoyle", section, "name");
									return name == "" ? "NA" : name;
								}
								tableRow.push( textListToSpanElement([getQosName(qosUpMask, connmark), getQosName(qosDownMask, connmark)]) );
							}
							tableRow.push(l7proto);
							tableData.push(tableRow);
						}
					}
					catch(e){}
				}

				//Sort on the total of up bytes + down bytes
				var tableSortFun = function(a,b){ return parseInt(b[0]) - parseInt(a[0]); }
				tableData.sort(tableSortFun);

				//remove integer totals we used to sort
				var rowIndex;
				for(rowIndex=0; rowIndex < tableData.length; rowIndex++)
				{
					(tableData[rowIndex]).shift();
				}


				var columnNames= [connTS.IPFam, connTS.PrNm, connTS.WLNm, connTS.UDNm ];
				if(qosEnabled) { columnNames.push(connTS.QSNm); };
				columnNames.push(connTS.LPNm);

				var connTable = createTable(columnNames, tableData, "connection_table", false, false);
				if(tableData.length > 0)
				{
					var headerIndex;
					try
					{
						for(headerIndex=0; headerIndex < 6; headerIndex++){ connTable.firstChild.firstChild.childNodes[headerIndex].style.textAlign="left"; }
					}
					catch(e){}
				}

				var tableContainer = document.getElementById('connection_table_container');
				if(tableContainer.firstChild != null)
				{
					storeTableFilter(TFilter_Data[getTFilterIDX('connection_table')][0]);
					tableContainer.removeChild(tableContainer.firstChild);
				}
				tableContainer.appendChild(connTable);
				reregisterTableSort('connection_table', 's', 's', 's', 'm', 's', 's');
				createTableFilter(TFilter_Data[getTFilterIDX('connection_table')]);

				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function checkIP6Local(address)
{
	retVal = false;

	for(var x = 0; x < currentLanIp6.length; x++)
	{
		var maskedLan = ip6_mask(currentLanIp6[x], currentLanMask6[x]);
		var maskedAddr = ip6_mask(address, currentLanMask6[x]);

		if(maskedLan == maskedAddr)
		{
			retVal = true;
			break;
		}
	}

	return retVal;
}
