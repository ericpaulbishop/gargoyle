
function resetData()
{
	routingData.shift();
	routingData.shift();
	
	var activeRouteTableData = [];
	while(routingData.length > 0)
	{
		var str = routingData.shift();
		var rLine = str.split(/[\t ]+/);
		var r = [];
		var iface = "";
		iface= rLine[7] == wanIface ? "WAN" : iface;
		iface= rLine[7] == lanIface ? "LAN" : iface;
		if(iface != "")
		{
			mask = (rLine[0] == "default" && rLine[2] == "0.0.0.0") ? "" : "/" + rLine[2];
			mask = mask == "/255.255.255.255" ? "" : mask;
			r.push(rLine[0] + mask, iface, rLine[1],  rLine[4]);
			activeRouteTableData.push(r);
		}
	}
	var tableContainer = document.getElementById("active_route_table_container");
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	var activeRouteTable = createTable(["Destination", "Interface", "Gateway", "Metric"], activeRouteTableData, "active_route_table", false, false);	
	tableContainer.appendChild( activeRouteTable );


	var routeSections = uciOriginal.getAllSectionsOfType("network", "route");
	var staticRouteTableData = [];
	while(routeSections.length > 0)
	{
		var section = routeSections.shift();
		var dest    = uciOriginal.get("network", section, "target");
		var mask    = uciOriginal.get("network", section, "netmask");		
		var iface   = uciOriginal.get("network", section, "interface");	
		var gateway = uciOriginal.get("network", section, "gateway");
		
		dest = mask == "" ? dest : dest + "/" + mask;
		staticRouteTableData.push( [ dest, iface, gateway ] );			
	}
	
	tableContainer = document.getElementById("static_route_table_container");
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	var staticRouteTable = createTable(["Destination", "Interface", "Gateway"],staticRouteTableData , "active_route_table", true, false);	
	tableContainer.appendChild( staticRouteTable );

}

function addStaticRoute()
{

}
