/*
 * This program is copyright © 2022 Michael Gray and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var diagStr=new Object(); //part of i18n

function resetData()
{
	generateRamoopsTable();
}

function generateRamoopsTable()
{
	var filesTableData = ramoopsFiles.map(function(file){return [file,createRamoopsDownloadButton()];});
	byId("device_section").style.display = filesTableData.length == 0 ? "none" : "block";
	var filesTable = createTable([diagStr.fName,""], filesTableData, "ramoops_files_table", true, false, deleteRamoopsFile);
	var tableContainer = byId('ramoops_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(filesTable);
}

function createRamoopsDownloadButton()
{
	downloadButton = createInput("button");
	downloadButton.textContent = UI.DNow;
	downloadButton.className = "btn btn-default";
	downloadButton.onclick = function(){downloadRamoopsFile(this);};

	return downloadButton;
}

function downloadRamoopsFile(triggerEl)
{
	setControlsEnabled(false,true,UI.Wait);
	var downloadId = triggerEl.parentElement.parentElement.firstElementChild.innerText;
	setControlsEnabled(true);
	window.location = "/utility/diagnostics_download_ramoops.sh?fileid=" + downloadId;
}

function deleteRamoopsFile(table,row)
{
	setControlsEnabled(false,true,UI.Wait);
	var fileId = row.firstElementChild.innerText;
	var commands = "rm /sys/fs/pstore/" + fileId + " 2>/dev/null";

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function doPing()
{
	var family = byId("ping_ipv4").checked ? "4" : "6";
	var pingtarget = byId("ping_target").value;
	var commands = "ping -" + family + " -c 5 " + pingtarget + " 2>&1";
	runCommandsWithOutput(commands);
}

function doTracert()
{
	var family = byId("tracert_ipv4").checked ? "4" : "6";
	var tracerttarget = byId("tracert_target").value;
	var commands = "traceroute -" + family + " -w 1 " + tracerttarget + " 2>&1";
	runCommandsWithOutput(commands);
}

function doNSLookup()
{
	var nslookuptarget = byId("nslookup_target").value;
	var commands = "nslookup " + nslookuptarget + " 2>&1";
	runCommandsWithOutput(commands);
}

function runCommandsWithOutput(commands)
{
	setControlsEnabled(false,true,UI.Wait);
	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			byId("output").value = req.responseText.replace(/Success\n$/,"");
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}