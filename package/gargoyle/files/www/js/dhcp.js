/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var dhcpS=new Object(); //part of i18n
var TSort_Data = new Array ('static_ip_table', 's', 's', 'p', '', '');

function saveChanges()
{
	errorList = proofreadDhcpForm();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		uci = uciOriginal.clone();

		// save dhcp changes
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

		// save Device changes to uci
		uci.removeAllSectionsOfType("dhcp", "host");
		var etherMap = map(etherData, 0);
		var hostMap = map(hostData, 0);
		var macMap = new Object();
		deviceTable = document.getElementById('device_table_container').firstChild;
		tableData = getTableDataArray(deviceTable, true, false);
		for (rowIndex = 0; rowIndex < tableData.length; rowIndex++)
		{
			rowData = tableData[rowIndex];
			var host = rowData[0];
			var macs = rowData[1];
			var ip = rowData[2];
			if (uci.get("dhcp", host).length == 0){
				uci.set("dhcp", host, null, "host");
				uci.set("dhcp", host, "name", host);
			}
			uci.set("dhcp", host, "mac", macs);
			uci.set("dhcp", host, "ip", ip);
			macMap[host]=macs;

			// remove devices moved to uci from /etc/ethers & /etc/hosts
			var macList = macs.split("\t");
			for (mIndex = 0; mIndex < macList.length; mIndex++)
			{
				uciMac = macList[mIndex];
				if (etherMap.hasOwnProperty(uciMac))
				{
					etherIP = etherMap[uciMac][1];
					delete etherMap[uciMac];
					if (hostMap.hasOwnProperty(etherIP))
					{
						delete hostMap[etherIP];
					}
				}
			}
		}

		// recreate /etc/ethers without redundant entries
		createEtherCommands = [ "touch /etc/ethers", "rm /etc/ethers" ];
		for (mac in etherMap)
		{
			if (etherMap.hasOwnProperty(mac))
			{
				createEtherCommands.push("echo \"" + mac.toLowerCase() + "\t" + etherMap[mac][1] + "\" >> /etc/ethers");
			}
		}

		// recreate /etc/hosts without redundant entries
		createHostCommands = [ "touch /etc/hosts", "rm /etc/hosts" ];
		for (ip in hostMap)
		{
			if (hostMap.hasOwnProperty(ip))
			{
				createHostCommands.push("echo \"" + ip + "\t" + hostMap[ip][1] + "\" >> /etc/hosts");
			}
		}

		// save Group changes
		uci.removeAllSectionsOfType("dhcp", "mac");
		var ipsetCommands = ["ipset destroy"]; // fails on ipsets with existing references

		groupTable = document.getElementById('group_table_container').firstChild;
		tableData = getTableDataArray(groupTable, true, false);
		for (rowIndex = 0; rowIndex < tableData.length; rowIndex++)
		{
			rowData = tableData[rowIndex];
			var group = rowData[0];
			var devices = rowData[1].split(" ");

			if (uci.get("dhcp", group).length == 0){
				uci.set("dhcp", group, null, "mac");
				uci.set("dhcp", group, "networkid", group);
			}

			for(dIndex=0; dIndex < devices.length; dIndex++)
			{
				var host = devices[dIndex];
				if(macMap.hasOwnProperty(host))
				{
					var macs = macMap[host].split(" ");
					for (mIndex=0; mIndex < macs.length; mIndex++)
					{
						uci.set("dhcp", group, "mac", macs[mIndex]);
					}
					uci.set("dhcp", host, "group", group);
				}
			}

			// create ipset
			ipsetCommands.push("ipset create " + group + " iphash");
		}

		// save blockMismatches changes
		var firewallCommands = [];
		var firewallDefaultSections = uci.getAllSectionsOfType("firewall", "defaults");
		var oldBlockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "enforce_dhcp_assignments") == "1" ? true : false;
		var newBlockMismatches = document.getElementById("block_mismatches").checked;
		if(newBlockMismatches != oldBlockMismatches)
		{
			if(newBlockMismatches)
			{
				uci.set("firewall", firewallDefaultSections[0], "enforce_dhcp_assignments", "1");
				firewallCommands.push("uci set firewall.@defaults[0].enforce_dhcp_assignments=1");
			}
			else
			{
				uci.remove("firewall", firewallDefaultSections[0], "enforce_dhcp_assignments");
				firewallCommands.push("uci del firewall.@defaults[0].enforce_dhcp_assignments");
			}
			firewallCommands.push("uci commit");
		}

		commands = uci.getScriptCommands(uciOriginal) + "\n";
		commands += ipsetCommands.join("\n") + "\n";
		commands += createEtherCommands.join("\n") + "\n";
		commands += createHostCommands.join("\n")  + "\n";
		commands += firewallCommands.join("\n") + "\n";

		//need to restart firewall here because for add/remove of static ips, we need to restart bandwidth monitor, as well as for firewall commands above if we have any
		commands += "\n/etc/init.d/dnsmasq restart ; \nsh /usr/lib/gargoyle/restart_firewall.sh\n" ;

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


