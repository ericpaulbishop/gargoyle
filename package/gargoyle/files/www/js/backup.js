/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var bkS=new Object(); //part of i18n

var toggleReload = false;
var globalLanIp;

function getBackup()
{
	setControlsEnabled(false, true, bkS.PrepBack);
	var param = getParameterDefinition("commands", "sh /usr/lib/gargoyle/create_backup.sh ;\n" )  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			window.location="/dump_backup_tarball.sh"
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function doRestore()
{
	if(document.getElementById('restore_file').value.length == 0)
	{
		alert(bkS.SelCErr);
	}
	else
	{
		confirmRestore = window.confirm(bkS.EraseWarn);
		if(confirmRestore)
		{
			document.getElementById('restore_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
			document.getElementById('restore_form').submit();
			setControlsEnabled(false, true, bkS.UpingC);	
		}
	}
}
function doDefaultRestore()
{
	var confirmRestore = window.confirm(bkS.EraseWarn);
	if(confirmRestore)
	{
		document.getElementById('restore_original_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
		document.getElementById('restore_original_form').submit();
		setControlsEnabled(false, true, bkS.LdOrig);	
	}

}
function restoreFailed()
{
	setControlsEnabled(true);
	alert(bkS.FailErr);
}

function restoreSuccessful(lanIp)
{
	setControlsEnabled(false, true, UI.waitText)
	
	globalLanIp = lanIp;

	var param = getParameterDefinition("commands", "sh /usr/lib/gargoyle/reboot.sh ;\n" )  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req) { return 0; } ;
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + globalLanIp + ":" + window.location.port + "/utility/reboot_test.sh";
	testReboot = function()
	{
		toggleReload = true;
		setTimeout( "testReboot()", 5*1000);  //try again after 5 seconds
		document.getElementById("reboot_test").src = testLocation;
	}
	setTimeout( "testReboot()", 25*1000);  //start testing after 25 seconds
	setTimeout( "reloadPage()", 240*1000); //after 4 minutes, try to reload anyway
}

function reboot(lanIp)
{
	setControlsEnabled(false, true, UI.waitText)

	globalLanIp = lanIp;

	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + globalLanIp + ":" + window.location.port + "/utility/reboot_test.sh";
	testReboot = function()
	{
		toggleReload = true;
		setTimeout( "testReboot()", 5*1000);
		document.getElementById("reboot_test").src = testLocation;
	}
	setTimeout( "testReboot()", 25*1000);
	setTimeout( "reloadPage()", 240*1000);
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
			currentProtocol = location.href.match(/^https:/) ? "https" : "http";
			window.location = currentProtocol + "://" + globalLanIp + ":" + window.location.port + window.location.pathname;
		}
	}
}

