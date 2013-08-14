/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var prtS=new Object(); //part of i18n

function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);

		var firewallSectionCommands = [];
		var redirectSectionTypes = ["redirect", "redirect_disabled", "dmz"];
		for(typeIndex=0; typeIndex < redirectSectionTypes.length; typeIndex++)
		{
			var sectionType = redirectSectionTypes[typeIndex];
			var sections = uciOriginal.getAllSectionsOfType("firewall", sectionType);
			while(sections.length > 0)
			{
				var lastSection = sections.pop();
				uciOriginal.removeSection("firewall", lastSection);
				firewallSectionCommands.push("uci del firewall." + lastSection);
			}
		}
		

		var uci = uciOriginal.clone();
		

		var singlePortTable = document.getElementById('portf_table_container').firstChild;	
		var singlePortData= getTableDataArray(singlePortTable, true, false);	
		var enabledIndex = 0;
		var disabledIndex = 0;
		for(rowIndex = 0; rowIndex < singlePortData.length; rowIndex++)
		{

			var rowData = singlePortData[rowIndex];
			var enabled = rowData[5].checked;
			
			var protos = rowData[1].toLowerCase() == UI.both.toLowerCase() ? ["tcp", "udp"] : [ rowData[1].toLowerCase() ];
			var protoIndex=0;
			for(protoIndex=0;protoIndex < protos.length; protoIndex++)
			{
				var id = "redirect_" + (enabled ? "enabled" : "disabled") + "_number_" +  (enabled ? enabledIndex : disabledIndex);
				firewallSectionCommands.push("uci set firewall." + id + "=" + (enabled ? "redirect" : "redirect_disabled"));
				uci.set("firewall", id, "", (enabled ? "redirect" : "redirect_disabled"));
				uci.set("firewall", id, "name", rowData[0]);
				uci.set("firewall", id, "src", "wan");
				uci.set("firewall", id, "dest", "lan");
				uci.set("firewall", id, "proto", protos[protoIndex]);
				uci.set("firewall", id, "src_dport", rowData[2]);
				uci.set("firewall", id, "dest_ip", rowData[3]);
				uci.set("firewall", id, "dest_port", rowData[4]);
				enabledIndex = enabledIndex + (enabled ? 1 : 0);
				disabledIndex = disabledIndex + (enabled ? 0 : 1);
			}
		}


		var portRangeTable = document.getElementById('portfrange_table_container').firstChild;	
		var portRangeData= getTableDataArray(portRangeTable, true, false);	
		for(rowIndex = 0; rowIndex < portRangeData.length; rowIndex++)
		{
			var rowData = portRangeData[rowIndex];
			var enabled = rowData[5].checked;

			var protos = rowData[1].toLowerCase() == UI.both.toLowerCase() ? ["tcp", "udp"] : [ rowData[1].toLowerCase() ];
			var protoIndex=0;
			for(protoIndex=0;protoIndex < protos.length; protoIndex++)
			{
				var id = "redirect_" + (enabled ? "enabled" : "disabled") + "_number_" +  (enabled ? enabledIndex : disabledIndex);
				firewallSectionCommands.push("uci set firewall." + id + "=" + (enabled ? "redirect" : "redirect_disabled"));
				uci.set("firewall", id, "", (enabled ? "redirect" : "redirect_disabled"));
				uci.set("firewall", id, "name", rowData[0]);
				uci.set("firewall", id, "src", "wan");
				uci.set("firewall", id, "dest", "lan");
				uci.set("firewall", id, "proto", protos[protoIndex]);
				uci.set("firewall", id, "src_dport", rowData[2] + "-" + rowData[3]);
				uci.set("firewall", id, "dest_port", rowData[2] + "-" + rowData[3]);
				uci.set("firewall", id, "dest_ip", rowData[4]);
				
				enabledIndex = enabledIndex + (enabled ? 1 : 0);
				disabledIndex = disabledIndex + (enabled ? 0 : 1);
			}
		}


		//dmz
		if(document.getElementById('dmz_enabled').checked )
		{
			var id = "dmz";
			firewallSectionCommands.push("uci firewall.dmz=dmz" );
			
			uci.set("firewall", id, "", "dmz");
			uci.set("firewall", id, "from", "wan");
			uci.set("firewall", id, "to_ip", document.getElementById('dmz_ip').value);
		}		

		firewallSectionCommands.push("uci commit");
			
		restartFirewallCommand = "\nsh /usr/lib/gargoyle/restart_firewall.sh ;\n";


		//upnp
		upnpStartCommands = new Array();
		upnpdEnabled = document.getElementById("upnp_enabled").checked;
		if(upnpdEnabled)
		{
			upnpStartCommands.push("/etc/init.d/miniupnpd enable");
			uci.set("upnpd", "config", "enable_upnp", "1");
			uci.set("upnpd", "config", "enable_natpmp", "1");
			uci.set("upnpd", "config", "upload", document.getElementById("upnp_up").value);
			uci.set("upnpd", "config", "download", document.getElementById("upnp_down").value);
		}
		else
		{
			uci.set("upnpd", "config", "enable_upnp", "0");
			uci.set("upnpd", "config", "enable_natpmp", "0");
			upnpStartCommands.push("/etc/init.d/miniupnpd disable");
		}
	

		commands = firewallSectionCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + upnpStartCommands.join("\n") + "\n" + restartFirewallCommand;
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	controlIds=['dmz_ip', 'upnp_up', 'upnp_down'];
	labelIds= ['dmz_ip_label', 'upnp_up_label', 'upnp_down_label'];
	functions = [validateIP, validateNumeric, validateNumeric];
	returnCodes = [0,0,0];
	visibilityIds=controlIds;
	errors = proofreadFields(controlIds, labelIds, functions, returnCodes, visibilityIds);
	return errors;
}

