/*
 *     Copyright (c) 2011 Cezary Jackiewicz <cezary@eko.one.pl>
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

function resetData()
{
	var columnNames = ['Package', 'Description', 'Version', 'Installed', ''];
	var pluginsTableData = new Array();
	for (var i in packages)
	{
		var package = packages[i]["Package"];
		if (package)
		{
			package = package.replace(/^\s+/, '');
			var description = packages[i]["Description"];
			if (!description) description = '';
			var version = packages[i]["Version"];
			
			var enabledCheckbox = createInput('checkbox');
			enabledCheckbox.disabled = true;
			enabledCheckbox.checked = packages[i]["Status"].indexOf('not-installed') != "-1" ? false : true;
			
			var button = createInput("button");
			button.className="default_button";
			if (enabledCheckbox.checked)
			{
				button.value = "Uninstall";
				button.onclick = uninstallPackage;
			}
			else
			{
				button.value = "Install";
				button.onclick = installPackage;
			}

			pluginsTableData.push([package, description, version, enabledCheckbox, button]);
		}
	}

	if (pluginsTableData.length == 0)
	{
		document.getElementById('no_packages').style.display="block";
	}
	else
	{
		var tableContainer = document.getElementById('packages_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		var pluginsTable = createTable(columnNames, pluginsTableData, "packages_table", false, false);
		tableContainer.appendChild(pluginsTable);
	}
}

function installPackage()
{
	var package = this.parentNode.parentNode.firstChild.firstChild.data;
	var cmd = [ "opkg install " + package ];
	cmd.push("sleep 2")
	cmd.push("for i in $(opkg files " + package + " | grep \"/etc/init.d\"); do $i enable; $i start; done");
	execute(cmd);
}

function uninstallPackage()
{
	var package = this.parentNode.parentNode.firstChild.firstChild.data;
	var cmd = [ "opkg remove " + package ];
	execute(cmd);
}

function updatePackagesList()
{
	var cmd = [ "opkg update" ];
	execute(cmd);
}

function execute(cmd,reload)
{
	var commands = cmd.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, "Please wait...");
	
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			window.location.href=window.location.href;
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
