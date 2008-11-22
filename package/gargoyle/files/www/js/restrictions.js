function saveChanges()
{
	document.body.style.cursor="wait";
	document.getElementById("save_button").style.display="none";
	document.getElementById("reset_button").style.display="none";
	document.getElementById("update_container").style.display="block";
	
	var pkg = "restricter_gargoyle";
	
	//set enabled status to corrospond with checked in table
	enabledRuleFound = false;
	var runCommands = [];
	var ruleTableContainer = document.getElementById('restriction_table_container');
	var ruleTable = ruleTableContainer.firstChild;
	var ruleData = getTableDataArray(ruleTable);
	for(ruleIndex =0; ruleIndex < ruleData.length; ruleIndex++)
	{
		var check = ruleData[ruleIndex][1];
		enabledRuleFound = enabledRuleFound || check.checked; 
		uci.set(pkg, check.id, "enabled", check.checked ? "1" : "0");
	}
	if(enabledRuleFound || restricterEnabled)
	{
		runCommands.push("/etc/init.d/restricter_gargoyle enable");
		runCommands.push("/etc/init.d/restricter_gargoyle restart");
	}


	//delete all block ingress sections in uciOriginal & remove them from uciOriginal
	var deleteSectionCommands = [];
	var originalBlockSections = uciOriginal.getAllSectionsOfType(pkg, "block");
	for(blockIndex=0; blockIndex < originalBlockSections.length; blockIndex++)
	{
		var isIngress = uciOriginal.get("restricter_gargoyle", originalBlockSections[blockIndex], "is_ingress");
		if(isIngress != "1")
		{
			uciOriginal.removeSection(pkg, originalBlockSections[blockIndex]);
			deleteSectionCommands.push("uci del " + pkg + "." + originalBlockSections[blockIndex]);
		}
	}
	deleteSectionCommands.push("uci commit");
	
	//create/initialize all block sections in uci
	var createSectionCommands = [];
	var newBlockSections = uci.getAllSectionsOfType(pkg, "block");
	for(blockIndex=0; blockIndex < newBlockSections.length; blockIndex++)
	{
		createSectionCommands.push("uci set " + pkg + "." + newBlockSections[blockIndex] + "=block");
	}
	createSectionCommands.push("uci commit");
	
	var commands = deleteSectionCommands.join("\n") + "\n" + createSectionCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + runCommands.join("\n") + "\n";


	var param = getParameterDefinition("commands", commands);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{	
			restricterEnabled = restricterEnabled || enabledRuleFound;
			uciOriginal = uci.clone();
			resetData();
			document.getElementById("update_container").style.display="none";		
			document.getElementById("save_button").style.display="inline";
			document.getElementById("reset_button").style.display="inline";
			document.body.style.cursor='auto';
		
			//alert(req.responseText);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}

function resetData()
{
	// Instead of having enabled/disabled button, just display all rules to disabled if 
	// the restricter_daemon is not enabled
	var blockSections = uciOriginal.getAllSectionsOfType("restricter_gargoyle", "block");
	var restrictionTableData = new Array();
	for(blockIndex=0; blockIndex < blockSections.length; blockIndex++)
	{
		var isIngress = uciOriginal.get("restricter_gargoyle", blockSections[blockIndex], "is_ingress");
		if(isIngress != "1")
		{
			var description = uciOriginal.get("restricter_gargoyle", blockSections[blockIndex], "description");
			description = description == "" ? blockSections[blockIndex] : description;
			
			var enabledStr =   uciOriginal.get("restricter_gargoyle", blockSections[blockIndex], "enabled");
			var enabledBool =  (enabledStr == "" || enabledStr == "1" || enabledStr == "true") && restricterEnabled;
			var enabledCheck = createEnabledCheckbox(enabledBool);
			enabledCheck.id = blockSections[blockIndex]; //save section id as checkbox name (yeah, it's kind of sneaky...)
		
			restrictionTableData.push([description, enabledCheck, createEditButton(enabledBool)]);
		}
	}

	columnNames=["Rule Description", "Enabled", ""];
	restrictionTable = createTable(columnNames, restrictionTableData, "restriction_table", true, false, removeRuleCallback);
	tableContainer = document.getElementById('restriction_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(restrictionTable);

	setDocumentFromUci(document, new UCIContainer(), "");

	setVisibility();
}

function addNewRule()
{
	var errors = validateRule(document);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\nCould not add rule.");
	}
	else
	{
		var tableContainer = document.getElementById('restriction_table_container');
		var table = tableContainer.firstChild;
		var tableData = getTableDataArray(table);
		var newId = "rule_" + (tableData.length+1);
		setUciFromDocument(document, newId);

		var description = uciOriginal.get("restricter_gargoyle", newId, "description");
		description = description == "" ? newId : description;

		var enabledCheck = createEnabledCheckbox(true);
		enabledCheck.id = newId; //save section id as checkbox name (yeah, it's kind of sneaky...)
		
		addTableRow(table, [description, enabledCheck, createEditButton(true)], true, false, removeRuleCallback);	

		setDocumentFromUci(document, new UCIContainer(), "");
	}
}

function setVisibility(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	
	setInvisibleIfAnyChecked(["all_access"], "restricted_resources", "block", controlDocument);
	setInvisibleIfAnyChecked(["all_day"], "hours_active_container", "block", controlDocument);
	setInvisibleIfAnyChecked(["every_day"], "days_active", "block", controlDocument);
	setInvisibleIfAnyChecked(["all_day", "every_day"], "days_and_hours_active_container", "block", controlDocument);
	setInvisibleIfAnyChecked(["all_day", "every_day"], "schedule_repeats", "inline", controlDocument);


	var scheduleRepeats = controlDocument.getElementById("schedule_repeats");
	if(scheduleRepeats.style.display != "none")
	{
		setInvisibleIfIdMatches("schedule_repeats", "daily", "days_and_hours_active_container", "block", controlDocument);
		setInvisibleIfIdMatches("schedule_repeats", "weekly", "days_active", "block", controlDocument);
		setInvisibleIfIdMatches("schedule_repeats", "weekly", "hours_active_container", "block", controlDocument);
	}


	setInvisibleIfIdMatches("rule_applies_to", "all", "rule_applies_to_container", "block", controlDocument);
	setInvisibleIfIdMatches("remote_ip_type", "all", "remote_ip_container", "block", controlDocument);
	setInvisibleIfIdMatches("remote_port_type", "all", "remote_port", "inline", controlDocument);
	setInvisibleIfIdMatches("local_port_type", "all", "local_port", "inline", controlDocument);
	setInvisibleIfIdMatches("app_protocol_type", "all", "app_protocol", "inline", controlDocument);
	setInvisibleIfIdMatches("url_type", "all", "url_match_list", "block", controlDocument);
}

function setInvisibleIfAnyChecked(checkIds, associatedElementId, defaultDisplayMode, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	defaultDisplayMode = defaultDisplayMode == null ? "block" : defaultDisplayMode;
	var visElement = controlDocument.getElementById(associatedElementId);

	var isChecked = false;
	for(checkIndex = 0; checkIndex < checkIds.length ; checkIndex++)
	{
		var checkElement = controlDocument.getElementById( checkIds[checkIndex] );
		if(checkElement != null)
		{
			isChecked = isChecked || checkElement.checked;
		}
	}

	if(isChecked && visElement != null)
	{
		visElement.style.display = "none";
	}
	else if(visElement != null)
	{
		visElement.style.display = defaultDisplayMode;
	}

}

function setInvisibleIfIdMatches(selectId, invisibleOptionValue, associatedElementId, defaultDisplayMode, controlDocument )
{
	controlDocument = controlDocument == null ? document : controlDocument;
	defaultDisplayMode = defaultDisplayMode == null ? "block" : defaultDisplayMode;
	var visElement = controlDocument.getElementById(associatedElementId);
	
	if(getSelectedValue(selectId, controlDocument) == invisibleOptionValue && visElement != null)
	{
		visElement.style.display = "none";
	}
	else if(visElement != null)
	{
		visElement.style.display = defaultDisplayMode;
	}
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
	editButton = createButton();
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editRule;
	
	editButton.className = enabled ? "default_button" : "default_button_disabled" ;
	editButton.disabled  = enabled ? false : true;

	return editButton;
}
function setRowEnabled()
{
	enabled= this.checked ? "1" : "0";
	enabledRow=this.parentNode.parentNode;
	enabledId = this.id;

	enabledRow.childNodes[2].firstChild.disabled  = this.checked ? false : true;
	enabledRow.childNodes[2].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

	uci.set("restricter_gargoyle", enabledId, "enabled", enabled);
}
function removeRuleCallback(table, row)
{
	var ruleId = row.childNodes[1].firstChild.id;
	uci.removeSection("restricter_gargoyle", id);
}

function editRule()
{
	
	if( typeof(editRuleWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editRuleWindow.close();
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


	editRuleWindow = window.open("restriction_edit_rule.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editRuleWindow.document);
	closeButton = createInput("button", editRuleWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	editRow=this.parentNode.parentNode;
	editRuleSectionId = editRow.childNodes[1].firstChild.id;

	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(editRuleWindow.document != null)
		{
			if(editRuleWindow.document.getElementById("bottom_button_container") != null)
			{
				editRuleWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editRuleWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
			
				setDocumentFromUci(editRuleWindow.document, uci, editRuleSectionId);
				setVisibility(editRuleWindow.document);

				closeButton.onclick = function()
				{
					editRuleWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = validateRule(editRuleWindow.document);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\nCould not add rule.");
					}
					else
					{
						setUciFromDocument(editRuleWindow.document, editRuleSectionId);
						editRuleWindow.close();
					}
					
				}
				editRuleWindow.moveTo(xCoor,yCoor);
				editRuleWindow.focus();
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

function addIpsToTable(controlDocument, textId, tableContainerId, tableId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var newIps = controlDocument.getElementById(textId).value;
	var valid = validateMultipleIps(newIps);
	if(valid == 0)
	{
		var tableContainer = controlDocument.getElementById(tableContainerId);
		var table = tableContainer.childNodes.length > 0 ? tableContainer.firstChild : createTable([""], [], tableId, true, false);
		newIps = newIps.replace(/^[\t ]*/, "");
		newIps = newIps.replace(/[\t ]*$/, "");
		var ips = newIps.split(/[\t ]*,[\t ]*/);
		
		while(ips.length > 0)
		{
			addTableRow(table, [ ips.shift() ], true, false);
		}
		
		if(tableContainer.childNodes.length == 0)
		{
			tableContainer.appendChild(table);
		}
		controlDocument.getElementById(textId).value = "";
	}
	else
	{
		alert("ERROR: Invalid IP or IP range\n");
	}
}

function addUrlToTable(controlDocument, textId, selectId, tableContainerId, tableId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var newUrl = controlDocument.getElementById(textId).value;
	var urlType = getSelectedValue(selectId, controlDocument);
	var valid = validateUrl(newUrl, selectId, controlDocument);
	if(valid == 0)
	{
		var urlSpan = createUrlSpan(newUrl, controlDocument);
		var tableContainer = controlDocument.getElementById(tableContainerId);
		var table = tableContainer.childNodes.length > 0 ? tableContainer.firstChild : createTable(["Match Type", "URL"], [], tableId, true, false);
		addTableRow(table, [urlType, urlSpan ], true, false);
		if(tableContainer.childNodes.length == 0)
		{
			tableContainer.appendChild(table);
		}
		controlDocument.getElementById(textId).value = "";
	}
	else
	{
		if( (!newUrl.match(/^http:\/\//)) && urlType == "exact")
		{
			alert("ERROR: Exact URL match must start with \"http://\"\n");
		}
		else
		{
			alert("ERROR: URL cannot contain quote or newline characters\n");
		}
	}
}

function validateRule(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var inputIds = ["hours_active", "days_and_hours_active", "remote_port", "local_port"];
	var labelIds = ["hours_active_label", "days_and_hours_active_label", "remote_port_label", "local_port_label"];
	var functions = [validateHours, validateDaysAndHours, validateMultiplePorts, validateMultiplePorts];
	var validReturnCodes = [0,0,0,0];
	var visibilityIds = ["hours_active_container", "days_and_hours_active_container", "remote_port", "local_port"];
	if(controlDocument.getElementById("all_access").checked)
	{
		visibilityIds[2] = "restricted_resources";
		visibilityIds[3] = "restricted_resources";
	}
	
	return proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );
}

function validateHours(hoursStr)
{
	var commaSplit = hoursStr.match(/,/) ? hoursStr.split(/,/) : [ hoursStr ] ;
	var valid = true;
	for(commaIndex = 0; commaIndex < commaSplit.length && valid; commaIndex++)
	{
		var splitStr = commaSplit[commaIndex].split(/-/);
		var nextValid = splitStr.length == 2;
		if(nextValid)
		{
			nextValid = nextValid && splitStr[0].match(/^[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5]?[0-9])?(:[0-5]?[0-9])?[\t ]*$/)
			nextValid = nextValid && splitStr[1].match(/^[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5]?[0-9])?(:[0-5]?[0-9])?[\t ]*$/)
		}
		valid = valid && nextValid;
	}
	return valid ? 0 : 1;
}
function proofreadHours(input)
{
	proofreadText(input, validateHours, 0);
}

function validateDaysAndHours(daysAndHoursStr)
{
	var commaSplit = hoursStr.match(/,/) ? hoursStr.split(/,/) : [ hoursStr ] ;
	var valid = true;
	for(commaIndex = 0; commaIndex < commaSplit.length && valid; commaIndex++)
	{
		var splitStr = commaSplit[commaIndex].split(/-/);
		var nextValid = splitStr.length == 2;
		if(nextValid)
		{
			splitStr[0] = splitStr[0].toLowerCase();
			splitStr[1] = splitStr[1].toLowerCase();
			nextValid = nextValid && splitStr[0].match(/^[\t ]*(sun|mon|tue|wed|thu|fri|sat)[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5]?[0-9])?(:[0-5]?[0-9])?[\t ]*$/);
			nextValid = nextValid && splitStr[1].match(/^[\t ]*(sun|mon|tue|wed|thu|fri|sat)[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5]?[0-9])?(:[0-5]?[0-9])?[\t ]*$/);
		}
		valid = valid && nextValid;
	}
	return valid ? 0 : 1;
}
function proofreadDaysAndHours(input)
{
	proofreadText(input, validateDaysAndHours, 0);
}

function validateMultipleIps(ips)
{
	ips = ips.replace(/^[\t ]+/g, "");
	ips = ips.replace(/[\t ]+$/g, "");
	var splitIps = ips.split(/[\t ]*,[\t ]*/);
	var valid = splitIps.length > 0 ? 0 : 1;
	while(valid == 0 && splitIps.length > 0)
	{
		var nextIp = splitIps.pop();
		if(nextIp.match(/-/))
		{
			var nextSplit = nextIp.split(/[\t ]*-[\t ]*/);
			valid = nextSplit.length==2 && validateIP(nextSplit[0]) == 0 && validateIP(nextSplit[1]) == 0 ? 0 : 1;
		}
		else
		{
			valid = validateIpRange(nextIp);
		}
	}
	return valid;
}
function proofreadMultipleIps(input)
{
	proofreadText(input, validateMultipleIps, 0);
}

function validateMultiplePorts(portStr)
{
	portStr = portStr.replace(/^[\t ]+/g, "");
	portStr = portStr.replace(/[\t ]+$/g, "");
	var splitStr = portStr.match(/,/) ?  portStr.split(/[\t ]*,[\t ]*/) : [portStr];
	var valid = true;
	for(splitIndex = 0; splitIndex < splitStr.length; splitIndex++)
	{
		splitStr[splitIndex].replace(/^[\t ]+/g, "");
		splitStr[splitIndex].replace(/[\t ]+$/g, "");
		valid = valid && (validatePortOrPortRange(splitStr[splitIndex]) == 0);
	}
	return valid ? 0 : 1;
}
function proofreadMultiplePorts(input)
{
	proofreadText(input, validateMultiplePorts, 0);
}

function validateUrl(url, selectId, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var urlType = getSelectedValue(selectId, controlDocument);
	var valid = url.match(/[\n\r\"\']/) ? 1 : 0;
	if(urlType == "exact")
	{
		valid = valid ==0 && url.match(/^http:\/\//) ? 0 : 1;
	}
	return valid;
}

function proofreadUrl(input)
{
	proofreadText(input, validateUrl, 0);
}

function createUrlSpan(urlStr, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var splitUrl = [];
	while(urlStr.length > 0)
	{
		var next = urlStr.substr(0, 30);
		urlStr = urlStr.substr(30);
		splitUrl.push(urlStr.length > 0 ? next + "-" : next);
	}
	
	var urlSpan = controlDocument.createElement('span');
	while(splitUrl.length > 0)
	{
		urlSpan.appendChild(controlDocument.createTextNode( splitUrl.shift() ));
		if(splitUrl.length > 0)
		{
			urlSpan.appendChild(controlDocument.createElement('br'));
		}
	}
	return urlSpan;
}
function parseUrlSpan(urlSpan)
{
	var children = urlSpan.childNodes;
	var parsedUrl = "";
	for(childIndex=0; childIndex < children.length; childIndex++)
	{
		if(childIndex %2 == 0)
		{
			var nextStr = children[childIndex].data;
			if(childIndex < children.length-1)
			{
				nextStr = nextStr.substr(0, nextStr.length-1);
			}
			parsedUrl = parsedUrl + nextStr;
		}
	}
	return parsedUrl;
}


function setDocumentFromUci(controlDocument, sourceUci, sectionId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var pkg = "restricter_gargoyle";

	var description = sourceUci.get(pkg, sectionId, "description");
	description = description == "" ? sectionId : description;	
	controlDocument.getElementById("restriction_name").value = description;

	setIpTableAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "local_addr", "applies_to_table_container", "applies_to_table", "rule_applies_to", "applies_to_ip");


	var daysAndHours = sourceUci.get(pkg, sectionId, "weekly_block_times");
	var hours = sourceUci.get(pkg, sectionId, "hours_blocked");
	var allDay = (daysAndHours == "" && hours == "");
	controlDocument.getElementById("hours_active").value = hours;
	controlDocument.getElementById("all_day").checked = allDay;
	controlDocument.getElementById("days_and_hours_active").value = daysAndHours;

	
	var days =  sourceUci.get(pkg, sectionId, "weekdays_blocked");
	var dayIds = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
	var everyDay = (daysAndHours == "");
	days = days.split(/,/);
	days = days.length != 7 ? ["1","1","1","1","1","1","1"] : days;
	
	for(dayIndex = 0; dayIndex < days.length; dayIndex++)
	{
		days[dayIndex] = days[dayIndex].replace(/^[\t ]*/, "");
		days[dayIndex] = days[dayIndex].replace(/[\t ]*$/, "");
		everyDay = everyDay && (days[dayIndex] != "0");
		controlDocument.getElementById(dayIds[dayIndex]).checked = (days[dayIndex] != "0");
	}
	controlDocument.getElementById("every_day").checked = everyDay;

	setIpTableAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "remote_addr", "remote_ip_table_container", "remote_ip_table", "remote_ip_type", "remote_ip");
	setTextAndSelectFromUci(controlDocument, sourceUci,  pkg, sectionId, "remote_port", "remote_port", "remote_port_type");
	setTextAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "local_port", "local_port", "local_port_type");

	var proto = sourceUci.get(pkg, sectionId, "proto");
	proto = proto != "tcp" && proto != "udp" ? "both" : proto;
	setSelectedValue("transport_protocol", proto, controlDocument);

	var app_proto = sourceUci.get(pkg, sectionId, "app_proto");
	var app_proto_type = app_proto == "" ? "except" : "only";
	app_proto = app_proto == "" ? sourceUci.get(pkg, sectionId, "not_app_proto") : app_proto;
	app_proto_type = app_proto == "" ? "all" : app_proto_type;
	setSelectedValue("app_protocol_type", app_proto_type, controlDocument);
	setSelectedValue("app_protocol", app_proto, controlDocument);
	
	var containsStr = sourceUci.get(pkg, sectionId, "url");
	var regexStr = sourceUci.get(pkg, sectionId, "url_regex");
	var urlType = containsStr == "" && regexStr == "" ? "except" : "only";
	if(urlType == "except")
	{
		containsStr = sourceUci.get(pkg, sectionId, "not_url");
		regexStr = sourceUci.get(pkg, sectionId, "not_url_regex");
		urlType = containsStr == "" && regexStr == "" ? "all" : "except";
	}
	setSelectedValue("url_type", urlType, controlDocument);
	if(urlType != "all")
	{
		var contains = [];
		var regex = [];
		var exact = [];

		if(containsStr != "")
		{
			containsStr = containsStr.replace(/^[\t ]*\"/, "");
			containsStr = containsStr.replace(/\"[\t ]*$/, "");
			contains = containsStr.match(/\".*\"/) ? containsStr.split(/\"[\t, ]*\"/) : [ containsStr ];
		}
		if(regexStr != "")
		{
			regexStr = regexStr.replace(/^[\t ]*\"/, "");
			regexStr = regexStr.replace(/\"[\t ]*$/, "");
			var splitStr = regexStr.match(/\".*\"/) ? regexStr.split(/\"[\t, ]*\"/) : [ regexStr ];
			for(splitIndex = 0; splitIndex < splitStr.length; splitIndex++)
			{
				//determine if this is an exact match
				var testStr = splitStr[splitIndex];
				var isExact = false;
				var matchesStartAndEnd = testStr.match(/^\^http:\/\/.*\$$/) ? true : false;
				
				testStr = testStr.replace(/^\^/, "");
				testStr = testStr.replace(/\$$/, "");
				
						
				var unescapedSpecial = 	testStr.match(/[^\\]\*/) || 
							testStr.match(/[^\\]\./) ||
							testStr.match(/[^\\]\]/) ||
							testStr.match(/[^\\]\[/) ||
							testStr.match(/[^\\]\)/) ||
							testStr.match(/[^\\]\(/) ||
							testStr.match(/[^\\]\}/) ||
							testStr.match(/[^\\]\{/) ||
							testStr.match(/[^\\]\+/) ||
							testStr.match(/[^\\]\-/) ||
							testStr.match(/[^\\]\?/) ||
							testStr.match(/[^\\]\$/) ||
							testStr.match(/[^\\]\^/) ? true : false;
				
				if(matchesStartAndEnd && (!unescapedSpecial))
				{
					testStr = testStr.replace(/\\\*/g, "*");
					testStr = testStr.replace(/\\\./g, ".");
					testStr = testStr.replace(/\\\]/g, "]");
					testStr = testStr.replace(/\\\[/g, "[");
					testStr = testStr.replace(/\\\)/g, ")");
					testStr = testStr.replace(/\\\(/g, "(");
					testStr = testStr.replace(/\\\}/g, "}");
					testStr = testStr.replace(/\\\{/g, "{");
					testStr = testStr.replace(/\\\+/g, "+");
					testStr = testStr.replace(/\\\-/g, "-");
					testStr = testStr.replace(/\\\?/g, "?");
					testStr = testStr.replace(/\\\^/g, "^");
					testStr = testStr.replace(/\\\$/g, "$");
					
					var testStr2 = testStr.replace(/\\\\/, "");
					if(!testStr2.match(/\\/))
					{
						testStr = testStr.replace(/\\\\/, /\\/);
						exact.push(testStr);
						isExact = true;
					}

				}
				if(!isExact)
				{
					regex.push( splitStr[splitIndex] );
				}
			}
		}
		if(exact.length > 0 || regex.length > 0 || contains.length > 0)
		{
			var tableContainer = controlDocument.getElementById("url_match_table_container");
			if(tableContainer.childNodes.length > 0)
			{
				tableContainer.removeChild(tableContainer.firstChild);
			}
			var table = createTable(["Match Type", "URL"], [], "url_match_table", true, false);
			for(containsIndex=0; containsIndex < contains.length; containsIndex++)
			{
				addTableRow(table, ["contains", createUrlSpan(contains[containsIndex], controlDocument) ], true, false);
			}
			for(regexIndex=0; regexIndex < regex.length; regexIndex++)
			{
				addTableRow(table, ["regex", createUrlSpan(regex[regexIndex], controlDocument)], true, false);
			}
			for(exactIndex=0; exactIndex < exact.length; exactIndex++)
			{
				addTableRow(table, ["exact", createUrlSpan(exact[exactIndex], controlDocument)], true, false);
			}
			tableContainer.appendChild(table);
		}
	}
	controlDocument.getElementById("url_match").value = "";
	
	var allResourcesBlocked = true;
	var resourceTypeIds = ["remote_ip_type", "remote_port_type", "local_port_type", "transport_protocol", "app_protocol_type", "url_type" ];
	for(typeIndex=0; typeIndex < resourceTypeIds.length; typeIndex++)
	{
		var type = getSelectedValue(resourceTypeIds[typeIndex], controlDocument);
		allResourcesBlocked = allResourcesBlocked && (type == "all" || type == "both");
	}
	controlDocument.getElementById("all_access").checked = allResourcesBlocked;
	
	setVisibility(controlDocument);
}
function setIpTableAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, optionId, tableContainerId, tableId, prefixSelectId, textId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var optionValue = sourceUci.get(pkg, sectionId, optionId);
	var type = "only";
	if(optionValue == "")
	{
		optionValue = sourceUci.get(pkg, sectionId, "not_" + optionId)
		type = optionValue != "" ? "except" : "all";
	}
	setSelectedValue(prefixSelectId, type, controlDocument);
	
	if(optionValue != "")
	{
		optionValue = optionValue.replace(/^[\t ]*/, "");
		optionValue = optionValue.replace(/[\t ]*$/, "");
		var ips = optionValue.split(/[\t ]*,[\t ]*/);

		var tableContainer = controlDocument.getElementById(tableContainerId);
		if(tableContainer.childNodes.length > 0)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		var table = createTable([""], [], tableId, true, false);
		while(ips.length > 0)
		{
			addTableRow(table, [ ips.shift() ], true, false);
		}
		tableContainer.appendChild(table);
		
		controlDocument.getElementById(textId).value = "";
	}
}
function setTextAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, optionId, textId, prefixSelectId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var optionValue = sourceUci.get(pkg, sectionId, optionId);
	var type = "only";
	if(optionValue == "")
	{
		optionValue = sourceUci.get(pkg, sectionId, "not_" + optionId)
		type = optionValue != "" ? "except" : "all";
	}
	setSelectedValue(prefixSelectId, type, controlDocument);

	// if option is not defined, optionValue is empty string, so no need to check for this case
	controlDocument.getElementById(textId).value = optionValue;
}


function setUciFromDocument(controlDocument, sectionId)
{
	// note: we assume error checking has already been done 
	uci.removeSection("restricter_gargoyle", sectionId);
	uci.set("restricter_gargoyle", sectionId, "", "block");
	uci.set("restricter_gargoyle", sectionId, "is_ingress", "0");


	controlDocument = controlDocument == null ? document : controlDocument;
	var pkg = "restricter_gargoyle";
	
	uci.set(pkg, sectionId, "", "block");
	uci.set(pkg, sectionId, "description", controlDocument.getElementById("restriction_name").value);
	
	setFromIpTable(controlDocument, pkg, sectionId, "local_addr", "applies_to_table_container", "rule_applies_to");

	var daysActive = controlDocument.getElementById("days_active");
	if(daysActive.style.display != "none")
	{
		var daysActiveStr = "";
		var dayIds = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
		for(dayIndex =0; dayIndex < dayIds.length; dayIndex++)
		{
			daysActiveStr = daysActiveStr + (controlDocument.getElementById(dayIds[dayIndex]).checked ? "1," : "0,");
		}
		daysActiveStr = daysActiveStr.replace(/,$/, "");
		uci.set(pkg, sectionId, "weekdays_blocked", daysActiveStr);
	}
	setIfVisible(controlDocument, pkg, sectionId, "hours_blocked", "hours_active");
	setIfVisible(controlDocument, pkg, sectionId, "weekly_block_times", "days_and_hours_active");

	if(!controlDocument.getElementById("all_access").checked)
	{
		setFromIpTable(controlDocument, pkg, sectionId, "remote_addr", "remote_ip_table_container", "remote_ip_type");
		setIfVisible(controlDocument, pkg, sectionId, "remote_port", "remote_port", "remote_port_type");
		setIfVisible(controlDocument, pkg, sectionId, "local_port", "local_port", "local_port_type");
		
		uci.set(pkg, sectionId, "proto", getSelectedValue("transport_protocol", controlDocument));

		var appProtocolType = getSelectedValue("app_protocol_type", controlDocument);
		if(appProtocolType != "all")
		{
			var prefix = appProtocolType == "except" ? "not_" : "";
			uci.set(pkg, sectionId, prefix + "app_proto", getSelectedValue("app_protocol", controlDocument));
		}

		var urlType = getSelectedValue("url_type", controlDocument);
		var urlTable = controlDocument.getElementById("url_match_table_container").firstChild;
		if(urlType != "all" && urlTable != null)
		{
			var regexStr = "";
			var containsStr = "";
			var urlData = getTableDataArray(urlTable, true, false);
			for(urlIndex = 0; urlIndex < urlData.length; urlIndex++)
			{
				var urlMatchType = urlData[urlIndex][0];
				var urlMatch = parseUrlSpan(urlData[urlIndex][1]);
			
				if(urlMatchType == "contains")
				{
					containsStr = containsStr + "\"" + urlMatch + "\",";
				}
				else if(urlMatchType == "exact")
				{
					urlMatch = urlMatch.replace("\\", "\\\\");
					urlMatch = urlMatch.replace("*", "\\*");
					urlMatch = urlMatch.replace(".", "\\.");
					urlMatch = urlMatch.replace("]", "\\]");
					urlMatch = urlMatch.replace("[", "\\[");
					urlMatch = urlMatch.replace(")", "\\)");
					urlMatch = urlMatch.replace("(", "\\(");
					urlMatch = urlMatch.replace("}", "\\}");
					urlMatch = urlMatch.replace("{", "\\{");
					urlMatch = urlMatch.replace("+", "\\+");
					urlMatch = urlMatch.replace("-", "\\-");
					urlMatch = urlMatch.replace("?", "\\?");
					urlMatch = urlMatch.replace("^", "\\^");
					urlMatch = urlMatch.replace("$", "\\$");
					urlMatch = urlMatch.replace("|", "\\|");
					regexStr = regexStr + "\"^" + urlMatch + "$\",";
				}
				else if(urlMatchType == "regex")
				{
					regexStr = regexStr + "\"" + urlMatch + "\",";
				}
			}
			containsStr = containsStr.replace(/,$/, "");
			regexStr = regexStr.replace(/,$/, "");
		
		
			var prefix = urlType == "except" ? "not_" : "";
			if(containsStr.length > 0)
			{
				uci.set(pkg, sectionId, prefix + "url", containsStr);
			}
			if(regexStr.length > 0)
			{
				uci.set(pkg, sectionId, prefix + "url_regex", regexStr);
			}
		}
	}
}



function setIfVisible(controlDocument, pkg, sectionId, optionId, textId, prefixSelectId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var element = controlDocument.getElementById(textId);
	if(element.style.display != "none")
	{
		if(prefixSelectId != null)
		{
			prefixValue = getSelectedValue(prefixSelectId, controlDocument);
			optionId = prefixValue == "except" ? "not_" + optionId : optionId;
		}
		uci.set(pkg, sectionId, optionId, element.value);
	}
}

function setFromIpTable(controlDocument, pkg, sectionId, optionId, containerId, prefixSelectId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var localAddrType = getSelectedValue(prefixSelectId, controlDocument);
	var table = controlDocument.getElementById(containerId).firstChild;
	if(localAddrType != "all" && table != null)
	{
		var ipData = getTableDataArray(table, true, false);
		var ipStr = "";
		for(ipIndex=0; ipIndex < ipData.length ; ipIndex++)
		{
			ipStr = ipStr + ipData[ipIndex][0] + ",";
		}
		ipStr = ipStr.replace(/,$/, "");
		if(ipStr.length > 0)
		{
			var prefix = localAddrType == "except" ? "not_" : "";
			uci.set(pkg, sectionId, prefix + optionId, ipStr);
		}
	}
}

