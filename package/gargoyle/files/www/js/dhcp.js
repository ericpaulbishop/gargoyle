/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var dhcpS=new Object(); //part of i18n

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

		uci = uciOriginal.clone();
		uci.remove('dhcp', dhcpSection, 'ignore');
		uci.set('dhcp', dhcpSection, 'interface', 'lan');
		dhcpIds =  ['dhcp_start', ['dhcp_start','dhcp_end'], 'dhcp_lease'];
		dhcpVisIds = ['dhcp_start', 'dhcp_end', 'dhcp_lease'];
		dhcpPkgs = ['dhcp','dhcp','dhcp'];
		dhcpSections = [dhcpSection,dhcpSection,dhcpSection];
		dhcpOptions = ['start', 'limit', 'leasetime'];
	
		dhcpFunctions = [setVariableFromValue, setVariableFromCombined, setVariableFromModifiedValue];
		limitParams =  [false, function(values){ return (parseInt(values[1]) - parseInt(values[0]) + 1); }];
		leaseParams = [false, function(value){ return value + "h"; }];
		dhcpParams = [false, limitParams,leaseParams];	
	
		setVariables(dhcpIds, dhcpVisIds, uci, dhcpPkgs, dhcpSections, dhcpOptions, dhcpFunctions, dhcpParams);

		dhcpWillBeEnabled = true;
		if(document.getElementById("dhcp_enabled").checked )
		{
			uci.remove("dhcp", "lan", "ignore");
		}
		else
		{
			uci.set("dhcp", "lan", "ignore", "1");
			dhcpWillBeEnabled = false;
		}
	

		staticIpTable = document.getElementById('staticip_table_container').firstChild;	
		tableData = getTableDataArray(staticIpTable, true, false);
	
		createEtherCommands = [ "touch /etc/ethers", "rm /etc/ethers" ];
		staticIpTableData = new Array();
		for (rowIndex in tableData)
		{
			rowData = tableData[rowIndex];
			createEtherCommands.push("echo \"" + rowData[1].toLowerCase() + "\t" + rowData[2] + "\" >> /etc/ethers");
			staticIpTableData.push( [ rowData[0], rowData[1], rowData[2] ] );
			if(rowData[0] != '-')
			{
				ipHostHash[ rowData[2] ] = rowData[0];
			}
		}

	
		createHostCommands = [ "touch /etc/hosts", "rm /etc/hosts" ];
		for (ip in ipHostHash)
		{
			host= ipHostHash[ip];
			createHostCommands.push("echo \"" + ip + "\t" + host + "\" >> /etc/hosts");
		}

		var firewallCommands = [];
		var firewallDefaultSections = uci.getAllSectionsOfType("firewall", "defaults");
		var oldBlockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "block_static_ip_mismatches") == "1" ? true : false;
		var newBlockMismatches = document.getElementById("block_mismatches").checked;
		if(newBlockMismatches != oldBlockMismatches)
		{
			if(newBlockMismatches)
			{ 
				uci.set("firewall", firewallDefaultSections[0], "block_static_ip_mismatches", "1");
				firewallCommands.push("uci set firewall.@defaults[0].block_static_ip_mismatches=1");
			}
			else
			{
				uci.remove("firewall", firewallDefaultSections[0], "block_static_ip_mismatches");
				firewallCommands.push("uci del firewall.@defaults[0].block_statip_mismatches");
			}
			firewallCommands.push("uci commit");
		}


		//need to restart firewall here because for add/remove of static ips, we need to restart bandwidth monitor, as well as for firewall commands above if we have any
		var restartDhcpCommand = "\n/etc/init.d/dnsmasq restart ; \nsh /usr/lib/gargoyle/restart_firewall.sh\n" ; 

		commands = uci.getScriptCommands(uciOriginal) + "\n" + createEtherCommands.join("\n") + "\n" + createHostCommands.join("\n") + "\n" + firewallCommands.join("\n") + "\n" + restartDhcpCommand ;

		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				dhcpEnabled = dhcpWillBeEnabled;
				dhcpWillBeEnabled = null;
				resetData();
				setControlsEnabled(true);
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function createEditButton()
{
	var editButton = createInput("button");
	editButton.value = UI.Edit;
	editButton.className="default_button";
	editButton.onclick = editStatic;
	return editButton;
}

