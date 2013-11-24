/*
 * This program is copyright © 2013 Cezary Jackiewicz and Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var dlna=new Object();

var pkg = "minidlna";
var sec = "config";

function resetData()
{
	var enabled = uciOriginal.get(pkg, sec, "enabled");
	document.getElementById("dlna_enable").checked = enabled == 1;
	updateStatus(enabled);

	var name = uciOriginal.get(pkg, sec, "friendly_name");
	document.getElementById('dlna_name').value = name;

	var strict = uciOriginal.get(pkg, sec, "strict_dlna");
	document.getElementById("dlna_strict").checked = strict == 1;

	var rootDriveDisplay = [];
	var rootDriveValues  = [];
	var driveIndex;
	for(driveIndex=0;driveIndex < storageDrives.length; driveIndex++)
	{
		rootDriveDisplay.push( storageDrives[driveIndex][0] + ": " + parseBytes(storageDrives[driveIndex][4]) + " " + dlna.Totl + ", " + parseBytes(storageDrives[driveIndex][5]) + " " + dlna.Free )
		rootDriveValues.push( storageDrives[driveIndex][0] )
	}
	setAllowableSelections("drive_select", rootDriveValues, rootDriveDisplay, document);
	document.getElementById("media_dir").value = "/";

	var columnNames = [dlna.Drv, dlna.Dir, dlna.DLNAMType];
	var mediaTableData = [];
	var mediaDir = [];
	mediaDir = uciOriginal.get(pkg, sec, "media_dir");
	for (idx=0; idx < mediaDir.length; idx++)
	{
		
		var md = mediaDir[idx];
		var mediaType = dlna.DLNAAll;
		if(md.charAt(1) == ',')
		{
			mediaType = "" + md.charAt(0);
			md = md.substr(2);
		}
		
		
		var drive = "root";
		var folder = md;
		var storageIndex;
		for(storageIndex=0; storageIndex < storageDrives.length && drive == "root"; storageIndex++)
		{
			var sd = storageDrives[storageIndex];
			var sdre = new RegExp("^" + sd[1], "g");
			if( md.match(sdre) )
			{
				drive = sd[0];
				folder = folder.replace(sdre, "/");
				while(folder.match(/\/\//g))
				{
					folder = folder.replace("//", "/");
				}
			}
		}
		mediaTableData.push([drive, folder, mediaType]);
	}
	var mediaTable = createTable(columnNames, mediaTableData, "media_table", true, false, removeCallback);
	var tableContainer = document.getElementById('media_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(mediaTable);
}

function removeCallback()
{
}

function addNewMediaDir()
{
	var drive = getSelectedValue("drive_select");
	var folder = document.getElementById('media_dir').value;


	var mediaType = getSelectedValue("media_type");
	if (mediaType == "")
	{
		mediaType = dlna.DLNAAll;
	}
	

	var errors = [];
	var mediaTable = document.getElementById("media_table");
	if(mediaTable != null)
	{
		var found = 0;
		var mediaData = getTableDataArray(mediaTable, true, false);
		var idx;
		for(idx=0; idx < mediaData.length && found == 0; idx++)
		{
			found = mediaData[idx][0] == folder ? 1 : found;
		}
		if(found)
		{
			errors.push(dlna.ERRAllrAdd)
		}
	}
	if(errors.length > 0)
	{
		alert( errors.join("\n") + "\n\n"+UI.ErrChanges);
	}
	else
	{
		if(mediaTable == null)
		{
			var tableContainer = document.getElementById("media_table_container");
			mediaTable = createTable([dlna.Drv, dlna.Dir, dlna.DLNAMType], [], "media_table", true, false, removeCallback);
			setSingleChild(tableContainer, mediaTable);
		}
		addTableRow(mediaTable, [ drive, folder, mediaType ], true, false, removeCallback)
		document.getElementById("media_dir").value = "/"
		setSelectedValue("media_type", "");
	}
}

function saveChanges()
{
	var Commands = [];
	var enabled = document.getElementById("dlna_enable").checked ? "1":"0";
	var name = document.getElementById('dlna_name').value;

	if(name == "")
	{
		alert(dlna.ERRSName);
		return;
	}


	var strict = document.getElementById("dlna_strict").checked ? "1":"0";
	

	var uci = uciOriginal.clone()

	uci.set(pkg, sec, "enabled", enabled);
	uci.set(pkg, sec, "friendly_name", name);
	uci.set(pkg, sec, "strict_dlna", strict);



	uci.remove(pkg, sec, "media_dir");
	var mediaTable = document.getElementById("media_table");

	if(mediaTable != null)
	{
		var mediaData = getTableDataArray(mediaTable, true, false);
		var idx;
		var media = [];

		for(idx=0; idx < mediaData.length; idx++)
		{
			var drive = mediaData[idx][0];
			var folder = "/" + mediaData[idx][1];
			var mediaType = mediaData[idx][2];
			var found = false;
		
			var mediaPath = folder;
			var storageIndex;
			for(storageIndex=0; storageIndex < storageDrives.length && (!found) ; storageIndex++)
			{
				if(drive == storageDrives[storageIndex][0])
				{
					mediaPath = storageDrives[storageIndex][1] + folder;
				}
			}
			while(mediaPath.match(/\/\//g))
			{
				mediaPath = mediaPath.replace("//", "/");
			}
			if(mediaType != dlna.DLNAAll)
			{
				mediaPath = mediaType + "," + mediaPath;
			}
			media.push(mediaPath);
		}
		if(media.length > 0)
		{
			var base = (media[0]).replace(/[\/]+$/g, "").replace(/.*,/, "");
			uci.set(pkg, sec, "db_dir", base + "/_minidlna");
			uci.set(pkg, sec, "log_dir", base + "/_minidlna");
			uci.createListOption(pkg, sec, "media_dir", true);
			uci.set(pkg, sec, "media_dir", media, false)
		}
	}

	Commands.push("/etc/init.d/minidlna stop");
	Commands.push("kill $(pidof minidlnad)");
	if(enabled==1)
	{
		Commands.push("sleep 2");
		Commands.push("/etc/init.d/minidlna enable");
		Commands.push("/etc/init.d/minidlna start");
	}
	else
	{
		Commands.push("/etc/init.d/minidlna disable");
	}
	var commands = uci.getScriptCommands(uciOriginal) + "\n" + Commands.join("\n");

	setControlsEnabled(false, true, UI.WaitSettings);
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			updateStatus(enabled);
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function updateStatus(enabled)
{
	setElementEnabled(document.getElementById("rescan_button"), enabled == 1);
	setElementEnabled(document.getElementById("status_button"), enabled == 1);
}

function statusDlna()
{
	window.location.href="http://" + currentLanIp + ":" + uciOriginal.get(pkg, sec, "port");
}

function rescanMedia()
{
	var Commands = [];
	Commands.push("/etc/init.d/minidlna stop");
	Commands.push("kill -9 $(pidof minidlnad)");
	Commands.push("rm -f $(uci get minidlna.config.db_dir)/files.db");
	Commands.push("/etc/init.d/minidlna start");

	setControlsEnabled(false, true, UI.WaitSettings);
	var param = getParameterDefinition("commands", Commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
