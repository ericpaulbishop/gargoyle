/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var upS=new Object();

var reloadTarget = "http://192.168.1.1/"

function doUpgrade()
{
	if(document.getElementById('upgrade_file').value.length == 0)
	{
		alert(upS.SelErr);
	}
	else
	{
		var curIp = uciOriginal.get("network", "lan", "ipaddr");
		curIp = curIp == "" ? "192.168.1.1" : curIp;
		reloadTarget = document.getElementById("upgrade_preserve").checked == true ? "http://" + curIp + "/" : "http://192.168.1.1/"


		confirmUpgrade = window.confirm(upS.Warn2);
		if(confirmUpgrade)
		{
			document.getElementById('upgrade_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
			document.getElementById('upgrade_form').submit();
			setControlsEnabled(false, true, upS.Uping);
		}
	}
}

function uploaded()
{
	setControlsEnabled(false, true, upS.UWait);
}
function upgraded()
{
	setControlsEnabled(false, true, upS.Rbtg);
	doRedirect= function()
	{
		window.location =  reloadTarget;
	}
	setTimeout( "doRedirect()", 120*1000);
}

function failure()
{
	setControlsEnabled(true);
	alert(upS.UErr+"\n");
}

function failureByBootloader()
{
	setControlsEnabled(true);
	alert(upS.UErrBB+"\n");
}

function setUpgradeFormat()
{
	document.getElementById("upgrade_arch").value = (platform == "broadcom") ? "brcm" : "not_brcm";
	if(platform == "broadcom")
	{
		setChildText("upgrade_text", upS.brcmT);
	}
	else if(platform == "ar71xx")
	{
		setChildText("upgrade_text", upS.ar71xxT);
	}
	else
	{
		setChildText("upgrade_text", upS.othrT);
	}

	setChildText("gargoyle_version", gargoyleVersion);
}