function createEditButton(onclick)
{
	var editButton = createInput("button");
	editButton.value = UI.Edit;
	editButton.className="default_button";
	editButton.onclick = onclick;
	return editButton;
}


function resetData()
{
	resetDhcp();

	var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");
	var blockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "block_static_ip_mismatches") == "1" ? true : false;
	document.getElementById("block_mismatches").checked = blockMismatches;

	resetDeviceTable();
	resetGroupTable();

	resetMacList();
	resetGroupList();
	resetDeviceList();

	setEnabled(document.getElementById('dhcp_enabled').checked);

	var host = document.getElementById("add_host").value = "";
	var macs = document.getElementById("add_mac").value = "";
	var group = document.getElementById("add_group").value = "";
	var devices = document.getElementById("add_device").value = "";
}

function resetDhcp()
{
		dhcpEnabled = uciOriginal.get("dhcp", "lan", "ignore") == "1" ? false : true;
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
}

function resetDeviceTable()
{
	var deviceTableData = new Array();

	// process uci dhcp hosts
	var hosts = uciOriginal.getAllSectionsOfType("dhcp", "host");
	var uciMacs = [];
	for (hIndex=0; hIndex < hosts.length; hIndex++)
	{	// process the MAC's assigned to each device
		var host = hosts[hIndex];
		var macs = uciOriginal.get("dhcp", host, "mac");
		var ip = uciOriginal.get("dhcp", host, "ip");
		ip = ip == null ? "" : ip;
		deviceTableData.push([host, macs, ip, createEditButton(editDevice)]);
		uciMacs = uciMacs.concat(macs.split(" "));
	}

	// process /etc/hosts & /etc/ethers
	var etherMap = map(etherData, 0);
	var hostMap = map(hostData, 0);
	for (mac in etherMap)
	{
		ucMAC = mac.toUpperCase();
		if (etherMap.hasOwnProperty(mac) && uciMacs.indexOf(ucMAC) == -1)
		{
			ip = etherMap[mac][1];
			host = (hostMap.hasOwnProperty(ip)) ? hostMap[ip][1] : "";
			deviceTableData.push([host, ucMAC, ip, createEditButton(editDevice)]);
		}
	}

	// create the device Table and place it into the document
	var columnNames=[dhcpS.DevNm, "MACs", dhcpS.StcIP, ''];
	var deviceTable=createTable(columnNames, deviceTableData, "device_table", true, false, removeDevice );
	var tableContainer = document.getElementById('device_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(deviceTable);
}


function resetGroupTable()
{
	var groupTableData = new Array();

	var groups = new Object();
	var hosts = uciOriginal.getAllSectionsOfType("dhcp", "host");
	for (hIndex=0; hIndex < hosts.length; hIndex++)
	{	// survey all of the devices and groups
		var host = hosts[hIndex];
		var group = uciOriginal.get("dhcp", host, "group");
		if (group != null && group.length > 0)
		{
			if (groups.hasOwnProperty(group))
			{
				groups[group].push(host);
			}
			else
			{
				groups[group] = [host];
			}
		}
	}

	var firewallDefaultSections = uciOriginal.getAllSectionsOfType("firewall", "defaults");
	var blockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "enforce_dhcp_assignments") == "1" ? true : false;
	document.getElementById("block_mismatches").checked = blockMismatches;

	for (var group in groups)
	{	// place each group in an array
    if (groups.hasOwnProperty(group))
		{	// with a list of member devices
			groupTableData.push([group, groups[group].join(" "), createEditButton(editGroup)]);
		}
	}

	// create the group Table and place it into the document
	var columnNames=[dhcpS.GpNm, dhcpS.DevNms, ''];
	var groupTable=createTable(columnNames, groupTableData, "group_table", true, false, removeGroup );
	var tableContainer = document.getElementById('group_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(groupTable);
}



/**
*	Populates a Select id=mac_list with know MACs which have not yet been assigned
* to a Known Device.
*/
function resetMacList()
{
	var kmMacIndex = 0;
	var kmHostIndex = 2;
	var knownMac = knownMacLookup();

	var deviceTable = document.getElementById("device_table_container").firstChild;
	var dtdMacIx = 1;
	var deviceTableData = (deviceTable == null) ? [] : getTableDataArray(deviceTable, true, false);
	var deviceMacs = '';
	for (dtdIndex=0; dtdIndex < deviceTableData.length; dtdIndex++){
		deviceMacs = deviceMacs.concat(deviceTableData[dtdIndex][dtdMacIx], ",");
	}

	var hlVals = [ "" ];
	var hlText = [ dhcpS.SelM ];
	for (var mac in knownMac)
	{
    	if (knownMac.hasOwnProperty(mac))
		{
			if( deviceMacs.indexOf(mac) == -1 )
			{ // exclude MAC's that are already assigned to a device
				var host = knownMac[mac][kmHostIndex];
				hlVals.push( host + "," + mac );
				hlText.push((host == "" || host == "*" ? mac : host ) + " " + mac);
			}
		}
	}

	setAllowableSelections("mac_list", hlVals, hlText, document);
}

/**
*	Populates a Select id=group_list with know MACs which have not yet been assigned
* to a Known Device.
*/
function resetGroupList()
{
	var groups = [];
	var hosts = uciOriginal.getAllSectionsOfType("dhcp", "host");
	for (hIndex=0; hIndex < hosts.length; hIndex++)
	{	// survey all of the devices and groups
		var host = hosts[hIndex];
		var group = uciOriginal.get("dhcp", host, "group");
		if (group.length > 0  && groups.indexOf(group) == -1)
		{
			groups.push(group);
		}
	}

	var gpVals = [ "" ];
	var gpText = [ dhcpS.SelG ];
	for (gIx = 0; gIx < groups.length; gIx++)
	{
		var group = groups[gIx];
		gpVals.push( group );
		gpText.push( group);
	}
	setAllowableSelections("group_list", gpVals, gpText, document);
}

/**
*	Populates a Select id=device_list with know MACs which have not yet been assigned
* to a Known Device.
*/
function resetDeviceList()
{
	var gpVals = [ "" ];
	var gpText = [ dhcpS.SelD ];

	var hosts = uciOriginal.getAllSectionsOfType("dhcp", "host");
	for (hIndex=0; hIndex < hosts.length; hIndex++)
	{	// survey all of the devices and groups
		var host = hosts[hIndex];
		var group = uciOriginal.get("dhcp", host, "group");
		if (group == null || group.length == 0)
		{
			gpVals.push( host );
			gpText.push( host );
		}
	}
	setAllowableSelections("device_list", gpVals, gpText, document);
}


function setEnabled(enabled)
{
	var ids=['dhcp_start', 'dhcp_end', 'dhcp_lease', 'block_mismatches', 'add_host', 'add_mac', 'add_ip', 'add_device_button', 'mac_list', 'add_group', 'add_device', 'add_device_to_group_button', 'device_list'];
	var idIndex;
	for (idIndex in ids)
	{
		var element = document.getElementById(ids[idIndex]);
		setElementEnabled(element, enabled, "");
	}

	var deviceTable = document.getElementById('device_table_container').firstChild;
	setRowClasses(deviceTable, enabled);
	resetDeviceList();

	var groupTable = document.getElementById('group_table_container').firstChild;
	setRowClasses(groupTable, enabled);
	resetGroupList();
}



/*******************************************************************************
* Event functions
*******************************************************************************/


function macSelected()
{
	var selectedVal = getSelectedValue("mac_list");
	var host = document.getElementById("add_host");
	var macs = document.getElementById("add_mac");
	if(selectedVal == "")
	{
		host.value = "";
		macs.value = "";
	}
	else
	{
		if (host.value == "")
		{
			var selectedHost = (selectedVal.split(/,/))[0];
			host.value = selectedHost.replace(/-| /g,"_");
		}
		var selMac = (selectedVal.split(/,/))[1];
		macs.value = (macs.value == "") ? selMac : macs.value.concat(" ", selMac);
		setSelectedValue("mac_list", "");
	}
}


function deviceSelected()
{
	var selectedVal = getSelectedValue("device_list");
	var devices = document.getElementById("add_device");
	devices.value = selectedValue + " " + devices.value;
}



function addDevice()
{
	errors = proofReadDeviceForm();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + dhcpS.AErr);
	}
	else
	{
		var host = document.getElementById("add_host");
		var macs = document.getElementById("add_mac");
		var ip = document.getElementById("add_ip");
		if (host.value != null && macs.value != null)
		{
			var deviceTable = document.getElementById('device_table_container').firstChild;
			var values = [host.value, macs.value, ip.value, createEditButton("editDevice")];
			addTableRow(deviceTable, values, true, false, resetMacList);
			addNewOption('device_list', host.value, host.value);
			host.value = "";
			macs.value = "";
			ip.value = "";
		}
	}
}



