/*
 *     Copyright (c) 2010 Artur Wronowski <arteqw@gmail.com>
 *     Copyright (c) 2011-2013 Cezary Jackiewicz <cezary@eko.one.pl>
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

initdS = new Object();

// Global
var updatedScripts;
var updatedState;
var updateCommand;

function saveChanges() 
{
	if (updatedScripts.length == 0)
	{
		alert(initdS.NoServ);
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
		setControlsEnabled(false, true, initdS.ActServ);
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

	var columnNames = initdS.ServColumn;
	var initdTableData = new Array();
	var initdEnabledData = new Array();
	var serviceIds = new Array();
	var blockedServices = ['boot','bwmon_gargoyle','create_original_backup','cron','ddns_gargoyle','defconfig','dnsmasq','done','dropbear','firewall','gargoyle_themes','httpd_gargoyle','led','miniupnpd','network','plugins','portmap','qos_gargoyle','rcS','set_kernel_timezone','share_users','sysctl','sysntpd','telnet','time_backup','ubus','umount','usb','usb_storage','watchdog','webmon_gargoyle','wol'];

	for (servicesIndex=0; servicesIndex < allInitScripts.length; servicesIndex++)
	{
		var service = allInitScripts[servicesIndex];

		if( blockedServices.indexOf(service) != -1) { continue; }

		var enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = enabledScripts.indexOf(service) != "-1" ? true : false;
		enabledCheckbox.id = service;
		serviceIds.push(enabledCheckbox.id);

		initdTableData.push([service, enabledCheckbox, createStartButton(), createResetButton(), createStopButton() ]);
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
	ssrService(this.parentNode.parentNode, "start");
}

function stopService()
{
	ssrService(this.parentNode.parentNode, "stop");
}

function restartService()
{
	ssrService(this.parentNode.parentNode, "restart");
}

function ssrService(row, action)
{
	var service = row.firstChild.firstChild.data;
	var executeCommand = [ "/etc/init.d/" + service + " " + action ];
	executeCommand.push("sleep 1");

	commands = executeCommand.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	switch(action)
	{
		case "start":
			setControlsEnabled(false, true, initdS.ServStart + " '" + service + "'");
			break;
		case "stop":
			setControlsEnabled(false, true, initdS.ServStop + " '" + service + "'");
			break;
		default:
			setControlsEnabled(false, true, initdS.ServRestart + " '" + service + "'");
	}

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
