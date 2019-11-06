/*
 * This program is copyright © 2019 Michael Gray and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var stamgrStr=new Object(); //part of i18n

var encryptionList = [];
encryptionList['none'] = 'Open';
encryptionList['psk'] = 'WPA PSK';
encryptionList['psk2'] = 'WPA2 PSK';
encryptionList['sae'] = 'WPA3 SAE';

function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n" + UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);

		//remove all old access points so we can reorder them
		var apSectionCommands = [];
		var apSections = uciOriginal.getAllSectionsOfType("gargoyle_stamgr", "stacfg");
		while(apSections.length > 0)
		{
			var lastSection = apSections.pop();
			uciOriginal.removeSection("gargoyle_stamgr", lastSection);
			apSectionCommands.push("uci del gargoyle_stamgr." + lastSection);
		}
		var uci = uciOriginal.clone();
		
		//Set global configs
		enableStamgr = document.getElementById("enablestamgr").checked;
		uci.set("gargoyle_stamgr","global","enabled",enableStamgr ? "1" : "0");
		uci.set("gargoyle_stamgr","global","max_retry",document.getElementById("maxretry").value);
		uci.set("gargoyle_stamgr","global","max_wait",document.getElementById("maxwait").value);
		uci.set("gargoyle_stamgr","global","disconnect_quality_threshold",document.getElementById("disconnectqualthresh").value);
		uci.set("gargoyle_stamgr","global","connect_quality_threshold",document.getElementById("connectqualthresh").value);
		uci.set("gargoyle_stamgr","global","blacklist_timer",document.getElementById("blacklisttimer").value);
		
		// add ap sections
		var addAP = function(cfg_id,radio,ssid,bssid,encryption,password)
		{
			uci.set("gargoyle_stamgr", cfg_id, "", "stacfg");
			uci.set("gargoyle_stamgr", cfg_id, "radio", radio);
			uci.set("gargoyle_stamgr", cfg_id, "ssid", ssid);
			if(bssid != "")
			{
				uci.set("gargoyle_stamgr", cfg_id, "bssid", bssid);
			}
			uci.set("gargoyle_stamgr", cfg_id, "encryption", encryption);
			if(password != "")
			{
				uci.set("gargoyle_stamgr", cfg_id, "key", password);
			}
			apSectionCommands.push("uci set gargoyle_stamgr." + cfg_id + "=stacfg");
		};
		
		HTMLapTable = document.getElementById("wireless_station_container").firstChild;
		apData = getTableDataArray(HTMLapTable, true, true);
		
		for(apIdx = 0; apIdx < apData.length; apIdx++)
		{
			cfgid = "stamgr_ap_" + (apIdx+1);
			radio = translateBackendFrontend(apData[apIdx][1],radioList);
			bssid = apData[apIdx][3] == "-" ? "" : apData[apIdx][3];
			encryption = translateBackendFrontend(apData[apIdx][4],encryptionList);
			password = encryption == "none" ? "" : apData[apIdx][5];
			
			addAP(cfgid,radio,apData[apIdx][2],bssid,encryption,password);
		}

		commands = "/etc/init.d/gargoyle_stamgr stop\n";
		commands += apSectionCommands.join("\n") + "\n";
		commands += uci.getScriptCommands(uciOriginal) + "\n";
		commands += enableStamgr ? "/etc/init.d/gargoyle_stamgr enable;/etc/init.d/gargoyle_stamgr start;\n" : "";

		var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				setControlsEnabled(true);
				window.location =  window.location;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function resetData()
{
	document.getElementById("enablestamgr").checked = uciOriginal.get("gargoyle_stamgr", "global", "enabled") == "1" ? true : false;
	if(uciOriginal.get("wireless","stacfg","mode") != "sta")
	{
		document.getElementById("no_stacfg").style.display = "block";
		document.getElementById("enablestamgr").disabled = true;
		document.getElementById("enablestamgr").checked = false;
	}
	if(!wpa3) { delete encryptionList['sae'] }
	document.getElementById("maxretry").value = uciOriginal.get("gargoyle_stamgr", "global", "max_retry");
	document.getElementById("maxwait").value = uciOriginal.get("gargoyle_stamgr", "global", "max_wait");
	document.getElementById("disconnectqualthresh").value = uciOriginal.get("gargoyle_stamgr", "global", "disconnect_quality_threshold");
	document.getElementById("connectqualthresh").value = uciOriginal.get("gargoyle_stamgr", "global", "connect_quality_threshold");
	blacklisttimer = uciOriginal.get("gargoyle_stamgr", "global", "blacklist_timer");
	document.getElementById("blacklisttimer").value = blacklisttimer;

	//Init STA Table
	stationTableColumns = [stamgrStr.cfgid, stamgrStr.Band, 'SSID', 'BSSID', stamgrStr.Encryption/*, stamgrStr.Password*/,''];
	stationTableData = new Array();
	stationSections = uciOriginal.getAllSectionsOfType('gargoyle_stamgr','stacfg');
	for(staidx = 0; staidx < stationSections.length; staidx++)
	{
		cfgid = stationSections[staidx];
		
		band = uciOriginal.get('gargoyle_stamgr',cfgid,'radio');
		band = radioList[band] == "" ? band : radioList[band];
		ssid = uciOriginal.get('gargoyle_stamgr',cfgid,'ssid');
		bssid = uciOriginal.get('gargoyle_stamgr',cfgid,'bssid');
		bssid = bssid == "" ? "-" : bssid;
		enc = uciOriginal.get('gargoyle_stamgr',cfgid,'encryption');
		enc = translateBackendFrontend(enc,encryptionList);
		pass = uciOriginal.get('gargoyle_stamgr',cfgid,'key');
		
		if(band && ssid && enc && (uciOriginal.get('gargoyle_stamgr',cfgid,'encryption') == "none" || pass))
		{
			stationTableData.push([cfgid,band,ssid,bssid,enc,pass,createEditButton()]);
		}
	}

	stationTable=createTable(stationTableColumns, stationTableData, "wireless_station_table", true, true);
	stationTableContainer = document.getElementById('wireless_station_container');
	if(stationTableContainer.firstChild != null)
	{
		stationTableContainer.removeChild(stationTableContainer.firstChild);
	}
	stationTableContainer.appendChild(stationTable);
	
	//Init Runtime Status
	runtimeObj = JSON.parse(runtime_status == '' ? '{}' : runtime_status);
	document.getElementById("lastupdate").innerHTML = typeof currentTime === 'undefined' ? "-" : currentTime;
	activecfgid = runtimeObj.active_cfg_id === undefined ? "-" : runtimeObj.active_cfg_id;
	document.getElementById("currentid").innerHTML = activecfgid;
	connected = runtimeObj.current_wireless_cfg === undefined ? "-" : runtimeObj.current_wireless_cfg.connected;
	document.getElementById("connected").innerHTML = connected == "true" ? UI.YES : UI.NO;
	document.getElementById("currentradio").innerHTML = runtimeObj.current_wireless_cfg === undefined ? "-" : translateBackendFrontend(runtimeObj.current_wireless_cfg.radio,radioList);
	document.getElementById("currentssid").innerHTML = runtimeObj.current_wireless_cfg === undefined ? "-" : runtimeObj.current_wireless_cfg.ssid;
	document.getElementById("currentbssid").innerHTML = runtimeObj.current_wireless_cfg === undefined ? "-" : runtimeObj.current_wireless_cfg.bssid;
	document.getElementById("currentencryption").innerHTML = runtimeObj.current_wireless_cfg === undefined ? "-" : translateBackendFrontend(runtimeObj.current_wireless_cfg.encryption,encryptionList);
	
	blacklistEl = document.getElementById("blacklistsection_div");
	blacklisted = runtimeObj.station_blacklist;
	for(idx in blacklisted)
	{
		//ID
		var liEl = document.createElement("li");
		liEl.className = "list-group-item";
		var titleEl = document.createElement("span");
		titleEl.className = "list-group-item-title";
		titleEl.innerHTML = "ID:";
		var textEl = document.createElement("span");
		textEl.innerHTML = blacklisted[idx].cfg_id;
		liEl.appendChild(titleEl);
		liEl.appendChild(textEl);
		blacklistEl.appendChild(liEl);
		//Time
		var liEl = document.createElement("li");
		liEl.className = "list-group-item";
		var titleEl = document.createElement("span");
		titleEl.className = "list-group-item-title";
		titleEl.innerHTML = stamgrStr.BlacklistTime + ":";
		var textEl = document.createElement("span");
		//textEl.innerHTML = new Date(blacklisted[idx].time*1000).toLocaleString();
		var now = new Date().getTime();
		textEl.innerHTML = Math.floor(((parseInt(blacklisted[idx].time)+parseInt(blacklisttimer))*1000-now)/1000) + " " + UI.seconds;
		liEl.appendChild(titleEl);
		liEl.appendChild(textEl);
		blacklistEl.appendChild(liEl);
	}
	
	//Apply page styling
	document.getElementById("runtimepanelheader").classList.add(connected == "true" ? "panel-success" : "panel-danger");
	if(stationTableContainer.firstChild != null)
	{
		tabRows = stationTableContainer.firstChild.children[1].children;
		for(x = 0; x < tabRows.length; x++)
		{
			tabRow = tabRows[x];
			tabRowID = tabRow.children[0].innerHTML;
			for(idx in blacklisted)
			{
				if(tabRowID == blacklisted[idx].cfg_id)
				{
					tabRow.classList.add("danger");
				}
			}
			if(tabRowID == activecfgid)
			{
				tabRow.classList.add(connected == "true" ? "success" : "warning");
			}
		}
	}
	
}

