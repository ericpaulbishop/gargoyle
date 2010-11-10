/*	
 * This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var driveToMountPoint = [];
var mountPointToDrive = [];
var mountPointToFs = [];
var mountPointToDriveSize = [];
var nameToMountPoint = [];



function saveChanges()
{
	//validate samba user/pass if samba isn't set to anonymous access
	var sambaAuth  = getSelectedValue("cifs_policy");
	var sambaUser  = document.getElementById("cifs_user").value;
	var sambaPass  = document.getElementById("cifs_pass").value;
	var sambaGroup = document.getElementById("cifs_workgroup").value;
	var errors = [];
	if(sambaGroup == "")
	{
		errors.push("Invalid CIFS Workgroup");
	}
	if(sambaAuth == "user" && (sambaUser == ""))
	{
		errors.push("Invalid CIFS User");
	}
	if(sambaAuth == "user" && (sambaPass == ""))
	{
		errors.push("Invalid CIFS Password");
	}
	if(errors.length > 0)
	{
		alert( errors.join("\n") + "\n\nCould not save settings" );
		return;
	}


	setControlsEnabled(false, true);
	
	//remove old shares 
	var preCommands = [];
	var sharePkg = ["samba", "samba", "nfsd"];
	var shareCfg = ["sambashare", "sambauser", "nfsshare"];
	while(sharePkg.length > 0)
	{
		pkg = sharePkg.shift();
		cfg = shareCfg.shift();
		var allOriginalConfigSections = uciOriginal.getAllSectionsOfType(pkg, cfg);
		while(allOriginalConfigSections.length > 0)
		{
			var section = allOriginalConfigSections.shift();
			uciOriginal.removeSection(pkg, section);
			preCommands.push("uci del " + pkg + "." + section);	
		}
	}

	
	
	var uci = uciOriginal.clone();
	if(sambaAuth == "user")
	{
		preCommands.push("uci set samba.sambauser=sambauser");
		uci.set("samba", "sambauser", "", "sambauser");
		uci.set("samba", "sambauser", "username", sambaUser);
		uci.set("samba", "sambauser", "password", sambaPass);
	}
	preCommands.push("uci set samba.@samba[0].workgroup='" + sambaGroup + "'" );
	
	var nfsAuth    = getSelectedValue("nfs_policy");
	var nfsIps = [];
	if(nfsAuth == "ip")
	{
		var nfsIpTable = document.getElementById("nfs_ip_table");
		if(nfsIpTable != null)
		{
			var nfsIpData = getTableDataArray(nfsIpTable, true, false);
			while(nfsIpData.length > 0)
			{
				var row = nfsIpData.shift();
				nfsIps.push(row[0]);
			}
		}
		if(nfsIps.length == 0)
		{
			setSelectedValue("nfs_policy", "share");
			document.getElementById("nfs_ip_container").style.display = "none";
		}
	}

	var shareTableData = getTableDataArray(document.getElementById("share_table"), true, false);
	var shIndex=0;
	for(shIndex=0; shIndex<shareTableData.length; shIndex++)
	{
		var share       = shareTableData[shIndex];
		var shareName   = share[0];
		var shareMount  = nameToMountPoint[shareName];
		var shareType   = share[3];
		var shareAccess = share[4];
		

		var shareId = shIndex + "_" + shareName;
		if(shareType.match(/CIFS/))
		{
			var pkg = "samba";
			var cfg = "sambashare";
			preCommands.push("uci set " + pkg + "." + shareId + "=" + cfg + "\n");
			uci.set(pkg, shareId, "", cfg);
			uci.set(pkg, shareId, "name", shareName);
			uci.set(pkg, shareId, "path", shareMount);
			uci.set(pkg, shareId, "read_only", (shareAccess == "Read Only" ? "yes" : "no"));
			uci.set(pkg, shareId, "create_mask", "0777");
			uci.set(pkg, shareId, "dir_mask", "0777");
			uci.set(pkg, shareId, "browseable", "yes");
			if(sambaAuth == "user")
			{
				uci.set(pkg, shareId, "guest_ok", "no");
				uci.set(pkg, shareId, "users", sambaUser);
			}
			else
			{
				uci.set(pkg, shareId, "guest_ok", "yes");
			}
			
		}
		if(shareType.match(/NFS/))
		{
			var pkg = "nfsd";
			var cfg = "nfsshare";
			preCommands.push("uci set " + pkg + "." + shareId + "=" + cfg + "\n");
			uci.set(pkg, shareId, "", cfg);
			uci.set(pkg, shareId, "name", shareName);
			uci.set(pkg, shareId, "path", shareMount);
			uci.set(pkg, shareId, "read_only", (shareAccess == "Read Only" ? "1" : "0"));
			uci.set(pkg, shareId, "sync", "1");
			uci.set(pkg, shareId, "insecure", "1");
			uci.set(pkg, shareId, "subtree_check", "0");
			if(nfsIps.length > 0)
			{
				uci.set(pkg, shareId, "allowed_hosts", nfsIps.join(","));
			}
			else
			{
				uci.set(pkg, shareId, "allowed_hosts", "*");
			}

		}
	}
	preCommands.push("uci commit");

	var postCommands = [];
	postCommands.push("/etc/init.d/samba restart");
	postCommands.push("/etc/init.d/nfsd restart");
	
	var commands = preCommands.join("\n") + "\n" +  uci.getScriptCommands(uciOriginal) + "\n" + postCommands.join("\n") + "\n";

	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			setControlsEnabled(true);
		}
	}

	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function resetData()
{
	document.getElementById("shared_disks").style.display = storageDrives.length > 0 ? "block" : "none";
	document.getElementById("bottom_button_container").style.display =  storageDrives.length > 0 ? "block" : "none";
	document.getElementById("no_disks").style.display =  storageDrives.length == 0 ? "block" : "none";

	if(storageDrives.length > 0)
	{
		//workgroup
		var s =  uciOriginal.getAllSectionsOfType("samba", "samba");
		document.getElementById("cifs_workgroup").value = s.length > 0 ? uciOriginal.get("samba", s.shift(), "workgroup") : "Workgroup";

		//access policy, default = anonymous
		setSelectedValue("cifs_policy", "share", document);
		setSelectedValue("nfs_policy",  "share", document);

		//full document, not edit, so share_disk option is select, not text
		document.getElementById("share_disk_text").style.display = "none";
		document.getElementById("share_disk").style.display = "block";
		
		//globals
		driveToMountPoint = [];
		mountPointToDrive = []; 
		mountPointToFs = [];
		mountPointToDriveSize = [];
		
		var driveIndex = 0;
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			mountPointToDrive[ storageDrives[driveIndex][1] ]      = storageDrives[driveIndex][0];
			driveToMountPoint[ storageDrives[driveIndex][0] ]      = storageDrives[driveIndex][1];
			mountPointToFs[ storageDrives[driveIndex][1] ]         = storageDrives[driveIndex][2];
			mountPointToDriveSize[ storageDrives[driveIndex][1] ]  = parseBytes( storageDrives[driveIndex][3] ).replace(/ytes/, "");
		}
		

		nameToMountPoint = []; //another global
		var mountPointToDriveData = [];
		var mountedDrives = [];
		var sambaShares = uciOriginal.getAllSectionsOfType("samba", "sambashare");
		var nfsShares = uciOriginal.getAllSectionsOfType("nfsd", "nfsshare");
		var getMounted = function(shareList, config)
		{
			var shareIndex;
			for(shareIndex=0; shareIndex < shareList.length; shareIndex++)
			{
				var share = uciOriginal.get(config, shareList[shareIndex], "path");
				if( mountPointToDrive[share] != null )
				{
					mountedDrives[ mountPointToDrive[share] ] = 1;
					
					//device->[name, filesystem, size, sharetype, access]
					var driveData = mountPointToDriveData[share] == null ? ["", "", "", "", "", createEditButton()] : mountPointToDriveData[share];
					driveData[0] = uciOriginal.get(config, shareList[shareIndex], "name");
					driveData[1] = mountPointToFs[share];
					driveData[2] = mountPointToDriveSize[share];
					driveData[3] = driveData[3].length > 0 ? driveData[3] + "+" + config : config;
					driveData[4] = uciOriginal.get(config, shareList[shareIndex], "read_only");
					driveData[4] = driveData[4] == "1" || driveData[4] == "yes" ? "Read Only" : "Read/Write";

					nameToMountPoint[ driveData[0] ] = share;

					if(config == "samba")
					{
						var user = uciOriginal.get(config, shareList[shareIndex], "users").replace(/ .*$/, "");
						
						if(user != "")
						{
							var pass = "";
							var sambaUsers = uciOriginal.getAllSectionsOfType("samba", "sambauser");
							var userValid = false;
							var sui = 0;
							for(sui=0; sui < sambaUsers.length; sui++)
							{
								var v = uciOriginal.get("samba", sambaUsers[sui], "username") == user ? true : false;
								pass = v ? uciOriginal.get("samba", sambaUsers[sui], "password") : pass;
								userValid = userValid || v;
							}
							if(userValid)
							{
								setSelectedValue("cifs_policy", "user", document);
								document.getElementById("cifs_user").value = user;
								document.getElementById("cifs_pass").value = pass;
							}
						}
					}
					if(config == "nfsd")
					{
						var allowedHostsStr = uciOriginal.get(config, shareList[shareIndex], "allowed_hosts");
						if(allowedHosts != "" && allowedHosts != "*")
						{
							var allowedHosts = allowedHostsStr.split(/[\t ]*,[\t ]*/);
							var allowedIps = [];
							while(allowedHosts.length > 0)
							{
								var h = allowedHosts.shift();
								if(validateIpRange(h) == 0)
								{
									allowedIps.push(h);
								}
							}
							if(allowedIps.length > 0)
							{
								var tableContainer = document.getElementById("nfs_ip_table_container");
								while(tableContainer.firstChild != null)
								{
									tableContainer.removeChild(tableContainer.firstChild);
								}
								setSelectedValue("nfs_policy", "ip", document);
								
								while(allowedIps.length > 0)
								{
									document.getElementById("nfs_ip").value = allowedIps.shift();
									addAddressesToTable(document,"nfs_ip","nfs_ip_table_container","nfs_ip_table",false, 2, true, 250);
								}
							}
						}
					}
					mountPointToDriveData[share] = driveData;
				}
			}
		}
		getMounted(sambaShares, "samba");
		getMounted(nfsShares, "nfsd");
		var unmountedList = [];
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			if( mountedDrives[ storageDrives[driveIndex][0] ] == null ) { unmountedList.push( storageDrives[driveIndex][0]); }
		}
		if(unmountedList.length > 0)
		{
			document.getElementById("sharing_add_heading_container").style.display  = "block";
			document.getElementById("sharing_add_controls_container").style.display = "block";

			setAllowableSelections("share_disk", unmountedList, unmountedList);
			document.getElementById("share_name").value = getSelectedValue("share_disk", document).replace(/^.*\//, "");
			setNfsPath();
		}
		else
		{
			document.getElementById("sharing_add_heading_container").style.display  = "none";
			document.getElementById("sharing_add_controls_container").style.display = "none";
		}
		setPolicyVisibility();

		//create current share table
		//name, file system, size, type, access, [edit], [remove]
		var shareTableData = [];
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			if( mountPointToDriveData[ storageDrives[driveIndex][1] ] != null )
			{
				mountType = (mountPointToDriveData[ storageDrives[driveIndex][1] ])[3];
				mountType = mountType.replace(/samba/, "CIFS");
				mountType = mountType.replace(/nfsd/, "NFS");
				(mountPointToDriveData[ storageDrives[driveIndex][1] ])[3] = mountType;
				shareTableData.push( mountPointToDriveData[ storageDrives[driveIndex][1] ] );
			}
		}
		var shareTable = createTable(["Name", "File System", "Size", "Mount Type", "Access", ""], shareTableData, "share_table", true, false, removeShareCallback);
		var tableContainer = document.getElementById('sharing_mount_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(shareTable);
	}


}


