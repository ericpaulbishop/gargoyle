var wgStr=new Object(); //part of i18n
var wgcHash = [];

function resetData()
{
	wgACs = uciOriginal.getAllSectionsOfType("wireguard_gargoyle","allowed_client");
	var wgACIdx = 0;
	for(wgACIdx = 0; wgACIdx < wgACs.length; wgACIdx ++)
	{
		var clientName = uciOriginal.get("wireguard_gargoyle",wgACs[wgACIdx],"name");
		var pubkey = uciOriginal.get("wireguard_gargoyle",wgACs[wgACIdx],"public_key");
		wgcHash[pubkey] = clientName;
	}

	updateTableFromData(statusFileLines)
	setInterval(doUpdate, 15*1000);
}

function updateTableFromData(statusData)
{
	var i;
	var clientData = [];

	while(statusData.length > 0 && statusData[0].match(/^peer:/) == null)
	{
		statusData.shift() ; 
	}
	while(statusData.length > 0)
	{
		var lineParts = statusData.shift().split(/: /);
		var clientName = wgcHash[lineParts[1]] !== undefined ? wgcHash[lineParts[1]] : lineParts[1];
		var ip = "-";
		var lastseen = "-";
		var rx = "-";
		var tx = "-";
		while(statusData.length > 0 && statusData[0].match(/^peer:/) == null)
		{
			lineParts = statusData.shift().split(/: /);
			if(lineParts[0].trim() == "endpoint")
			{
				ip = lineParts[1];
			}
			else if(lineParts[0].trim() == "latest handshake")
			{
				lastseen = lineParts[1];
			}
			else if(lineParts[0].trim() == "transfer")
			{
				subLineParts = lineParts[1].split(/, /);
				rx = subLineParts[0];
				tx = subLineParts[1];
			}
		}
		clientData.push( [ clientName, ip.replace(/:.*$/, ""), rx.replace(/ received/,""), tx.replace(/ sent/,""), lastseen] )
	}

	var clientTable = createTable([ wgStr.ClntN, "IP", wgStr.rx, wgStr.tx, wgStr.seen ], clientData, "wireguard_connection_table", false, false)
	
	var tableContainer = document.getElementById("wireguard_connection_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild( tableContainer.firstChild)
	}
	if(clientData.length > 0)
	{
		tableContainer.appendChild(clientTable);
	}
	else
	{
		var emptyDiv = document.createElement("div");
		emptyDiv.innerHTML = "<span style=\"text-align:center\"><em>"+wgStr.NoCConn+"</em></span>";
		tableContainer.appendChild(emptyDiv);
	}

}

function doUpdate()
{
	var param = getParameterDefinition("commands", "wg show")  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var data=req.responseText.split(/[\r\n]+/);
			updateTableFromData(data)
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
