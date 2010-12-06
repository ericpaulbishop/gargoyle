var toggleReload = false;

function saveChanges()
{
	if(wirelessDriver == "broadcom")
	{
		setControlsEnabled(false, true, "Please Wait While Settings Are Applied");
	}
	else
	{
		setControlsEnabled(false, true, "Please Wait While Settings Are Applied And Device Is Restarted");
	}
	
	var removeCommands = [];
	var oldSections = uciOriginal.getAllSectionsOfType("network", "route");
	while(oldSections.length > 0)
	{
		var delSection = oldSections.pop();
		uciOriginal.removeSection("network", delSection);
		removeCommands.push("uci del network." + delSection);
	}
	removeCommands.push("uci commit");

	var uci = uciOriginal.clone();
	var staticRouteTable = document.getElementById('static_route_table_container').firstChild;
	var staticRouteData = getTableDataArray(staticRouteTable, true, false);
	var routeIndex = 0;
	for(routeIndex=0; routeIndex < staticRouteData.length; routeIndex++)
	{
		var row = staticRouteData[routeIndex];
		var destParts = parseDest(row[0]);
		var dest = destParts[0];
		var netmask = destParts[1];
		var iface = row[1];
		var gateway = row[2];
		var routeId = "route" + (routeIndex+1);
		uci.set("network", routeId, "", "route");
		uci.set("network", routeId, "target", dest);
		uci.set("network", routeId, "interface", iface);
		uci.set("network", routeId, "gateway", gateway);
		if(netmask != ""){ uci.set("network", routeId, "netmask", netmask); }
	}
	
	commands = removeCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal)  +  "\nsh /usr/lib/gargoyle/restart_network.sh ;\n";
;
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + window.location.host + "/utility/reboot_test.sh";
	testReboot = function()
	{
		toggleReload = true;
		setTimeout( "testReboot()", 5*1000);  //try again after 12 seconds
		document.getElementById("reboot_test").src = testLocation; 
	}
	setTimeout( "testReboot()", 15*1000);  //start testing after 15 seconds
	setTimeout( "reloadPage()", 240*1000); //after 4 minutes, try to reload anyway
}


function reloadPage()
{
	if(toggleReload)
	{
		//IE calls onload even when page isn't loaded -- it just times out and calls it anyway
		//We can test if it's loaded for real by looking at the (IE only) readyState property
		//For Browsers NOT designed by dysfunctional cretins whose mothers were a pack of sewer-dwelling, shit-eating rodents,
		//well, for THOSE browsers, readyState (and therefore reloadState) should be null 
		var reloadState = document.getElementById("reboot_test").readyState;
		if( typeof(reloadState) == "undefined" || reloadState == null || reloadState == "complete")
		{
			toggleReload = false;
			document.getElementById("reboot_test").src = "";
			setTimeout( "window.location.href = window.location.href;", 1500);
		}
	}
}


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
		var iface = rLine[7];
		iface= rLine[7] == wanIface ? iface+ " (WAN)" : iface;
		iface= rLine[7] == lanIface ? iface + " (LAN)" : iface;
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
	var activeRouteTable = createTable(["Destination", "Interface(Network)", "Gateway", "Metric"], activeRouteTableData, "active_route_table", false, false);	
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
		staticRouteTableData.push( [ dest, iface, gateway, createEditButton() ] );			
	}
	
	tableContainer = document.getElementById("static_route_table_container");
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	var staticRouteTable = createTable(["Destination", "Interface", "Gateway", ""], staticRouteTableData , "static_route_table", true, false);	
	tableContainer.appendChild( staticRouteTable );

}


function createEditButton()
{
	var editButton = createInput("button");
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editStaticRoute;
	return editButton;
}



function addStaticRoute()
{
	errors = proofreadStaticRoute(document);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add row.");
	}
	else
	{
		var destData = parseDest(document.getElementById("add_dest").value);
		var dest = destData[1] == "" ? destData[0] : destData[0] + "/" + destData[1];
		var iface = getSelectedValue("add_iface");
		var gateway  = document.getElementById("add_gateway").value;
		
		var row = [dest, iface, gateway, createEditButton() ];
		var staticRouteTable = document.getElementById('static_route_table_container').firstChild;
		addTableRow(staticRouteTable,row, true, false);
		document.getElementById("add_dest").value = "";
		document.getElementById("add_gateway").value = "";

	}
}

