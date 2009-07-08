var pkg = "firewall";
var allIps = [];
var changedIps = [];

function saveChanges()
{
	setControlsEnabled(false, true);
	
	//remove old quotas
	var preCommands = [];
	var allOriginalQuotas = uciOriginal.getAllSectionsOfType(pkg, "quota");
	while(allOriginalQuotas.length > 0)
	{
		var section = allOriginalQuotas.shift();
		uciOriginal.removeSection(pkg, section);
		preCommands.push("uci del " + pkg + "." + section);	
	}
	preCommands.push("uci commit");

	var allNewQuotas = uci.getAllSectionsOfType(pkg, "quota");
	var quotaUseVisibleCommand = "\nuci del gargoyle.status.quotause ; uci commit ;\n"
	while(allNewQuotas.length > 0)
	{
		//if ip has changed, reset saved data
		var section = allNewQuotas.shift()
		if( changedIps[ uci.get(pkg,section,"ip") ] == 1 )
		{
			uci.set(pkg, section, "ignore_backup_at_next_restore", "1");
		}
		quotaUseVisibleCommand = "\nuci set gargoyle.status.quotause=\"225\" ; uci commit ;\n"
	}

	//set enabled / disabled	
	var quotaTable = document.getElementById('quota_table_container').firstChild;
	var quotaTableData = getTableDataArray(quotaTable, true, false);
	var qtIndex=0;
	for(qtIndex=0; qtIndex < quotaTableData.length; qtIndex++)
	{
		var enabledCheck = quotaTableData[qtIndex][4];
		uci.set(pkg, enabledCheck.id, "enabled", (enabledCheck.checked ? "1" : "0") )
	}

	var postCommands = [];
	postCommands.push("sh /usr/lib/gargoyle/restart_firewall.sh");
	postCommands.push("if [ -d \"/usr/data/quotas/\" ] ; then rm -rf /usr/data/quotas/* ; fi ;");
	postCommands.push("backup_quotas");
	var commands = preCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + quotaUseVisibleCommand + "\n" + postCommands.join("\n");

	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
;
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			//just reload page -- it's easier than any other mechanism to load proper quota data from uci
			setControlsEnabled(true);	
			window.location.href = window.location.href;	
		}
	}

	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function resetData()
{
	//table columns: ip, percent upload used, percent download used, percent combined used, enabled, edit, remove
	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var quotaTableData = [];
	var checkElements = []; //because IE is a bitch and won't register that checkboxes are checked/unchecked unless they are part of document
	var areChecked = [];
	allIps = [];
	changedIps = [];
	for(sectionIndex = 0; sectionIndex < quotaSections.length; sectionIndex++)
	{
		var ip = uciOriginal.get(pkg, quotaSections[sectionIndex], "ip").toUpperCase();
		var pctUp       = "N/A";
		var pctDown     = "N/A";
		var pctCombined = "N/A";
		ip = ip.replace(/\//g, "_");
		if(quotaPercents[ip] != null)
		{
			var pcts = quotaPercents[ip];
			pctUp = pcts[0] >= 0 ? pcts[0] + "%" : pctUp;
			pctDown = pcts[1] >= 0 ? pcts[1] + "%" : pctDown;
			pctCombined = pcts[2] >= 0 ? pcts[2] + "%" : pctCombined;
		}		
		
		var enabled = uciOriginal.get(pkg, quotaSections[sectionIndex], "enabled");
		enabled = enabled != "0" ? true : false;
	
		
		var enabledCheck = createEnabledCheckbox(enabled);
		enabledCheck.id= quotaSections[sectionIndex];
		checkElements.push(enabledCheck);
		areChecked.push(enabled);

		quotaTableData.push( [ ip.replace(/_/g, " "), pctUp, pctDown, pctCombined, enabledCheck, createEditButton(enabled) ] );
	
		allIps[ip] = 1;
	}

	
	columnNames=["IP", "% Upload Used", "% Download Used", "% Combined Used", "", "" ];
	quotaTable = createTable(columnNames, quotaTableData, "quota_table", true, false, removeQuotaCallback);
	tableContainer = document.getElementById('quota_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(quotaTable);

	while(checkElements.length > 0)
	{
		var c = checkElements.shift();
		var b = areChecked.shift();
		c.checked = b;
	}
	
	setDocumentFromUci(document, new UCIContainer(), "");
	
	setVisibility(document);
}

function getIpFromDocument(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var ip = "ALL";
	if(getSelectedValue("applies_to_type", controlDocument) == "all")
	{
		ip = "ALL";
	}
	else if(getSelectedValue("applies_to_type", controlDocument) == "others_combined")
	{
		ip = "ALL_OTHERS_COMBINED";
	}
	else if(getSelectedValue("applies_to_type", controlDocument) == "others_individual")
	{
		ip = "ALL_OTHERS_INDIVIDUAL";
	}
	else if(getSelectedValue("applies_to_type", controlDocument) == "only")
	{
		ip = controlDocument.getElementById("applies_to").value; 
	}
	return ip;
}

function setDocumentIp(ip, controlDocument)
{
	ip = ip== ""  ? "ALL" : ip;
	controlDocument = controlDocument == null ? document : controlDocument;
	controlDocument.getElementById("applies_to").value = "";
	if(ip == "ALL")
	{
		setSelectedValue("applies_to_type", "all", controlDocument);
	}
	else if(ip == "ALL_OTHERS_COMBINED")
	{
		setSelectedValue("applies_to_type", "others_combined", controlDocument);
	}
	else if(ip == "ALL_OTHERS_INDIVIDUAL")
	{
		setSelectedValue("applies_to_type", "others_individual", controlDocument);
	}
	else
	{
		setSelectedValue("applies_to_type", "only", controlDocument);
		controlDocument.getElementById("applies_to").value = ip;
	}
}


function addNewQuota()
{
	var errors = validateQuota(document, "none");
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\nCould not add quota.");
	}
	else
	{
		var quotaNum = 1;
		while( uci.get(pkg, "quota_" + quotaNum, "") != "") { quotaNum++; }

		setUciFromDocument(document);

		var ip = getIpFromDocument(document);
		allIps[ip] = 1;
		
		var quotaSection = "quota_" + quotaNum;
		var enabledCheck = createEnabledCheckbox(true);
		enabledCheck.id = quotaSection;

		var tableContainer = document.getElementById("quota_table_container");
		var table = tableContainer.firstChild;
		var down = uci.get(pkg, "quota_" + quotaNum, "ingress_limit") == "" ? "N/A" : "0"; 
		var up = uci.get(pkg, "quota_" + quotaNum, "egress_limit") == "" ? "N/A" : "0"; 
		var combined = uci.get(pkg, "quota_" + quotaNum, "combined_limit") == "" ? "N/A" : "0"; 
		addTableRow(table, [ip.replace(/_/g, " "), up, down, combined, enabledCheck, createEditButton(true)], true, false, removeQuotaCallback);	

		setDocumentFromUci(document, new UCIContainer(), "");

		enabledCheck.checked = true;
	}
}

function setVisibility(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	setInvisibleIfIdMatches("applies_to_type", ["all","others_combined", "others_individual"], "applies_to", "inline", controlDocument);
	setInvisibleIfIdMatches("quota_active", ["always"], "offpeak_hours_container", "inline", controlDocument);
	setInvisibleIfIdMatches("quota_reset", ["hour", "day"], "quota_day_container", "block", controlDocument);
	setInvisibleIfIdMatches("quota_reset", ["hour"], "quota_hour_container", "block", controlDocument);
	setInvisibleIfIdMatches("max_up_type", ["unlimited"], "max_up_container", "inline", controlDocument);
	setInvisibleIfIdMatches("max_down_type", ["unlimited"], "max_down_container", "inline", controlDocument);
	setInvisibleIfIdMatches("max_combined_type", ["unlimited"], "max_combined_container", "inline", controlDocument);


	var qri=getSelectedValue("quota_reset", controlDocument);
	if(qri == "month")
	{
		var vals = [];	
		var names = [];	
		var day=1;
		for(day=1; day <= 28; day++)
		{
			var dayStr = "" + day;
			var lastDigit = dayStr.substr( dayStr.length-1, 1);
			var suffix="th"
			if( day % 100  != 11 && lastDigit == "1")
			{
				suffix="st"
			}
			if( day % 100 != 12 && lastDigit == "2")
			{
				suffix="nd"
			}
			if( day %100 != 13 && lastDigit == "3")
			{
				suffix="rd"
			}
			names.push(dayStr + suffix);
			vals.push( ((day-1)*60*60*24) + "" );
		}
		setAllowableSelections("quota_day", vals, names, controlDocument);
	}
	if(qri == "week")
	{
		var names = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
		var vals = [];
		var dayIndex;
		for(dayIndex=0; dayIndex < 7; dayIndex++)
		{
			vals.push( (dayIndex*60*60*24) + "")
		}
		setAllowableSelections("quota_day", vals, names, controlDocument);
	}
}

function setInvisibleIfIdMatches(selectId, invisibleOptionValues, associatedElementId, defaultDisplayMode, controlDocument )
{
	controlDocument = controlDocument == null ? document : controlDocument;
	defaultDisplayMode = defaultDisplayMode == null ? "block" : defaultDisplayMode;
	var visElement = controlDocument.getElementById(associatedElementId);
	var matches = false;
	var matchIndex=0;
	if(visElement != null)
	{
		for (matchIndex=0; matchIndex < invisibleOptionValues.length; matchIndex++)
		{
			matches = getSelectedValue(selectId, controlDocument) == invisibleOptionValues[matchIndex] ? true : matches;
		}
		if(matches)
		{
			visElement.style.display = "none";
		}
		else
		{
			visElement.style.display = defaultDisplayMode;
		}
	}
}

function getDaySeconds(offset)
{
	return ( Math.floor(offset/(60*60*24))*(60*60*24)) ;
}
function getHourSeconds(offset)
{
	return ( Math.floor((offset%(60*60*24))/(60*60)) * (60*60) );
}

function validateQuota(controlDocument, originalQuotaIp)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var inputIds = ["applies_to", "max_up", "max_down", "max_combined", "offpeak_hours"];
	var labelIds = ["applies_to_label", "max_up_label", "max_down_label", "max_combined_label", "quota_active_label"];
	var functions = [validateIP, validateDecimal, validateDecimal, validateDecimal, validateHours];
	var validReturnCodes = [0,0,0,0,0];
	var visibilityIds = ["applies_to", "max_up_container","max_down_container","max_combined_container", "offpeak_hours_container"];
	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );

	//also validate 1) ip is either currentQuotaIp or is unique in table, 2) up & down aren't both unlimited
	if(errors.length == 0)
	{
		if( 	getSelectedValue("max_up_type", controlDocument) == "unlimited" && 
			getSelectedValue("max_down_type", controlDocument) == "unlimited" && 
			getSelectedValue("max_combined_type", controlDocument) == "unlimited"
			)
		{
			errors.push("Upload, download and combined bandwidth limits cannot all be unlimited");
		}
		var ip = getIpFromDocument(controlDocument);
		if(ip != originalQuotaIp)
		{
			if(allIps[ip] == 1 || (ip.match(/OTHER/) && (allIps["ALL_OTHERS_COMBINED"] == 1 || allIps["ALL_OTHERS_INDIVIDUAL"]))  )
			{	
				if(!ip.match(/ALL/))
				{
					errors.push("Duplicate IP -- only one quota per IP is allowed");
				}
				else if(ip.match(/OTHER/))
				{
					errors.push("You may have only one quota for hosts without explicit quotas");
				}
				else
				{
					errors.push("You may have only one quota that applies to entire network");
				}
			}
		}
	}
	return errors;
}
function setDocumentFromUci(controlDocument, srcUci, ip)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	ip = ip.toUpperCase();

	var quotaSection = "";
	var sections = srcUci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "ip") == ip || (ip == "ALL" && uci.get(pkg, sections[sectionIndex], "ip") == ""))
		{
			quotaSection = sections[sectionIndex];
		}
	}


	var resetInterval = srcUci.get(pkg, quotaSection, "reset_interval");
	var uploadLimit = srcUci.get(pkg, quotaSection, "egress_limit");
	var downloadLimit = srcUci.get(pkg, quotaSection, "ingress_limit");
	var combinedLimit = srcUci.get(pkg, quotaSection, "combined_limit");
	resetInterval = resetInterval == "" || resetInterval == "minute" ? "day" : resetInterval;
	var offset = srcUci.get(pkg, quotaSection, "reset_time");
	offset = offset == "" ? 0 : parseInt(offset);
	var resetDay = getDaySeconds(offset);
	var resetHour = getHourSeconds(offset);
	var offpeakHours = srcUci.get(pkg, quotaSection, "offpeak_hours");

	setDocumentIp(ip, controlDocument);
	setSelectedValue("quota_reset", resetInterval, controlDocument);
	setSelectedValue("quota_active", offpeakHours == "" ? "always" : "except", controlDocument);
	controlDocument.getElementById("offpeak_hours").value = offpeakHours;
	
	setSelectedValue("max_up_type", uploadLimit == "" ? "unlimited" : "limited", controlDocument );
	setSelectedValue("max_down_type", downloadLimit == "" ? "unlimited" : "limited", controlDocument );
	setSelectedValue("max_combined_type", combinedLimit == "" ? "unlimited" : "limited", controlDocument );

	
	setDocumentLimit(uploadLimit,   "max_up",       "max_up_unit", controlDocument);
	setDocumentLimit(downloadLimit, "max_down",     "max_down_unit", controlDocument);
	setDocumentLimit(combinedLimit, "max_combined", "max_combined_unit", controlDocument);

	setVisibility(controlDocument);
	setSelectedValue("quota_day", resetDay + "", controlDocument);
	setSelectedValue("quota_hour", resetHour + "", controlDocument);
}