function addPortfRule()
{
	errors = proofreadForwardSingle();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n"+prtS.AFRErr);
	}
	else
	{
		values = new Array();
		ids = ['add_desc', 'add_prot', 'add_fp', 'add_ip', 'add_dp'];
		for (idIndex in ids)
		{
			element = document.getElementById(ids[idIndex]);
			v = element.value;
			v = v== '' ? '-' : v;
			values.push(v);
			if(element.type == "text")
			{
				element.value = "";
			}
		}
		values[4] = values[4] == '-' ? values[2] : values[4];


		//check if this is identical to another rule, but for a different protocol
		//if so, just merge the two by setting the protocol on the old data to 'both'
		//
		portfTable = document.getElementById('portf_table_container').firstChild;
		currentPortfData = getTableDataArray(portfTable, true, false);
		otherProto = values[1] == 'TCP' ? 'UDP' : 'TCP';
		mergedWithExistingRule = false;
		for (rowDataIndex in currentPortfData)
		{
			rowData = currentPortfData[rowDataIndex];
			
			if( otherProto == rowData[1] &&  values[2] == rowData[2] && values[3] == rowData[3] && values[4] == rowData[4])
			{

				portfTable.rows[(rowDataIndex*1)+1].childNodes[1].firstChild.data = UI.both;
				if(values[0] != '-' && rowData[0] == '-')
				{
					portfTable.rows[(rowDataIndex*1)+1].childNodes[0].firstChild.data = values[0];
				}
				
				table1Container = document.getElementById('portf_table_container');
				if(table1Container.firstChild != null)
				{
					table1Container.removeChild(table1Container.firstChild);
				}
				table1Container.appendChild(portfTable);

				mergedWithExistingRule = true;
			}
		}

		if(!mergedWithExistingRule)
		{
			checkbox = createInput('checkbox');
			checkbox.checked = true;
			values.push(checkbox);
			values.push(createEditButton(true));	
			addTableRow(portfTable,values, true, false);
		}
	}
}



function addPortfRangeRule()
{
	errors = proofreadForwardRange();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n"+prtS.AFRErr);
	}
	else
	{
		values = new Array();
		ids = ['addr_desc', 'addr_prot', 'addr_sp', 'addr_ep', 'addr_ip'];
		for (idIndex in ids)
		{
			element = document.getElementById(ids[idIndex]);
			v = element.value;
			v = v== '' ? '-' : v;
			values.push(v);
			if(element.type == 'text')
			{
				element.value = "";
			}
		}

		portfRangeTable = document.getElementById('portfrange_table_container').firstChild;
		currentRangeData = getTableDataArray(portfRangeTable, true, false);
		otherProto = values[1] == 'TCP' ? 'UDP' : 'TCP';
		mergedWithExistingRule = false;
		for (rowDataIndex in currentRangeData)
		{
			rowData = currentRangeData[rowDataIndex];
			if( otherProto == rowData[1] &&  values[2] == rowData[2] && values[3] == rowData[3] && values[4] == rowData[4])
			{
				portfRangeTable.rows[(rowDataIndex*1)+1].childNodes[1].firstChild.data = UI.both;
				if(values[0] != '-' && rowData[0] == '-')
				{
					portfRangeTable.rows[(rowDataIndex*1)+1].childNodes[0].firstChild.data = values[0];
				}
				
				table2Container = document.getElementById('portfrange_table_container');
				if(table2Container.firstChild != null)
				{
					table2Container.removeChild(table2Container.firstChild);
				}
				table2Container.appendChild(portfRangeTable);

				mergedWithExistingRule = true;

			}
		}


		if(!mergedWithExistingRule)
		{
			checkbox = createInput('checkbox');	
			checkbox.checked = true;
			values.push(checkbox);
			values.push(createEditButton(false));	

			portfrangeTable = document.getElementById('portfrange_table_container').firstChild;
			addTableRow(portfrangeTable,values, true, false);
		}
	}
}

