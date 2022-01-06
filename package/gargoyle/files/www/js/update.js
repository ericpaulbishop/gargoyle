/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var upS=new Object();

var reloadTarget = "http://192.168.1.1/"
var fwtoolJson = JSON.parse("{}");
var validateFwJson = JSON.parse("{}");
var firmware_hash = "";
var devcompatversion = "1.0";
var storage_size = 0;
var toggleReload = false;
var reloadTarget = "";

function doUpgrade()
{
	confirmUpgrade = window.confirm(upS.Warn2);
	if(confirmUpgrade)
	{
		var curIp = uciOriginal.get("network", "lan", "ipaddr");
		curIp = curIp == "" ? "192.168.1.1" : curIp;
		curProto = location.href.match(/^https:/) ? "https" : "http";
		reloadTarget = document.getElementById("preserve_settings").checked == true ? curProto + "://" + curIp + "/" : "http://192.168.1.1/";

		closeModalWindow('upgrade_confirm_modal');
		setControlsEnabled(false, true, upS.UWait);

		preservesettings = document.getElementById("preserve_settings").checked;
		forceupgrade = document.getElementById("force_upgrade").checked;
		var Commands = [];
		Commands.push("/etc/init.d/tor stop >/dev/null 2>&1");
		Commands.push("sysupgrade " + (preservesettings ? "" : "-n ") + (forceupgrade ? "-F " : "") + "/tmp/up/upgrade");
		var commands = Commands.join("\n");
		var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				//do nothing
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

		testReboot = function()
		{
			toggleReload = true;
			setTimeout( "testReboot()", 5*1000);  //try again after 5 seconds
			document.getElementById("reboot_test").src = reloadTarget;
		}
		setTimeout( "testReboot()", 120*1000);  //start testing after 2 minutes
		setTimeout( "reloadPage()", 420*1000); //after 7 minutes, try to reload anyway
	}
}

function reloadPage()
{
	if(toggleReload)
	{
		var reloadState = document.getElementById("reboot_test").readyState;
		if( typeof(reloadState) == "undefined" || reloadState == null || reloadState == "complete")
		{
			toggleReload=false;
			document.getElementById("reboot_test").src = "";
			window.location = reloadTarget;
		}
	}
}

