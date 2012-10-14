/*	
 * This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

/* 
 * mountPoint refers to the blkid mount point 
 * mountPath may refer to either blkid mount point
 * or symlink to dev_[devid], wherever share is 
 * actually mounted
 */
var driveToMountPoints = [];
var mountPointToDrive = [];
var mountPointToFs = [];
var mountPointToDriveSize = [];

//these global data structure are special -- they contain
//the data that will be saved once the user hits the save changes button
var userNames = [];
var nameToSharePath = [];  
var sharePathList = [];
var sharePathToShareData = []; 

//temporary
nullFunc = function() { return 0 ; };
removeUserCallback = nullFunc
// end temporary


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
		var shareMount  = nameToMountPath[shareName];
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


function addUser()
{
	var user = document.getElementById("new_user").value
	var pass1 = document.getElementById("user_pass").value
	var pass2 = document.getElementById("user_pass_confirm").value


	//validate
	var errors = [];	
	var testUser = user.replace(/[0-9a-zA-Z]+/g, "");
	if(user == "")
	{
		errors.push("Username cannot be empty")
	}
	if(testUser != "")
	{
		errors.push("Username can contain only letters and numbers")
	}
	if(pass1 == "" && pass2 == "")
	{
		errors.push("Password cannot be empty")
	}
	if(pass1 != pass2)
	{
		errors.push("Passwords don't match")
	}
	if(errors.length == 0)
	{
		//check to make sure user isn't a dupe
		var userTable = document.getElementById("share_user_table");
		if(userTable != null)
		{
			var found = 0;
			var userData = getTableDataArray(userTable, true, false);
			var userIndex;
			for(userIndex=0; userIndex < userData.length && found == 0; userIndex++)
			{
				found = userData[userIndex][0] == user ? 1 : found;
			}
			if(found)
			{
				errors.push("Duplicate Username")
			}
		}
	}
	if(errors.length > 0)
	{
		alert( errors.join("\n") + "\n\nCould not add user" );
	}
	else
	{
		var userTable = document.getElementById("share_user_table");
		if(userTable == null)
		{
			var tableContainer = document.getElementById("user_table_container");
			userTable = createTable(["", ""], [], "share_user_table", true, false, removeUserCallback); 
			setSingleChild(tableContainer, userTable);
		}
		var userPass = createInput("hidden")
		userPass.value = pass1
		var editButton = createEditButton(editUser);
		addTableRow(userTable, [ user, userPass, editButton], true, false, removeUserCallback)
	}

}

