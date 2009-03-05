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
	
	var commands = "\n/etc/init.d/webmon_gargoyle stop\n/etc/init.d/bwmon_gargoyle stop\n/etc/init.d/restricter_gargoyle stop\nreboot\n";
	var param = getParameterDefinition("commands", commands);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
			
	doRedirect= function()
	{
		window.location.reload(false);
	}
	setTimeout( "doRedirect()", 90000);
}

