/*
 * This program is copyright © 2015 Michael Gray based on the work of teffalump and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var Adblk=new Object(); //part of i18n

var pkg = "adblock";
var sec = "config";

function resetData()
{
	var enabled = uciOriginal.get(pkg, sec, "enabled");
	document.getElementById("adblock_enable").checked = enabled == 1;
	updateStatus(enabled);
	updateLastrun();
	var onlywireless = uciOriginal.get(pkg, sec, "onlywireless");
	document.getElementById("adblock_wireless").checked = onlywireless == 1;
	var transparent = uciOriginal.get(pkg, sec, "trans");
	document.getElementById("adblock_transparent").checked = transparent == 1;
	var exempt = uciOriginal.get(pkg, sec, "exempt");
	document.getElementById("adblock_exempten").checked = exempt == 1;
	var exstart = uciOriginal.get(pkg, sec, "exstart");
	document.getElementById('adblock_exempts').value = exstart;
	var exend = uciOriginal.get(pkg, sec, "exend");
	document.getElementById('adblock_exemptf').value = exend;
	initializeDescriptionVisibility(uciOriginal, "adblock_help");
	uciOriginal.removeSection("gargoyle", "help");
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

function adblockUpdate()
{
	var Commands = [];
	Commands.push("/usr/lib/gargoyle/runadblock.sh");
	Commands.push("sleep 2");

	setControlsEnabled(false, true, UI.WaitSettings);
	var param = getParameterDefinition("commands", Commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			document.getElementById('adblock_lastrunval').innerHTML = 'Today';
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	updateLastrun();
}

function saveChanges()
{
	var Commands = [];
	var enabled = document.getElementById("adblock_enable").checked ? "1":"0";
	var onlywireless = document.getElementById('adblock_wireless').checked ? "1":"0";
	var transparent = document.getElementById('adblock_transparent').checked ? "1":"0";
	var exempt = document.getElementById('adblock_exempten').checked ? "1":"0";
	var exstart = document.getElementById('adblock_exempts').value;
	var exend = document.getElementById('adblock_exemptf').value;

	if((exempt == 1 && exstart == "") || (exempt == 1 && exend == ""))
	{
		alert(adblock.ERRrange);
		return;
	}


	var uci = uciOriginal.clone()

	uci.set(pkg, sec, "enabled", enabled);
	uci.set(pkg, sec, "onlywireless", onlywireless);
	uci.set(pkg, sec, "trans", transparent);
	uci.set(pkg, sec, "exempt", exempt);
	uci.set(pkg, sec, "exstart", exstart);
	uci.set(pkg, sec, "exend", exend);



	if(enabled==1)
	{
		Commands.push("/usr/lib/gargoyle/runadblock.sh -enable");
	}
	else
	{
		Commands.push("/usr/lib/gargoyle/runadblock.sh -disable");
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
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
