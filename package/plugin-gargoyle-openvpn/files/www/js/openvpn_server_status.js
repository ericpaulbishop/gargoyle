function resetData()
{
	updateTableFromData(statusFileLines)
	setInterval(doUpdate, 15*1000);
}

function updateTableFromData(statusData)
{
	var i;
	var clientData = [];

	while( statusData[0] != "OpenVPN CLIENT LIST")
	{
		statusData.shift() ; 
	}
	for(i=0; i<3; i++)
	{
		statusData.shift()
	}
	while( statusData[0] != "ROUTING TABLE" &&  statusData[0] != "GLOBAL STATS" && statusData[0] != "END")
	{
		var lineParts = statusData.shift().split(/,/);
		var clientName = uciOriginal.get("openvpn_gargoyle", lineParts[0]) == "allowed_client" ? uciOriginal.get("openvpn_gargoyle", lineParts[0], "name") : lineParts[0]
		clientData.push( [ clientName, lineParts[1].replace(/:.*$/, ""), lineParts[4]] )
	}

	var clientTable = createTable([ "Client Name", "Connected From", "Connected Since" ], clientData, "openvpn_server_status_table", false, false)
	
	var tableContainer = document.getElementById("openvpn_server_status_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild( tableContainer.firstChild)
	}
	tableContainer.appendChild(clientTable);

}

function doUpdate()
{
	var param = getParameterDefinition("commands", "cat /etc/openvpn/current_status")  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
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
