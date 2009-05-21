/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var toggleReload = false;

function reboot()
{
	setControlsEnabled(false, true, "System Is Now Rebooting");
	
	var commands = "\nsh /usr/lib/gargoyle/reboot.sh\n";
	var param = getParameterDefinition("commands", commands);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + window.location.host + "/utility/reboot_test.sh";
	testReboot = function()
	{
		toggleReload = true;
		setTimeout( "testReboot()", 5*1000);  //try again after 5 seconds
		document.getElementById("reboot_test").src = testLocation; 
	}
	setTimeout( "testReboot()", 25*1000);  //start testing after 15 seconds
	setTimeout( "reloadPage()", 240*1000); //after 4 minutes, try to reload anyway

}

function reloadPage()
{
	if(toggleReload)
	{
		//IE calls onload even when page isn't loaded -- it just times out and calls it anyway
		//We can test if it's loaded for real by looking at the (IE only) readyState property
		//For Browsers NOT designed by dysfunctional cretins whose mothers were a pack of sewer-dwelling, shit-eating rodents,
		//well, for THOSE browsers, readyState (and therefore reloadState) should be null 
		var reloadState = document.getElementById("reboot_test").readyState;
		if( typeof(reloadState) == "undefined" || reloadState == null || reloadState == "complete")
		{
			toggleReload = false;
			document.getElementById("reboot_test").src = "";
			window.location.href = window.location.href;
		}
	}
}