function setDocumentLimit(bytes, text_id, unit_select_id, controlDocument)
{
	bytes = bytes == "" ? 0 : parseInt(bytes);
	var textEl = controlDocument.getElementById(text_id);
	if(bytes <= 0)
	{
		setSelectedValue(unit_select_id, "MB", controlDocument);
		textEl.value = "0";
	}
	else
	{
		var pb = parseBytes(bytes);
		var unit = "MB";
		var multiple = 1024*1024;
		if(pb.match(/GBytes/)) { unit = "GB"; multiple = 1024*1024*1024; }
		if(pb.match(/TBytes/)) { unit = "TB"; multiple = 1024*1024*1024*1024; }
		setSelectedValue(unit_select_id, unit, controlDocument);
		var adjustedVal = truncateDecimal(bytes/multiple);
		textEl.value = adjustedVal;
	}
}

function setUciFromDocument(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var ip = getIpFromDocument(controlDocument);

	var quotaSection = "";
	var sections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "ip") == ip || (ip == "ALL" && uci.get(pkg, sections[sectionIndex], "ip") == ""))
		{
			quotaSection = sections[sectionIndex];
		}
	}
	if(quotaSection == "")
	{
		var quotaNum = 1;
		while( uci.get(pkg, "quota_" + quotaNum, "") != "") { quotaNum++; }
		quotaSection = "quota_" + quotaNum;
		uci.set(pkg, quotaSection, "", "quota");
	}

	var oldIp = uci.get(pkg, quotaSection, "ip");
	if(oldIp != ip)
	{
		changedIps[ip] = 1;
	}

	
	uci.set(pkg, quotaSection, "ingress_limit",  getDocumentLimit("max_down", "max_down_type", "max_down_unit", controlDocument)  );
	uci.set(pkg, quotaSection, "egress_limit",   getDocumentLimit("max_up", "max_up_type", "max_up_unit", controlDocument) );
	uci.set(pkg, quotaSection, "combined_limit", getDocumentLimit("max_combined", "max_combined_type", "max_combined_unit", controlDocument) );
	uci.set(pkg, quotaSection, "reset_interval", getSelectedValue("quota_reset", controlDocument));
	uci.set(pkg, quotaSection, "ip", ip);

	var qd = getSelectedValue("quota_day", controlDocument);
	var qh = getSelectedValue("quota_hour", controlDocument);
	qd = qd == "" ? "0" : qd;
	qh = qh == "" ? "0" : qh;
	var resetTime= parseInt(qd) + parseInt(qh);
	if(resetTime > 0)
	{
		var resetTimeStr = resetTime + "";
		uci.set(pkg, quotaSection, "reset_time", resetTimeStr);
	}
	else
	{
		uci.remove(pkg, quotaSection, "reset_time");
	}

	var offpeakHours = getSelectedValue("quota_active", controlDocument) == "always" ? "" : controlDocument.getElementById("offpeak_hours").value;
	if(offpeakHours != "")
	{
		uci.set(pkg, quotaSection, "offpeak_hours", offpeakHours);
	}
}

