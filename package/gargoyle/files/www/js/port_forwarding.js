/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		document.body.style.cursor="wait";
		document.getElementById("save_button").style.display="none";
		document.getElementById("reset_button").style.display="none";
		document.getElementById("update_container").style.display="block";



		/*
		* IN PORTF FIREWALL DATA:
		* 	index0=name 
		*	index1=protocol
		* 	index2=multiport forward (true/false)
		* 	index3=from port (start)
		* 	index4=to port / end source port range (depending on whether this is a multi-port forward)
		* 	index5=destination ip
		* 	index6=enabled (true/false)
		*/
		newPortfData = [];
		portfTable = document.getElementById('portf_table_container').firstChild;	
		tableData = getTableDataArray(portfTable, true, false);
		for(rowIndex = 0; rowIndex < tableData.length; rowIndex++)
		{
			rowData = tableData[rowIndex];
			newPortfData.push( [rowData[0], rowData[1], false, rowData[2], rowData[4], rowData[3], rowData[5].checked] );	
		}
		
		rangeTable = document.getElementById('portfrange_table_container').firstChild;	
		tableData = getTableDataArray(rangeTable, true, false);
		for(rowIndex = 0; rowIndex < tableData.length; rowIndex++)
		{
			rowData = tableData[rowIndex];
			newPortfData.push( [rowData[0], rowData[1], true, rowData[2], rowData[3], rowData[4], rowData[5].checked] );	
		}
		firewallData[1] = newPortfData;
		createFirewallCommands = getFirewallWriteCommands(firewallData, currentLanIp);
		restartFirewallCommand = "\nsh " + gargoyleBinRoot + "/utility/restart_firewall.sh ;\n";


		//upnp
		upnpStartCommands = new Array();
		upnpStartCommands.push("/etc/init.d/miniupnpd stop");
		uci = uciOriginal.clone();
		upnpdEnabled = document.getElementById("upnp_enabled").checked;
		if(upnpdEnabled)
		{
			upnpStartCommands.push("/etc/init.d/miniupnpd enable");
			uci.set("upnpd", "config", "upload", document.getElementById("upnp_up").value);
			uci.set("upnpd", "config", "download", document.getElementById("upnp_down").value);
			upnpStartCommands.push(uci.getScriptCommands(uciOriginal));
			upnpStartCommands.push("/etc/init.d/miniupnpd start");
		}
		else
		{
			upnpStartCommands.push("/etc/init.d/miniupnpd disable");
		}
	


		commands = createFirewallCommands.join("\n") + restartFirewallCommand + upnpStartCommands.join("\n");
		//document.getElementById("output").value = commands;

		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				document.getElementById("update_container").style.display="none";		
				document.getElementById("save_button").style.display="inline";
				document.getElementById("reset_button").style.display="inline";
				document.body.style.cursor='auto';
		
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	controlIds=['upnp_up', 'upnp_down'];
	labelIds= ['upnp_up_label', 'upnp_down_label'];
	functions = [validateNumeric, validateNumeric];
	returnCodes = [0,0];
	visibilityIds=controlIds;
	errors = proofreadFields(controlIds, labelIds, functions, returnCodes, visibilityIds);
	return errors;
}

function addPortfRule()
{
	errors = proofreadAdd();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add forwarding rule.");
	}
	else
	{
		values = new Array();
		ids = ['add_app', 'add_prot', 'add_fp', 'add_ip', 'add_dp'];
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
		//if so, just merge the two by setting the protocol on the old data to 'Both'
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

				portfTable.rows[(rowDataIndex*1)+1].childNodes[1].firstChild.data = 'Both';
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
			addTableRow(portfTable,values, true, false);
		}
	}
}



function addPortfRangeRule()
{
	errors = proofreadAddRange();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add forwarding rule.");
	}
	else
	{
		values = new Array();
		ids = ['addr_app', 'addr_prot', 'addr_sp', 'addr_ep', 'addr_ip'];
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
				portfRangeTable.rows[(rowDataIndex*1)+1].childNodes[1].firstChild.data = 'Both';
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

			portfrangeTable = document.getElementById('portfrange_table_container').firstChild;
			addTableRow(portfrangeTable,values, true, false);
		}
	}
}

function proofreadAddRange()
{
	addIds = ['addr_sp', 'addr_ep', 'addr_ip'];
	labelIds = ['addr_sp_label', 'addr_ep_label', 'addr_ip_label'];
	functions = [validateNumeric, validateNumeric, validateIP];
	returnCodes = [0,0,0];
	visibilityIds = addIds;
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds);
	if(errors.length == 0)
	{
		if( (1*document.getElementById('addr_sp').value) > (1*document.getElementById('addr_ep').value) )
		{
			errors.push("Start Port > End Port");
		}
		
		
		portfTable = document.getElementById('portf_table_container').firstChild;
		currentPortfData = getTableDataArray(portfTable, true, false);
		addStartPort = document.getElementById('addr_sp').value;
		addEndPort = document.getElementById('addr_ep').value;
		addProtocol = document.getElementById('addr_prot').value;
		for (rowDataIndex in currentPortfData)
		{
			rowData = currentPortfData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == 'Both' || rowData[1] == 'Both') &&  addStartPort*1 <= rowData[2]*1 && addEndPort*1 >= rowData[2]*1 )
			{
				errors.push("Port(s) Within Range Is/Are Already Being Forwarded");
			}
		}

		portfRangeTable = document.getElementById('portfrange_table_container').firstChild;
		currentRangeData = getTableDataArray(portfRangeTable, true, false);
		for (rowDataIndex in currentRangeData)
		{
			rowData = currentRangeData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == 'Both' || rowData[1] == 'Both') && rowData[2]*1 <= addEndPort*1 && rowData[3]*1 >= addStartPort*1)
			{
				errors.push("Port(s) Within Range Is/Are Already Being Forwarded");
			}
		}
	}

	
	return errors;

}

