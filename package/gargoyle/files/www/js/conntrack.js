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


function initializeConnectionTable()
{
	httpsPort = uciOriginal.get("httpd_gargoyle", "server", "https_port");
	httpPort= uciOriginal.get("httpd_gargoyle", "server", "http_port");
	
	var remoteAccepts=firewallData[2];
	remoteHttpsPort = "";
	remoteHttpPort = "";
	for(acceptIndex = 0; acceptIndex < remoteAccepts.length; acceptIndex++)
	{
		localPort = remoteAccepts[acceptIndex][0];
		remotePort = remoteAccepts[acceptIndex][1];
		if(localPort == httpsPort)
		{
			remoteHttpsPort = remotePort;
		}
		else if(localPort == httpPort)
		{
			remoteHttpPort = remotePort;
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


function updateConnectionTable()
{
	if(!updateInProgress)
	{
		updateInProgress = true;
		commands="cat /proc/net/ip_conntrack"
		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				//alert(req.responseText);
				conntrackLines = req.responseText.split(/[\n\r]+/);
				tableData = new Array();
				for(conntrackIndex=0; conntrackLines[conntrackIndex].match(/^Success/) == null ; conntrackIndex++)
				{
					line = conntrackLines[conntrackIndex];
					try
					{
						protocol= (line.split(/[\t ]+/))[0];
						srcIp   = (line.match(/src=([^ \t]*)[\t ]+/))[1];
						srcPort = (line.match(/sport=([^ \t]*)[\t ]+/))[1];
						dstIp   = (line.match(/dst=([^ \t]*)[\t ]+/))[1];
						dstPort = (line.match(/dport=([^ \t]*)[\t ]+/))[1];
						uploadBytes = (line.match(/bytes=([^ \t]*)[\t ]+/))[1];
						downloadBytes = (line.match(/bytes=([^ \t]*)[\t ]+.*bytes=([^ \t]*)[\t ]+/))[2];
						//filter web connections to and from the router
						if(	(srcPort == httpPort || srcPort == httpsPort || srcPort==remoteHttpPort || srcPort == remoteHttpsPort) && (srcIp == currentLanIp || srcIp == currentWanIp) ||
							(dstPort == httpPort || dstPort == httpsPort || dstPort==remoteHttpPort || dstPort == remoteHttpsPort)  && (dstIp == currentLanIp || dstIp == currentWanIp)
						)
						{
							//filter out
						}
						else
						{
							tableData.push([protocol, srcIp + ":" + srcPort, dstIp + ":" + dstPort, parseBytes(uploadBytes), parseBytes(downloadBytes)]);
						}
					}
					catch(e){}
				}
				
				columnNames=['Protocol', 'Source', 'Destination', "Total Uploaded", "Total Downloaded"];
				connTable = createTable(columnNames, tableData, "connection_table", false, false);
				tableContainer = document.getElementById('connection_table_container');
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

function parseBytes(bytes)
{
	var parsed;
	if(bytes == "0" || bytes == "" || bytes == null )
	{
		parsed = "0 bytes";
	}
	else if(bytes > 1024*1024*1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024*1024)) + " TBytes";
	}
	else if(bytes > 1024*1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024)) + " GBytes";
	}
	else if(bytes > 1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024)) + " MBytes";
	}
	else if(bytes > 1024)
	{
		parsed = truncateDecimal(bytes/(1024)) + " KBytes";
	}
	else
	{
		parsed = bytes + " bytes"
	}
	return parsed;
}

function truncateDecimal(dec)
{
	result = "" + ((Math.floor(dec*1000))/1000);
	
	//make sure we have exactly three decimal places so 
	//results line up properly in table presentation
	decMatch=result.match(/.*\.(.*)$/);
	if(decMatch == null)
	{
		result = result + ".000"
	}
	else 
	{
		if(decMatch[1].length==1)
		{
			result = result + "00";
		}
		else if(decMatch[1].length==2)
		{
			result = result + "0";
		}
	}
	return result;
}
