/*
 * This program is copyright © 2012 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

function checkKey(e)
{
        var keycode = 0;

        if ( window.event )
                keycode = window.event.keyCode;
        else if ( e )
                keycode = e.which;
        if ( keycode == 13 )
                runCmd();
}

function runCmd()
{
	var Commands = [];

	Commands.push(document.getElementById("cmd").value);

	setControlsEnabled(false, true, UI.Wait);

	var param = getParameterDefinition("commands", Commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			document.getElementById("output").value = req.responseText;
			setControlsEnabled(true);
		}
	}

	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