function proofreadForwardRange(controlDocument, tableDocument, excludeRow)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	tableDocument = tableDocument == null ? document : tableDocument;

	var addIds = ['addr_sp', 'addr_ep', 'addr_ip'];
	var labelIds = ['addr_sp_label', 'addr_ep_label', 'addr_ip_label'];
	var functions = [validateNumeric, validateNumeric, validateIP];
	var returnCodes = [0,0,0];
	var visibilityIds = addIds;
	var errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, controlDocument);
	if(errors.length == 0)
	{
		if( (1*controlDocument.getElementById('addr_sp').value) > (1*controlDocument.getElementById('addr_ep').value) )
		{
			errors.push(prtS.GTErr);
		}
		
		
		var portfTable = tableDocument.getElementById('portf_table_container').firstChild;
		var currentPortfData = getTableDataArray(portfTable, true, false);
		var addStartPort = controlDocument.getElementById('addr_sp').value;
		var addEndPort = controlDocument.getElementById('addr_ep').value;
		var addProtocol = controlDocument.getElementById('addr_prot').value;
		var rowDataIndex=0;
		for (rowDataIndex=0; rowDataIndex < currentPortfData.length ; rowDataIndex++)
		{
			var rowData = currentPortfData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == UI.both || rowData[1] == UI.both) &&  addStartPort*1 <= rowData[2]*1 && addEndPort*1 >= rowData[2]*1 )
			{
				errors.push(prtS.DupErr);
			}
		}

		var portfRangeTable = tableDocument.getElementById('portfrange_table_container').firstChild;
		var currentRangeData = getTableDataArray(portfRangeTable, true, false);
		for (rowDataIndex=0; rowDataIndex < currentRangeData.length; rowDataIndex++)
		{
			if(portfRangeTable.rows[rowDataIndex+1] != excludeRow)
			{
				var rowData = currentRangeData[rowDataIndex];
				if( (addProtocol == rowData[1] || addProtocol == UI.both || rowData[1] == UI.both) && rowData[2]*1 <= addEndPort*1 && rowData[3]*1 >= addStartPort*1)
				{
					errors.push(prtS.DupErr);
				}
			}
		}
	}

	
	return errors;

}

function proofreadForwardSingle(controlDocument, tableDocument, excludeRow)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	tableDocument = tableDocument == null ? document : tableDocument;

	var addIds = ['add_fp', 'add_ip'];
	var labelIds = ['add_fp_label', 'add_ip_label', 'add_dp_label'];
	var functions = [validateNumeric, validateIP, validateNumeric];
	var returnCodes = [0,0,0];
	var visibilityIds = addIds;
	if(controlDocument.getElementById('add_dp').value.length > 0)
	{
		addIds.push('add_dp');
	}
	var errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, controlDocument);



	if(errors.length == 0)
	{
		var portfTable = tableDocument.getElementById('portf_table_container').firstChild;
		var currentPortfData = getTableDataArray(portfTable, true, false);
		var addPort = controlDocument.getElementById('add_fp').value;
		var addProtocol = controlDocument.getElementById('add_prot').value;
		var rowDataIndex=0;
		for (rowDataIndex=0; rowDataIndex < currentPortfData.length; rowDataIndex++)
		{
			if(portfTable.rows[rowDataIndex+1] != excludeRow)
			{
				var rowData = currentPortfData[rowDataIndex];
				if( (addProtocol == rowData[1] || addProtocol == UI.both || rowData[1] == UI.both) &&  addPort == rowData[2])
				{
					errors.push(prtS.CopErr);
				}
			}
		}

		var portfRangeTable = tableDocument.getElementById('portfrange_table_container').firstChild;
		var currentRangeData = getTableDataArray(portfRangeTable, true, false);
		for (rowDataIndex=0; rowDataIndex < currentRangeData; rowDataIndex++)
		{
			var rowData = currentRangeData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == UI.both || rowData[1] == UI.both) && rowData[2]*1 <= addPort*1 && rowData[3]*1 >= addPort*1)
			{
				errors.push(prtS.CopErr);
			}
		}
	}

	return errors;
}