/* returns a number if there is a limit "" if no limit defined */
function getDocumentLimit(text_id, unlimited_select_id, unit_select_id, controlDocument)
{
	var ret = "";
	if(getSelectedValue(unlimited_select_id, controlDocument) != "unlimited")
	{
		var unit = getSelectedValue(unit_select_id, controlDocument);
		var multiple = 1024*1024;
		if(unit == "MB") { multiple = 1024*1024; }
		if(unit == "GB") { multiple = 1024*1024*1024; }
		if(unit == "TB") { multiple = 1024*1024*1024*1024; }
		var bytes = Math.round(multiple * parseFloat(controlDocument.getElementById(text_id).value));
		ret =  "" + bytes;
	}
	return ret;
}

function createEnabledCheckbox(enabled)
{
	enabledCheckbox = createInput('checkbox');
	enabledCheckbox.onclick = setRowEnabled;
	enabledCheckbox.checked = enabled;
	return enabledCheckbox;
}

function createEditButton(enabled)
{
	editButton = createInput("button");
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editQuota;
	
	editButton.className = enabled ? "default_button" : "default_button_disabled" ;
	editButton.disabled  = enabled ? false : true;

	return editButton;
}
function setRowEnabled()
{
	enabled= this.checked ? "1" : "0";
	enabledRow=this.parentNode.parentNode;

	enabledRow.childNodes[5].firstChild.disabled  = this.checked ? false : true;
	enabledRow.childNodes[5].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

	var idStr = this.id;
	var ids = idStr.split(/\./);
	if(uci.get(pkg, ids[0]) != "")
	{
		uci.set(pkg, ids[0], "enabled", enabled);
	}
	if(uci.get(pkg, ids[1]) != "")
	{
		uci.set(pkg, ids[1], "enabled", enabled);
	}
}
function removeQuotaCallback(table, row)
{
	var id = row.childNodes[4].firstChild.id;
	uci.removeSection(pkg, id);
	allIps[ row.childNodes[0].firstChild.data.replace(/ /g, "_") ] = null;

	changedIps [ row.childNodes[0].firstChild.data.replace(/ /g, "_") ] = 1;
}

