/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */



var updateInProgress;
var timeSinceUpdate;
var webmonUpdater = null;


function saveChanges()
{
	if(updateInProgress)
	{
		return;
	}

	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		updateInProgress = true;
		document.body.style.cursor="wait";
		document.getElementById("bottom_button_container").style.display="none";
		document.getElementById("update_container").style.display="block";
		if(webmonUpdater != null)
		{
			clearInterval(webmonUpdater);
		}

	
		var enabled = document.getElementById("webmon_enabled").checked ;
		var enabledCommand = "/etc/init.d/webmon_gargoyle " + (enabled ? "enable" : "disable") + "\n";
		var includeExcludeFileCommand = "";
		var startStopCommand = "/etc/init.d/webmon_gargoyle stop";
		uci = uciOriginal.clone();
		if(enabled)
		{	
			uci.set("webmon_gargoyle", "webmon", "num_records", document.getElementById("num_records").value );
			
			var ipTable = document.getElementById('ip_table_container').firstChild;
			var ipList = getTableDataArray(ipTable, true, false);
			var fileCommands = ["touch /tmp/webmon_ips.tmp", "rm /tmp/webmon_ips.tmp"];
			for(ipIndex = 0; ipIndex < ipList.length; ipIndex++)
			{
				fileCommands.push( "echo \"" + ipList[ipIndex] + "\" >> /tmp/webmon_ips.tmp" );
			}
			fileCommands.push( "mv /tmp/webmon_ips.tmp /etc/webmon_ips" );


			var include_exclude_type = getSelectedValue("include_exclude");
			if(include_exclude_type == "all")
			{
				uci.remove("webmon_gargoyle", "webmon", "include_ip_file");
				uci.remove("webmon_gargoyle", "webmon", "exclude_ip_file");
			}
			else if(include_exclude_type == "include")
			{
				uci.remove("webmon_gargoyle", "webmon", "exclude_ip_file");
				uci.set("webmon_gargoyle", "webmon", "include_ip_file", "/etc/webmon_ips");
				includeExcludeFileCommand = fileCommands.join("\n") + "\n";
			}
			else if(include_exclude_type == "exclude")
			{
				uci.remove("webmon_gargoyle", "webmon", "include_ip_file");
				uci.set("webmon_gargoyle", "webmon", "exclude_ip_file", "/etc/webmon_ips");
				includeExcludeFileCommand = fileCommands.join("\n") + "\n";
			}
			startStopCommand = "/etc/init.d/webmon_gargoyle restart\n";
		}
		
		
		

		
		commands = uci.getScriptCommands(uciOriginal) + "\n" + enabledCommand + includeExcludeFileCommand + startStopCommand;
		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				webmonEnabled = document.getElementById("webmon_enabled").checked;
				includeData = [];
				excludeData = [];
				var ipTable = document.getElementById('ip_table_container').firstChild;
				if(uciOriginal.get("webmon_gargoyle", "webmon", "include_ip_file") != "")
				{
					includeData=getTableDataArray(ipTable, true, false);
				}
				else if(uciOriginal.get("webmon_gargoyle", "webmon", "exclude_ip_file") != "")
				{
					excludeData=getTableDataArray(ipTable, true, false);
				}

				uciOriginal = uci.clone();
				document.getElementById("update_container").style.display="none";		
				document.getElementById("bottom_button_container").style.display="block";
				document.body.style.cursor='auto';
				updateInProgress = false;
				if(webmonEnabled)
				{	
					updateMonitorTable();
				}
				resetData();
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function clearHistory()
{
	if(updateInProgress)
	{
		return;
	}
	updateInProgress = true;
	document.body.style.cursor="wait";
	document.getElementById("bottom_button_container").style.display="none";
	document.getElementById("update_container").style.display="block";

	
	var commands = "/etc/init.d/webmon_gargoyle stop\nrm -rf " + uciOriginal.get("webmon_gargoyle", "webmon", "save_path") + " 2>/dev/null";
	if(webmonEnabled)
	{
		commands = commands +  "\n/etc/init.d/webmon_gargoyle start";
	}
	
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			document.getElementById("update_container").style.display="none";		
			document.getElementById("bottom_button_container").style.display="block";
			document.body.style.cursor='auto';
			updateInProgress = false;
			
			tableContainer = document.getElementById('webmon_table_container');
			if(tableContainer.firstChild != null)
			{
				tableContainer.removeChild(tableContainer.firstChild);
			}

			if(webmonEnabled)
			{	
				updateMonitorTable();
			}
		}
	}
	var param = getParameterDefinition("commands", commands);
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function proofreadAll()
{
	var validator = function(n){ return validateNumericRange(n,1,9999); };
	return proofreadFields( ["num_records"], ["num_records_label"], [validator], [0], ["num_records_label"]);
}

