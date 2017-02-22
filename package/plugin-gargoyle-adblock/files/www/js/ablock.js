/*
 * This program is copyright © 2015 Michael Gray based on the work of teffalump and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var ablock=new Object(); //part of i18n

var pkg = "adblock";
var sec = "config";

function resetData()
{
	var enabled = uciOriginal.get(pkg, sec, "enabled");
	document.getElementById("adblock_enable").checked = enabled == 1;
	updateStatus(enabled);
	updateLastrun();
	var transparent = uciOriginal.get(pkg, sec, "trans");
	document.getElementById("adblock_transparent").checked = transparent == 1;
	var exempt = uciOriginal.get(pkg, sec, "exempt");
	document.getElementById("adblock_exempten").checked = exempt == 1;
	var exstart = uciOriginal.get(pkg, sec, "exstart");
	document.getElementById('adblock_exempts').value = exstart;
	var exend = uciOriginal.get(pkg, sec, "exend");
	document.getElementById('adblock_exemptf').value = exend;
	initializeDescriptionVisibility(uciOriginal, "adblock_help");
	initializeDescriptionVisibility(uciOriginal, "adblock_help2");
	uciOriginal.removeSection("gargoyle", "help");
	
	if(enabled == 1)
	{
		document.getElementById("list_gui").style.display = "block";
		document.getElementById("adblock_displayed_count").innerHTML=ablock.ADBLOCKCounter+" "+document.getElementById("adblock_blocklist_list").length+"/"+blocklistlines.length;
		document.getElementById("adblock_blocklist_list").innerHTML="";
		
		var errorflag = 0;
		
		for (x = 0; x < whitelistlines.length; x++)
		{
			var list = whitelistlines[x].toString();
			var toobig = 0;
			AddOpt = new Option(list, list);
			document.getElementById("adblock_whitelist_list").options[x] = AddOpt;
			toobig = document.getElementById("adblock_whitelist_list").length;
			if(toobig == 10000)
			{
				errorflag = 1;
				break;
			}
		}
		if(errorflag == 1)
		{
			alert(ablock.ADBLOCKWhitebig);
		}
		errorflag = 0;
		for (x = 0; x < blacklistlines.length; x++)
		{
			var list = blacklistlines[x].toString();
			AddOpt = new Option(list, list);
			document.getElementById("adblock_blacklist_list").options[x] = AddOpt;
			toobig = document.getElementById("adblock_blacklist_list").length;
			if(toobig == 10000)
			{
				errorflag = 1;
				break;
			}
		}
		if(errorflag == 1)
		{
			alert(ablock.ADBLOCKBlackbig);
		}
		errorflag = 0;
	}
	else
	{
		document.getElementById("list_gui").style.display = "none";
	}
}

function updateLastrun()
{
	var enabled = uciOriginal.get(pkg, sec, "enabled");
	var lastrun = uciOriginal.get(pkg, sec, "lastrun");
	if(enabled == 1)
	{
		document.getElementById('adblock_lastrunval').innerHTML = lastrun;
	}
	else
	{
		document.getElementById('adblock_lastrunval').innerHTML = '-';
	}
}

function updateStatus(enabled)
{
	setElementEnabled(document.getElementById("adblock_update"), enabled == 1);
}

function populateLists(params)
{
	params.push("touch /tmp/blacklist");
	params.push("touch /tmp/whitelist");
	for (x = 0; x < document.getElementById("adblock_blacklist_list").length; x++)
	{
		params.push("echo \"" + document.getElementById("adblock_blacklist_list").options[x].value.toString() + "\" >> /tmp/blacklist");
	}
	for (x = 0; x < document.getElementById("adblock_whitelist_list").length; x++)
	{
		params.push("echo \"" + document.getElementById("adblock_whitelist_list").options[x].value.toString() + "\" >> /tmp/whitelist");
	}
	
	params.push("logger -t ADBLOCK Modifying black/white lists from GUI");
	
	params.push("sort -u /tmp/blacklist > /plugin_root/usr/lib/adblock/black.list");
	params.push("sort -u /tmp/whitelist > /plugin_root/usr/lib/adblock/white.list");
	params.push("rm -f /tmp/blacklist");
	params.push("rm -f /tmp/whitelist");
	
	return params;
}

function adblockUpdate()
{
	var Commands = [];
	
	populateLists(Commands);
	
	Commands.push("sh /usr/lib/adblock/runadblock.sh");
	

	setControlsEnabled(false, true, UI.WaitSettings);
	var param = getParameterDefinition("commands", Commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			location.reload(true);	//reload the page
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function transferwhiteList()
{
	var index = document.getElementById("adblock_blocklist_list").selectedIndex;
	var x;
	
	while(index != -1)
	{
		x = document.getElementById("adblock_blocklist_list").options[index].value.toString();
		y = document.getElementById("adblock_whitelist_list").length;
		AddOpt = new Option(x, x);
		document.getElementById("adblock_whitelist_list").options[y] = AddOpt;
		document.getElementById("adblock_blocklist_list").options[index] = null;
		index = document.getElementById("adblock_blocklist_list").selectedIndex;
	}
}

function deleteList(id)
{
	var index = id.selectedIndex;
	
	while(index != -1)
	{
		id.options[index] = null;
		index = id.selectedIndex;
	}
}

function searchBlocklist()
{
	var x = 0;
	var y = 0;
	var errorflag = 0;
	var toobig = 0;
	
	document.getElementById("adblock_blocklist_list").innerHTML = "";
	
	if(document.getElementById("adblock_blocklist_search").value.toString() == "")
	{
		return false;
	}
	
	var searchvar = new RegExp(document.getElementById("adblock_blocklist_search").value.toString(),'i');
	for (x = 0; x < blocklistlines.length; x++)
	{
		var list = blocklistlines[x].toString();
		var check = list.match(searchvar);
		if (check != null)
		{
			AddOpt = new Option(list, list);
			document.getElementById("adblock_blocklist_list").options[y] = AddOpt;
			y+=1;
		}
		
		toobig = document.getElementById("adblock_blocklist_list").length;
		if(toobig == 10000)
		{
			errorflag = 1;
			break;
		}
	}
	
	document.getElementById("adblock_displayed_count").innerHTML=ablock.ADBLOCKCounter+" "+document.getElementById("adblock_blocklist_list").length+"/"+blocklistlines.length;
	
	if(errorflag == 1)
	{
		window.alert(ablock.ADBLOCKSearchtoobig);
	}
}

function addBlacklist()
{
	var x = document.getElementById("adblock_blacklist_add").value.toString();
	var y = document.getElementById("adblock_blacklist_list").length;
	if(x == "")
	{
		return false;
	}
	AddOpt = new Option(x, x);
	document.getElementById("adblock_blacklist_list").options[y] = AddOpt;
	document.getElementById("adblock_blacklist_add").value = null;
}

function saveChanges()
{
	var Commands = [];
	var enabled = document.getElementById("adblock_enable").checked ? "1":"0";
	var transparent = document.getElementById('adblock_transparent').checked ? "1":"0";
	var exempt = document.getElementById('adblock_exempten').checked ? "1":"0";
	var exstart = document.getElementById('adblock_exempts').value;
	var exend = document.getElementById('adblock_exemptf').value;

	if((exempt == 1 && exstart == "") || (exempt == 1 && exend == ""))
	{
		alert(ablock.ERRrange);
		return;
	}


	var uci = uciOriginal.clone()

	uci.set(pkg, sec, "enabled", enabled);
	uci.set(pkg, sec, "trans", transparent);
	uci.set(pkg, sec, "exempt", exempt);
	uci.set(pkg, sec, "exstart", exstart);
	uci.set(pkg, sec, "exend", exend);



	if(enabled==1)
	{
		if(document.getElementById("list_gui").style.display == "block")
		{
			populateLists(Commands);
		}
		Commands.push("sh /usr/lib/adblock/runadblock.sh -enable");
	}
	else
	{
		Commands.push("sh /usr/lib/adblock/runadblock.sh -disable");
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
			updateLastrun();
			setControlsEnabled(true);
			location.reload(true);	//reload the page
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