function editUser()
{
	if( typeof(editUserWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editUserWindow.close();
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


	editUserWindow = window.open("share_user_edit.sh", "edit", "width=560,height=300,left=" + xCoor + ",top=" + yCoor );
	var okButton = createInput("button", editUserWindow.document);
	var cancelButton = createInput("button", editUserWindow.document);
	
	okButton.value         = "Change Password";
	okButton.className     = "default_button";
	cancelButton.value     = "Cancel";
	cancelButton.className = "default_button";


	editShareUserRow=this.parentNode.parentNode;
	editShareUser=editShareUserRow.childNodes[0].firstChild.data;


	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(editUserWindow.document != null)
		{
			if(editUserWindow.document.getElementById("bottom_button_container") != null)
			{
				editUserWindow.document.getElementById("share_user_text").appendChild( document.createTextNode(editShareUser) )
				
				
				editUserWindow.document.getElementById("bottom_button_container").appendChild(okButton);
				editUserWindow.document.getElementById("bottom_button_container").appendChild(cancelButton);
				
				cancelButton.onclick = function()
				{
					editUserWindow.close();
				}
				okButton.onclick = function()
				{
					var pass1 = editUserWindow.document.getElementById("new_password");
					var pass2 = editUserWindow.document.getElementById("new_password_confirm");
					var errors = []
					if(pass1 == "" && pass2 == "")
					{
						errors.push("Password cannot be empty")
					}
					if(pass1 != pass2)
					{
						errors.push("Passwords don't match")
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nPassword unchanged.");

					}
					else
					{
						editShareUserRow.childNodes[1].firstChild.value = pass1
						editUserWindow.close();
					}
				}
				editUserWindow.moveTo(xCoor,yCoor);
				editUserWindow.focus();
				updateDone = true;
			}
		}
		if(!updateDone)
		{
			setTimeout( "runOnEditorLoaded()", 250);
		}
	}
	runOnEditorLoaded();
}


function resetData()
{
	document.getElementById("no_disks").style.display =  storageDrives.length == 0 ?   "block" : "none";
	document.getElementById("shared_disks").style.display = storageDrives.length > 0 ? "block" : "none";
	document.getElementById("disk_unmount").style.display = storageDrives.length > 0 ? "block" : "none";
	document.getElementById("disk_format").style.display  = storageDrives.length > 0 || drivesWithNoMounts.length > 0 ? "block" : "none"



	if(storageDrives.length > 0)
	{

		//workgroup
		var s =  uciOriginal.getAllSectionsOfType("samba", "samba");
		document.getElementById("cifs_workgroup").value = s.length > 0 ? uciOriginal.get("samba", s.shift(), "workgroup") : "Workgroup";


		//share users
		userNames = uciOriginal.getAllSectionsOfType("share_users", "user"); //global
		var tableObject;
		if(userNames.length > 0)
		{
			var userTableData = []
			var userIndex
			for(userIndex=0; userIndex < userNames.length; userIndex++)
			{
				userEditButton = createEditButton( editUser )
				userPass = createInput("hidden")
				userPass.value = ""
				userTableData.push( [ userNames[userIndex], userPass, userEditButton ] )
			}
			var tableObject = createTable(["", "", ""], userTableData, "share_user_table", true, false, removeUserCallback);
		}
		else
		{
			tableObject = document.createElement("div");
                	tableObject.innerHTML = "<span style=\"text-align:center\"><em>No Share Users Defined</em></span>";
		}
		var tableContainer = document.getElementById("user_table_container");
		while(tableContainer.firstChild != null)
		{
			tableContainer.removeChild( tableContainer.firstChild);
		}
		tableContainer.appendChild(tableObject);
		setAllowableSelections("user_access", userNames, userNames);



		//full document, not edit, so share_disk option is select, not text
		document.getElementById("share_disk_text").style.display = "none";
		document.getElementById("share_disk").style.display = "block";
		
		//globals
		driveToMountPoints = [];
		mountPointToDrive = []; 
		mountPointToFs = [];
		mountPointToDriveSize = [];
		mountPointList = []

		var driveIndex = 0;
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			var mountPoints = [ storageDrives[driveIndex][1], storageDrives[driveIndex][2] ];
			driveToMountPoints[ storageDrives[driveIndex][0] ] = mountPoints;
			var mpIndex;
			for(mpIndex=0;mpIndex < 2; mpIndex++)
			{
				var mp = mountPoints[mpIndex];
				mountPointToDrive[ mp ]      = storageDrives[driveIndex][0];
				mountPointToFs[ mp ]         = storageDrives[driveIndex][3];
				mountPointToDriveSize[ mp ]  = parseBytes( storageDrives[driveIndex][4] ).replace(/ytes/, "");
				mountPointList.push(mp);
			}
		}

		
		sharePathList = [];	   //global
		sharePathToShareData = []; //global
		nameToSharePath = [];      //global
		
		var mountedDrives = [];
		var sambaShares = uciOriginal.getAllSectionsOfType("samba", "sambashare");
		var nfsShares = uciOriginal.getAllSectionsOfType("nfsd", "nfsshare");
		var ftpShares = uciOriginal.getAllSectionsOfType("vsftpd", "share");
		var getMounted = function(shareList, config)
		{
			var shareIndex;
			for(shareIndex=0; shareIndex < shareList.length; shareIndex++)
			{
				var shareId = shareList[shareIndex]
				var fullSharePath = uciOriginal.get(config, shareId, (config == "vsftpd" ? "share_dir" : "path") );
				var shareMountPoint = null;
				var shareDirectory = null;
				var shareDrive = null;
				var mpIndex;
				for(mpIndex=0;mpIndex<mountPointList.length && shareMountPoint==null && fullSharePath !=null; mpIndex++)
				{
					var mp = mountPointList[mpIndex]
					if( fullSharePath.indexOf(mp) == 0 )
					{
						shareMountPoint = mp
						shareDirectory  = fullSharePath.substr( mp.length);
						shareDirectory = shareDirectory.charAt(0) == "/" ? shareDirectory.substr(1) : shareDirectory;
						shareDrive = mountPointToDrive[shareMountPoint];
					}
				}

				
				if( shareDrive != null )
				{
					mountedDrives[ shareDrive ] = 1;
					
					//shareMountPoint->[shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, isFtp, isCifs, isNfs, anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
					var shareData = mountPointToShareData[shareMountPoint] == null ? ["", "", "", "", "", false, false, false, "none", [], [], "ro", "*" ] :  mountPointToShareData[shareMountPoint] ;
					
					//name
					if( shareData[0] == "" || config == "samba")
					{
						shareData[0] = uciOriginal.get(config, shareId, "name");
						shareData[0] == "" ? shareId : shareData[0];
					}
					shareData[1] = shareDrive                                //drive
					shareData[2] = shareMountPoint                           //share drive mount
					shareData[3] = shareDirectory                            //directory
					shareData[4] = fullSharePath				 //full path
					shareData[5] = config == "vsftpd" ? true : shareData[6]  //isFTP
					shareData[6] = config == "samba"  ? true : shareData[7]  //isCIFS
					shareData[7] = config == "nfsd"   ? true : shareData[8]  //isNFS

					//both samba and vsftpd have ro_users and rw_users list options
					//however, they handle anonymous access a bit differently
					//samba has guest_ok option, while vsftpd includes "anonymous" (or "ftp") user in lists
					if(config == "vsftpd" || config == "samba")
					{
						var readTypes = ["rw", "ro"]
						var readTypeShareDataIndices = [9,10]
						var rtIndex;
						for(rtIndex=0; rtIndex < 2; rtIndex++)
						{
							var shareDataUserList = [];
							var readType = readTypes[rtIndex]
							var userVar  = "users_" + readType
							var userList = uciOriginal.get(config, shareId, userVar);
							if(userList instanceof Array)
							{
								var uIndex;
								for(uIndex=0; uIndex < userList.length; uIndex++)
								{
									var user = userList[uIndex];
									if(user == "anonymous" || user == "ftp")
									{
										//handle anonymous for vsftpd
										sareData[ 8 ] = readType
										
									}
									else
									{
										shareDataUserList.push(user);
									}
								}
							}
							shareData[  readTypeShareDataIndices[rtIndex] ] = shareDataUserList;
						}
						if(config == "samba")
						{
							//handle anonymous for samba
							if( uciOriginal.get(config, shareId, "guest_ok").toLowerCase() == "yes" || uciOriginal.get(config, shareId, "public").toLowerCase() == "yes" )
							{
								shareData[ 8 ] = uciOriginal.get(config, shareId, "read_only").toLowerCase() == "yes" ? "ro" : "rw"
							}
						}
					}
					if(config == "nfsd")
					{
						shareData[ 11 ] = uciOriginal.get(config, shareList[shareIndex], "read_only") == "1" ? "ro" : "rw";

						var allowedHostsStr = uciOriginal.get(config, shareList[shareIndex], "allowed_hosts");
						if(alowedHostsStr instanceof Array)
						{
							allowedHostStr = allowedHostsStr.join(",");
						}

						if(allowedHosts != "" && allowedHosts != "*")
						{
							var allowedHosts = allowedHostsStr.split(/[\t ]*,[\t ]*/);
							var allowedIps = [];
							var foundStar = false;
							while(allowedHosts.length > 0)
							{
								var h = allowedHosts.shift();
								foundStar = h == "*" ? true : foundStar
								if(validateIpRange(h) == 0)
								{
									allowedIps.push(h);
								}
							}
							shareData[ 12 ] = foundStar ? "*" : allowedIps;
						}
					}
					sharePathToShareData[ fullSharePath ] = shareData
					sharePathList.push(fullSharePath)
					nameToSharePath[ shareData[0] ] = fullSharePath
				}
			}
		}
		
		getMounted(sambaShares, "ftp");
		getMounted(sambaShares, "samba");
		getMounted(nfsShares, "nfsd");
		
		var driveList = [];
		var driveDisplayList = [];
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			var driveName = storageDrives[driveIndex][0]
			var driveFs = storageDrives[driveIndex][3];
			var driveSize = parseBytes( storageDrives[driveIndex][4] ).replace(/ytes/, "");
			driveList.push( driveName )
			driveDisplayList.push( driveName + " ("  + driveFs + ", " + driveSize + ")" )
		}
		if(driveList.length > 0)
		{
			document.getElementById("sharing_add_heading_container").style.display  = "block";
			document.getElementById("sharing_add_controls_container").style.display = "block";
			setAllowableSelections("share_disk", driveList, driveDisplayList);
			shareSettingsToDefault();
		}
		else
		{
			document.getElementById("sharing_add_heading_container").style.display  = "none";
			document.getElementById("sharing_add_controls_container").style.display = "none";
		}


		
		//create current share table
		//name, disk, subdirectory, type, [edit], [remove]
		var shareTableData = [];
		var shareIndex;
		for(shareIndex=0; shareIndex < sharePathList; shareIndex++)
		{

			var shareData = sharePathToShareData[ sharePathList[shareIndex] ];
			var shareTypes=["ftp", "cifs", "nfs"]
			var shareType = "";
			var stIndex;
			for(stIndex=0; stIndex < shareTypes.length; stIndex++)
			{
				var plus = shareType == "" ? "" : "+"
				shareType = shareType + ( document.getElementById( "share_type_" + shareTypes[stIndex] ).checked ? plus + (shareTypes[stIndex]).toUpperCase() : "" )
			}
			shareTableData.push( [ shareData[0], shareData[1], shareData[3], shareType, createEditButton(editShare) ] )
		}
		var shareTable = createTable(["Name", "Disk", "Subdirectory", "Share Type", ""], shareTableData, "share_table", true, false, removeShareCallback);
		var tableContainer = document.getElementById('sharing_mount_table_container');
		setSingleChild(tableContainer, shareTable);
	}

	// format setttings
	//
	// note that 'drivesWithNoMounts', refers to drives not mounted on the OS,
	// not lack of network shared/mounts which is what the other variables
	// refer to.  This can be confusing, so I'm putting this comment here

	
	if(drivesWithNoMounts.length > 0)
	{
		var dindex;
		var driveIds  = [];
		var driveText = [];
		for(dindex=0; dindex< drivesWithNoMounts.length ; dindex++)
		{
			driveIds.push( "" + dindex);
			driveText.push( drivesWithNoMounts[dindex][0] + " (" + parseBytes(parseInt(drivesWithNoMounts[dindex][1])) + ")")
		}
		setAllowableSelections("format_disk_select", driveIds, driveText);
	}
	document.getElementById("swap_percent").value  = "25";
	document.getElementById("storage_percent").value = "75";
	var vis = (drivesWithNoMounts.length > 0);
	setVisibility( ["no_unmounted_drives", "format_warning", "format_disk_select_container", "swap_percent_container", "storage_percent_container", "usb_format_button_container"],  [ (!vis), vis, vis, vis, vis, vis ] )
	updateFormatPercentages()
}