function proofreadAP(excludeRow)
{
	errors = [];
	if(radioList[document.getElementById('add_radio').value] === undefined)
	{
		errors.push(UI.prfErr + ' ' + document.getElementById('add_radio_label').innerText);
	}
	if(validateLengthRange(document.getElementById('add_ssid').value,1,31))
	{
		errors.push(UI.prfErr + ' ' + document.getElementById('add_ssid_label').innerText);
	}
	if(document.getElementById('add_bssid').value != "" && document.getElementById('add_bssid').value != "-" && validateMac(document.getElementById('add_bssid').value))
	{
		errors.push(UI.prfErr + ' ' + document.getElementById('add_bssid_label').innerText);
	}
	if(encryptionList[document.getElementById('add_enc').value] === undefined)
	{
		errors.push(UI.prfErr + ' ' + document.getElementById('add_enc_label').innerText);
	}
	else
	{
		if(document.getElementById('add_enc').value == 'none' && validateLengthRange(document.getElementById('add_password').value,0,0))
		{
			errors.push(UI.prfErr + ' ' + document.getElementById('add_enc_label').innerText + '/' + document.getElementById('add_password_label').innerText);
		}
		else if(document.getElementById('add_enc').value == 'wep' && validateLengthRange(document.getElementById('add_password').value,5,13))
		{
			errors.push(UI.prfErr + ' ' + document.getElementById('add_enc_label').innerText + '/' + document.getElementById('add_password_label').innerText);
		}
		else if(document.getElementById('add_enc').value == 'psk' && validateLengthRange(document.getElementById('add_password').value,8,63))
		{
			errors.push(UI.prfErr + ' ' + document.getElementById('add_enc_label').innerText + '/' + document.getElementById('add_password_label').innerText);
		}
		else if(document.getElementById('add_enc').value == 'psk2' && validateLengthRange(document.getElementById('add_password').value,8,63))
		{
			errors.push(UI.prfErr + ' ' + document.getElementById('add_enc_label').innerText + '/' + document.getElementById('add_password_label').innerText);
		}
	}
	return errors;
}

