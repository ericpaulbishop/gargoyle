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
		setControlsEnabled(false, true);

		var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
		var dnsmasqSections= uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq");
	
		
		var uci = uciOriginal.clone();
		var hostname = document.getElementById("hostname").value;
		var domain =   document.getElementById("domain").value;
		uci.set("system", systemSections[0], "hostname", hostname);
		uci.set("dhcp", dnsmasqSections[0], "domain", domain);

		
		var gargLogoHostname = document.getElementById("garg_host");
		gargLogoHostname.replaceChild( document.createTextNode("Device Name: " + hostname), gargLogoHostname.firstChild );

		
		var commands = uci.getScriptCommands(uciOriginal) + "\necho \"" + hostname + "\" > /proc/sys/kernel/hostname \n";
		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);
				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	var notEmpty = function(text){ return validateLengthRange(text,1,999); }
	return proofreadFields( ["hostname", "domain"], ["hostname_label", "domain"], [notEmpty, notEmpty], [0,0], ["hostname", "domain"]); 
}

function resetData()
{
	var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	var dnsmasqSections= uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq");
	var hostname = uciOriginal.get("system", systemSections[0], "hostname");
	var domain = uciOriginal.get("dhcp", dnsmasqSections[0], "domain");
	document.getElementById("hostname").value = hostname;
	document.getElementById("domain").value = domain;
}