function shareSettingsToDefault()
{
	var vis = getVis(document)
	for(type in vis)
	{
		document.getElementById( "share_type_" + type ).checked = true;
	}
	document.getElementById("share_dir").value = "/";
	document.getElementById("share_name").value = "share_" + (sharePathList.length + 1)
	setShareTypeVisibility();
	setSharePaths();
}

function addUserAccess(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument
	var addUser = getSelectedValue("user_access", controlDocument)
	if(addUser == null || addUser == "")
	{
		alert("No User To Add -- You must configure a new share user above.")	
	}
	else
	{
		var access = getSelectedValue("user_access_type", controlDocument)
		removeOptionFromSelectElement("user_access", addUser, controlDocument);
		
		var userAccessTable = controlDocument.getElementById("user_access_table");
		if(userAccessTable ==  null)
		{
			var container = document.getElementById("user_access_table_container");
			var userAccessTable =  createTable(["", ""], [], "user_access_table", true, false, removeUserAccessCallback);
			setSingleChild(container, userAccessTable);
		}
		addTableRow(userAccessTable, [ addUser, access ], true, false, removeUserAccessCallback, null, controlDocument)
	}
}
function removeUserAccessCallback(table, row)
{
	var removeUser=row.childNodes[0].firstChild.data;
	addOptionToSelectElement("user_access", removeUser, removeUser, table.ownerDocument);
}