function addNewShare()
{
	var name = document.getElementById("share_name").value;
	if(name.length == "")
	{
		alert("Error: Invalid Share Name\n\nCould not add shared drive");
		return;
	}
	var drive = getSelectedValue("share_disk");
	var mountPoint = driveToMountPoint[drive];
	var fs = mountPointToFs[mountPoint];
	var size = mountPointToDriveSize[mountPoint];
	var access = getSelectedText("share_access");
	var type = getSelectedText("share_type");
	
	var table = document.getElementById("share_table");
	addTableRow(table, [name, fs, size, type, access, createEditButton() ], true, false, removeShareCallback);
	nameToMountPoint[name] = mountPoint;

	//remove the drive we just used from the available list
	removeOptionFromSelectElement("share_disk", drive);
	if(document.getElementById("share_disk").options.length == 0)
	{
		document.getElementById("sharing_add_heading_container").style.display  = "none";
		document.getElementById("sharing_add_controls_container").style.display = "none";
	}
}


function setPolicyVisibility()
{
	setInvisibleIfIdMatches("cifs_policy", ["share"], "cifs_user_container", "block", document);
	setInvisibleIfIdMatches("cifs_policy", ["share"], "cifs_pass_container", "block", document);
	setInvisibleIfIdMatches("nfs_policy",  ["share"], "nfs_ip_container",    "block", document);
}