function resetData()
{
	document.getElementById("webmon_enabled").checked = webmonEnabled;
	
	var numRecords = uciOriginal.get("webmon_gargoyle", "webmon", "num_records");
	document.getElementById("num_records").value = numRecords == "" ? 300 : numRecords;
	
	var data = [];
	if(includeData.length == 0 && excludeData == 0)
	{
		setSelectedValue("include_exclude", "all");
	}
	else if(includeData.length > 0)
	{
		setSelectedValue("include_exclude", "include");
		data = includeData;
	}
	else
	{
		setSelectedValue("include_exclude", "exclude");
		data = excludeData;
	}
	var ipTableData = [];
	for(tableIndex=0; tableIndex < data.length; tableIndex++)
	{
		ipTableData.push( [ data[tableIndex] ] );
	}
	ipTable=createTable([""], ipTableData, "ip_table", true, false);
	tableContainer = document.getElementById('ip_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(ipTable);


	setIncludeExclude();
	setWebmonEnabled();

	tableContainer = document.getElementById('webmon_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}

	if(webmonEnabled)
	{
		updateMonitorTable();
		if(webmonUpdater != null)
		{
			clearInterval(webmonUpdater);
		}
		webmonUpdater = setInterval("updateMonitorTable()", 10000); //check for updates every 10 seconds
	}
}

function setIncludeExclude()
{
	document.getElementById("add_ip_container").style.display = (getSelectedValue("include_exclude")=="all") ? "none" : "block";
}

function setWebmonEnabled()
{
	var enabled = document.getElementById("webmon_enabled").checked;
	var ids=[ 'num_records', 'include_exclude', 'add_ip'];
	for (idIndex in ids)
	{
		element = document.getElementById(ids[idIndex]);
		element.disabled= !enabled;
		element.readonly= !enabled;
		element.style.color = !enabled ? "#AAAAAA" : "#000000";
	}
	var addButton = document.getElementById('add_ip_button');
	addButton.className = enabled ? "default_button" : "default_button_disabled";
	addButton.disabled = !enabled;

	addIpTable = document.getElementById('ip_table_container').firstChild;
	if(addIpTable != null)
	{
		setRowClasses(addIpTable, enabled);
	}
}

function addIp()
{
	ipStr=document.getElementById("add_ip").value;
	if(validateIP(ipStr) != 0)
	{
		alert("ERROR: Specified IP is not valid.");
	}
	else
	{
		ipTable = document.getElementById('ip_table_container').firstChild;
		ipData = getTableDataArray(ipTable, true, false);
		inTable = false;
		for(ipIndex = 0; ipIndex < ipData.length; ipIndex++)
		{
			var testIp = ipData[ipIndex];
			inTable = inTable || (ipStr == testIp);
		}
		if(inTable)
		{
			alert("ERROR: Duplicate IP.");
		}
		else
		{
			addTableRow(ipTable, [ipStr], true, false, null, null );
			document.getElementById("add_ip").value = "";
		}
	}
}



function updateMonitorTable()
{
	if(!updateInProgress)
	{
		updateInProgress = true;
		var commands="/usr/sbin/webmon_gargoyle";
		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var tableData = [];
				var webmonLines = req.responseText.split(/[\n\r]+/);
				var loadedData = (webmonLines != null);
				if(loadedData)
				{
					for(wmIndex=0; loadedData && webmonLines[wmIndex].match(/^Success/) == null && wmIndex <  webmonLines.length; wmIndex++)
					{
						var splitLine = webmonLines[wmIndex].split(/[\t]+/);
						loadedData = loadedData && parseInt(splitLine[0]) != "NaN";
						var lastVisitDate = new Date();
						lastVisitDate.setTime( 1000*parseInt(splitLine[0]) );
						var seconds = lastVisitDate.getSeconds() < 10 ? "0" + lastVisitDate.getSeconds() : lastVisitDate.getSeconds();
						var minutes = lastVisitDate.getMinutes() < 10 ? "0" + lastVisitDate.getMinutes() : lastVisitDate.getMinutes();
						var lastVisit = (1*1 + parseInt(lastVisitDate.getMonth())) + "/" + lastVisitDate.getDate() + " " + lastVisitDate.getHours() + ":" + minutes + ":" + seconds;
						var ip = splitLine[1];	
						var domain = splitLine[3];
				
						tableData.push([ip, lastVisit, domain]);
					}
				}	
				
				//loadedData = loadedData && (webmonLines[webmonLines.length -1].match(/^Success/) != null);
				if(loadedData)
				{	
					columnNames=['Local IP', 'Last Access Time', 'Website'];
					webmonTable = createTable(columnNames, tableData, "webmon_table", false, false);
					tableContainer = document.getElementById('webmon_table_container');
					if(tableContainer.firstChild != null)
					{
						tableContainer.removeChild(tableContainer.firstChild);
					}
					tableContainer.appendChild(webmonTable);
				}
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


