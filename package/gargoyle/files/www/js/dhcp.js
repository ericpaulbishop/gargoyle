/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var dhcpS=new Object(); //part of i18n
var TSort_Data = new Array ('static_ip_table', 's', 's', 'p', 's', 's', '', '');

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

		var staticHostCommands = [];
		var staticHostSections = uciOriginal.getAllSectionsOfType("dhcp", "host");
		while(staticHostSections.length > 0)
		{
			var lastSection = staticHostSections.pop();
			uciOriginal.removeSection("dhcp", lastSection);
			staticHostCommands.push("uci del dhcp." + lastSection);
		}
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
			uci.set("dhcp","lan","dhcpv6",document.getElementById("dhcpv6").value);
			uci.set("dhcp","lan","ra",document.getElementById("ra").value);
			uci.set("dhcp","lan","ra_management",document.getElementById("ra_management").value);
		}
		else
		{
			uci.set("dhcp", "lan", "ignore", "1");
			uci.set("dhcp","lan","dhcpv6","disabled");
			uci.set("dhcp","lan","ra","disabled");
			uci.remove("dhcp","lan","ra_management");
			dhcpWillBeEnabled = false;
		}

		staticIpTable = document.getElementById('staticip_table_container').firstChild;
		tableData = getTableDataArray(staticIpTable, true, false);
		for(hostIdx = 0; hostIdx < tableData.length; hostIdx++)
		{
			cfgid = "static_host_" + (hostIdx+1);
			uci.set("dhcp",cfgid,"","host");
			hostname = tableData[hostIdx][0];
			if(hostname != "" && hostname != "-")
			{
				uci.set("dhcp",cfgid,"name",hostname);
			}
			uci.set("dhcp",cfgid,"mac",tableData[hostIdx][1]);
			uci.set("dhcp",cfgid,"ip",tableData[hostIdx][2]);
			hostid = tableData[hostIdx][3];
			if(hostid != "-")
			{
				var splitHostId = hostid.split(':');
				if(splitHostId.length == 4)
				{
					splitHostId[3] = ('0000' + splitHostId[3]).slice(-4);
				}
				uci.set("dhcp",cfgid,"hostid",splitHostId.join(''));
			}
			duid = tableData[hostIdx][4];
			if(duid != "-")
			{
				uci.set("dhcp",cfgid,"duid",duid);
			}

			staticHostCommands.push("uci set dhcp." + cfgid + "=host");
		}

		// We don't use /etc/ethers anymore
		createEtherCommands = [ "touch /etc/ethers", "rm /etc/ethers" ];
		var dnsmasqsec = uci.getAllSectionsOfType("dhcp","dnsmasq");
		if(dnsmasqsec.length > 0)
		{
			uci.set("dhcp",dnsmasqsec[0],"readethers","0");
		}

		createHostCommands = [ "touch /etc/hosts", "rm /etc/hosts" ];
		createHostCommands.push("echo \"127.0.0.1\tlocalhost localhost4\" >> /etc/hosts");
		createHostCommands.push("echo \"::1\tlocalhost localhost6\" >> /etc/hosts");

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

		//need to restart firewall here because for add/remove of static ips, we need to restart bandwidth monitor, as well as for firewall commands above if we have any
		var restartDhcpCommand = "\n/etc/init.d/dnsmasq restart ; \n/etc/init.d/odhcpd restart ; \nsh /usr/lib/gargoyle/restart_firewall.sh\n" ;

		commands = staticHostCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + createEtherCommands.join("\n") + "\n" + createHostCommands.join("\n") + "\n" + firewallCommands.join("\n") + "\n" + restartDhcpCommand ;

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
	editButton.textContent = UI.Edit;
	editButton.className = "btn btn-default btn-edit";
	editButton.onclick = editStaticModal;
	return editButton;
}

