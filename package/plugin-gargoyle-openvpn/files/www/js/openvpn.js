/*
 * This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function saveChanges()
{
	var errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		
		var openvpnConfig = getSelectedValue("openvpn_config")
		if(openvpnConfig == "disabled")
		{
			uci.set("openvpn_gargoyle", "server", "enabled", "false")
			uci.set("openvpn_gargoyle", "client", "enabled", "false")
		}
		if(openvpnConfig == "server")
		{
			uci.set("openvpn_gargoyle", "server", "enabled", "true")
			uci.set("openvpn_gargoyle", "client", "enabled", "false")

			var prefix = "openvpn_server_"
			uci.set("openvpn_gargoyle", "server", "internal_ip", document.getElementById(prefix + "ip").value)
			uci.set("openvpn_gargoyle", "server", "internal_mask", document.getElementById(prefix + "mask").value)
			uci.set("openvpn_gargoyle", "server", "port", document.getElementById(prefix + "port").value)
			uci.set("openvpn_gargoyle", "server", "proto", getSelectedValue(prefix + "protocol"))
			uci.set("openvpn_gargoyle", "server", "client_to_client", getSelectedValue(prefix + "client_to_client"))
			uci.set("openvpn_gargoyle", "server", "subnet_access", getSelectedValue(prefix + "subnet_access"))
			uci.set("openvpn_gargoyle", "server", "duplicate_cn", getSelectedValue(prefix + "duplicate_cn"))
			uci.set("openvpn_gargoyle", "server", "redirect_gateway", getSelectedValue(prefix + "redirect_gateway"))
			
			var cipher = getSelectedValue(prefix + "cipher")
			if(cipher.match(/:/))
			{
				var cipherParts = cipher.split(/:/)
				cipher = cipherParts[0]
				var keysize = cipherParts[1]
				uci.set("openvpn_gargoyle", "server", "keysize", keysize)
			}
			else
			{
				uci.remove("openvpn_gargoyle", "server", "keysize")
			}
			uci.set("openvpn_gargoyle", "server", "cipher", cipher);
		}
		if(openvpnConfig == "client")
		{
			uci.set("openvpn_gargoyle", "server", "enabled", "false")
			uci.set("openvpn_gargoyle", "client", "enabled", "true")

		}

		var commands = uci.getScriptCommands(uciOriginal);

		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uci = uciOriginal.clone()
				setControlsEnabled(true)
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


function proofreadAll()
{
	

	var prefix = "openvpn_server_"
	var inputIds = [ prefix + "ip", prefix + "mask", prefix + "port" ]
	var labelIds = [ prefix + "ip_label", prefix + "mask_label", prefix + "port_label" ]
	var functions = [ validateIP, validateNetMask, validatePort  ];
	var validReturnCodes = [0,0,0]

	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, inputIds, document );
	return errors;
}



function resetData()
{
	var serverEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled") 
	var clientEnabled = uciOriginal.get("openvpn_gargoyle", "client", "enabled")
	serverEnabled = serverEnabled == "true" || serverEnabled == "1" ? true : false;
	clientEnabled = clientEnabled == "true" || clientEnabled == "1" ? true : false;
	
	var openvpnMode = "disabled"
	openvpnMode = serverEnabled ? "server" : openvpnMode
	openvpnMode = clientEnabled ? "client" : openvpnMode
	setSelectedValue("openvpn_config", openvpnMode)

	
	getServerVarWithDefault = function(variable, defaultDef) {
		var def = uciOriginal.get("openvpn_gargoyle", "server", variable)
		def = def == "" ? defaultDef : def
		return def
	}

	document.getElementById("openvpn_server_ip").value = getServerVarWithDefault("internal_ip", "10.8.0.1")
	document.getElementById("openvpn_server_mask").value = getServerVarWithDefault("internal_mask", "255.255.255.0")
	document.getElementById("openvpn_server_port").value = getServerVarWithDefault("port", "1194")

	
	var serverCipher  = uciOriginal.get("openvpn_gargoyle", "server", "cipher")
	var serverKeysize = uciOriginal.get("openvpn_gargoyle", "server", "keysize")
	if(serverCipher == "")
	{
		serverCipher = "BF-CBC"
		serverKeysize = "128"
	}
	serverCipher = serverKeysize == "" ? serverCipher : serverCipher + ":" + serverKeysize

	setSelectedValue("openvpn_server_protocol", getServerVarWithDefault("proto", "udp"))
	setSelectedValue("openvpn_server_cipher", serverCipher)
	setSelectedValue("openvpn_server_client_to_client", getServerVarWithDefault("client_to_client", "false"))
	setSelectedValue("openvpn_server_subnet_access", getServerVarWithDefault("subnet_access", "false"))
	setSelectedValue("openvpn_server_duplicate_cn", getServerVarWithDefault("duplicate_cn", "false"))
	setSelectedValue("openvpn_server_redirect_gateway", getServerVarWithDefault("redirect_gateway", "true"))

	var acTableData = []
	var allowedClients = uciOriginal.getAllSectionsOfType("openvpn_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var rowData = []
		var id          = allowedClients[aci]
		var name        = uciOriginal.get("openvpn_gargoyle", id, "name")
		var ip          = uciOriginal.get("openvpn_gargoyle", id, "ip")
		var subnetIp   = uciOriginal.get("openvpn_gargoyle", id, "subnet_ip")
		var subnetMask = uciOriginal.get("openvpn_gargoyle", id, "subnet_mask")
		var enabled     = uciOriginal.get("openvpn_gargoyle", id, "enabled")
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""

		var ipElementContainer = document.createElement("span")
		var naContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		naContainer.appendChild( document.createTextNode("---") )
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(naContainer)
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id
		


		rowData.push(name + "\n ")
		rowData.push(ipElementContainer)
		
		var controls = createAllowedClientControls()
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}

		enabled = enabled != "false" && enabled != "0" ? true : false;
		rowData[2].checked = enabled
		
		acTableData.push(rowData)
	}

	var acTable = createTable([ "Client Name", "Internal IP\n(Routed Subnet)", "Enabled", "Credentials\n& Config Files", ""], acTableData, "openvpn_allowed_client_table", true, false, removeAcCallback)
	var tableContainer = document.getElementById("openvpn_allowed_client_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(acTable);


	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"

	setAcDocumentFromUci(document, new UCIContainer(), "dummy", dupeCn, document.getElementById("openvpn_server_ip").value )


	setOpenvpnVisibility()
}


function createAllowedClientControls()
{

	var dummyFunc = function dummy() { return "" }

	var enabledCheck = createInput("checkbox")
	enabledCheck.onclick = dummyFunc;
	var downloadButton = createButton("Download", "default_button", dummyFunc)
	var editButton     = createButton("Edit",     "default_button", editAc)

	return [enabledCheck, downloadButton, editButton]
}

function createButton(text, cssClass, actionFunction)
{
	var button = createInput("button")
	button.value = text
	button.className=cssClass
	button.onclick = actionFunction
	return button;
}

function setOpenvpnVisibility()
{
	openvpnMode = getSelectedValue("openvpn_config");
	
	document.getElementById("openvpn_server_fieldset").style.display         = openvpnMode == "server" ? "block" : "none"
	document.getElementById("openvpn_allowed_client_fieldset").style.display = openvpnMode == "server" ? "block" : "none"
	document.getElementById("openvpn_client_fieldset").style.display         = openvpnMode == "client" ? "block" : "none"
	

	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"
	

	var allowedTable = document.getElementById("openvpn_allowed_client_table");
	if(allowedTable != null)
	{
		var rows = allowedTable.rows;
		var ri;
		for(ri =1; ri < rows.length ; ri++)
		{
			var ipElementContainer = rows[ri].childNodes[1].firstChild;
			var ipChildIndex;
			for(ipChildIndex=0; ipChildIndex < ipElementContainer.childNodes.length ; ipChildIndex++)
			{
				ipElementContainer.childNodes[ipChildIndex].style.display = (ipChildIndex == 0 && dupeCn) || (ipChildIndex > 0 && (!dupeCn)) ? "inline" : "none"
			}
		}
	}


	initializeAllowedClientVisibility(document, dupeCn);
}

function initializeAllowedClientVisibility(controlDocument, dupeCn)
{
	controlDocument.getElementById("openvpn_allowed_client_ip_container").style.display          = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_have_subnet_container").style.display = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip_container").style.display   = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask_container").style.display = dupeCn ? "none" : "block"
	setAllowedClientVisibility(controlDocument)

}

function setAllowedClientVisibility( controlDocument )
{
	var selectedVis = document.getElementById("openvpn_allowed_client_remote_container").style.display == "none" ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_remote_custom_container").style.display  = getSelectedValue("openvpn_allowed_client_remote", controlDocument) == "custom" ? selectedVis : "none";
	

	var selectedVis = document.getElementById("openvpn_allowed_client_have_subnet_container").style.display == "none" ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip_container").style.display   = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? selectedVis : "none";
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask_container").style.display = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? selectedVis : "none";
}


function setRemoteNames( controlDocument, selectedRemote)
{
	var selectId = "openvpn_allowed_client_remote";
	selectedRemote = selectedRemote == null ? "" : selectedRemote;

	var names = []
	var values = []
	
	var definedDdns = uciOriginal.getAllSectionsOfType("ddns_gargoyle", "service")
	var ddi
	var selectedFound = false
	for(ddi=0; ddi < definedDdns.length; ddi++)
	{
		var enabled = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "enabled")
		var domain  = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "domain")
		if( (enabled != "0" && enabled != "false") && domain != "")
		{
			names.push("Dynamic DNS: " + domain)
			values.push(domain)
			selectedFound = selectedRemote == domain ? true : selectedFound
		}
	}
	selectedFound = (selectedRemote == currentWanIp) || selectedFound
	names.push("WAN IP: " + currentWanIp, "Other IP or Domain (specfied below)")
	values.push(currentWanIp, "custom")
	
	setAllowableSelections(selectId, values, names, controlDocument)
	var chosen = selectedRemote == "" ? values[0] : selectedRemote
	chosen = (!selectedFound) && selectedRemote != "" ? "custom" : selectedRemote
	setSelectedValue(selectId, chosen, controlDocument)
	if(chosen == "custom")
	{
		controlDocument.getElementById("openvpn_allowed_client_remote_custom").value = selectedRemote
	}
}


function setAcDocumentFromUci(controlDocument, srcUci, id, dupeCn, serverInternalIp)
{
	var name = srcUci.get("openvpn_gargoyle", id, "name")
	
	if( srcUci.get("openvpn_gargoyle", id, "remote") == "" )
	{
		var allIdList = getDefinedAcIds(false)
		var allIdHash = getDefinedAcIds(true)
		var clientCount = allIdList.length +1
		name = "Client" + clientCount
		id = "client" + clientCount
		while(allIdHash[id] == 1)
		{
			clientCount++
			name = "Client" + clientCount
			id = "client" + clientCount
		}
		controlDocument.getElementById("openvpn_allowed_client_default_id").value = id
	}
	else
	{
		controlDocument.getElementById("openvpn_allowed_client_initial_id").value = id
	}

	controlDocument.getElementById("openvpn_allowed_client_name").value = name
	

	var ip = srcUci.get("openvpn_gargoyle", id, "ip")
	if(ip == "")
	{
		var ipParts = serverInternalIp.split(/\./)
		var lastIpPart = ipParts.pop()
		lastIpPart = lastIpPart == "1" ? 2 : 1;
		

		var candidateDefaultIp = ipParts.join(".") + "." + lastIpPart
		var definedIps = getDefinedAcIps(true);
		definedIps[serverInternalIp] = 1
		while(lastIpPart < 255 && definedIps[candidateDefaultIp] == 1)
		{
			lastIpPart++
			candidateDefaultIp = ipParts.join(".") + "." + lastIpPart
		}
		ip = candidateDefaultIp
	}
	controlDocument.getElementById("openvpn_allowed_client_ip").value = ip
	
	setRemoteNames(controlDocument, srcUci.get("openvpn_gargoyle", id, "remote"))

	var subnetIp   = srcUci.get("openvpn_gargoyle", id, "subnet_ip")
	var subnetMask = srcUci.get("openvpn_gargoyle", id, "subnet_mask")

	setSelectedValue("openvpn_allowed_client_have_subnet", (subnetIp != "" && subnetMask != "" ? "true" : "false"), controlDocument)
	subnetIp   = subnetIp   == "" ? "192.168.2.1" : subnetIp;
	subnetMask = subnetMask == "" ? "255.255.255.0" : subnetMask;
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip").value = subnetIp
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask").value = subnetMask



	initializeAllowedClientVisibility(controlDocument, dupeCn)
}

function getDefinedAcIps(retHash)
{
	var ips = []
	var allowedClients = getDefinedAcIds(false)
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var ip = uci.get("openvpn_gargoyle", allowedClients[aci], "ip")
		if(ip != "")
		{
			if(retHash)
			{
				ips[ip] = 1;
			}
			else
			{
				ips.push(ip)
			}
		}
	}
	return ips;
}

function getDefinedAcIds(retHash)
{
	var ids = []
	var allowedClients = uci.getAllSectionsOfType("openvpn_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var id = allowedClients[aci]
		var enabled = uci.get("openvpn_gargoyle", id, "enabled")
		if(enabled != "0" && enabled != "false")
		{
			if(retHash)
			{
				ids[id] = 1;
			}
			else
			{
				ids.push(id)
			}
		}
	}
	return ids;
}




function setAcUciFromDocument(controlDocument, id)
{
	var name = controlDocument.getElementById("openvpn_allowed_client_name").value;
	
	var ipContainer = controlDocument.getElementById("openvpn_allowed_client_ip_container")
	var ip = controlDocument.getElementById("openvpn_allowed_client_ip").value
	ip = ipContainer.style.display == "none" ? "" : ip
	
	var remote = getSelectedValue("openvpn_allowed_client_remote", controlDocument)
	remote = remote == "custom" ? controlDocument.getElementById("openvpn_allowed_client_remote_custom").value : remote
	
	var haveSubnet = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? true : false
	haveSubnet     = ipContainer.style.display == "none" ? false : haveSubnet
	var subnetIp   = controlDocument.getElementById("openvpn_allowed_client_subnet_ip").value
	var subnetMask = controlDocument.getElementById("openvpn_allowed_client_subnet_mask").value

	var pkg = "openvpn_gargoyle"
	uci.set(pkg, id, "", "allowed_client")
	uci.set(pkg, id, "name", name)
	if(ip != "")
	{
		uci.set(pkg, id, "ip", ip)
	}
	else
	{
		uci.remove(pkg, id, "ip")
	}
	uci.set(pkg, id, "remote", remote)
	if(haveSubnet)
	{
		uci.set(pkg, id, "subnet_ip",   subnetIp)
		uci.set(pkg, id, "subnet_mask", subnetMask)
	}
}

function getNumericIp(ip)
{
	var i = ip.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/)
	if(i)
	{
        	return (+i[1]<<24) + (+i[2]<<16) + (+i[3]<<8) + (+i[4])
	}
	return null
}

function getNumericMask(mask)
{
	if(mask.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/))
	{
		return getNumericIp(mask)
	}
	else
	{
		return -1<<(32-mask)
	}
}





function validateAc(controlDocument, internalServerIp, internalServerMask)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var validateHaveText = function(txt) {  return txt.length > 0 ? 0 : 1 }

	var prefix = "openvpn_allowed_client_"
	var inputIds = [ prefix + "name", prefix + "ip", prefix + "remote_custom", prefix + "subnet_ip", prefix + "subnet_mask" ]
	var labelIds = [ prefix + "name_label", prefix + "ip_label", prefix + "remote_label",  prefix + "have_subnet_label", prefix + "have_subnet_label" ]
	var functions = [ validateHaveText, validateIP, validateHaveText, validateIP, validateNetMask  ];
	var validReturnCodes = [0,0,0,0,0]
	var visibilityIds = [  prefix + "name_container", prefix + "ip_container", prefix + "remote_custom_container", prefix + "subnet_ip_container", prefix + "subnet_mask_container" ]

	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );
	if(errors.length == 0 && controlDocument.getElementById(prefix + "ip_container").style.display != "none")
	{
		var testIp  = getNumericIp(controlDocument.getElementById(prefix + "ip").value)
		var vpnIp   = getNumericIp(internalServerIp)
		var vpnMask = getNumericMask(internalServerMask)
		if( ( testIp & vpnMask ) != ( vpnIp & vpnMask ) )
		{
			errors.push("Specified Client Internal IP " + ip + " is not in OpenVPN Subnet")
		}
	}
	return errors;

}

function removeAcCallback(table, row)
{
	var id = row.childNodes[2].firstChild.id;
	uci.removeSection("openvpn_gargoyle", id);
}

function addAc()
{
	var errors = validateAc(document, document.getElementById("openvpn_server_ip").value , document.getElementById("openvpn_server_mask").value );
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\nCould not add client configuration.");
	}
	else
	{
		var name       = document.getElementById("openvpn_allowed_client_name").value
		var ip         = document.getElementById("openvpn_allowed_client_ip").value
		var subnetIp   = ""
		var subnetMask = ""
		if( getSelectedValue("openvpn_allowed_client_have_subnet", document) == "true")
		{
			subnetIp   = document.getElementById("openvpn_allowed_client_subnet_ip").value
			subnetMask = document.getElementById("openvpn_allowed_client_subnet_mask").value
		}
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""
	
		var id = name.replace(/[\t\r\n ]+/g, "_").toLowerCase().replace(/[^a-z0-9_-]/g, "");
		var idCount = 1;
		var testId = id
		while(uci.get("openvpn_gargoyle", testId) != "")
		{
			testId = id + "_" + idCount
			idCount++
		}
		id = testId

		setAcUciFromDocument(document, id)


		var ipElementContainer = document.createElement("span")
		var naContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		naContainer.appendChild( document.createTextNode("---") )
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(naContainer)
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id

		var acTable = document.getElementById("openvpn_allowed_client_table");


		var rowData = [ name, ipElementContainer ]
		var controls = createAllowedClientControls()
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}
		rowData[2].checked = true
		addTableRow(acTable, rowData, true, false, removeAcCallback);
	
		var dupeCn = getSelectedValue("openvpn_server_duplicate_cn")
		dupeCn= dupeCn == "true" || dupeCn == "1"
		setAcDocumentFromUci(document, new UCIContainer(), "dummy", dupeCn, document.getElementById("openvpn_server_ip").value )
		
		setOpenvpnVisibility()
	}

}

function editAc()
{
	if( typeof(editAcWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editAcWindow.close();
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


	editAcWindow = window.open("openvpn_allowed_client_edit.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	var saveButton = createInput("button", editAcWindow.document);
	var closeButton = createInput("button", editAcWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	var editRow=this.parentNode.parentNode;
	var editId = editRow.childNodes[1].firstChild.id;
	
	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"
	var serverInternalIp   = document.getElementById("openvpn_server_ip").value 
	var serverInternalMask = document.getElementById("openvpn_server_mask").value 


	var runOnEditorLoaded = function () 
	{
		var updateDone=false;
		if(editAcWindow.document != null)
		{
			if(editAcWindow.document.getElementById("bottom_button_container") != null)
			{
				editAcWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editAcWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
				
				setAcDocumentFromUci(editAcWindow.document, uci, editId, dupeCn, serverInternalIp)


				closeButton.onclick = function()
				{
					editAcWindow.close();
				}
				saveButton.onclick = function()
				{
					var errors = validateAc(editAcWindow.document, serverInternalIp, serverInternalMask);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not update client configuration.");
					}
					else
					{
						var name       = editAcWindow.document.getElementById("openvpn_allowed_client_name").value
						var ip         = editAcWindow.document.getElementById("openvpn_allowed_client_ip").value
						var subnetIp   = ""
						var subnetMask = ""
						if( getSelectedValue("openvpn_allowed_client_have_subnet", editAcWindow.document) == "true")
						{
							subnetIp   = editAcWindow.document.getElementById("openvpn_allowed_client_subnet_ip").value
							subnetMask = editAcWindow.document.getElementById("openvpn_allowed_client_subnet_mask").value
						}
						var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""


						setAcUciFromDocument(editAcWindow.document, editId)
					
						while( editRow.childNodes[0].firstChild != null)
						{
							editRow.childNodes[0].removeChild( editRow.childNodes[0].firstChild )
						}
						editRow.childNodes[0].appendChild(document.createTextNode(name))


						var ipElementContainer = document.createElement("span")
						var naContainer = document.createElement("span")
						var ipContainer = document.createElement("span")
						naContainer.appendChild( document.createTextNode("---") )
						ipContainer.appendChild( document.createTextNode(ip) )
						ipContainer.appendChild( document.createElement("br") )
						ipContainer.appendChild( document.createTextNode(subnet) )
						ipElementContainer.appendChild(naContainer)
						ipElementContainer.appendChild(ipContainer)
						ipElementContainer.id = editId

						while( editRow.childNodes[1].firstChild != null)
						{
							editRow.childNodes[1].removeChild( editRow.childNodes[1].firstChild )
						}
						
						editRow.childNodes[1].appendChild( ipElementContainer )
						setOpenvpnVisibility()

						editAcWindow.close();
					}

					
				}
				editAcWindow.moveTo(xCoor,yCoor);
				editAcWindow.focus();
				updateDone = true;
				
			}
		}
		if(!updateDone)
		{
			setTimeout(runOnEditorLoaded, 250);
		}
	}
	runOnEditorLoaded();
}