function proofreadAll()
{
	stamgrIds = ['maxretry', 'maxwait', 'disconnectqualthresh', 'connectqualthresh','blacklisttimer'];
	labelIds= ['maxretry_label', 'maxwait_label', 'disconnectqualthresh_label', 'connectqualthresh_label','blacklisttimer_label'];
	functions = [validateNumeric, validateNumeric, validateNumeric, validateNumeric, validateNumeric];
	returnCodes = [0,0,0,0,0];
	visibilityIds = stamgrIds;
	errors = proofreadFields(stamgrIds, labelIds, functions, returnCodes, visibilityIds);

	//test that thresholds are within sensible ranges
	if(errors.length == 0 && document.getElementById("enablestamgr").checked)
	{
		var disconqual = document.getElementById("disconnectqualthresh").value;
		var conqual = document.getElementById("connectqualthresh").value;
		if(disconqual > 90)
		{
			errors.push(stamgrStr.discontoohighErr);
		}
		if(conqual < 30)
		{
			errors.push(stamgrStr.contoolowErr);
		}
		if(disconqual >= conqual)
		{
			errors.push(stamgrStr.discongtconErr);
		}
	}

	return errors;
}

function addAP()
{
	errors = proofreadAP();
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\n" + stamgrStr.AErr);
	}
	else
	{
		values = new Array();
		//push dummy ID
		values.push("-");
		ids = ['add_radio', 'add_ssid', 'add_bssid', 'add_enc', 'add_password'];
		for (idIndex in ids)
		{
			v = document.getElementById(ids[idIndex]).value;
			v = v== '' ? '-' : v;
			if(ids[idIndex] == 'add_radio')
			{
				v=radioList[v];
			}
			else if(ids[idIndex] == 'add_enc')
			{
				v = translateBackendFrontend(v,encryptionList);
			}
			values.push(v);
			document.getElementById(ids[idIndex]).value = "";
		}
		values.push(createEditButton());
		stationTable = document.getElementById('wireless_station_container').firstChild;
		addTableRow(stationTable,values, true, true);
		closeModalWindow('stamgr_ap_modal');
	}
}