function resetData()
{
	dhcpEnabled = uciOriginal.get("dhcp", "lan", "ignore") == "1" ? false : true;
	var staticIpTableData = [];
	hostSections = uciOriginal.getAllSectionsOfType("dhcp","host");
	var secIndex=0;
	for(secIndex=0; secIndex < hostSections.length ; secIndex++)
	{
		//Host, MAC, IPv4, IPv6, DUID, Edit btn
		var rowData;
		hostSection = hostSections[secIndex];
		host = uciOriginal.get("dhcp",hostSection,"name");
		mac = uciOriginal.get("dhcp",hostSection,"mac");
		ipv4 = uciOriginal.get("dhcp",hostSection,"ip");
		hostid = uciOriginal.get("dhcp",hostSection,"hostid");
		ipv6 = hostid == "" ? "" : ("00000000" + hostid).slice(-8).replace(/([0-9a-f]{4})([0-9a-f]{4})/i,"::$1:$2");
		ipv6 = validateIP6(ipv6) == 0 ? ip6_canonical(ipv6) : "-";
		duid = uciOriginal.get("dhcp",hostSection,"duid");
		duid = duid == "" ? "-" : duid;

		rowData = [host, mac, ipv4, ipv6, duid, createEditButton()];

		staticIpTableData.push(rowData);
	}
	columnNames=[UI.HsNm, 'MAC', 'IPv4', dhcpS.Suff, 'DUID', ''];
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
	var blockMismatches = uciOriginal.get("firewall", firewallDefaultSections[0], "enforce_dhcp_assignments") == "1" ? true : false;
	document.getElementById("block_mismatches").checked = blockMismatches;

	dhcpv6 = uciOriginal.get("dhcp", "lan", "dhcpv6");
	document.getElementById("dhcpv6").value = dhcpv6 == "" ? "disabled" : dhcpv6;

	ra = uciOriginal.get("dhcp", "lan", "ra");
	document.getElementById("ra").value = ra == "" ? "disabled" : ra;

	ra_management = uciOriginal.get("dhcp", "lan", "ra_management");
	document.getElementById("ra_management").value = ra_management == "" ? "2" : ra_management;

	var ip6txt = "";
	for(var x = 0; x < currentLanIp6.length; x++)
	{
		if(ip6_scope(currentLanIp6[x])[0] == "Global")
		{
			ip6txt = ip6txt + (x == 0 ? "" : "\n") + ip6_mask(currentLanIp6[x], currentLanMask6[x]) + "/" + currentLanMask6[x];
		}
	}
	setChildText("ip6prefix", ip6txt);

	//setup hostname/mac list
	resetHostnameMacList();
}

function removeStaticIp(table, row)
{
	var removedIp = row.childNodes[2].firstChild.data
	//delete ipHostHash[removedIp]
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
		var host = lease[2];
		var duid = null;
		for(ipIndex in ipHostHash)
		{
			if(ipHostHash[ipIndex] == host)
			{
				duid = ipDUIDHash[ipIndex];
				if(duid != null)
				{
					break;
				}
			}
		}
		if(duid == null)
		{
			for(ipIndex in ipMacHash)
			{
				if(ipMacHash[ipIndex] !== undefined && mac == ipMacHash[ipIndex].toUpperCase())
				{
					duid = ipDUIDHash[ipIndex];
					if(duid != null)
					{
						break;
					}
				}
			}
		}

		if( staticMacs[ mac ] == null )
		{
			hmVals.push( lease[2] + "," + mac + "," + (duid == null ? "-" : duid));
			hmText.push( (lease[2] == "" || lease[2] == "*" ? lease[1] : lease[2] ) + " (" + mac + ")" );
		}
	}
	setAllowableSelections("static_from_connected", hmVals, hmText);

	var hmEnabled = hmText.length > 1 && document.getElementById('dhcp_enabled').checked ? true : false;
	setElementEnabled(document.getElementById("static_from_connected"), hmEnabled, "none");

}

function setEnabled(enabled)
{
	var ids=['dhcp_start', 'dhcp_end', 'dhcp_lease', 'block_mismatches', 'add_host', 'add_mac', 'add_ip', 'add_hostid', 'add_duid', 'dhcpv6', 'ra', 'add_button'];
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
	errors = proofreadStatic();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + dhcpS.AErr);
	}
	else
	{
		values = new Array();
		ids = ['add_host', 'add_mac', 'add_ip', 'add_hostid', 'add_duid'];
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
		closeModalWindow('static_ip_modal');
	}
}