function doCancel()
{
	var commands = "rm -rf /tmp/up";
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			closeModalWindow('upgrade_confirm_modal');
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
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

function resetData()
{
	setUpgradeFormat();
	var systemSecs = uciOriginal.getAllSectionsOfType("system","system");
	devcompatversion = uciOriginal.get("system",systemSecs[0],"compat_version");
	fwtoolJson = JSON.parse(fwtoolStr == "" ? "{}" : fwtoolStr);
	validateFwJson = JSON.parse(validateFwStr == "" ? "{}" : validateFwStr);

	var kernsize = 0;
	var rootsize = 0;
	var wholesize = 0;
	procmtdLines.forEach(function(ln) {
		var match = ln.match(/^mtd\d+: ([0-9a-f]+) [0-9a-f]+ (.+)$/);
		var size = match ? parseInt(match[1], 16) : 0;

		switch (match ? match[2] : '') {
			case 'linux':
			case 'firmware':
				if (size > wholesize)
					wholesize = size;
				break;

			case 'kernel':
			case 'kernel0':
				kernsize = size;
				break;

			case 'rootfs':
			case 'rootfs0':
			case 'ubi':
			case 'ubi0':
				rootsize = size;
				break;
		}
	});

	if(wholesize > 0)
	{
		storage_size = wholesize;
	}
	else if(kernsize > 0 && rootsize > kernsize)
	{
		storage_size = kernsize + rootsize;
	}
	else
	{
		procpartLines.forEach(function(ln) {
			var match = ln.match(/^\s*\d+\s+\d+\s+(\d+)\s+(\S+)$/);
			if (match) {
				var size = parseInt(match[1], 10);

				if (!match[2].match(/\d/) && size > 2048 && wholesize == 0)
					wholesize = size * 1024;
			}
		});
		storage_size = wholesize;
	}


	if(upgradePresent)
	{
		confirmUpgradeModal();
	}
}

function setUpgradeFormat()
{
	if(distribTarget.match(/brcm47xx/))
	{
		setChildText("upgrade_text", upS.brcmT);
	}
	else if(distribTarget.match(/ar71xx/))
	{
		setChildText("upgrade_text", upS.ar71xxT);
	}
	else if(distribTarget.match(/mvebu/))
	{
		setChildText("upgrade_text", upS.mvebu);
	}
	else if(distribTarget.match(/ramips/))
	{
		setChildText("upgrade_text", upS.ar71xxT);
	}
	else if(distribTarget.match(/ath25/))
	{
		setChildText("upgrade_text", upS.othrT);
	}
	else
	{
		setChildText("upgrade_text", "");
	}

	setChildText("gargoyle_version", gargoyleVersion);
}

function doUpload()
{
	if(document.getElementById('upgrade_file').value.length == 0)
	{
		alert(upS.SelErr);
	}
	else
	{
		document.getElementById('upgrade_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
		document.getElementById('upgrade_form').submit();
		setControlsEnabled(false, true, upS.Uping);
	}
}

function uploaded()
{
	document.getElementById("upgrade_file").value = "";
	document.getElementById("firmware_hash").value = "";
	document.getElementById("upgrade_hash").value = "";
	setControlsEnabled(true, false);
	upgradeFrame = document.getElementById("do_upgrade");
	upgradeFrameDoc = (upgradeFrame.contentDocument) ? upgradeFrame.contentDocument : upgradeFrame.contentWindow.document;

	fwtoolStr = upgradeFrameDoc.getElementById("fwtool").innerText;
	validateFwStr = upgradeFrameDoc.getElementById("validate_firmware_image").innerText;

	fwtoolJson = JSON.parse(fwtoolStr == "" ? "{}" : fwtoolStr);
	validateFwJson = JSON.parse(validateFwStr == "" ? "{}" : validateFwStr);

	md5hash = upgradeFrameDoc.getElementById("md5sum").innerText;
	sha1hash = upgradeFrameDoc.getElementById("sha1sum").innerText;
	sha256hash = upgradeFrameDoc.getElementById("sha256sum").innerText;

	firmware_hash = upgradeFrameDoc.getElementById("firmware_hash").innerText;

	firmware_size = upgradeFrameDoc.getElementById("firmware_size").innerText;

	confirmUpgradeModal();
}

function confirmUpgradeModal()
{
	var firmware_valid = "";
	if (validateFwJson.valid !== undefined)
	{
		firmware_valid = validateFwJson.valid == true ? UI.YES : UI.NO;
	}
	var firmware_dev_match = "";
	if (validateFwJson.tests !== undefined)
	{
		firmware_dev_match = validateFwJson.tests.fwtool_device_match == true ? UI.YES : UI.NO;
	}
	var imgcompatversion = fwtoolJson.compat_version !== undefined ? fwtoolJson.compat_version : "1.0";
	var firmware_compat_match = imgcompatversion == devcompatversion ? UI.YES : UI.NO + (fwtoolJson.compat_message !== undefined ? ". " + fwtoolJson.compat_message : "");
	var supported_devsarr = [];
	supported_devsarr = fwtoolJson.supported_devices !== undefined ? fwtoolJson.supported_devices : supported_devsarr;
	supported_devsarr = fwtoolJson.new_supported_devices !== undefined && imgcompatversion != "1.0" ? fwtoolJson.new_supported_devices : supported_devsarr;
	var supported_devs = "";
	supported_devsarr.forEach(function(item) {
		supported_devs = supported_devs == "" ? item : supported_devs + "\n" + item;
	});
	var size_ok = "";
	if(firmware_size > 0 && storage_size > 0)
	{
		size_ok = firmware_size > storage_size ? UI.NO : UI.YES;
	}
	else
	{
		size_ok = "-";
	}
	var checksum_match = "";
	if(firmware_hash != "")
	{
		if(firmware_hash == md5hash || firmware_hash == sha1hash || firmware_hash == sha256hash)
		{
			checksum_match = UI.YES;
		}
		else
		{
			checksum_match = UI.NO;
		}
	}
	else
	{
		checksum_match = "-";
	}

	if(validateFwJson.allow_backup == true)
	{
		document.getElementById("preserve_settings").disabled = false;
		document.getElementById("preserve_settings_warning").classList.add("hidden");
	}
	else
	{
		document.getElementById("preserve_settings").disabled = true;
		document.getElementById("preserve_settings_warning").classList.remove("hidden");
	}

	modalButtons = [];
	if ((validateFwJson.tests.fwtool_device_match == false || validateFwJson.valid == false || (firmware_size > 0 && storage_size > 0 && firmware_size > storage_size)) && validateFwJson.forceable == true)
	{
		document.getElementById("force_upgrade").disabled = false;
		document.getElementById("force_upgrade_warning").classList.remove("hidden");
		modalButtons.push({"id" : "modal_button_upgrade", "title" : upS.UpFrm, "classes" : "btn btn-danger", "function" : doUpgrade, "disabled" : true});
	}
	else if((validateFwJson.tests.fwtool_device_match == false && validateFwJson.forceable == false) || (validateFwJson.tests.fwtool_device_match == true))
	{
		document.getElementById("force_upgrade").disabled = true;
		document.getElementById("force_upgrade_warning").classList.add("hidden");
		modalButtons.push({"id" : "modal_button_upgrade", "title" : upS.UpFrm, "classes" : "btn btn-danger", "function" : doUpgrade, "disabled" : false});
	}

	modalButtons.push({"title" : UI.Cancel, "classes" : "btn btn-warning", "function" : doCancel});

	modalElements = [
		{"id" : "firmware_valid", "innertext" : firmware_valid},
		{"id" : "firmware_dev_match", "innertext" : firmware_dev_match},
		{"id" : "supported_devs", "innertext" : supported_devs},
		{"id" : "firmware_compat_match", "innertext" : firmware_compat_match},
		{"id" : "size_ok", "innertext" : size_ok},
		{"id" : "checksum_match", "innertext" : checksum_match},
		{"id" : "md5sum", "innertext" : md5hash},
		{"id" : "sha1sum", "innertext" : sha1hash},
		{"id" : "sha256sum", "innertext" : sha256hash},
	];
	modalPrepare('upgrade_confirm_modal', upS.UpConfirm, modalElements, modalButtons);
	openModalWindow('upgrade_confirm_modal');
}

function forceUpgradeToggle()
{
	if(document.getElementById("force_upgrade").checked)
	{
		document.getElementById("modal_button_upgrade").disabled = false;
	}
	else
	{
		document.getElementById("modal_button_upgrade").disabled = true;
	}

}