function proofreadStaticRoute(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	addIds=['add_dest', 'add_gateway'];
	labelIds= ['add_dest_label', 'add_gateway_label'];
	functions = [validateIpRange, validateIP];
	returnCodes = [0,0];
	visibilityIds=addIds;
	return proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, controlDocument);
}

function parseDest(origDest)
{
	var dest = origDest;
	var mask = "";
	if(dest.match(/\//))
	{
		var splitDest = dest.split(/\//);
		dest = splitDest[0];
		mask = splitDest[1];
		if(!mask.match(/\./))
		{
			//convert to full numeric format
			var bits = parseInt(mask);
			var maskParts = [];
			while(maskParts.length < 4)
			{
				var nextBits = bits > 8 ? 8 : bits;
				var nextVal = 255;
				var bitCount = nextBits;
				while(bitCount < 8)
				{
					var nextPow = 7-bitCount;
					nextVal = nextVal - Math.pow(2, nextPow);
					bitCount++;
				}
				bits = bits- nextBits;
				maskParts.push("" + nextVal);
			}
			mask = maskParts.join(".");
		}

		//for some really stupid reason busybox route command requires that everything after mask is 0 in dest ip
		splitDest = dest.split(/\./);
		splitMask = mask.split(/\./);
		var byteIndex=3;
		while(byteIndex > 0)
		{
			splitDest[byteIndex] = "" + (parseInt(splitDest[byteIndex]) & parseInt(splitMask[byteIndex]));
			byteIndex--;
		}
		dest = splitDest.join(".");
		mask = splitMask.join(".");
	}
	return [ dest, mask ];
}


function editStaticRoute()
{
	if( typeof(editStaticWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editStaticWindow.close();
		}
		catch(e){}
	}

	
	try
	{
		xCoor = window.screenX + 225;
		yCoor = window.screenY+ 225;
	}
	catch(e)
	{
		xCoor = window.left + 225;
		yCoor = window.top + 225;
	}


	editStaticWindow = window.open("static_route_edit.sh", "edit", "width=560,height=180,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editStaticWindow.document);
	closeButton = createInput("button", editStaticWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	editRow=this.parentNode.parentNode;

	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(editStaticWindow.document != null)
		{
			if(editStaticWindow.document.getElementById("bottom_button_container") != null)
			{
				editStaticWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editStaticWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
			
				//set edit values
				editStaticWindow.document.getElementById("add_dest").value = editRow.childNodes[0].firstChild.data;
				setSelectedValue("add_iface", editRow.childNodes[1].firstChild.data, editStaticWindow.document);
				editStaticWindow.document.getElementById("add_gateway").value   = editRow.childNodes[2].firstChild.data;
				editStaticWindow.document.getElementById("add_button").style.display="none";				
				closeButton.onclick = function()
				{
					editStaticWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = proofreadStaticRoute(editStaticWindow.document, document, editRow);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not update static route.");
					}
					else
					{
						//update document with new data
						var adjDest = parseDest( editStaticWindow.document.getElementById("add_dest").value );
						editRow.childNodes[0].firstChild.data = adjDest[1] == "" ? adjDest[0] : adjDest[0] + "/" + adjDest[1];
						editRow.childNodes[1].firstChild.data = getSelectedValue("add_iface", editStaticWindow.document);
						editRow.childNodes[2].firstChild.data = editStaticWindow.document.getElementById("add_gateway").value;
						
						editStaticWindow.close();
					}
				}
				editStaticWindow.moveTo(xCoor,yCoor);
				editStaticWindow.focus();
				updateDone = true;
			}
		}
		if(!updateDone)
		{
			setTimeout( "runOnEditorLoaded()", 250);
		}
	}
	runOnEditorLoaded();
}


