
function resetData()
{
	document.getElementById("shared_disks").style.display = storageDrives.length > 0 ? "block" : "none";
	document.getElementById("unmount").style.display = storageDrives.length > 0 ? "block" : "none";
	document.getElementById("format").style.display = physicalDrives.length > 0 ? "block" : "none";
	document.getElementById("no_disks").style.display = physicalDrives.length == 0 & storageDrives.length == 0 ? "block" : "none";

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

		var mountPointToDrive = [];
		var mountPointToFs = [];
		var mountPointToDriveSize = [];
		var driveIndex = 0;
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			mountPointToDrive[ storageDrives[driveIndex][1] ] = storageDrives[driveIndex][0];
			mountPointToFs[ storageDrives[driveIndex][1] ]    = storageDrives[driveIndex][2];
			mountPointToDriveSize[ storageDrives[driveIndex][1] ]    = storageDrives[driveIndex][3];

		}
		
		var mountedDrives = [];
		var mountPointToDriveData = [];
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
					
					//device->[mountpoint, name, filesystem, size, sharetype, access]
					var driveData = mountPointToDriveData[share] == null ? ["", "", "", "", "", createEditButton()] : mountPointToDriveData[share];
					driveData[0] = uciOriginal.get(config, shareList[shareIndex], "name");
					driveData[1] = mountPointToFs[share];
					driveData[2] = parseBytes( mountPointToDriveSize[share] ).replace(/ytes/, "");
					driveData[3] = driveData[3].length > 0 ? driveData[3] + "+" + config : config;
					driveData[4] = uciOriginal.get(config, shareList[shareIndex], "read_only");
					driveData[4] = driveData[5] == "1" || driveData[5] == "yes" ? "Read Only" : "Read/Write";



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
		}
		else
		{
			document.getElementById("sharing_add_heading_container").style.display  = "none";
			document.getElementById("sharing_add_controls_container").style.display = "none";
		}
		setPolicyVisibility();

		//create current share table
		//name, file system, size, type, access, [edit], [remove]
		var mountTableData = [];
		for(driveIndex=0; driveIndex < storageDrives.length; driveIndex++)
		{
			if( mountPointToDriveData[ storageDrives[driveIndex][1] ] != null )
			{
				mountType = (mountPointToDriveData[ storageDrives[driveIndex][1] ])[3];
				mountType = mountType.replace(/samba/, "CIFS");
				mountType = mountType.replace(/nfsd/, "NFS");
				(mountPointToDriveData[ storageDrives[driveIndex][1] ])[3] = mountType;
				mountTableData.push( mountPointToDriveData[ storageDrives[driveIndex][1] ] );
			}
		}
		mountTable = createTable(["Name", "File System", "Size", "Mount Type", "Access", ""], mountTableData, "mount_table", true, false, function(){ return 1; });
		tableContainer = document.getElementById('sharing_mount_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(mountTable);
		

	}

	//format settings
	//if(physicalDrives.length > 0)
	//{
	//
	//}

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
	//editButton.onclick = editQuota;
	
	editButton.className = "default_button"  ;
	editButton.disabled  = false ;

	return editButton;
}
