/*
 * This program is copyright © 2015 John Brown and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var deviceS=new Object(); //part of i18n

function saveChanges()
{
	//errorList = proofreadAll();
	errorList = "";
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);

		uci = uciOriginal.clone();

		saveDeviceChanges(uci);
		saveGroupChanges(uci);

		commands = uci.getScriptCommands(uciOriginal) ;

		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				//resetData();
				setControlsEnabled(true);
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


function saveDeviceChanges(uci){
	deviceTable = document.getElementById('device_table_container').firstChild;
	tableData = getTableDataArray(deviceTable, true, false);

	for (rowIndex = 0; rowIndex < tableData.length; rowIndex++)
	{
		rowData = tableData[rowIndex];
		var device = rowData[0];
		var macs = rowData[1];
		if (uci.get("known", device).length == 0){
			uci.set("known", device, null, "device");
		}
		uci.set("known", device, "mac", macs.split("\n"), true);
	}
}


function saveGroupChanges(uci){
	groupTable = document.getElementById('group_table_container').firstChild;
	tableData = getTableDataArray(groupTable, true, false);

	for (rowIndex = 0; rowIndex < tableData.length; rowIndex++)
	{
		rowData = tableData[rowIndex];
		var group = rowData[0];
		var devices = rowData[1].split("\n");
		for(dIx=0; dIx < devices.length; dIx++)
		{
			var device = devices[dIx];
			if (uci.get("known", device).length == 0){
				uci.set("known", device, null, "device");
			}
			uci.set("known", device, "group", group, true);
		}
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
	resetDeviceTable();
	resetGroupTable();

	//setup hostname/mac list
	resetMacList();
	resetGroupList();
	resetDeviceList();
}



function resetDeviceTable()
{
	var deviceTableData = new Array();

	var devices = uciOriginal.getAllSectionsOfType("known", "device");
	for (dIx=0; dIx < devices.length; dIx++)
	{	// process the MAC's assigned to each device
		var device = devices[dIx];
		var devMacs = uciOriginal.get("known", device, "mac");
		var macs = (devMacs instanceof Array) ? devMacs.join("\n") : "" ;
		macs = macs.slice(1); // truely horrible brittle removal of the first extraneous /n
		deviceTableData.push([device, macs, createEditButton("editDevice")]);
	}

	// create the device Table and place it into the document
	var columnNames=[deviceS.DevNm, "MACs", ''];
	var deviceTable=createTable(columnNames, deviceTableData, "device_table", true, false, removeDevice );
	var tableContainer = document.getElementById('device_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(deviceTable);
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
	for (dtdIx=0; dtdIx < deviceTableData.length; dtdIx++){
		deviceMacs = deviceMacs.concat(deviceTableData[dtdIx][dtdMacIx], ",");
	}

	var hlVals = [ "" ];
	var hlText = [ deviceS.SelM ];
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



function removeDevice(table, row)
{
	resetMacList();
}



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
			host.value = (selectedVal.split(/,/))[0];
			fixGroupName(host);
		}
		var selMac = (selectedVal.split(/,/))[1];
		macs.value = (macs.value == "") ? selMac : macs.value.concat(" ", selMac);
		setSelectedValue("mac_list", "");
	}
}


function addMac()
{
	//errors = proofreadDevice(document);
	errors = "";
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + deviceS.AErr);
	}
	else
	{
		var host = document.getElementById("add_host");
		var macs = document.getElementById("add_mac");
		if (host.value != null && macs.value != null)
		{
			var deviceTable = document.getElementById('device_table_container').firstChild;
			var values = [host.value, macs.value.replace(/ /g, "\n"), createEditButton("editDevice")];
			addTableRow(deviceTable, values, true, false, resetMacList);
			host.value = "";
			macs.value = "";
		}
	}
}

function fixGroupName(element)
{
		var name = element.value;
		name = name.replace(/-|\s/g,"_");
		element.value = name;
}


function proofReadMac(input)
{
		// please implement proofReadMAC
}



function resetGroupTable()
{
	var groupTableData = new Array();

	var groups = new Object();
	var devices = uciOriginal.getAllSectionsOfType("known", "device");

	for (dIx=0; dIx < devices.length; dIx++)
	{	// survey all of the devices and groups
		var device = devices[dIx];
		var group = uciOriginal.get("known", device, "group");
		if (group != null && group.length > 0)
		{
			if (groups.hasOwnProperty(group))
			{
				groups[group].push(device);
			}
			else
			{
				groups[group] = [device];
			}
		}
	}

	for (var group in groups)
	{	// place each group in an array
    if (groups.hasOwnProperty(group))
		{	// with a list of member devices
			groupTableData.push([group, groups[group].join("\n"), createEditButton("editGroup")]);
		}
	}

	// create the group Table and place it into the document
	var columnNames=[deviceS.GpNm, deviceS.DevNms, ''];
	var groupTable=createTable(columnNames, groupTableData, "group_table", true, false, removeGroup );
	var tableContainer = document.getElementById('group_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(groupTable);
}


/**
*	Populates a Select id=group_list with know MACs which have not yet been assigned
* to a Known Device.
*/
function resetGroupList()
{
	var groups = [];
	var devices = uciOriginal.getAllSectionsOfType("known", "device");
	for (dIx=0; dIx < devices.length; dIx++)
	{	// survey all of the devices and groups
		var device = devices[dIx];
		var group = uciOriginal.get("known", device, "group");
		if (groups.indexOf(group) == -1)
		{
			groups.push(group);
		}
	}

	var gpVals = [ "" ];
	var gpText = [ deviceS.SelG ];
	for (gIx = 0; gIx < groups.length; gIx++)
	{
		var group = groups[gIx];
		gpVals.push( group );
		gpText.push( group);
	}
	setAllowableSelections("group_list", gpVals, gpText, document);
}