function resetData()
{
	var singlePortTableData = new Array();
	var portRangeTableData = new Array();
	var singlePortEnabledStatus = new Array();
	var portRangeEnabledStatus = new Array();
	var dmzIp = "";

	var singlePortProtoHash = [];
	var portRangeProtoHash = [];
	singlePortProtoHash["tcp"] = [];
	singlePortProtoHash["udp"] = [];
	portRangeProtoHash["tcp"] = [];
	portRangeProtoHash["udp"] = [];


	// parse (both enabled & disabled) redirects
	// uci firewall doesn't parse redirect_disabled sections, so we can store this info there
	// without any complications.  Likewise we store rule name in "name" variable that doesn't
	// get parsed by the uci firewall script.
	var redirectSectionTypes = ["redirect", "redirect_disabled"];
	for(typeIndex=0; typeIndex < redirectSectionTypes.length; typeIndex++)
	{
		var sectionType = redirectSectionTypes[typeIndex];
		var redirectSections = uciOriginal.getAllSectionsOfType("firewall", redirectSectionTypes[typeIndex]);
		for(rdIndex=0; rdIndex < redirectSections.length; rdIndex++)
		{
			var rId = redirectSections[rdIndex];
			var name = uciOriginal.get("firewall", rId, "name");
			name = name == "" ? "-" : name;
			var proto	= uciOriginal.get("firewall", rId, "proto").toLowerCase();
			var srcdport	= uciOriginal.get("firewall", rId, "src_dport");
			var destip	= uciOriginal.get("firewall", rId, "dest_ip");
			var destport	= uciOriginal.get("firewall", rId, "dest_port");
	
			
			if(srcdport == "" && destport == "" && sectionType == "redirect")
			{	
				dmzIp = dmzIp == "" ? destip : dmzIp;
			}
			else if(proto.toLowerCase() == "tcp" || proto.toLowerCase() == "udp")
			{
				checkbox = createInput('checkbox');
				checkbox.checked = sectionType == "redirect" ? true : false;

				destport = destport == "" ? srcdport : destport;
				otherProto = proto == "tcp" ? "udp" : "tcp";
				hashStr = name + "-" + srcdport + "-" + destip + "-" + destport;
				if(srcdport.match(/-/))
				{
					var splitPorts = srcdport.split(/-/);
					// if same rule, different protocol exists, merge into one rule
					// otherwise, add rule to table data
					if(portRangeProtoHash[otherProto][hashStr] != null)
					{
						portRangeProtoHash[otherProto][hashStr][1] = UI.both;
					}
					else
					{
						var nextTableRowData = [name, proto.toUpperCase(), splitPorts[0], splitPorts[1], destip, checkbox, createEditButton(false)];
						portRangeTableData.push(nextTableRowData);
						portRangeProtoHash[proto][hashStr] = nextTableRowData;
						portRangeEnabledStatus.push(checkbox.checked);
					}
				}
				else
				{
					// if same rule, different protocol exists, merge into one rule
					// otherwise, add rule to table data
					if(singlePortProtoHash[otherProto][hashStr] != null)
					{
						singlePortProtoHash[otherProto][hashStr][1] = UI.both;
					}
					else
					{
						var nextTableRowData = [name, proto.toUpperCase(), srcdport, destip, destport, checkbox, createEditButton(true)];
						singlePortTableData.push(nextTableRowData);
						singlePortProtoHash[proto][hashStr] = nextTableRowData;
						singlePortEnabledStatus.push(checkbox.checked);
					}
				}
			}
		}
	}


	columnNames = [prtS.Desc, prtS.Proto, prtS.FPrt, prtS.TIP, prtS.TPrt, UI.Enabled, '']
	portfTable=createTable(columnNames, singlePortTableData, "portf_table", true, false);
	table1Container = document.getElementById('portf_table_container');
	
	if(table1Container.firstChild != null)
	{
		table1Container.removeChild(table1Container.firstChild);
	}
	table1Container.appendChild(portfTable);
	
	
	
	

	columnNames = [prtS.Desc, prtS.Proto, prtS.SPrt, prtS.EPrt, prtS.TIP, UI.Enabled, '']
	portfrangeTable=createTable(columnNames, portRangeTableData, "portf_range_table", true, false);
	table2Container = document.getElementById('portfrange_table_container');
	if(document.getElementById('portfrange_table_container').firstChild != null)
	{
		table2Container.removeChild(table2Container.firstChild);
	}
	table2Container.appendChild(portfrangeTable);



	// Because IE6 was designed by programmers whose only qualification was participation in the Special Olympics,
	// checkboxes become unchecked when added to table.  We need to reset checked status here.
	for(spIndex = 0; spIndex < singlePortEnabledStatus.length; spIndex++)
	{
		singlePortTableData[spIndex][5].checked = singlePortEnabledStatus[spIndex];
	}
	for(prIndex = 0; prIndex < portRangeEnabledStatus.length; prIndex++)
	{
		portRangeTableData[prIndex][5].checked = portRangeEnabledStatus[prIndex];
	}



	clearIds = ['add_desc', 'add_fp', 'add_ip', 'add_dp', 'addr_desc', 'addr_sp', 'addr_ep', 'addr_ip'];
	for(clearIndex = 0; clearIndex < clearIds.length; clearIndex++)
	{
		document.getElementById(clearIds[clearIndex]).value = '';
	}


	//dmz
	var dmzSections = uciOriginal.getAllSectionsOfType("firewall", "dmz");
	document.getElementById("dmz_enabled").checked = (dmzSections.length > 0);
	if( dmzSections.length > 0)
	{
		document.getElementById("dmz_ip").value = uciOriginal.get("firewall", dmzSections[0], "to_ip");
	}
	else
	{
		var defaultDmz = (currentLanIp.split(/\.[^\.]*$/))[0];
		var lanIpEnd = parseInt((currentLanIp.split("."))[3]);
		if(lanIpEnd >= 254)
		{
			lanIpEnd--;
		}
		else
		{
			lanIpEnd++;
		}
		defaultDmz = defaultDmz + "." + lanIpEnd;
		document.getElementById("dmz_ip").value = defaultDmz;
	}
	setDmzEnabled();


	//upnp
	document.getElementById("upnp_enabled").checked = upnpdEnabled;
	upElement = document.getElementById("upnp_up");
	downElement = document.getElementById("upnp_down");
	
	upElement.value = uciOriginal.get("upnpd", "config", "upload");
	upElement.value = upElement.value == '' ? 1250 : upElement.value;
	
	downElement.value = uciOriginal.get("upnpd", "config", "download");
	downElement.value = downElement.value == '' ? 1250 : downElement.value;

	setUpnpEnabled();
	initializeDescriptionVisibility(uciOriginal, "upnp_help");
	uciOriginal.removeSection("gargoyle", "help"); //necessary, or we over-write the help settings when we save


	if (upnpdEnabled) {
		update_upnp();
		timerid=setInterval("update_upnp()", 10000);
	} else {
		clearInterval(timerid);
		timerid = null;

		var tableData = new Array();
		var tableRow =['***','***********','***** '];
		tableData.push(tableRow);

		var columnNames= [prtS.Prot, prtS.LHst, prtS.Port ]; 
		var upnpTable = createTable(columnNames, tableData, "upnp_table", false, false);
		var tableContainer = document.getElementById('upnp_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(upnpTable);


	}


	
}

function setUpnpEnabled()
{
	enableAssociatedField(document.getElementById("upnp_enabled"), 'upnp_up', document.getElementById('upnp_up').value);
	enableAssociatedField(document.getElementById("upnp_enabled"), 'upnp_down', document.getElementById('upnp_down').value);
}

function setDmzEnabled()
{
	enableAssociatedField(document.getElementById("dmz_enabled"), 'dmz_ip', document.getElementById('dmz_ip').value);
}


function createEditButton(isSingle)
{
	var editButton = createInput("button");
	editButton.value = UI.Edit;
	editButton.className="default_button";
	editButton.onclick = isSingle ? function(){ editForward(true, this); } : function(){ editForward(false, this); } ;
	return editButton;
}

function editForward(isSingle, triggerElement)
{
	if( typeof(editForwardWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editForwardWindow.close();
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


	var editLocation = isSingle ? "single_forward_edit.sh" : "multi_forward_edit.sh";
	editForwardWindow = window.open(editLocation, "edit", "width=560,height=180,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editForwardWindow.document);
	closeButton = createInput("button", editForwardWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
	closeButton.className = "default_button";

	editRow=triggerElement.parentNode.parentNode;

	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(editForwardWindow.document != null)
		{
			if(editForwardWindow.document.getElementById("bottom_button_container") != null)
			{
				editForwardWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editForwardWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
			
				//set edit values
				var r= isSingle ? "" : "r";
				editForwardWindow.document.getElementById("add" + r + "_button").style.display="none";
				editForwardWindow.document.getElementById("add" + r + "_desc").value = editRow.childNodes[0].firstChild.data;
				setSelectedText("add" + r + "_prot", editRow.childNodes[1].firstChild.data, editForwardWindow.document);
				if(isSingle)
				{
					editForwardWindow.document.getElementById("add_fp").value   = editRow.childNodes[2].firstChild.data;
					editForwardWindow.document.getElementById("add_ip").value   = editRow.childNodes[3].firstChild.data;
					editForwardWindow.document.getElementById("add_dp").value   = editRow.childNodes[4].firstChild.data;
				}
				else
				{
					editForwardWindow.document.getElementById("addr_sp").value   = editRow.childNodes[2].firstChild.data;
					editForwardWindow.document.getElementById("addr_ep").value   = editRow.childNodes[3].firstChild.data;
					editForwardWindow.document.getElementById("addr_ip").value   = editRow.childNodes[4].firstChild.data;
				}
				
				closeButton.onclick = function()
				{
					editForwardWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors;
				       	if(isSingle)
					{
						errors = proofreadForwardSingle(editForwardWindow.document, document, editRow);
					}
					else
					{
						errors = proofreadForwardRange(editForwardWindow.document, document, editRow);
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n"+prtS.UpErr);
					}
					else
					{
						//update document with new data
						
						editRow.childNodes[0].firstChild.data = editForwardWindow.document.getElementById("add" + r + "_desc").value;
						editRow.childNodes[1].firstChild.data = getSelectedValue( "add" + r + "_prot", editForwardWindow.document );
						if(isSingle)
						{
							editRow.childNodes[2].firstChild.data = editForwardWindow.document.getElementById("add_fp").value;
							editRow.childNodes[3].firstChild.data = editForwardWindow.document.getElementById("add_ip").value;
							editRow.childNodes[4].firstChild.data = editForwardWindow.document.getElementById("add_dp").value;
						}
						else
						{
							editRow.childNodes[2].firstChild.data = editForwardWindow.document.getElementById("addr_sp").value;
							editRow.childNodes[3].firstChild.data = editForwardWindow.document.getElementById("addr_ep").value;
							editRow.childNodes[4].firstChild.data = editForwardWindow.document.getElementById("addr_ip").value;
						}
						editForwardWindow.close();
					}
				}
				editForwardWindow.moveTo(xCoor,yCoor);
				editForwardWindow.focus();
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

var updateInProgress=false;
var timerid=null;

function update_upnp()
{
	if (!updateInProgress)
	{
		updateInProgress = true;
		var commands="iptables -nL MINIUPNPD | grep ACCEPT"
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var lines = req.responseText.split("\n");
				var tableData = new Array();
				var i;
				var upnpcnt=0;

				if (lines != null)
				{
					for(i = 0; i < lines.length; i++)
					{
						var upnd = lines[i].split(/\s+/);
						if (typeof(upnd[6]) != "undefined") {
						var tableRow =[upnd[1],upnd[4],upnd[6].substr(4)];
						tableData.push(tableRow);
						upnpcnt = upnpcnt+1;
						}
					}

				}

				//Always display at least on blank line
				if (upnpcnt == 0 ) {
					var tableRow =['***','***********','***** '];
					tableData.push(tableRow);
				}

				var columnNames= [prtS.Prot, prtS.LHst, prtS.Port ]; 
				
				var upnpTable = createTable(columnNames, tableData, "upnp_table", false, false);
				var tableContainer = document.getElementById('upnp_table_container');
				if(tableContainer.firstChild != null)
				{
					tableContainer.removeChild(tableContainer.firstChild);
				}
				tableContainer.appendChild(upnpTable);


				updateInProgress = false;
			}
		}

		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}