function editAP(editRow)
{
	var errors = proofreadAP(editRow);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+stamgrStr.upErr);
	}
	else
	{
		//update document with new data
		radio = document.getElementById("add_radio").value;
		radio = translateBackendFrontend(radio,radioList);
		enc = document.getElementById("add_enc").value;
		enc = translateBackendFrontend(enc,encryptionList);
		
		editRow.childNodes[0].firstChild.data = "-";
		editRow.childNodes[1].firstChild.data = radio;
		editRow.childNodes[2].firstChild.data = document.getElementById("add_ssid").value;
		editRow.childNodes[3].firstChild.data = document.getElementById("add_bssid").value;
		editRow.childNodes[4].firstChild.data = enc;
		editRow.childNodes[5].firstChild.data = document.getElementById("add_password").value;

		closeModalWindow('stamgr_ap_modal');
	}
}

function addAPModal()
{
	modalButtons = [
		{"title" : UI.Add, "classes" : "btn btn-primary", "function" : addAP},
		"defaultDismiss"
	];

	var radio = "";
	var ssid = "";
	var bssid = "";
	var enc = "";
	var password = "";

	modalElements = [
		{"id" : "add_radio", "options" : radioList, "value" : radio},
		{"id" : "add_ssid", "value" : ssid},
		{"id" : "add_bssid", "value" : bssid},
		{"id" : "add_enc", "options" : encryptionList, "value" : enc},
		{"id" : "add_password", "value" : password},
	];
	modalPrepare('stamgr_ap_modal', stamgrStr.AddAP, modalElements, modalButtons);
	openModalWindow('stamgr_ap_modal');
}

function editAPModal()
{
	editRow=this.parentNode.parentNode;
	modalButtons = [
		{"title" : UI.CApplyChanges, "classes" : "btn btn-primary", "function" : function(){editAP(editRow);}},
		"defaultDiscard"
	];

	radio = editRow.childNodes[1].firstChild.data;
	radio = translateBackendFrontend(radio,radioList);
	ssid = editRow.childNodes[2].firstChild.data;
	bssid = editRow.childNodes[3].firstChild.data;
	enc = editRow.childNodes[4].firstChild.data;
	enc = translateBackendFrontend(enc,encryptionList);
	password = editRow.childNodes[5].firstChild.data;

	modalElements = [
		{"id" : "add_radio", "options" : radioList, "value" : radio},
		{"id" : "add_ssid", "value" : ssid},
		{"id" : "add_bssid", "value" : bssid},
		{"id" : "add_enc", "options" : encryptionList, "value" : enc},
		{"id" : "add_password", "value" : password},
	];
	modalPrepare('stamgr_ap_modal', stamgrStr.EAP, modalElements, modalButtons);
	openModalWindow('stamgr_ap_modal');
}

function createEditButton()
{
	var editButton = createInput("button");
	editButton.textContent = UI.Edit;
	editButton.className = "btn btn-default btn-edit";
	editButton.onclick = editAPModal;
	return editButton;
}

function translateBackendFrontend(str,lookup)
{
	//Try backend to userfriendly first
	retVal = lookup[str] !== undefined ? lookup[str] : "";
	if(retVal == "")
	{
		//Try userfriendly to backend
		for(key in lookup)
		{
			if(lookup[key] == str)
			{
				retVal = key;
				break;
			}
		}
	}
	return retVal;
}
