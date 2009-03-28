/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

function reboot()
{
	setControlsEnabled(false, true, "System Is Now Rebooting");
	
	var commands = "\nsh /www/utility/reboot.sh\n";
	var param = getParameterDefinition("commands", commands);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + window.location.host + "/utility/reboot_test.sh";
	rebootTests=0;	
	doRebootTest= function()
	{
		document.getElementById("reboot_test").src = testLocation ; 
		rebootTests++;
			
		//give up after 5 minutes
		if(rebootTests < 60)
		{
			setTimeout("doRebootTest()", 5*1000);
		}
		else
		{
			reloadPage();
		}
	}
	setTimeout( "doRebootTest()", 25*1000);
}

function reloadPage()
{
	document.getElementById("reboot_test").src = "";
	window.location.href = window.location.href;
}