function addDeviceToGroup()
{
	errors = proofReadGroupForm();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + dhcpS.AErr);
	}
	else
	{
		var group = document.getElementById("add_group");
		var devices = document.getElementById("add_device");
		if (group.value != null && devices.value != null)
		{
			var groupTable = document.getElementById('group_table_container').firstChild;
			var values = [group.value, devices.value, createEditButton("editGroup")];
			addTableRow(groupTable,values, true, false, resetDeviceList);
			group.value="";
			devices.value="";
		}
	}
}


function editDevice()
{
	location.hash="#device_form";
	editRow=this.parentNode.parentNode;
	editRow.parentNode.removeChild(editRow);
	document.getElementById('add_host').value = editRow.childNodes[0].firstChild.data;
	document.getElementById('add_mac').value = editRow.childNodes[1].firstChild.data;
	document.getElementById('add_ip').value = editRow.childNodes[2].firstChild.data;
}


function editGroup()
{
	location.hash="#group_form";
	editRow=this.parentNode.parentNode;
	editRow.parentNode.removeChild(editRow);
	document.getElementById('add_group').value = editRow.childNodes[0].firstChild.data;
	document.getElementById('add_device').value = editRow.childNodes[1].firstChild.data;
}



function removeDevice(table, row)
{
	resetMacList();
}


