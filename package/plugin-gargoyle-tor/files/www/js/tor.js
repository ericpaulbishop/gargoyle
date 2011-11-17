/*
 * This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function saveChanges()
{
	setControlsEnabled(false, true);
	var uci = uciOriginal.clone();
	uci.set('tor', 'global', 'enabled', getSelectedValue("tor_enabled"))
	if(getSelectedValue("tor_enabled") == "1")
	{
		uci.set('tor', 'global', 'block_unsupported_proto',  getSelectedValue("tor_other_proto"))
	}
	var commands = uci.getScriptCommands(uciOriginal) + "\n" + "/etc/init.d/tor restart" + "\n";
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			resetData();
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}

function resetData()
{
	var en = uciOriginal.get("tor", "global", "enabled") 
	var torEnabled = uciOriginal.get("tor", "global", "enabled")
	torEnabled = (torEnabled != "1" && torEnabled != "2") ? "0" : torEnabled
	var blockOtherProtos = uciOriginal.get("tor", "global", "block_unsupported_proto") == "1" ? "1" : "0"
	setSelectedValue("tor_enabled", torEnabled)
	setSelectedValue("tor_other_proto", blockOtherProtos)
	setVisibility()
}

function setVisibility()
{
	document.getElementById("tor_other_proto_container").style.display = getSelectedValue("tor_enabled") == "0" ? "none" : "block"
}