function editQuota()
{
	if( typeof(editQuotaWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editQuotaWindow.close();
		}
		catch(e){}
	}

	
	try
	{
		xCoor = window.screenX + 225;
		yCoor = window.screenY+ 225;
	}
	catch(e)
	{
		xCoor = window.left + 225;
		yCoor = window.top + 225;
	}


	editQuotaWindow = window.open("quotas_edit.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editQuotaWindow.document);
	closeButton = createInput("button", editQuotaWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	editRow=this.parentNode.parentNode;
	editIp          = editRow.childNodes[0].firstChild.data.replace(/ /g, "_");
	editUpPrc       = editRow.childNodes[1].firstChild.data.replace(/%/g, "");
	editDownPrc     = editRow.childNodes[2].firstChild.data.replace(/%/g, "");
	editCombinedPrc = editRow.childNodes[3].firstChild.data.replace(/%/g, "");

	editSection = "";
	var sections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "ip") == editIp || (editIp == "ALL" && uci.get(pkg, sections[sectionIndex], "ip") == ""))
		{
			editSection = sections[sectionIndex];
		}
	}

	editUpMax       = uci.get(pkg, editSection, "egress_limit");
	editDownMax     = uci.get(pkg, editSection, "ingress_limit");
	editCombinedMax = uci.get(pkg, editSection, "combined_limit");

	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(editQuotaWindow.document != null)
		{
			if(editQuotaWindow.document.getElementById("bottom_button_container") != null)
			{
				editQuotaWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editQuotaWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
				setDocumentFromUci(editQuotaWindow.document, uci, editIp);

				closeButton.onclick = function()
				{
					editQuotaWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = validateQuota(editQuotaWindow.document, editIp);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not add quota.");
					}
					else
					{
						var newIp = getIpFromDocument(editQuotaWindow.document);
						setUciFromDocument(editQuotaWindow.document);

						if(newIp != editIp)
						{
							editRow.childNodes[0].firstChild.data = newIp.replace(/_/g, " ");
							changedIps[editIp] = 1;
							changedIps[newIp] = 1;
							editRow.childNodes[1].firstChild.data = uci.get(pkg, editSection, "egress_limit") == "" ? "N/A" : "0%";
							editRow.childNodes[2].firstChild.data = uci.get(pkg, editSection, "ingress_limit") == "" ? "N/A" : "0%";
							editRow.childNodes[3].firstChild.data = uci.get(pkg, editSection, "combined_limit") == "" ? "N/A" : "0%";
						}
						else
						{
							var adjustPercent = function(usedOptionIndex, newMaxStr)
							{
								var oldUsedQ = quotaUsed[newIp];
								var newPercent = "0";
								if(oldUsedQ != null)
								{
									var oldUsed = oldUsedQ[usedOptionIndex];
									oldUsed = oldUsed == "" ? 0 : parseInt(oldUsed);
									var limit = parseFloat(newMaxStr)*1024.0*1024.0;
									newPercent =  Math.round((oldUsed*100*1000)/(limit))/1000 ;
								}
								return newPercent + "%";
							}

							var upMax   = editQuotaWindow.document.getElementById("max_up").value;
							var downMax = editQuotaWindow.document.getElementById("max_down").value;
							var combinedMax = editQuotaWindow.document.getElementById("max_combined").value;
							var useUpMax = getSelectedValue("max_up_type", editQuotaWindow.document) != "unlimited";
							var useDownMax = getSelectedValue("max_down_type", editQuotaWindow.document) != "unlimited";
							var useCombinedMax = getSelectedValue("max_combined_type", editQuotaWindow.document) != "unlimited";
							
							editRow.childNodes[1].firstChild.data = useUpMax   ? adjustPercent(0, upMax) : "N/A";
							editRow.childNodes[2].firstChild.data = useDownMax   ? adjustPercent(1, downMax) : "N/A";
							editRow.childNodes[3].firstChild.data = useCombinedMax  ? adjustPercent(2, combinedMax) : "N/A";

						}
						
						editQuotaWindow.close();
					}
				}
				editQuotaWindow.moveTo(xCoor,yCoor);
				editQuotaWindow.focus();
				updateDone = true;
				
			}
		}
		if(!updateDone)
		{
			setTimeout( "runOnEditorLoaded()", 250);
		}
	}
	runOnEditorLoaded();
}