function proofreadAdd()
{
	
	addIds = ['add_fp', 'add_ip'];
	labelIds = ['add_fp_label', 'add_ip_label', 'add_dp_label'];
	functions = [validateNumeric, validateIP, validateNumeric];
	returnCodes = [0,0,0];
	visibilityIds = addIds;
	if(document.getElementById('add_dp').value.length > 0)
	{
		addIds.push('add_dp');
	}
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds);



	if(errors.length == 0)
	{
		portfTable = document.getElementById('portf_table_container').firstChild;
		currentPortfData = getTableDataArray(portfTable, true, false);
		addPort = document.getElementById('add_fp').value;
		addProtocol = document.getElementById('add_prot').value;
		for (rowDataIndex in currentPortfData)
		{
			rowData = currentPortfData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == 'Both' || rowData[1] == 'Both') &&  addPort == rowData[2])
			{
				errors.push("Port Is Already Being Forwarded");
			}
		}

		portfRangeTable = document.getElementById('portfrange_table_container').firstChild;
		currentRangeData = getTableDataArray(portfRangeTable, true, false);
		for (rowDataIndex in currentRangeData)
		{
			rowData = currentRangeData[rowDataIndex];
			if( (addProtocol == rowData[1] || addProtocol == 'Both' || rowData[1] == 'Both') && rowData[2]*1 <= addPort*1 && rowData[3]*1 >= addPort*1)
			{
				errors.push("Port Is Already Being Forwarded");
			}
		}
	}

	return errors;
}

function resetData()
{
	//asume firewallData already initialized
	portfData = firewallData[1];	
	
	singlePortTableData = new Array();
	portRangeTableData = new Array();
	singlePortEnabledStatus = new Array();
	portRangeEnabledStatus = new Array();
	/*
	* 	index0=name 
	*	index1=protocol
	* 	index2=multiport forward (true/false)
	* 	index3=from port (start)
	* 	index4=to port / end source port range (depending on whether this is a multi-port forward)
	* 	index5=destination ip
	* 	index6=enabled (true/false)
	*/
	for(portfIndex=0; portfIndex < portfData.length; portfIndex++)
	{
		portf = portfData[portfIndex];
		checkbox = createInput('checkbox');
		checkbox.checked = portf[6];
	
		if(portf[2]) //if range
		{
			portRangeTableData.push([portf[0], portf[1], portf[3], portf[4], portf[5],checkbox]);
			portRangeEnabledStatus.push(checkbox.checked);
		}
		else
		{
			singlePortTableData.push([portf[0], portf[1], portf[3], portf[5], portf[4],checkbox]);
			singlePortEnabledStatus.push(checkbox.checked);
		}	
	}



	columnNames = ['Application', 'Protocol', 'From Port', 'To IP', 'To Port', 'Enabled']
	portfTable=createTable(columnNames, singlePortTableData, "portf_table", true, false);
	table1Container = document.getElementById('portf_table_container');
	
	if(table1Container.firstChild != null)
	{
		table1Container.removeChild(table1Container.firstChild);
	}
	table1Container.appendChild(portfTable);
	
	
	
	

	columnNames = ['Application', 'Protocol', 'Start Port', 'End Port', 'To IP', 'Enabled']
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



	clearIds = ['add_app', 'add_fp', 'add_ip', 'add_dp', 'addr_app', 'addr_sp', 'addr_ep', 'addr_ip'];
	for(clearIndex = 0; clearIndex < clearIds.length; clearIndex++)
	{
		document.getElementById(clearIds[clearIndex]).value = '';
	}

	//upnp
	document.getElementById("upnp_enabled").checked = upnpdEnabled;
	upElement = document.getElementById("upnp_up");
	downElement = document.getElementById("upnp_down");
	
	upElement.value = uciOriginal.get("upnpd", "config", "upload");
	upElement.value = upElement.value == '' ? 512 : upElement.value;
	
	downElement.value = uciOriginal.get("upnpd", "config", "download");
	downElement.value = downElement.value == '' ? 1024 : downElement.value;

	setUpnpEnabled();
	
}

function setUpnpEnabled()
{
	enableAssociatedField(document.getElementById("upnp_enabled"), 'upnp_up', document.getElementById('upnp_up').value);
	enableAssociatedField(document.getElementById("upnp_enabled"), 'upnp_down', document.getElementById('upnp_down').value);
}