function setSingleChild(container, child)
{
	while(container.firstChild != null)
	{
		container.removeChild( container.firstChild);
	}
	container.appendChild(child);
}


function updateFormatPercentages(ctrlId)
{
	var valid = false;


	if(ctrlId == null)
	{
		ctrlId="swap_percent";
	}
	var otherCtrlId  = ctrlId == "swap_percent" ? "storage_percent" : "swap_percent"
	var sizeId       = ctrlId == "swap_percent" ? "swap_size"       : "storage_size"
	var otherSizeId  = ctrlId == "swap_percent" ? "storage_size"    : "swap_size"
	
				
	var driveId = getSelectedValue("format_disk_select")
	if(driveId != null)
	{
		try
		{
			var percent1 = parseFloat(document.getElementById(ctrlId).value)
			if( percent1 != "NaN" && percent1 <= 100 && percent1 >= 0)
			{
				document.getElementById(ctrlId).style.color = "black"
				var percent2 = 100 - percent1;
				var size = parseInt(drivesWithNoMounts[parseInt(driveId)][1]);
	
				var size1 = (percent1 * size)/100;
				var size2 = size - size1;
	
				document.getElementById(otherCtrlId).value = "" + percent2
				setChildText(sizeId, "(" + parseBytes(size1) + ")");
				setChildText(otherSizeId, "(" + parseBytes(size2) + ")");
				valid=true
			}
			else
			{
				document.getElementById(ctrlId).style.color = "red"
			}
		}
		catch(e)
		{
			document.getElementById(ctrlId).style.color = "red"
		}
	}
	return valid
}