function createEditButton()
{
	editButton = createInput("button");
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editShare;
	
	editButton.className = "default_button"  ;
	editButton.disabled  = false ;

	return editButton;
}

function removeShareCallback(table, row)
{
	var editName=row.childNodes[0].firstChild.data;

	var oldMountPoint = nameToMountPoint[editName];
	var oldDrive = mountPointToDrive[oldMountPoint];
	addOptionToSelectElement("share_disk", oldDrive, oldDrive);
	document.getElementById("sharing_add_heading_container").style.display  = "block";
	document.getElementById("sharing_add_controls_container").style.display = "block";
	
	
	nameToMountPoint[editName] = null;
	setNfsPath();
}

function editShare()
{
	if( typeof(editShareWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editShareWindow.close();
		}
		catch(e){}
	}

	
	try
	{
		xCoor = window.screenX + 225;
		yCoor = window.screenY+ 225;
	}
	catch(e)
	{
		xCoor = window.left + 225;
		yCoor = window.top + 225;
	}


	editShareWindow = window.open("usb_storage_edit.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	var saveButton = createInput("button", editShareWindow.document);
	var closeButton = createInput("button", editShareWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	var editRow=this.parentNode.parentNode;
	var editName=editRow.childNodes[0].firstChild.data;
	var editMount=nameToMountPoint[editName];
	var editDrive=mountPointToDrive[editMount];
	
	var editType=editRow.childNodes[3].firstChild.data;
	var editAccess = editRow.childNodes[4].firstChild.data;

	var runOnEditorLoaded = function () 
	{
		var updateDone=false;
		if(editShareWindow.document != null)
		{
			if(editShareWindow.document.getElementById("bottom_button_container") != null)
			{
				updateDone = true;
				
				editShareWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editShareWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
				
				//don't need set/load from UCI, since all data is in row text itself
				//load data from row text here
				//device->[name, filesystem, size, sharetype, access]
				editShareWindow.document.getElementById("share_disk").style.display = "none";
				editShareWindow.document.getElementById("share_disk_text").style.display = "block";
				setChildText("share_disk_text", editDrive, null, null, null, editShareWindow.document);
				editShareWindow.document.getElementById("share_name").value = editName;
				setSelectedText("share_type", editType, editShareWindow.document);
				setSelectedText("share_access", editAccess, editShareWindow.document);
				setNfsPath(editShareWindow.document);

				
				closeButton.onclick = function()
				{
					editShareWindow.close();
				}
				saveButton.onclick = function()
				{
					var errors = [];
					var shareName = editShareWindow.document.getElementById("share_name").value;
					if(shareName == "")
					{
						errors.push("Invalid Share Name");
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not update share.");
					}
					else
					{
						//set data
						editRow.childNodes[0].firstChild.data = shareName; 
						editRow.childNodes[3].firstChild.data = getSelectedText("share_type", editShareWindow.document);
						editRow.childNodes[4].firstChild.data = getSelectedText("share_access", editShareWindow.document);
						editShareWindow.close();
					}
				}
				editShareWindow.moveTo(xCoor,yCoor);
				editShareWindow.focus();
			}
		}
		if(!updateDone)
		{
			setTimeout(runOnEditorLoaded, 250);
		}
	}
	runOnEditorLoaded();
}

function setNfsPath(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var shareType = getSelectedText("share_type", controlDocument);
	if(shareType.match(/NFS/))
	{

		var name=controlDocument.getElementById("share_name").value;
		var ip = uciOriginal.get("network", "lan", "ipaddr");
		controlDocument.getElementById("nfs_path_container").style.display = "block";
		setChildText("nfs_path", ip + ":/nfs/" + name, null, null, null, controlDocument);
	}
	else
	{
		controlDocument.getElementById("nfs_path_container").style.display = "none";
	}
}