function resetData()
{
	dhcpEnabled = uciOriginal.get("dhcp", "lan", "ignore") == "1" ? false : true;
	var rowIndex=0;
	for(rowIndex=0; rowIndex < staticIpTableData.length ; rowIndex++)
	{
		var rowData = staticIpTableData[rowIndex];
		rowData.push(createEditButton());
		staticIpTableData[rowIndex] = rowData;
	}
	columnNames=[UI.HsNm, 'MAC', 'IP', ''];
	staticIpTable=createTable(columnNames, staticIpTableData, "static_ip_table", true, false, removeStaticIp );
	tableContainer = document.getElementById('staticip_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(staticIpTable);


	dhcpIds =  ['dhcp_start', 'dhcp_end', 'dhcp_lease'];
	dhcpPkgs = ['dhcp',['dhcp','dhcp'],'dhcp'];
	dhcpSections = [dhcpSection,[dhcpSection,dhcpSection],dhcpSection];
	dhcpOptions = ['start', ['start','limit'], 'leasetime'];
	
	enabledTest = function(value){return value != 1;};
	endCombineFunc= function(values) { return (parseInt(values[0])+parseInt(values[1])-1); };
	leaseModFunc = function(value)
	{
		var leaseHourValue;
		if(value.match(/.*h/))
		{
			leaseHourValue=value.substr(0,value.length-1);
		}
		else if(value.match(/.*m/))
		{
			leaseHourValue=value.substr(0,value.length-1)/(60);
		}
		else if(value.match(/.*s/))
		{
			leaseHourValue=value.substr(0,value.length-1)/(60*60);
		}
		return leaseHourValue;  
	};
	
	dhcpParams = [100, [endCombineFunc,150],[12,leaseModFunc]];
	dhcpFunctions = [loadValueFromVariable, loadValueFromMultipleVariables, loadValueFromModifiedVariable];

	loadVariables(uciOriginal, dhcpIds, dhcpPkgs, dhcpSections, dhcpOptions, dhcpParams, dhcpFunctions);


	document.getElementById("dhcp_enabled").checked = dhcpEnabled;
	setEnabled(document.getElementById('dhcp_enabled').checked);

	var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");
	var blockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "block_static_ip_mismatches") == "1" ? true : false;
	document.getElementById("block_mismatches").checked = blockMismatches;


	//setup hostname/mac list
	resetHostnameMacList();
}

function removeStaticIp(table, row)
{
	var removedIp = row.childNodes[2].firstChild.data
	delete ipHostHash[removedIp]
	resetHostnameMacList()

}

function resetHostnameMacList()
{
	var staticTable = document.getElementById("staticip_table_container").firstChild;
	var staticTableData = staticTable == null ? [] : getTableDataArray(staticTable, true, false);
	var staticMacs = [];
	var staticIndex=0;
	for(staticIndex=0; staticIndex < staticTableData.length; staticIndex++)
	{
		var mac = (staticTableData[staticIndex][1]).toUpperCase();
		staticMacs[ mac ] = 1;
	}

	var hmVals = [ "none" ];
	var hmText = [ dhcpS.SelH ];
	var leaseIndex = 0;
	for(leaseIndex=0; leaseIndex < leaseData.length; leaseIndex++)
	{
		var lease = leaseData[leaseIndex];
		var mac = (lease[0]).toUpperCase();
		if( staticMacs[ mac ] == null )
		{
			hmVals.push( lease[2] + "," + mac );
			hmText.push( (lease[2] == "" || lease[2] == "*" ? lease[1] : lease[2] ) + " (" + mac + ")" );
		}
	}
	setAllowableSelections("static_from_connected", hmVals, hmText);
	
	var hmEnabled = hmText.length > 1 && document.getElementById('dhcp_enabled').checked ? true : false;
	setElementEnabled(document.getElementById("static_from_connected"), hmEnabled, "none");

}



function staticFromConnected()
{
	var selectedVal = getSelectedValue("static_from_connected");
	if(selectedVal != "none")
	{
		var host = (selectedVal.split(/,/))[0];
		var mac  = (selectedVal.split(/,/))[1];
		document.getElementById("add_host").value = host;
		document.getElementById("add_mac").value  = mac;
		setSelectedValue("static_from_connected", "none");
	}
}


function setEnabled(enabled)
{
	var ids=['dhcp_start', 'dhcp_end', 'dhcp_lease', 'block_mismatches', 'add_host', 'add_mac', 'add_ip', 'add_button'];
	var idIndex;
	for (idIndex in ids)
	{
		var element = document.getElementById(ids[idIndex]);
		setElementEnabled(element, enabled, "");
	}

	var staticIpTable = document.getElementById('staticip_table_container').firstChild;
	setRowClasses(staticIpTable, enabled);
	
	resetHostnameMacList();

	
}

