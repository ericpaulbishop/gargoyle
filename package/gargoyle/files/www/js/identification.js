/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		document.body.style.cursor="wait";
		document.getElementById("save_button").style.display="none";
		document.getElementById("reset_button").style.display="none";
		document.getElementById("update_container").style.display="block";

		systemSections = uciOriginal.getAllSections("system");
		uciOriginal.removeSection("system", systemSections[0]);
		
		
		uci = uciOriginal.clone();
		hostname =  document.getElementById("hostname").value;
		uci.set("system", "system", "", "system");
		uci.set("system", "system", "hostname", document.getElementById("hostname").value);

		
		gargLogoHostname = document.getElementById("garg_host");
		gargLogoHostname.replaceChild( document.createTextNode("Device Name: " + hostname), gargLogoHostname.firstChild );

		
		commands = "uci del system." + systemSections[0] + "\nuci commit\n" + uci.getScriptCommands(uciOriginal) + "\necho \"" + hostname + "\" > /proc/sys/kernel/hostname \n";
		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				document.getElementById("update_container").style.display="none";		
				document.getElementById("save_button").style.display="inline";
				document.getElementById("reset_button").style.display="inline";
				document.body.style.cursor='auto';
			
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	return proofreadFields( ["hostname"], ["hostname_label"], [function(text){ return validateLengthRange(text,1,999); }], [0], ["hostname"]);
}

function resetData()
{
	systemSections = uciOriginal.getAllSections("system");
	hostname = uciOriginal.get("system", systemSections[0], "hostname");
	document.getElementById("hostname").value = hostname;
}
