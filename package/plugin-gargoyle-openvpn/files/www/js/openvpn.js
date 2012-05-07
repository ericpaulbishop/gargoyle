/*
 * This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


function saveChanges()
{
	var errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		var uci = uciOriginal.clone();
		var commands = "";
		
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				setControlsEnabled(true)
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


function proofreadAll()
{
	var errors = []
	return errors;
}



function resetData()
{
	var serverEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled") 
	var clientEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled")
	serverEnabled = serverEnabled == "true" || serverEnabled == "1" ? true : false;
	clientEnabled = clientEnabled == "true" || clientEnabled == "1" ? true : false;


}