function proofreadStatic(excludeRow)
{
	var proofreadIP6Suffix = function()
	{
		if(arguments[0].length == 0 || arguments[0] == "-")
		{
			//We want ipv6 optional for now
			return 0;
		}
		if(!arguments[0].match(/^::([0-9a-f]{0,4}:)?[0-9a-f]{0,4}/))
		{
			return 1;
		}
		return 0;
	};
	var proofreadDUID = function()
	{
		if(arguments[0] == "-")
		{
			//We want ipv6 optional for now
			return 0;
		}
		if(!arguments[0].match(/^[0-9a-f]{0,130}$/i))
		{
			return 1;
		}
		return 0;
	};

	addIds=['add_mac', 'add_ip', 'add_hostid', 'add_duid'];
	labelIds= ['add_mac_label', 'add_ip_label', 'add_hostid_label', 'add_duid_label'];
	functions = [validateMac, validateIP, proofreadIP6Suffix, proofreadDUID];
	returnCodes = [0,0,0,0];
	visibilityIds=addIds;
	errors = proofreadFields(addIds, labelIds, functions, returnCodes, visibilityIds, document);
	if(errors.length == 0)
	{
		var staticIpTable = document.getElementById('staticip_table_container').firstChild;
		var currentData = getTableDataArray(staticIpTable, true, false);
		var rowDataIndex = 0;
		for (rowDataIndex=0; rowDataIndex < currentData.length ; rowDataIndex++)
		{
			if(staticIpTable.rows[rowDataIndex+1] != excludeRow)
			{
				rowData = currentData[rowDataIndex];
				if(rowData[0] != '' && rowData[0] != '-' && rowData[0] == document.getElementById('add_host').value)
				{
					errors.push(dhcpS.dHErr);
				}
				if(rowData[1] == document.getElementById('add_mac').value)
				{
					errors.push(dhcpS.dMErr);
				}
				if(rowData[2] == document.getElementById('add_ip').value)
				{
					errors.push(dhcpS.dIPErr);
				}
				if(rowData[3] != '-' && rowData[3] == document.getElementById('add_hostid').value)
				{
					errors.push(dhcpS.dHIDErr);
				}
				if(rowData[4] != '-' && rowData[4] == document.getElementById('add_duid').value)
				{
					errors.push(dhcpS.dDUIDErr);
				}
			}
		}
	}
	if(errors.length == 0)
	{
		var hostid = document.getElementById("add_hostid").value;
		var duid = document.getElementById('add_duid').value;
		if((hostid != "" && hostid != "-") && (duid == "" || duid == "-"))
		{
			errors.push(dhcpS.NoDUID);
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

function editStatic(editRow)
{
	var errors = proofreadStatic(editRow);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+dhcpS.upErr);
	}
	else
	{
		var add_host = document.getElementById("add_host").value;
		var add_hostid = document.getElementById("add_hostid").value;
		var add_duid = document.getElementById("add_duid").value;
		//update document with new data
		editRow.childNodes[0].firstChild.data = add_host == "" ? "-" : add_host;
		editRow.childNodes[1].firstChild.data = document.getElementById("add_mac").value;
		editRow.childNodes[2].firstChild.data = document.getElementById("add_ip").value;
		editRow.childNodes[3].firstChild.data = add_hostid == "" ? "-" : add_hostid;
		editRow.childNodes[4].firstChild.data = add_duid == "" ? "-" : add_duid;

		closeModalWindow('static_ip_modal');

		resetHostnameMacList();
	}
}

function addStaticModal()
{
	modalButtons = [
		{"title" : UI.Add, "classes" : "btn btn-primary", "function" : addStatic},
		"defaultDismiss"
	];

	var host = "";
	var mac = "";
	var duid = "";
	var selectedVal = getSelectedValue("static_from_connected");
	if(selectedVal != "none")
	{
		host	= (selectedVal.split(/,/))[0];
		mac	= (selectedVal.split(/,/))[1];
		duid	= (selectedVal.split(/,/))[2];
		setSelectedValue("static_from_connected", "none");
	}

	modalElements = [
		{"id" : "add_host", "value" : host},
		{"id" : "add_mac", "value" : mac},
		{"id" : "add_ip", "value" : ""},
		{"id" : "add_hostid", "value" : ""},
		{"id" : "add_duid", "value" : duid}
	];
	modalPrepare('static_ip_modal', dhcpS.AdSIP, modalElements, modalButtons);
	openModalWindow('static_ip_modal');
}

function editStaticModal()
{
	editRow=this.parentNode.parentNode;
	modalButtons = [
		{"title" : UI.CApplyChanges, "classes" : "btn btn-primary", "function" : function(){editStatic(editRow);}},
		"defaultDiscard"
	];

	host	= editRow.childNodes[0].firstChild.data;
	mac	= editRow.childNodes[1].firstChild.data;
	ip	= editRow.childNodes[2].firstChild.data;
	hostid	= editRow.childNodes[3].firstChild.data;
	duid	= editRow.childNodes[4].firstChild.data;

	modalElements = [
		{"id" : "add_host", "value" : host},
		{"id" : "add_mac", "value" : mac},
		{"id" : "add_ip", "value" : ip},
		{"id" : "add_hostid", "value" : hostid},
		{"id" : "add_duid", "value" : duid}
	];
	modalPrepare('static_ip_modal', dhcpS.ESIP, modalElements, modalButtons);
	openModalWindow('static_ip_modal');
}