// FIXME
function mountPathToMountPoint(mountPath)
{
	mountPath = mountPath == null ? "" : mountPath;
	var mountPoint = mountPath.replace(/^.*\//g, "").match(/dev_/) ? driveToMountPoint[ "/dev/" + mountPath.replace(/^.*\//g, "").substr(4) ] : mountPath;
	mountPoint = mountPoint == null ? "" : mountPoint;
	return mountPoint;

}
function driveToDevMountPath(drive)
{
	return drive.replace(/^.*\//, "/tmp/usb_mount/dev_");
}
//

function getVis(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument
	var vis = []
	var visTypes = ["ftp", "cifs", "nfs"]
	var vIndex;
	for(vIndex=0; vIndex < visTypes.length; vIndex++)
	{
		vis[ visTypes[vIndex] ] = controlDocument.getElementById( "share_type_" + visTypes[vIndex] ).checked
	}
	return vis;
}

function getVisStr(vis)
{
	var shareTypeStr = function(type){ return vis[type] ? type.toUpperCase() : ""; }
	var visStr = shareTypeStr("ftp") + "+" + shareTypeStr("cifs") + "+" + shareTypeStr("nfs");
	return visStr.replace(/\+\+/g, "+").replace(/^\+/, "").replace(/\+$/, "");
}

function addNewShare()
{
	//shareMountPoint->[shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, isFtp, isCifs, isNfs, anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
	//var shareData = mountPointToShareData[shareMountPoint] == null ? ["", "", "", "", false, false, false, "none", [], [], "ro", "*" ] :  mountPointToShareData[shareMountPoint] ;
	//var shareData = ["", "", "", "", false, false, false, "none", [], [], "ro", "*" ]; //defaults
	
	var shareDrive = getSelectedValue("share_disk");
	var shareSubdir = document.getElementById("share_dir").value
	shareSubdir = shareSubdir == "" ? "/" : shareSubdir;
	var shareSpecificity = getSelectedValue("share_specificity");
	var shareName = document.getElementById("share_name").value;
	var shareDiskMount =  driveToMountPoints[shareDrive][ ( shareSpecificity == "blkid" ? 0 : 1 ) ];
	var altDiskMount   = driveToMountPoints[shareDrive][ ( shareSpecificity == "blkid" ? 1 : 0 ) ];
	var fullSharePath  = (shareDiskMount + "/" + shareSubdir).replace(/\/\//g, "/").replace(/\/$/, "");
	var altSharePath   = (altDiskMount + "/" + shareSubdir).replace(/\/\//g, "/").replace(/\/$/, "");


	var enabledTypes = getVis();
	var shareType = getVisStr(enabledTypes);

	var anonymousAccess = getSelectedValue("anonymous_access")
	var roUsers = [];
	var rwUsers = [];
	var userAccessTable = document.getElementById("user_access_table")
	if(userAccessTable != null)
	{
		var userAccessTableData = getTableDataArray(userAccessTable, true, false);
		var uatIndex;
		for(uatIndex=0; uatIndex < userAccessTableData.length; uatIndex++)
		{
			var rowData = userAccessTableData[uatIndex];
			if(rowData[1] == "R/O") { roUsers.push(rowData[0]); }
			if(rowData[1] == "R/W") { rwUsers.push(rowData[0]); }
		}
	}
	var nfsAccess = getSelectedValue("nfs_access")
	var nfsAccessIps = getSelectedValue("nfs_policy") == "share" ? "*" : [];
	if( typeof(nfsAccessIps) != "String")
	{
		var nfsIpTable = document.getElementById("nfs_ip_table");
		if(nfsIpTable != null)
		{
			var nfsIpData = getTableDataArray(nfsIpTable);
			var nipIndex;
			for(nipIndex=0; nipIndex < nfsIpData.length; nipIndex++)
			{
				nfsAccessIps.push( nfsIpData[nipIndex][0] );
			}
		}
	}
	
	
	// error checking
	var errors = [];
	if( !(enabledTypes["ftp"] || enabledTypes["cifs"] || enabledTypes["nfs"]) )
	{
		errors.push("You must select at least one share type (FTP, CIFS, NFS)")
	}
	if( enabledTypes["ftp"] || enabledTypes["cifs"])
	{
		if( roUsers.length == 0 && rwUsers.length == 0 && anonymousAccess == "none" )
		{
			errors.push("FTP and/or CIFS share type selected, but neither anonymous access or users are configured");
		}
	}
	if( sharePathToShareData[ fullSharePath ] != null || sharePathToShareData[ altSharePath ] != null )
	{
		var existing = sharePathToShareData[ fullSharePath ];
		existing = existing == null ? sharePathToShareData[ altSharePath ] : existing
		errors.push("Specified Shared Directory Is Already Configured: " + existing[0])
	}
	if( nameToSharePath[ shareName ] != null)
	{
		errors.push( "Share name is a duplicate" )
	}
	if(errors.length > 0)
	{
		var errStr = errors.join("\n") + "\n\nCould Not Add Share"
		alert(errStr)
	}
	else
	{
		var shareData = [shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, enabledTypes["ftp"], enabledTypes["cifs"], enabledTypes["nfs"], anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
		sharePathToShareData[ fullSharePath ] = shareData
		sharePathList.push(fullSharePath)
		nameToSharePath [ shareName ] = fullSharePath

		var shareTable = document.getElementById("share_table")
		addTableRow(shareTable, [shareName, shareDrive, shareSubdir, shareType, createEditButton(editShare) ], true, false, removeShareCallback)
	}

}

function setShareTypeVisibility()
{
	var vis = getVis();
	vis["ftp_or_cifs"] = vis["ftp"] || vis["cifs"]

	var getTypeDisplay = function(type) { return vis[type] ? type.toUpperCase() : "" }
	var userLabel = (getTypeDisplay("ftp") + "/" + getTypeDisplay("cifs")).replace(/^\//, "").replace(/\/$/, "");
	setChildText("anonymous_access_label",  userLabel + " Anonymous Access:")
	setChildText("user_access_label",  userLabel + " Users With Access:")

	var visIds = [];
	visIds[ "ftp_or_cifs" ] =  [ "anonymous_access_container", "user_access_container" ];
	visIds["nfs"] = [ "nfs_access_container", "nfs_policy_container", "nfs_ip_container", "nfs_spacer", "nfs_path_container" ];
	visIds["ftp"] = [ "ftp_path_container" ]
	for(idType in visIds)
	{
		var ids = visIds[idType]
		var idIndex;
		for(idIndex=0; idIndex<ids.length;idIndex++)
		{
			document.getElementById( ids[idIndex] ).style.display = vis[ idType ] ? "block" : "none"
		}
	}
	if(vis["nfs"])
	{
		setInvisibleIfIdMatches("nfs_policy",  ["share"], "nfs_ip_container",    "block", document);
	}
}


function createEditButton( editFunction )
{
	editButton = createInput("button");
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editFunction;
	
	editButton.className = "default_button"  ;
	editButton.disabled  = false ;

	return editButton;
}

function removeShareCallback(table, row)
{
	var editName=row.childNodes[0].firstChild.data;

	var oldMountPoint = mountPathToMountPoint(nameToMountPath[editName]);
	var oldDrive = mountPointToDrive[oldMountPoint];
	addOptionToSelectElement("share_disk", oldDrive, oldDrive);
	document.getElementById("sharing_add_heading_container").style.display  = "block";
	document.getElementById("sharing_add_controls_container").style.display = "block";
	
	
	nameToMountPath[editName] = null;
	setSharePaths();
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
	var editMountPath=nameToMountPath[editName];
	var editMountPoint=mountPathToMountPoint(editMountPath);
	var editDrive=mountPointToDrive[editMountPoint];
	
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
				setSelectedValue("share_specificity", (editMountPath.replace(/^.*\//g, "").match(/^dev_/) ? "dev" : "blkid"), editShareWindow.document);
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
						nameToMountPath[editName] = null;
						nameToMountPath[shareName] = getSelectedValue("share_specificity", editShareWindow.document) == "blkid" ? editMountPoint : driveToDevMountPath(editDrive);

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

function setSharePaths(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var escapedName=escape(controlDocument.getElementById("share_name").value);
	var ip = uciOriginal.get("network", "lan", "ipaddr");
	if(document.getElementById("share_type_nfs").checked)
	{
		controlDocument.getElementById("nfs_path_container").style.display = "block";
		setChildText("nfs_path", ip + ":/nfs/" + escapedName, null, null, null, controlDocument);
	}
	else
	{
		controlDocument.getElementById("nfs_path_container").style.display = "none";
	}
	if(document.getElementById("share_type_ftp").checked)
	{
		controlDocument.getElementById("ftp_path_container").style.display = "block";
		setChildText("ftp_path", "ftp://" + ip + "/" + escapedName, null, null, null, controlDocument);
	}
	else
	{
		controlDocument.getElementById("ftp_path_container").style.display = "none";
	}
}


function unmountAllUsb()
{
	setControlsEnabled(false, true, "Unmounting Disks");

	
	var commands = "/etc/init.d/usb_storage stop ; "

	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			//reload page
			window.location=window.location
			setControlsEnabled(true);
		}
	}

	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function formatDiskRequested()
{

	if( !updateFormatPercentages("swap_percent") )
	{
		alert("ERROR: Invalid Allocation Percentages Specified");
		return;
	}

	var validFunc   = doDiskFormat;
	var invalidFunc = function(){ alert("ERROR: Invalid Password"); }

	var driveId = drivesWithNoMounts[ parseInt(getSelectedValue("format_disk_select")) ][0]
	confirmPassword("Are you sure you want to format drive " + driveId + "?\n\n" + "All data on this drive will be lost.\n\nTo proceed, enter your router login password:", validFunc, invalidFunc);

}

function doDiskFormat()
{
	var driveId        = drivesWithNoMounts[ parseInt(getSelectedValue("format_disk_select")) ][0]
	var swapPercent    = parseFloat(document.getElementById("swap_percent").value)
	
	//format shell script requires percent as an integer, round as necessary
	if(swapPercent >0 && swapPercent <1)
	{
		swapPercent = 1
	}
	if(swapPercent >99 && swapPercent < 100)
	{
		swapPerent = 99
	}
	swapPercent=Math.round(swapPercent)

	if(confirm("Password Accepted.\n\nThis is your LAST CHANCE to cancel.  Press 'OK' only if you are SURE you want to format " + driveId))
	{
	
		setControlsEnabled(false, true, "Formatting,\nPlease Be Patient...");
	
	
		var commands = "/usr/sbin/gargoyle_format_usb \"" + driveId + "\" \"" + swapPercent + "\" \"4\" ; sleep 1 ; /etc/init.d/usb_storage restart ; sleep 1 "
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				alert("Formatting Complete.");
				window.location=window.location
				setControlsEnabled(true)
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	}
	else
	{
		setControlsEnabled(true)
	}

}


