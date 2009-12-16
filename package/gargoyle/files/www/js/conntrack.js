/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var updateInProgress;
var timeSinceUpdate;

var httpsPort ="";
var httpPort = "";
var remoteHttpsPort = "";
var remoteHttpPort = "";

var qosUpMask = "";
var qosDownMask = "";
var markToQosClass = [];

function initializeConnectionTable()
{
	httpsPort = uciOriginal.get("httpd_gargoyle", "server", "https_port");
	httpPort= uciOriginal.get("httpd_gargoyle", "server", "http_port");


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


function updateConnectionTable()
{

	if(!updateInProgress)
	{
		updateInProgress = true;
		var commands="cat /proc/net/ip_conntrack"
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
						var protocol= (line.split(/[\t ]+/))[0];
						var srcIp   = (line.match(/src=([^ \t]*)[\t ]+/))[1];
						var srcPort = (line.match(/sport=([^ \t]*)[\t ]+/))[1];
						var dstIp   = (line.match(/dst=([^ \t]*)[\t ]+/))[1];
						var dstPort = (line.match(/dport=([^ \t]*)[\t ]+/))[1];
						var uploadBytes = (line.match(/bytes=([^ \t]*)[\t ]+/))[1];
						var downloadBytes = (line.match(/bytes=([^ \t]*)[\t ]+.*bytes=([^ \t]*)[\t ]+/))[2];
						var connmark    = line.match(/mark=/) ? parseInt((line.match(/mark=([^ \t]*)[\t ]+/))[1]) : "";
						var l7proto = line.match(/l7proto=/) ? (line.match(/l7proto=([^ \t]*)[\t ]+/))[1] : "";


						

						//filter web connections to and from the router
						if(	(srcPort == httpPort || srcPort == httpsPort || srcPort==remoteHttpPort || srcPort == remoteHttpsPort) && (srcIp == currentLanIp || srcIp == currentWanIp) ||
							(dstPort == httpPort || dstPort == httpsPort || dstPort==remoteHttpPort || dstPort == remoteHttpsPort)  && (dstIp == currentLanIp || dstIp == currentWanIp)
						)
						{
							//filter out
						}
						else
						{
							var tableRow =	[	parseInt(uploadBytes) + parseInt(downloadBytes),
										protocol, 
										textListToSpanElement([srcIp + ":" + srcPort, dstIp + ":" + dstPort]), 
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


				var columnNames= ['Proto', 'Src/Dest', 'Bytes Up/Down' ]; 
				if(qosEnabled) { columnNames.push("Qos Up/Down"); };
				columnNames.push("L7 Proto");
				
				var connTable = createTable(columnNames, tableData, "connection_table", false, false);
				if(tableData.length > 0)
				{
					var headerIndex;
					try
					{
						for(headerIndex=0; headerIndex < 5; headerIndex++){ connTable.firstChild.firstChild.childNodes[headerIndex].style.textAlign="left"; }
					}
					catch(e){}
				}
				
				var tableContainer = document.getElementById('connection_table_container');
				if(tableContainer.firstChild != null)
				{
					tableContainer.removeChild(tableContainer.firstChild);
				}
				tableContainer.appendChild(connTable);

				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


