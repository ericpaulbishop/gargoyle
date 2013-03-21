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

/* 
 * These global data structure are special -- they contain
 * the data that will be saved once the user hits the save changes button
 *
 * In other sections we load/save to uci variable, but we have 3 packages (samba, nfsd and vsftpd) to worry
 * about for shares, and the conversion to uci isn't particularly straightforward.  So, we save in a global
 * variable in a simpler format, and then update uci when we do the final save
*/
var userNames = [];
var nameToSharePath = [];  
var sharePathList = [];
var sharePathToShareData = []; 


var badUserNames = [ "ftp", "anonymous", "root", "daemon", "network", "nobody" ];

var ftpFirewallRule = "wan_ftp_server_command";
var pasvFirewallRule = "wan_ftp_server_pasv";

function saveChanges()
{

	//proofread
	var errors = [];
	var sambaGroup   = document.getElementById("cifs_workgroup").value
	var ftpWanAccess = document.getElementById("ftp_wan_access").checked
	var ftpWanPasv   = document.getElementById("ftp_wan_pasv").checked
	var pasvMin      = document.getElementById("pasv_min_port").value
	var pasvMax      = document.getElementById("pasv_max_port").value

	if( document.getElementById("shared_disks").style.display != "none")
	{
		if(sambaGroup == "")
		{
			errors.push("Invalid CIFS Workgroup");
		}
		if(ftpWanAccess && uciOriginal.get("firewall", ftpFirewallRule, "local_port") != "21")
		{
			var conflict = checkForPortConflict("21", "tcp");
			if(conflict != "")
			{
				errors.push("Cannot enable WAN FTP access because port conflicts with " + conflict);
			}
		}
		if(ftpWanAccess && ftpWanPasv)
		{
			var intPasvMin = parseInt(pasvMin)
			var intPasvMax = parseInt(pasvMax)
			var pasvValid = false
			if( (!isNaN(intPasvMin)) && (!isNaN(intPasvMax)) )
			{
				if(intPasvMin < intPasvMax)
				{
					pasvValid = true
				}
			}
			if(!pasvValid)
			{
				errors.push("Invalid FTP passive port range")
			}
			else
			{
				var oldPasvMin = uciOriginal.get("firewall", pasvFirewallRule, "start_port");
				var oldPasvMax = uciOriginal.get("firewall", pasvFirewallRule, "end_port");
				var ignorePorts = [];
				if(oldPasvMin != "" && oldPasvMax != "")
				{
					ignorePorts["tcp"] = [];
					ignorePorts["tcp"][oldPasvMin + "-" + oldPasvMax] = 1
				}
				var conflict = checkForPortConflict( "" + intPasvMin + "-" + intPasvMax, "tcp", ignorePorts )
				if(conflict != "")
				{
					errors.push("Pasive FTP port range conflicts with " + conflict);
				}
			}
		}
	}
	if(errors.length > 0)
	{
		alert( errors.join("\n") + "\n\nCould not save settings" );
		return;
	}


	//prepare to save
	setControlsEnabled(false, true);
	var uci = uciOriginal.clone();


	//update share_users
	uci.removeAllSectionsOfType("share_users", "user");
	var userTable = document.getElementById("share_user_table");
	if(userTable != null)
	{
		var userTableData = getTableDataArray(userTable);
		while(userTableData.length >0)
		{
			var nextUserData = userTableData.shift();
			var user = nextUserData[0]
			var pass = (nextUserData[1]).value
			if( pass != null && pass != "")
			{
				uci.set("share_users", user, "", "user")
				uci.set("share_users", user, "password", pass)
			}
			else
			{
				var salt = uciOriginal.get("share_users", user, "password_salt")
				var sha1 = uciOriginal.get("share_users", user, "password_sha1")
				if(salt != "" && sha1 != "")
				{
					uci.set("share_users", user, "", "user")
					uci.set("share_users", user, "password_salt", salt)
					uci.set("share_users", user, "password_sha1", sha1)
				}
			}
		}
	}

	//update firewall
	if(ftpWanAccess)
	{
		uci.set("firewall", ftpFirewallRule, "",            "remote_accept")
		uci.set("firewall", ftpFirewallRule, "proto",       "tcp")
		uci.set("firewall", ftpFirewallRule, "zone",        "wan")
		uci.set("firewall", ftpFirewallRule, "local_port",  "21")
		uci.set("firewall", ftpFirewallRule, "remote_port", "21")
		if(ftpWanPasv)
		{
			uci.set("firewall", pasvFirewallRule, "",           "remote_accept")
			uci.set("firewall", pasvFirewallRule, "proto",      "tcp")
			uci.set("firewall", pasvFirewallRule, "zone",       "wan")
			uci.set("firewall", pasvFirewallRule, "start_port", pasvMin)
			uci.set("firewall", pasvFirewallRule, "end_port",   pasvMax)
		}
		else
		{
			uci.removeSection("firewall", pasvFirewallRule);
		}
	}
	else
	{
		uci.removeSection("firewall", ftpFirewallRule);
		uci.removeSection("firewall", pasvFirewallRule);
	}
		
	

	//update shares
	uci.removeAllSectionsOfType("samba",  "samba")
	uci.removeAllSectionsOfType("samba",  "sambashare")
	uci.removeAllSectionsOfType("vsftpd", "vsftpd")
	uci.removeAllSectionsOfType("vsftpd", "share")
	uci.removeAllSectionsOfType("nfsd",   "nfsshare")
	
	uci.set("samba", "global", "", "samba")
	uci.set("samba", "global", "workgroup", sambaGroup);

	var haveAnonymousFtp = false;
	var makeDirCmds = [];
	for( fullMountPath in sharePathToShareData )
	{
		//[shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, isCifs, isFtp, isNfs, anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
		var shareData = sharePathToShareData[fullMountPath]
		if( shareData[3] != "" )
		{
			makeDirCmds.push("mkdir -p \""  + fullMountPath + "\"" )
		}


		var shareName = shareData[0]
		var shareId   = shareName.replace(/[^0-9A-Za-z]+/, "_").toLowerCase()
		
		var isCifs = shareData[5];
		var isFtp  = shareData[6];
		var isNfs  = shareData[7];
		
		var anonymousAccess = shareData[8]
		var rwUsers = shareData[9]
		var roUsers = shareData[10]

		var nfsAccess = shareData[11];
		var nfsAccessIps = shareData[12]

		if(isCifs)
		{
			var pkg = "samba"
			uci.set(pkg, shareId, "", "sambashare")
			uci.set(pkg, shareId, "name", shareName);
			uci.set(pkg, shareId, "path", fullMountPath);
			uci.set(pkg, shareId, "create_mask", "0777");
			uci.set(pkg, shareId, "dir_mask", "0777");
			uci.set(pkg, shareId, "browseable", "yes");
			
			uci.set(pkg, shareId, "read_only", (anonymousAccess == "ro" ? "yes" : "no"));
			uci.set(pkg, shareId, "guest_ok", (anonymousAccess == "none" ? "no" : "yes"));
			if(rwUsers.length > 0)
			{
				uci.set(pkg, shareId, "users_rw", rwUsers, false)
			}
			if(roUsers.length > 0)
			{
				uci.set(pkg, shareId, "users_ro", roUsers, false)
			}
		}
		if(isFtp)
		{
			var pkg = "vsftpd"
			uci.set(pkg, shareId, "", "share")
			uci.set(pkg, shareId, "name", shareName);
			uci.set(pkg, shareId, "share_dir", fullMountPath);
			if(rwUsers.length > 0)
			{
				uci.set(pkg, shareId, "users_rw", rwUsers, false)
			}
			if(roUsers.length > 0)
			{
				uci.set(pkg, shareId, "users_ro", roUsers, false)
			}
			if(anonymousAccess != "none")
			{
				haveAnonymousFtp = true;
				uci.set(pkg, shareId, "users_" + anonymousAccess, [ "anonymous" ], true);
			}
			
		}
		if(isNfs)
		{
			var pkg = "nfsd"
			uci.set(pkg, shareId, "", "nfsshare")
			uci.set(pkg, shareId, "name", shareName);
			uci.set(pkg, shareId, "path", fullMountPath);
			uci.set(pkg, shareId, "sync", "1");
			uci.set(pkg, shareId, "insecure", "1");
			uci.set(pkg, shareId, "subtree_check", "0");
			
			uci.set(pkg, shareId, "read_only", (nfsAccess == "ro" ? "1" : "0"));
			if(nfsAccessIps instanceof Array)
			{
				uci.set(pkg, shareId, "allowed_hosts", nfsAccessIps, false)
			}
			else
			{
				uci.set(pkg, shareId, "allowed_hosts", [ "*" ], false)
			}

		}	
	}
	uci.set("vsftpd", "global", "", "vsftpd")
	uci.set("vsftpd", "global", "anonymous", (haveAnonymousFtp ? "yes" : "no"))
	uci.set("vsftpd", "global", "anonymous_write", (haveAnonymousFtp ? "yes" : "no")) //write possible but write on individual share dirs set individually elsewhere
	if(ftpWanPasv)
	{
		uci.set("vsftpd", "global", "pasv_min_port", pasvMin)
		uci.set("vsftpd", "global", "pasv_max_port", pasvMax)
	}


	var postCommands = [];
	if(
		uciOriginal.get("firewall", ftpFirewallRule,  "local_port") != uci.get("firewall", ftpFirewallRule,  "local_port") || 
		uciOriginal.get("firewall", pasvFirewallRule, "start_port") != uci.get("firewall", pasvFirewallRule, "start_port") || 
		uciOriginal.get("firewall", pasvFirewallRule, "end_port")   != uci.get("firewall", pasvFirewallRule, "end_port") 
	)
	{
		postCommands.push("/etc/init.d/firewall restart");
	}
	postCommands.push("/etc/init.d/share_users restart");
	postCommands.push("/etc/init.d/samba restart");
	postCommands.push("/etc/init.d/vsftpd restart");
	postCommands.push("/etc/init.d/nfsd restart");



	var commands = uci.getScriptCommands(uciOriginal) + "\n" +  makeDirCmds.join("\n") + "\n" + postCommands.join("\n") + "\n";

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

		var badUserIndex;
		for(badUserIndex=0;badUserIndex < badUserNames.length && errors.length == 0; badUserIndex++)
		{
			if(user.toLowerCase() == (badUserNames[badUserIndex]).toLowerCase() )
			{
				errors.push("Username '" + user + "' is reserved and not permitted");
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
		addOptionToSelectElement("user_access", user, user);
		userNames.push(user)

		document.getElementById("new_user").value = ""
		document.getElementById("user_pass").value = ""
		document.getElementById("user_pass_confirm").value = ""

	}

}


function removeUserCallback(table, row)
{
	var removeUser=row.childNodes[0].firstChild.data;

	//remove from userNames
	var newUserNames = removeStringFromArray(userNames, removeUser)
	userNames = newUserNames;

	//if no users left, set a message indicating this instead of an empty table
	if(userNames.length == 0)
	{
		var container = document.getElementById("user_table_container");
		var tableObject = document.createElement("div");
                tableObject.innerHTML = "<span style=\"text-align:center\"><em>No Share Users Defined</em></span>";
		setSingleChild(container, tableObject);
	}

	//remove in all shares
	for (sharePath in sharePathToShareData)
	{
		var shareData = sharePathToShareData[sharePath]
		var rwUsers   = shareData[9]
		var roUsers   = shareData[10]
		shareData[9]  = removeStringFromArray(rwUsers, removeUser) 
		shareData[10] = removeStringFromArray(roUsers, removeUser)
		sharePathToShareData[sharePath] = shareData;
	}

	//remove from controls of share currently being configured
	removeOptionFromSelectElement("user_access", removeUser);
	var accessTable = document.getElementById("user_access_table")
	if(accessTable != null)
	{
		var accessTableData = getTableDataArray(accessTable, true, false);
		var newTableData = [];
		while(accessTableData.length >0)
		{
			var next = accessTableData.shift();
			if(next[0] != removeUser)
			{
				newTableData.push(next)
			}
		}

		var newAccessTable =  createTable(["", ""], newTableData, "user_access_table", true, false, removeUserAccessCallback);
		setSingleChild(document.getElementById("user_access_table_container"), newAccessTable);
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
					var pass1 = editUserWindow.document.getElementById("new_password").value;
					var pass2 = editUserWindow.document.getElementById("new_password_confirm").value;
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

function updateWanFtpVisibility()
{
	document.getElementById("ftp_pasv_container").style.display = document.getElementById("ftp_wan_access").checked ? "block" : "none"
	var pasvCheck = document.getElementById("ftp_wan_pasv")
	enableAssociatedField(pasvCheck, "pasv_min_port", 50990)
	enableAssociatedField(pasvCheck, "pasv_max_port", 50999)
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

		// wan access to FTP
		var wanFtp = uciOriginal.get("firewall", ftpFirewallRule, "local_port") == "21"
		document.getElementById("ftp_wan_access").checked = wanFtp;
		var pminText = document.getElementById("pasv_min_port");
		var pmaxText = document.getElementById("pasv_max_port");
		var pmin = uciOriginal.get("firewall", pasvFirewallRule, "start_port")
		var pmax = uciOriginal.get("firewall", pasvFirewallRule, "end_port")
		pminText.value = pmin == "" || pmax == "" ? 50990 : pmin;
		pmaxText.value = pmin == "" || pmax == "" ? 50999 : pmax;
		document.getElementById("ftp_wan_pasv").checked = (wanFtp && pmin != "" && pmax != "") || (!wanFtp); //enable pasv by default when WAN FTP access is selected
		updateWanFtpVisibility()
	

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
						shareDirectory  = shareDirectory.replace(/^\//, "").replace(/\/$/, "")
						shareDrive = mountPointToDrive[shareMountPoint];
					}
				}

				
				if( shareDrive != null )
				{
					mountedDrives[ shareDrive ] = 1;
					
					//shareMountPoint->[shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, isCifs, isFtp, isNfs, anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
					var shareData = sharePathToShareData[fullSharePath] == null ? ["", "", "", "", "", false, false, false, "none", [], [], "ro", "*" ] :  sharePathToShareData[fullSharePath] ;
					
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
					shareData[5] = config == "samba"  ? true : shareData[5]  //isCIFS
					shareData[6] = config == "vsftpd" ? true : shareData[6]  //isFTP
					shareData[7] = config == "nfsd"   ? true : shareData[7]  //isNFS


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
										shareData[ 8 ] = readType
										
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
						if(allowedHostsStr instanceof Array)
						{
							allowedHostsStr = allowedHostsStr.join(",");
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
					if(nameToSharePath [ shareData[0] ] != fullSharePath)
					{
						nameToSharePath[ shareData[0] ] = fullSharePath
						sharePathList.push(fullSharePath)
					}
				}
			}
		}
		
		getMounted(sambaShares, "samba");
		getMounted(ftpShares, "vsftpd");
		getMounted(nfsShares, "nfsd");
		
		if(setDriveList(document))
		{
			document.getElementById("sharing_add_heading_container").style.display  = "block";
			document.getElementById("sharing_add_controls_container").style.display = "block";
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
		for(shareIndex=0; shareIndex < sharePathList.length; shareIndex++)
		{

			var shareData = sharePathToShareData[ sharePathList[shareIndex] ]
			var vis = []
			vis["cifs"] = shareData[5]
			vis["ftp"]  = shareData[6]
			vis["nfs"]  = shareData[7]
			shareTableData.push( [ shareData[0], shareData[1], "/" + shareData[3], getVisStr(vis), createEditButton(editShare) ] )
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

//returns (boolean) whether drive list is empty
function setDriveList(controlDocument)
{
		
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
	setAllowableSelections("share_disk", driveList, driveDisplayList, controlDocument);

	return (driveList.length > 0)
}

function shareSettingsToDefault(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument
	var currentDrive = getSelectedValue("share_disk", controlDocument);
	var defaultData = [ "share_" + (sharePathList.length + 1), currentDrive,  driveToMountPoints[currentDrive][0], "", driveToMountPoints[currentDrive][0], true, true, true, "none", [], [], "ro", "*" ] ;
	setDocumentFromShareData(controlDocument, defaultData)
}

function createUserAccessTable(clear, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument
	var userAccessTable = controlDocument.getElementById("user_access_table");
	if(clear || userAccessTable ==  null)
	{
		var container = controlDocument.getElementById("user_access_table_container");
		userAccessTable =  createTable(["", ""], [], "user_access_table", true, false, removeUserAccessCallback, null, controlDocument);
		setSingleChild(container, userAccessTable);
	}
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
		
		createUserAccessTable(false)
		var userAccessTable = controlDocument.getElementById("user_access_table");
		addTableRow(userAccessTable, [ addUser, access ], true, false, removeUserAccessCallback, null, controlDocument)
	}
}
function removeUserAccessCallback(table, row)
{
	var removeUser=row.childNodes[0].firstChild.data;
	addOptionToSelectElement("user_access", removeUser, removeUser, null, table.ownerDocument);
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
	var visStr = shareTypeStr("cifs") + "+" + shareTypeStr("ftp") + "+" + shareTypeStr("nfs");
	return visStr.replace(/\+\+/g, "+").replace(/^\+/, "").replace(/\+$/, "");
}

function getShareDataFromDocument(controlDocument, originalName)
{
	controlDocument = controlDocument == null ? document : controlDocument
	
	var shareDrive = getSelectedValue("share_disk", controlDocument);
	
	var shareSubdir = controlDocument.getElementById("share_dir").value
	shareSubdir     = shareSubdir.replace(/^\//, "").replace(/\/$/, "")

	var shareSpecificity = getSelectedValue("share_specificity", controlDocument);
	var shareName = controlDocument.getElementById("share_name").value;
	var shareDiskMount =  driveToMountPoints[shareDrive][ ( shareSpecificity == "blkid" ? 0 : 1 ) ];
	var altDiskMount   = driveToMountPoints[shareDrive][ ( shareSpecificity == "blkid" ? 1 : 0 ) ];
	var fullSharePath  = (shareDiskMount + "/" + shareSubdir).replace(/\/\//g, "/").replace(/\/$/, "");
	var altSharePath   = (altDiskMount + "/" + shareSubdir).replace(/\/\//g, "/").replace(/\/$/, "");

	var enabledTypes = getVis(controlDocument);
	



	var anonymousAccess = getSelectedValue("anonymous_access", controlDocument)
	var roUsers = [];
	var rwUsers = [];
	var userAccessTable = controlDocument.getElementById("user_access_table")
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
	var nfsAccess = getSelectedValue("nfs_access", controlDocument)
	var nfsAccessIps = getSelectedValue("nfs_policy", controlDocument) == "share" ? "*" : [];
	if( typeof(nfsAccessIps) != "string")
	{
		var nfsIpTable = controlDocument.getElementById("nfs_ip_table");
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
		if(originalName == null || existing[0] != originalName )
		{
			errors.push("Specified Shared Directory Is Already Configured: " + existing[0])
		}
	}
	if( nameToSharePath[ shareName ] != null && (originalName == null || originalName != shareName))
	{
		errors.push( "Share name is a duplicate" )
	}

	var result = [];
	result["errors"] = errors;
	if(errors.length == 0)
	{
		result["share"] = [shareName, shareDrive, shareDiskMount, shareSubdir, fullSharePath, enabledTypes["cifs"], enabledTypes["ftp"], enabledTypes["nfs"], anonymousAccess, rwUsers, roUsers, nfsAccess, nfsAccessIps]
	}
	return result;
}

function setDocumentFromShareData(controlDocument, shareData)
{
	controlDocument = controlDocument == null ? document : controlDocument
	
	var shareDrive = shareData[1]
	setDriveList(controlDocument);
	setSelectedValue("share_disk", shareDrive, controlDocument)
	controlDocument.getElementById("share_dir").value = "/" + shareData[3]
	controlDocument.getElementById("share_name").value = shareData[0]
	
	var shareSpecificity = (shareData[2] == driveToMountPoints[shareDrive][0]) ? "blkid" : "dev";
	setSelectedValue("share_specificity", shareSpecificity, controlDocument)
	
	
	controlDocument.getElementById("share_type_cifs").checked = shareData[5]
	controlDocument.getElementById("share_type_ftp").checked  = shareData[6]
	controlDocument.getElementById("share_type_nfs").checked  = shareData[7]

	setSelectedValue("anonymous_access", shareData[8], controlDocument)
	createUserAccessTable(true,controlDocument)
	var userAccessTable = controlDocument.getElementById("user_access_table")
	var userIndices = [];
	userIndices[9] = "rw"
	userIndices[10] = "ro"
	var userTypeIndex
	var usersWithEntries = [];
	for( userTypeIndex in userIndices)
	{
		var userType = userIndices[userTypeIndex]
		var userList = shareData[ userTypeIndex ];
		var ulIndex;
		for(ulIndex=0;ulIndex < userList.length; ulIndex++)
		{
			addTableRow(userAccessTable, [ userList[ulIndex], userType == "rw" ? "R/W" : "R/O" ], true, false, removeUserAccessCallback, null, controlDocument)
			usersWithEntries[ userList[ulIndex] ] = 1
		}
	}
	
	setSelectedValue("nfs_access", shareData[11], controlDocument);
	var nfsAccessIps = shareData[12];
	setSelectedValue("nfs_policy", typeof(nfsAccessIps) == "string" ? "share" : "ip", controlDocument);
	if(nfsAccessIps instanceof Array)
	{
		addAddressStringToTable(controlDocument,nfsAccessIps.join(","),"nfs_ip_table_container","nfs_ip_table",false, 2, true, 250)
	}

	//update user select element
	var usersWithoutEntries = [];
	var uIndex;
	for(uIndex = 0; uIndex < userNames.length; uIndex++)
	{
		var u = userNames[uIndex];
		if(usersWithEntries[ u ] != 1)
		{
			usersWithoutEntries.push(u);
		}
	}
	setAllowableSelections("user_access", usersWithoutEntries, usersWithoutEntries, controlDocument);

	setShareTypeVisibility(controlDocument)
	setSharePaths(controlDocument);

}

function addNewShare()
{
	var rawShareData = getShareDataFromDocument(document, null)
	var errors = rawShareData["errors"]
	if(errors.length > 0)
	{
		var errStr = errors.join("\n") + "\n\nCould Not Add Share"
		alert(errStr)
	}
	else
	{
		var shareData = rawShareData["share"]
		
		var shareName = shareData[0]
		var fullSharePath = shareData[4];
		var shareType = getVisStr( getVis() );

		sharePathToShareData[ fullSharePath ] = shareData
		sharePathList.push(fullSharePath)
		nameToSharePath [ shareName ] = fullSharePath

		var shareTable = document.getElementById("share_table")
		addTableRow(shareTable, [shareName, shareData[1], "/" + shareData[3], shareType, createEditButton(editShare) ], true, false, removeShareCallback)
		
		shareSettingsToDefault();
	}

}

function setShareTypeVisibility(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument
	
	var vis = getVis(controlDocument);
	vis["ftp_or_cifs"] = vis["ftp"] || vis["cifs"]

	var getTypeDisplay = function(type) { return vis[type] ? type.toUpperCase() : "" }
	var userLabel = (getTypeDisplay("cifs") + "/" + getTypeDisplay("ftp")).replace(/^\//, "").replace(/\/$/, "");
	setChildText("anonymous_access_label",  userLabel + " Anonymous Access:", null, null, null, controlDocument)
	setChildText("user_access_label",  userLabel + " Users With Access:", null, null, null, controlDocument)

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
			controlDocument.getElementById( ids[idIndex] ).style.display = vis[ idType ] ? "block" : "none"
		}
	}
	if(vis["nfs"])
	{
		setInvisibleIfIdMatches("nfs_policy",  ["share"], "nfs_ip_container",    "block", controlDocument);
	}
}

function setSharePaths(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var escapedName=escape(controlDocument.getElementById("share_name").value);
	var ip = uciOriginal.get("network", "lan", "ipaddr");
	if(controlDocument.getElementById("share_type_nfs").checked)
	{
		controlDocument.getElementById("nfs_path_container").style.display = "block";
		setChildText("nfs_path", ip + ":/nfs/" + escapedName, null, null, null, controlDocument);
	}
	else
	{
		controlDocument.getElementById("nfs_path_container").style.display = "none";
	}
	if(controlDocument.getElementById("share_type_ftp").checked)
	{
		controlDocument.getElementById("ftp_path_container").style.display = "block";
		setChildText("ftp_path", "ftp://" + ip + "/" + escapedName, null, null, null, controlDocument);
	}
	else
	{
		controlDocument.getElementById("ftp_path_container").style.display = "none";
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
	var removeName=row.childNodes[0].firstChild.data;
	var removePath = nameToSharePath[removeName]

	delete nameToSharePath[removeName]
	delete sharePathToShareData[removePath]
	
	var newSharePathList = []
	while(sharePathList.length > 0)
	{
		var next = sharePathList.shift()
		if(next != removePath){ newSharePathList.push(next) }
	}
	sharePathList = newSharePathList;

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

	editRow=this.parentNode.parentNode;
	editName=editRow.childNodes[0].firstChild.data;
	editPath=nameToSharePath[editName];
	editShareData=sharePathToShareData[ editPath ];


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
			
				setDocumentFromShareData(editShareWindow.document, editShareData)	
				
				closeButton.onclick = function()
				{
					editShareWindow.close();
				}
				saveButton.onclick = function()
				{
					var rawShareData = getShareDataFromDocument(editShareWindow.document, editName)
					var errors = rawShareData["errors"];
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not update share.");
					}
					else
					{
						var shareData = rawShareData["share"]
						var shareName = shareData[0]
						var fullSharePath = shareData[4];
						var shareType = getVisStr( getVis(editShareWindow.document) );

						if(editName != shareName)
						{
							delete nameToSharePath[ editName ] 
						}
						if(editPath != fullSharePath)
						{
							var newSharePathList = []
							while(sharePathList.length > 0)
							{
								var next = sharePathList.shift();
								if(next != editPath)
								{
									newSharePathList.push(next)
								}
							}
							newSharePathList.push(fullSharePath)
							sharePathList = newSharePathList
							delete sharePathToShareData[ editPath ]  
						}
						sharePathToShareData[ fullSharePath ] = shareData
						nameToSharePath [ shareName ] = fullSharePath

						editRow.childNodes[0].firstChild.data = shareName
						editRow.childNodes[1].firstChild.data = shareData[1]
						editRow.childNodes[2].firstChild.data = "/" + shareData[3]
						editRow.childNodes[3].firstChild.data = shareType

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












function unmountAllUsb()
{
	setControlsEnabled(false, true, "Unmounting Disks");

	
	var commands = "/etc/init.d/samba stop ; /etc/init.d/vsftpd stop ; /etc/init.d/nfsd stop ; /etc/init.d/usb_storage stop ; "

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
	var invalidFunc = function(){ alert("ERROR: Invalid Password"); setControlsEnabled(true); }

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