function addStatic()
{
	errors = proofreadStatic(document);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + dhcpS.AErr);
	}
	else
	{
		values = new Array();
		ids = ['add_host', 'add_mac', 'add_ip'];
		for (idIndex in ids)
		{
			v = document.getElementById(ids[idIndex]).value;
			v = v== '' ? '-' : v;
			values.push(v);
			document.getElementById(ids[idIndex]).value = "";
		}
		values.push(createEditButton());
		staticIpTable = document.getElementById('staticip_table_container').firstChild;
		addTableRow(staticIpTable,values, true, false, resetHostnameMacList);
		resetHostnameMacList();
	}
}

function proofreadStatic(controlDocument, tableDocument, excludeRow)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	tableDocument = tableDocument == null ? document : tableDocument;
	
	addIds=['add_mac', 'add_ip'];
	labelIds= ['add_mac_label', 'add_ip_label'];
	functions = [validateMac, validateIP];
	returnCodes = [0,0];
	visibilityIds=addIds;
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, controlDocument);
	if(errors.length == 0)
	{
		var staticIpTable = tableDocument.getElementById('staticip_table_container').firstChild;
		var currentData = getTableDataArray(staticIpTable, true, false);
		var rowDataIndex = 0;
		for (rowDataIndex=0; rowDataIndex < currentData.length ; rowDataIndex++)
		{
			if(staticIpTable.rows[rowDataIndex+1] != excludeRow)
			{
				rowData = currentData[rowDataIndex];
				if(rowData[0] != '' && rowData[0] != '-' && rowData[0] == controlDocument.getElementById('add_host').value)
				{
					errors.push(dhcpS.dHErr);
				}
				if(rowData[1] == controlDocument.getElementById('add_mac').value)
				{
					errors.push(dhcpS.dMErr);
				}
				if(rowData[2] == controlDocument.getElementById('add_ip').value)
				{
					errors.push(dhcpS.dIPErr);
				}
			}
		}
	}
	if(errors.length == 0)
	{
		var dhcpSection = getDhcpSection(uciOriginal);
		var mask = uciOriginal.get("network", "lan", "netmask");
		var ip = uciOriginal.get("network", "lan", "ipaddr");
		var testIp = controlDocument.getElementById('add_ip').value;
		var testEnd = parseInt( (testIp.split("."))[3] );

		if(!rangeInSubnet(mask, ip, testEnd, testEnd))
		{
			errors.push(dhcpS.subErr);
		}
		if(ip == testIp)
		{
			errors.push(dhcpS.ipErr);
		}	
	}
	return errors;
}

function proofreadAll()
{
	dhcpIds = ['dhcp_start', 'dhcp_end', 'dhcp_lease'];
	labelIds= ['dhcp_start_label', 'dhcp_end_label', 'dhcp_lease_label'];
	functions = [validateNumeric, validateNumeric, validateNumeric];
	returnCodes = [0,0,0];
	visibilityIds= dhcpIds;
	errors = proofreadFields(dhcpIds, labelIds, functions, returnCodes, visibilityIds);

	//test that dhcp range is within subnet
	if(errors.length == 0 && document.getElementById("dhcp_enabled").checked)
	{
		var dhcpSection = getDhcpSection(uciOriginal);
		var mask = uciOriginal.get("network", "lan", "netmask");
		var ip = uciOriginal.get("network", "lan", "ipaddr");
		var start = parseInt(document.getElementById("dhcp_start").value);
		var end = parseInt(document.getElementById("dhcp_end").value );
		if(!rangeInSubnet(mask, ip, start, end))
		{
			errors.push(dhcpS.dsubErr);
		}
		
		var ipEnd = parseInt( (ip.split("."))[3] );
		if(ipEnd >= start && ipEnd <= end)
		{
			errors.push(dhcpS.dipErr);
		}
	}

	return errors;
}

function editStatic()
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


	editStaticWindow = window.open("static_ip_edit.sh", "edit", "width=560,height=180,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editStaticWindow.document);
	closeButton = createInput("button", editStaticWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
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
				editStaticWindow.document.getElementById("add_host").value = editRow.childNodes[0].firstChild.data;
				editStaticWindow.document.getElementById("add_mac").value  = editRow.childNodes[1].firstChild.data;
				editStaticWindow.document.getElementById("add_ip").value   = editRow.childNodes[2].firstChild.data;
				editStaticWindow.document.getElementById("add_button").style.display="none";				
				closeButton.onclick = function()
				{
					editStaticWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = proofreadStatic(editStaticWindow.document, document, editRow);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n"+dhcpS.upErr);
					}
					else
					{
						//update document with new data
						editRow.childNodes[0].firstChild.data = editStaticWindow.document.getElementById("add_host").value;
						editRow.childNodes[1].firstChild.data = editStaticWindow.document.getElementById("add_mac").value;
						editRow.childNodes[2].firstChild.data = editStaticWindow.document.getElementById("add_ip").value;
						
						editStaticWindow.close();

						resetHostnameMacList();

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

