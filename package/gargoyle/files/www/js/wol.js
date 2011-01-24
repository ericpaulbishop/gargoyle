
function initWolTable()
{
	var dataList = [];
	var ipToHostAndMac = [];
	

	arpLines.shift(); //skip header
	var lineIndex = 0;
	for(lineIndex=0; lineIndex < arpLines.length; lineIndex++)
	{
		var nextLine = arpLines[lineIndex];
		var splitLine = nextLine.split(/[\t ]+/);
		var mac = splitLine[3].toUpperCase();
		var ip = splitLine[0];
		dataList.push( [ getHostname(ip), ip, mac, createWakeUpButton() ] );
		ipToHostAndMac[ip] = 1;
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


	sort2dStrArr(dataList, 1);
	var columnNames = ["Hostname", "IP", "MAC", "" ]
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
	var hostname = ipToHostname[ip] == null ? "(unknown)" : ipToHostname[ip];
	hostname = hostname.length < 25 ? hostname : hostname.substr(0,22)+"...";
	return hostname;
}


function createWakeUpButton()
{
        var WakeUpButton = createInput("button");
        WakeUpButton.value = "WakeUp";
        WakeUpButton.className="default_button";
        WakeUpButton.onclick = wakeHost;
        return WakeUpButton;
}

function wakeHost() {

	getRow=this.parentNode.parentNode;

	var mac = getRow.childNodes[2].firstChild.data;
	var wakeHostCommand = [ "wol -i " + bcastIp + " " + mac ];

	commands = wakeHostCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	runAjax("POST", "utility/run_commands.sh", param, function(){ return 0; });

        alert("Wakeup request sent");

}



