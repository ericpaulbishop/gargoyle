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
		limitParams =  [false, function(values){ return (1*values[1] + 1*1 - 1*values[0]); }];
		leaseParams = [false, function(value){ return value + "h"; }];
		dhcpParams = [false, limitParams,leaseParams];	
	
		setVariables(dhcpIds, dhcpVisIds, uci, dhcpPkgs, dhcpSections, dhcpOptions, dhcpFunctions, dhcpParams);

		setEnabledCommand = "";
		dhcpWillBeEnabled = true;
		if(document.getElementById("dhcp_enabled").checked )
		{
			setEnabledCommand = "/etc/init.d/dnsmasq enable\n";
		}
		else
		{
			setEnabledCommand = "/etc/init.d/dnsmasq disable\n";
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

		var restartNetworkCommand = "\nsh " + gargoyleBinRoot + "/utility/restart_network.sh ;\n";
		commands = uci.getScriptCommands(uciOriginal) + "\n" + setEnabledCommand + "\n" + createEtherCommands.join("\n") + "\n" + createHostCommands.join("\n") + restartNetworkCommand ;
		

		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands);
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

function resetData()
{
	columnNames=['Hostname', 'MAC', 'IP'];
	staticIpTable=createTable(columnNames, staticIpTableData, "static_ip_table", true, false);
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
	endCombineFunc= function(values) { return (1*values[0])+(1*values[1])-1; };
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

}

function setEnabled(enabled)
{
	var disabled = (enabled != true);
	ids=['dhcp_start', 'dhcp_end', 'dhcp_lease', 'add_host', 'add_mac', 'add_ip', 'add_button'];
	for (idIndex in ids)
	{
		element = document.getElementById(ids[idIndex]);
		element.disabled=disabled;
		element.readonly=disabled;
		element.style.color = disabled==true ? "#AAAAAA" : "#000000";
	}
	if(enabled)
	{
		document.getElementById('add_button').className='default_button';
	}
	else
	{
		document.getElementById('add_button').className='default_button_disabled';
	}
	

	staticIpTable = document.getElementById('staticip_table_container').firstChild;
	setRowClasses(staticIpTable, enabled);
	
}

function addStatic()
{
	errors = proofreadAdd();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add row.");
	}
	else
	{
		values = new Array();
		ids = ['add_host', 'add_mac', 'add_ip'];
		for (idIndex in ids)
		{
			v = document.getElementById(ids[idIndex]).value;
			v = v== '' ? '-' : v;
			values.push(v);;
			document.getElementById(ids[idIndex]).value = "";
		}
		staticIpTable = document.getElementById('staticip_table_container').firstChild;
		addTableRow(staticIpTable,values, true, false);

	}
}

function proofreadAdd()
{
	addIds=['add_mac', 'add_ip'];
	labelIds= ['add_mac_label', 'add_ip_label'];
	functions = [validateMac, validateIP];
	returnCodes = [0,0];
	visibilityIds=addIds;
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds);
	if(errors.length == 0)
	{
		staticIpTable = document.getElementById('staticip_table_container').firstChild;
		currentData = getTableDataArray(staticIpTable, true, false);
		for (rowDataIndex in currentData)
		{
			rowData = currentData[rowDataIndex];
			if(rowData[0] != '' && rowData[0] != '-' && rowData[0] == document.getElementById('add_host').value)
			{
				errors.push("duplicate Hostname");
			}
			if(rowData[1] == document.getElementById('add_mac').value)
			{
				errors.push("duplicate MAC");
			}
			if(rowData[2] == document.getElementById('add_ip').value)
			{
				errors.push("duplicate IP address");
			}
		}
	}
	if(errors.length == 0)
	{
		var dhcpSection = getDhcpSection(uciOriginal);
		var mask = uciOriginal.get("network", "lan", "netmask");
		var ip = uciOriginal.get("network", "lan", "ipaddr");
		var testIp = document.getElementById('add_ip').value;
		var testEnd = parseInt( (testIp.split("."))[3] );

		if(!rangeInSubnet(mask, ip, testEnd, testEnd))
		{
			errors.push("Specified static IP falls outside LAN subnet.");
		}
		if(ip == testIp)
		{
			errors.push("Specified static IP is current router IP.");
		}	
	}

	return errors;

}

function proofreadAll()
{
	dhcpIds = ['dhcp_start', 'dhcp_end', 'dhcp_lease'];
	labelIds= ['dhcp_start_label', 'dhcp_end_label, dhcp_lease_label'];
	functions = [validateNumeric, validateNumeric, validateNumeric];
	returnCodes = [0,0,0];
	visibilityIds= dhcpIds;
	errors = proofreadFields(dhcpIds, labelIds, functions, returnCodes, visibilityIds);

	//test that dhcp range is within subnet
	if(errors.length == 0)
	{
		var dhcpSection = getDhcpSection(uciOriginal);
		var mask = uciOriginal.get("network", "lan", "netmask");
		var ip = uciOriginal.get("network", "lan", "ipaddr");
		var start = parseInt(document.getElementById("dhcp_start").value);
		var end = parseInt(document.getElementById("dhcp_end").value );
		if(!rangeInSubnet(mask, ip, start, end))
		{
			errors.push("Specified DHCP range falls outside LAN subnet.");
		}
		
		var ipEnd = parseInt( (ip.split("."))[3] );
		if(ipEnd >= start && ipEnd <= end)
		{
			errors.push("Specified DHCP range contains current LAN IP.");
		}
	}

	return errors;
}
