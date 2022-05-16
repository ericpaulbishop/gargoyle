/*
 * This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var wolS=new Object(); //part of i18n

var TSort_Data = new Array ('wol_table', 's', 'p', 's');

function initWolTable()
{
	var dataList = [];
	var ipToHostAndMac = [];

	initializeDescriptionVisibility(uciOriginal, "wol_help"); // set description visibility
	uciOriginal.removeSection("gargoyle", "help"); // necessary, or we overwrite the help settings when we save

	var lineIndex = 0;
	for(lineIndex=0; lineIndex < arpLines.length; lineIndex++)
	{
		var nextLine = arpLines[lineIndex];
		var splitLine = nextLine.split(/[\t ]+/);
		if(splitLine.length > 4)
		{
			var mac = splitLine[4].toUpperCase();
			var ip = splitLine[0];
			if(mac != "00:00:00:00:00:00" && ip6_scope(ip)[1] != "Link-local Address")
			{
				dataList.push( [ getHostname(ip), ip, mac, createWakeUpButton() ] );
				ipToHostAndMac[ip] = 1;
			}
		}
	}

	for(lineIndex=0; lineIndex < dhcpLeaseLines.length; lineIndex++)
	{
		var leaseLine = dhcpLeaseLines[lineIndex];
		var splitLease = leaseLine.split(/[\t ]+/);
		var mac = splitLease[1].toUpperCase();
		var ip = splitLease[2];
		if(ipToHostAndMac[ip] == null)
		{
			dataList.push( [ getHostname(ip), ip, mac, createWakeUpButton() ] );
			ipToHostAndMac[ip] = 1;
		}
	}

	for(lineIndex=0; lineIndex < etherData.length; lineIndex++)
	{
		var ether = etherData[lineIndex];
		var mac = ether[0].toUpperCase();
		var ip = ether[1];
		if(ipToHostAndMac[ip] == null)
		{
			dataList.push( [ getHostname(ip), ip, mac, createWakeUpButton() ] );
			ipToHostAndMac[ip] = 1;
		}
	}

	var staticLeases = uciOriginal.getAllSectionsOfType('dhcp','host');
	staticLeases.forEach(function(lease) {
		var ether = uciOriginal.get('dhcp',lease,'mac');
		var ip = uciOriginal.get('dhcp',lease,'ip');
		if(ipToHostAndMac[ip] == null && ether != '')
		{
			dataList.push( [ getHostname(ip), ip, ether, createWakeUpButton() ] );
			ipToHostAndMac[ip] = 1;
		}
	});

	sort2dStrArr(dataList, 1);
	var columnNames = [UI.HsNm, "IP", "MAC", "" ]
	var table = createTable(columnNames, dataList, "wol_table", false, false);
	var tableContainer = document.getElementById('wol_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(table);
}

function sort2dStrArr(arr, testIndex)
{
	var str2dSort = function(a,b){  return a[testIndex] == b[testIndex] ? 0 : (a[testIndex] < b[testIndex] ? -1 : 1);  }
	arr.sort(str2dSort);
}

function getHostname(ip)
{
	var hostname = ipToHostname[ip] == null ? "("+UI.unk+")" : ipToHostname[ip];
	hostname = hostname.length < 25 ? hostname : hostname.substr(0,22)+"...";
	return hostname;
}

function createWakeUpButton()
{
	var WakeUpButton = createInput("button");
	WakeUpButton.textContent = wolS.WkUp;
	WakeUpButton.className = "btn btn-default btn-wakeup";
	WakeUpButton.onclick = wakeHost;
	return WakeUpButton;
}

function wakeHost()
{
	getRow = this.parentNode.parentNode;

	var mac = getRow.childNodes[2].firstChild.data;
	var wakeHostCommand = [ "etherwake -b -i br-lan " + mac ];

	commands = wakeHostCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	runAjax("POST", "utility/run_commands.sh", param, function(){ return 0; });

	alert(wolS.WAlr);
}