function removeGroup(table, row)
{
	resetGroupList();
}


function groupSelected()
{
	document.getElementById("add_group").value = getSelectedValue("group_list");
}


/**
*	Populates a Select id=device_list with know MACs which have not yet been assigned
* to a Known Device.
*/
function resetDeviceList()
{
	var gpVals = [ "" ];
	var gpText = [ deviceS.SelD ];

	var devices = uciOriginal.getAllSectionsOfType("known", "device");
	for (dIx=0; dIx < devices.length; dIx++)
	{	// survey all of the devices and groups
		var device = devices[dIx];
		var group = uciOriginal.get("known", device, "group");
		if (group == null || group.length == 0)
		{
			gpVals.push( device );
			gpText.push( device );
		}
	}

	setAllowableSelections("device_list", gpVals, gpText, document);
}


function deviceSelected()
{
	document.getElementById("add_device").value = getSelectedValue("device_list");
}


function addDevice()
{
	//errors = proofreadDevice(document);
	errors = "";
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + deviceS.AErr);
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

/**
* Returns an Object able to be used as a lookup table indexed by the MAC and
* containing an [MAC, IP, hostname] for hosts listed in /etc/ethers and
* /tmp/dhcp.leases
*/
function knownMacLookup()
{	// gather known MACs from /etc/ethers
	var knownMac = mergeEtherHost();
	var kmMacIndex = 0;
	var macLookup = lookupList(knownMac, kmMacIndex);

	var ldMacIndex = 0;
	var ldIpIndex = 1;
	var ldHostIndex = 2;
	for(ldIndex=0; ldIndex < leaseData.length; ldIndex++)
	{	// gather known MACs from dhcp leases
		var leaseRow = leaseData[ldIndex];
		var mac = leaseRow[ldMacIndex].toUpperCase();
		if(macLookup[mac] == null)
		{
			macLookup[mac] = [mac, leaseRow[ldIpIndex], leaseRow[ldHostIndex]];
		}
	}
	return macLookup;
}


/**
* Returns an Array of Arrays containing [mac, ip, host] generated by
* combining /etc/hosts with /etc/ethers
*/
function mergeEtherHost()
{
	var hdIpIndex = 0;
	var hdHostIndex = 1;
	var hostLookup = lookupList(hostData, hdIpIndex);

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
function lookupList(data, field){
	var lookup = new Object();
	var index=0;
	for(index=0; index < data.length; index++)
	{
		var key = (data[index][field]);
		lookup[key] = data[index];
	}
	return lookup;
}
