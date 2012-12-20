/*
 *     Copyright (c) 2011 Cezary Jackiewicz <cezary@eko.one.pl>
 *     Copyright (c) 2012 Eric Bishop <eric@gargoyle-router.com>
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



function createDisplayDiv(pkgName, pkgData)
{
	var div=document.createElement('div');
	var elAdd=function(par, childData, isTxt, addBr)
	{
		par.appendChild(isTxt ? document.createTextNode(childData) : document.createElement(childData));
		if(addBr){ par.appendChild(document.createElement('br')); }
	}
	var statusTypes = [];
	statusTypes["not_installed"] = "Not Installed"
	statusTypes["root"] = "Pre-Installed"
	statusTypes["plugin_root"] = "Installed"
	var pkgStatus = statusTypes[ pkgData["Install-Destination"] ];

	elAdd(div, "strong", false, false)
	elAdd(div.firstChild, pkgName, true, true)
	elAdd(div, 'Description: ' + pkgData["Description"], true, true)
	elAdd(div, 'Status: ' + pkgStatus, true, pkgStatus == "Not Installed" ? true : false)
	if(pkgStatus == "Not Installed")
	{
		var dependsMatchUsb = false
		var requiredSize = 0

	}
	
}

function resetData()
{
	var columnNames = ['Package', 'Installed', ''];
	var pluginsTableData = new Array();
	var pkgIndex=0;
	for(pkgIndex=0;pkgIndex < opkg_matching_packages.length; pkgIndex++)
	{
		var pkgName = opkg_matching_packages[pkgIndex];
		var pkgData = opkg_info[pkgName];
		if (pkgData != null)
		{
			
			var div=document.createElement('div');
			div.appendChild(controlDocument.createElement('strong'));
		       	div.firstChild.appendChild(controlDocument.createTextNode(pkgName));
			div.appendChild('br');
			div.appendChild(controlDocument.createTextNode('Description: ' + pkgData["Description"]));
			div.appendChild('br');
			div.appendChild(controlDocument.createTextNode(''))

			
			/*
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
			*/
		}
	}

	if (pluginsTableData.length == 0)
	{
		document.getElementById('no_packages').style.display="block";
	}
	else
	{
		pluginsTableData.sort();
		var tableContainer = document.getElementById('packages_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		var pluginsTable = createTable(columnNames, pluginsTableData, "packages_table", false, false);
		tableContainer.appendChild(pluginsTable);
	}
}

/*
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
*/