function removeGroup(table, row)
{
	resetGroupList();
}


/*******************************************************************************
* Utility functions
*******************************************************************************/

/**
* Returns an Object able to be used as a lookup table indexed by the MAC and
* containing an [MAC, IP, hostname] for hosts listed in /etc/ethers and
* /tmp/dhcp.leases
*/
function knownMacLookup()
{	// gather known MACs from /etc/ethers
	var knownMac = mergeEtherHost();
	var kmMacIndex = 0;
	var macMap = map(knownMac, kmMacIndex);

	var ldMacIndex = 0;
	var ldIpIndex = 1;
	var ldHostIndex = 2;
	for(ldIndex=0; ldIndex < leaseData.length; ldIndex++)
	{	// gather known MACs from dhcp leases
		var leaseRow = leaseData[ldIndex];
		var mac = leaseRow[ldMacIndex].toUpperCase();
		if(macMap[mac] == null)
		{
			macMap[mac] = [mac, leaseRow[ldIpIndex], leaseRow[ldHostIndex]];
		}
	}
	return macMap;
}


/**
* Returns an Array of Arrays containing [mac, ip, host] generated by
* combining /etc/hosts with /etc/ethers
*/
function mergeEtherHost()
{
	var hdIpIndex = 0;
	var hdHostIndex = 1;
	var hostLookup = map(hostData, hdIpIndex);

	var mhdMacIndex = 0;
	var mhdIpIndex = 1;
	var mhdHostIndex = 2;
	var macHostData = etherData.slice();
	for(var mhdIndex=0; mhdIndex < macHostData.length; mhdIndex++)
	{
		var host = null;
		var mhdRow = macHostData[mhdIndex];
		mhdRow[mhdMacIndex] = mhdRow[mhdMacIndex].toUpperCase();
		var ip = mhdRow[mhdIpIndex];
		var hdRow = hostLookup[ ip ];
		if (hdRow instanceof Array)
		{
			host = hdRow[hdHostIndex] ;
		}
		mhdRow[mhdHostIndex] = (host == null) ? "?" : host ;
	}
	return macHostData;
}


