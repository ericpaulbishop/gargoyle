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
	var div=document.createElement('div')
	var elAdd=function(par, childData, isTxt, addBr)
	{
		par.appendChild(isTxt ? document.createTextNode(childData) : document.createElement(childData));
		if(addBr){ par.appendChild(document.createElement('br')); }
	}
	var statusTypes = [];
	statusTypes["not_installed"] = "Not Installed"
	statusTypes["root"] = "Pre-Installed"
	statusTypes["plugin_root"] = "Installed"
	var pkgStatus = statusTypes[ pkgData["Install-Destination"] ]

	elAdd(div, "strong", false, false)
	elAdd(div.firstChild, pkgName, true, true)
	elAdd(div, 'Description: ' + pkgData["Description"], true, true)
	elAdd(div, 'Version: ' + pkgData["Version"], true, true)
	elAdd(div, 'Status: ' + pkgStatus, true, pkgStatus == "Not Installed" ? true : false)
	if(pkgStatus == "Not Installed")
	{
		var dependsMatchUsb = false
		for (var dep in pkgData["Required-Depends"])
		{
			if(dep.match(/^kmod.*usb/))
			{
				dependsMatchUsb = true
			}
		}
		canInstall = (!dependsMatchUsb) && pkgData["Will-Fit"] == "true"
		elAdd(div, "Required Disk Space: " + parseBytes(pkgData["Required-Size"]), true, (!canInstall))
		
		if(!canInstall)
		{
			elAdd(div, "em", false, false)
			div.color = "#FF0000"
			if(dependsMatchUsb)
			{
				elAdd(div.firstChild, "Package Cannot Be Installed (Requires USB support)", true, false)
			}
			if(pkgData["Will-Fit"] == "false")
			{
				elAdd(div.firstChild, "Package Cannot Be Installed (Insufficient Disk Space)", true, false)
			}
			pkgData["Can-Install"] = false;
		}
		else
		{
			pkgData["Can-Install"] = true;
		}
	}
	return div;
	

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
			
			var div=createDisplayDiv(pkgName, pkgData)

			
			var enabledCheckbox = createInput('checkbox');
			enabledCheckbox.disabled = true;
			enabledCheckbox.checked = pkgData["Install-Destination"] == 'not_installed' ? false : true;
			
			var button = createInput("button");
			button.className="default_button";
			if (enabledCheckbox.checked)
			{
				button.value = "Uninstall";
				if( pkgData["Install-Destination"] == "root" )
				{
					button.disabled = true;
					button.className = "default_button_disabled"
				}
				else
				{
					button.onclick = uninstallPackage;
				}
			}
			else
			{
				button.value = "Install";
				if( pkgData["Can-Install" ] )
				{	
					button.onclick = installPackage;
				}
				else
				{
					button.disabled = true;
					button.className = "default_button_disabled"
				}
			}
			pluginsTableData.push([div, enabledCheckbox, button]);
			
		}
	}

	if (pluginsTableData.length == 0)
	{
		document.getElementById('no_packages').style.display="block";
	}
	else
	{
		pluginsTableData.sort();
		var pluginsTable = createTable(columnNames, pluginsTableData, "packages_table", false, false);
		var tableContainer = document.getElementById('packages_table_container');
		setSingleChild(tableContainer, pluginsTable)
	}	
}

function installPackage()
{
}
function uninstallPackage()
{
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
