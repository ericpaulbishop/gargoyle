/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var idtS=new Object(); //part of i18n

function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
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
		if(!isBridge(uciOriginal))
		{
			uci.set("dhcp", dnsmasqSections[0], "domain", domain);
		}
		
		var gargLogoHostname = document.getElementById("garg_host");
		gargLogoHostname.replaceChild( document.createTextNode(idtS.DevNm+": " + hostname), gargLogoHostname.firstChild );

		
		var commands = uci.getScriptCommands(uciOriginal) + "\necho \"" + hostname + "\" > /proc/sys/kernel/hostname \n" + (havePrinterScript ? "\nsh /usr/lib/gargoyle/configure_printer.sh\n" : "")
		
		
		//document.getElementById("output").value = commands;


		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);
				//alert(req.responseText);
				window.location.href=window.location.href; //need to reload to refresh section names which may have changed
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	var notEmpty = function(text){ return validateLengthRange(text,1,999); }
	var errors;
	if(!isBridge(uciOriginal))
	{
		errors = proofreadFields( ["hostname", "domain"], ["hostname_label", "domain_label"], [notEmpty, notEmpty], [0,0], ["hostname", "domain"]); 
	}
	else
	{
		errors = proofreadFields( ["hostname"], ["hostname_label"], [notEmpty], [0], ["hostname"]);
	}
	return errors;
}

function resetData()
{
	var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	var dnsmasqSections= uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq");
	var hostname = uciOriginal.get("system", systemSections[0], "hostname");
	var domain = uciOriginal.get("dhcp", dnsmasqSections[0], "domain");
	document.getElementById("hostname").value = hostname;
	document.getElementById("domain").value = domain;
	if(isBridge(uciOriginal))
	{
		document.getElementById("domain_container").style.display = "none";
	}

}
