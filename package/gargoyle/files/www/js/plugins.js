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
	div.style.width="320px";
	div.id = pkgName;
	var elAdd=function(par, childData, isTxt, addBr)
	{
		var el;
		if(isTxt)
		{
			el = document.createElement('span');
			var lines = childData.split(/\n/);
			while(lines.length >0)
			{
				el.appendChild(document.createTextNode(lines.shift()))
				if(lines.length > 0){ el.appendChild(document.createElement('br')); }
			}		
		}
		else
		{
			el = document.createElement(childData)
		}
		par.appendChild(el);
		if(addBr){ par.appendChild(document.createElement('br')); }
		return el;
	}

	var nameDisplay = pkgData["Description"] == null ? pkgName : (pkgData["Description"])
	var statusTypes = [];
	statusTypes["not_installed"] = "Not Installed"
	statusTypes["root"] = "Pre-Installed"
	statusTypes["plugin_root"] = "Installed"
	var pkgStatus = statusTypes[ pkgData["Install-Destination"] ]


	//deliberately add 2 newlines to spearate name/description from other data
	elAdd(div, "strong", false, true)
	elAdd(div.firstChild, nameDisplay, true, true)

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
			var emEl = elAdd(div, "em", false, false)
			emEl.style.color = "#FF0000"
			if(dependsMatchUsb)
			{
				elAdd(emEl, "Package Cannot Be Installed (Requires USB support)", true, false)
			}
			else if(pkgData["Will-Fit"] == "false")
			{
				elAdd(emEl, "Package Cannot Be Installed (Insufficient Disk Space)", true, false)
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

	//set data for plugin options
	var pluginRootDir   = uciOriginal.get("gargoyle", "plugin_options", "root_dir")
	var pluginRootDrive = uciOriginal.get("gargoyle", "plugin_options", "root_drive")
	pluginRootDrive  = pluginRootDrive == "" ? "root" : pluginRootDrive;
	pluginRootDir    = pluginRootDir   == "" || pluginRootDrive == "root" ? "/plugin_root" : pluginRootDir;
	
	
	






	//set data for plugin list
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
	var pkg = this.parentNode.parentNode.firstChild.firstChild.id;
	var cmd = [ "sh /usr/lib/gargoyle/install_gargoyle_package.sh " + pkg  ];

	// This should be done by implementing post-inst script for a given package, not as part of package installation procedure
	//cmd.push("for i in $(opkg files " + package + " | grep \"/etc/init.d\"); do $i enable; $i start; done"); 
	
	execute(cmd);
}

function uninstallPackage()
{
	var pkg = this.parentNode.parentNode.firstChild.firstChild.id;
	var cmd = [ "sh /usr/lib/gargoyle/remove_gargoyle_package.sh " + pkg ];
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