/**
* Returns an Object able to be used as a Lookup table on the elements of the
* supplied data[][], indexed by the data[][field].
* data: an Array of Arrays
* field: the index of a field in the inner array to be used as the lookup index.
*/
function map(data, field)
{
	var map = new Object();
	if (data instanceof Array)
	{
		for (index=0; index<data.length; index++)
		{
			value = data[index];
			if (value instanceof Array)
			{
				key = value[field];
				map[key] = value;
			}
		}
	}
	return map;
}



function addNewOption(selectId, optionText, optionValue)
{
	var options = document.getElementById(selectId).options;
	var exists = false;
	for(oIndex=0; oIndex < options.length; oIndex++)
	{
		exists = exists | (options[oIndex].text == optionText);
	}
	if (!exists)
	{
		addOptionToSelectElement(selectId, optionText, optionValue, null, document)
	}
}

/******************************************************************************
* Validation functions
******************************************************************************/

/**
* Checks for invalid or duplicate host names and MACs
*/
function proofReadDeviceForm()
{
	addIds=['add_host', 'add_mac'];
	labelIds= ['add_host_label', 'add_mac_label'];
	functions = [validateUCI, validateMultipleMacs];
	returnCodes = [0,0];
	ip = document.getElementById('add_host').value.length;
	if (ip != null && ip.length > 0)
	{
		addIds.push('add_ip');
		labelIds.push('add_ip_label');
		functions.push(validateIP);
		returnCodes.push(0);
	}
	visibilityIds=addIds;
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, document);

	if(errors.length == 0)
	{	// check that the host and mac are not duplicates of existing values
		var newHost = document.getElementById('add_host').value;
		var newMac = document.getElementById('add_mac').value;
		var deviceTable = document.getElementById('device_table_container').firstChild;
		var currentData = getTableDataArray(deviceTable, true, false);
		for (cdIndex=0; cdIndex < currentData.length ; cdIndex++)
		{
			var rowData = currentData[cdIndex];
			var oldHost = rowData[0];
			var oldMac = rowData[1];
			if(oldHost != '' && oldHost != '-' && oldHost == newHost)
			{
				errors.push(dhcpS.dHErr);
			}
			if(oldMac == newMac)
			{
				errors.push(dhcpS.dMErr);
			}
		}
	}
	return errors;
}


/**
* Checks for invalid host or group names
*/
function proofReadGroupForm()
{
	addIds=['add_group', 'add_device'];
	labelIds= ['add_group_label', 'add_known_device_label'];
	functions = [validateGroup, validateMultipleUCIs];
	returnCodes = [0,0];
	visibilityIds=addIds;
	return proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, document);
}

function proofreadDhcpForm()
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
