/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var globalLanIp;

function getBackup()
{
	setControlsEnabled(false, true, "Preparing Backup File");
	var param = getParameterDefinition("commands", "sh " + gargoyleBinRoot + "/utility/create_backup.sh ;\n" );
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			window.location="backup.tar.gz"
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function doRestore()
{
	if(document.getElementById('restore_file').value.length == 0)
	{
		alert("ERROR: You must select a configuration file to restore from.");
	}
	else
	{
		confirmRestore = window.confirm("This will completely erase your current settings and replace them with new ones from the selected configuration file.  Are you sure you want to continue?");
		if(confirmRestore)
		{
			document.getElementById('restore_form').submit();
			setControlsEnabled(false, true, "Uploading Configuration File");	
		}
	}
}
function doDefaultRestore()
{
	var confirmRestore = window.confirm("This will completely erase your current settings and replace them with the original, default settings.  Are you sure you want to continue?");
	if(confirmRestore)
	{
		document.getElementById('restore_original_form').submit();
		setControlsEnabled(false, true, "Loading Original Configuration File");	
	}

}
function restoreFailed()
{
	setControlsEnabled(true);
	alert("Restore failed.  Ensure that uploaded file is a valid Gargoyle configuration file and try again.");
}

function restoreSuccessful(lanIp)
{
	setControlsEnabled(false, true, "Please wait while new settings are applied")
	
	globalLanIp = lanIp;

	var param = getParameterDefinition("commands", "sh " + gargoyleBinRoot + "/utility/reboot.sh ;\n" );
	var stateChangeFunction = function(req) { return 0; } ;
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);




	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + globalLanIp + ":" + window.location.port + "/utility/reboot_test.sh";
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
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	window.location = currentProtocol + "://" + globalLanIp + ":" + window.location.port + window.location.pathname;
}

