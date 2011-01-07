/*	
 *     Copyright (c) 2010 Artur Wronowski <arteqw@gmail.com>
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *     
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *     
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *     MA 02110-1301, USA.
 */

// Global
var updatedScripts;
var updatedState;
var updateCommand;

function saveChanges() 
{
	if (updatedScripts.length == 0)
	{
		alert("Nothing to update!");
	}
	else
	{
		for (index = 0; index < updatedScripts.length; index++)
		{
			updateCommand.push("/etc/init.d/" + updatedScripts[index] + " " + updatedState[index]);
		}

		updateCommand.push("sleep 1");

		commands = updateCommand.join("\n");
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		setControlsEnabled(false, true, "Updating services...");
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
                        	setControlsEnabled(true);
							window.location.reload(true);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function setEnabled()
{
	var enabled= this.checked ? "enable" : "disable";
	var enabledRow=this.parentNode.parentNode;
	var service = enabledRow.childNodes[1].firstChild.id;
	updatedScripts.push(service);
	updatedState.push(enabled);
}

function createEnabledCheckbox()
{
	enabledCheckbox = createInput('checkbox');
	enabledCheckbox.onclick = setEnabled;
	return enabledCheckbox;
}

function createStartButton()
{
    var startButton = createInput("button");
	startButton.value = "Start";
    startButton.className="default_button";
    startButton.onclick = startService;
    return startButton;
}

function createResetButton()
{
    var restartButton = createInput("button");
    restartButton.value = "Restart";
    restartButton.className="default_button";
    restartButton.onclick = restartService;
    return restartButton;
}

function createStopButton()
{
	var stopButton = createInput("button");
	stopButton.value = "Stop";
	stopButton.className="default_button";
	stopButton.onclick = stopService;
	return stopButton;
}

function resetData()
{
	updatedScripts = [];
	updatedState = [];
	updateCommand = [];

	var columnNames = ['Service', 'Autostart', 'Start', 'Restart', 'Stop'];
	var initdTableData = new Array();
	var initdEnabledData = new Array();
	var serviceIds = new Array();

	for (servicesIndex=0; servicesIndex < allInitScripts.length; servicesIndex++)
	{
		var service = allInitScripts[servicesIndex];

		var enabledCheckbox = createEnabledCheckbox();		
		enabledCheckbox.checked = enabledScripts.indexOf(service) != "-1" ? true : false;
		enabledCheckbox.id = service;
		serviceIds.push(enabledCheckbox.id);
		
		initdTableData.push([service, enabledCheckbox, createStartButton(), createResetButton(), createStopButton() ]);
	}

	var blockedServices = ['network', 'boot', 'defconfig', 'dropbear', 'telnet', 'sysctl', 'done'];
	
	for(index=0; index < blockedServices.length; index++)
	{
		initdTableData[serviceIds.indexOf(blockedServices[index])][1].disabled = true;
	}

	var initdTable = createTable(columnNames, initdTableData, "initd_table", false, false);
	var tableContainer = document.getElementById('initd_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(initdTable);

	//for IE6
	for(index = 0; index < initdEnabledData.length; index++)
	{
		initdTableData[index][1].checked = initdEnabledData[index];
	}

}

function startService()
{
	getRow=this.parentNode.parentNode;
	var service = getRow.firstChild.firstChild.data;	
	var executeCommand = [ "/etc/init.d/" + service +  " start" ];
	executeCommand.push("sleep 1");
	
	commands = executeCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, "Try to start "+ service);
        var stateChangeFunction = function(req)
        {
			if(req.readyState == 4)
			{
				setControlsEnabled(true);
			}
        }
        runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function stopService()
{
	getRow=this.parentNode.parentNode;
	var service = getRow.firstChild.firstChild.data;
	var executeCommand = [ "/etc/init.d/" + service +  " stop" ];
	executeCommand.push("sleep 1");
	
	commands = executeCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, "Try to stop "+ service);
	var stateChangeFunction = function(req)
        {
			if(req.readyState == 4)
			{
				setControlsEnabled(true);
			}
        }
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function restartService()
{
	getRow=this.parentNode.parentNode;
	var service = getRow.firstChild.firstChild.data;
	var executeCommand = [ "/etc/init.d/" + service +  " restart" ];
	executeCommand.push("sleep 1");

	commands = executeCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, "Try to restart "+ service);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
