var restStr=new Object(); //part of i18n

var pkg = "firewall";

function saveChanges()
{
	setControlsEnabled(false, true);
	
	var enabledRuleFound = false;
	var runCommands = [];


	var ruleTypes = [ "restriction_rule", "whitelist_rule" ];
	var rulePrefixes   = [ "rule_", "exception_" ];
	var typeIndex=0;
	var deleteSectionCommands = [];
	var createSectionCommands = [];
	for(typeIndex=0; typeIndex < ruleTypes.length; typeIndex++)
	{
		//set enabled status to corrospond with checked in table
		var ruleTableContainer = document.getElementById(rulePrefixes[typeIndex] + 'table_container');
		var ruleTable = ruleTableContainer.firstChild;
		var ruleData = getTableDataArray(ruleTable);
		for(ruleIndex =0; ruleIndex < ruleData.length; ruleIndex++)
		{
			var check = ruleData[ruleIndex][1];
			enabledRuleFound = enabledRuleFound || check.checked; 
			uci.set(pkg, check.id, "enabled", check.checked ? "1" : "0");
		}

		
		//delete all sections of type in uciOriginal & remove them from uciOriginal
		var originalSections = uciOriginal.getAllSectionsOfType(pkg, ruleTypes[typeIndex]);
		var sectionIndex = 0;
		for(sectionIndex=0; sectionIndex < originalSections.length; sectionIndex++)
		{
			var isIngress = uciOriginal.get(pkg, originalSections[sectionIndex], "is_ingress");
			if(isIngress != "1")
			{
				uciOriginal.removeSection(pkg, originalSections[sectionIndex]);
				deleteSectionCommands.push("uci del " + pkg + "." + originalSections[sectionIndex]);
			}
		}
	
		//create/initialize  sections in uci
		var newSections = uci.getAllSectionsOfType(pkg, ruleTypes[typeIndex]);
		for(sectionIndex=0; sectionIndex < newSections.length; sectionIndex++)
		{
			createSectionCommands.push("uci set " + pkg + "." + newSections[sectionIndex] + "='" + ruleTypes[typeIndex] + "'");
		}
	}
	deleteSectionCommands.push("uci commit");
	createSectionCommands.push("uci commit");
	

	var commands = deleteSectionCommands.join("\n") + "\n" + createSectionCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + runCommands.join("\n") + "\n" + "sh /usr/lib/gargoyle/restart_firewall.sh";

	var param = getParameterDefinition("commands", commands) +  "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{	
			uciOriginal = uci.clone();
			resetData();
			setControlsEnabled(true);	
			//alert(req.responseText);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}

function resetData()
{
	var ruleTypes = [ "restriction_rule", "whitelist_rule" ];
	var rulePrefixes   = [ "rule_", "exception_" ];
	var typeIndex=0;
	for(typeIndex=0; typeIndex < ruleTypes.length; typeIndex++)
	{
		var ruleType = ruleTypes[typeIndex];
		var rulePrefix = rulePrefixes[typeIndex];

		var sections = uciOriginal.getAllSectionsOfType(pkg, ruleType);
		var ruleTableData = new Array();
		var checkElements = []; //because IE is a bitch and won't register that checkboxes are checked/unchecked unless they are part of document
		var areChecked = [];
		for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
		{
			var isIngress = uciOriginal.get(pkg, sections[sectionIndex], "is_ingress");
			if(isIngress != "1")
			{
				var description = uciOriginal.get(pkg, sections[sectionIndex], "description");
				description = description == "" ? sections[sectionIndex] : description;
				
				var enabledStr =   uciOriginal.get(pkg, sections[sectionIndex], "enabled");
				var enabledBool =  (enabledStr == "" || enabledStr == "1" || enabledStr == "true") ;
				var enabledCheck = createEnabledCheckbox(enabledBool);
				enabledCheck.id = sections[sectionIndex]; //save section id as checkbox name (yeah, it's kind of sneaky...)
				
				checkElements.push(enabledCheck);
				areChecked.push(enabledBool);
	
				ruleTableData.push([description, enabledCheck, createEditButton(enabledBool)]);
			}
		}
		
		var firstColumn = ruleType == "restriction_rule" ? restStr.RDesc : restStr.ESect;
		columnNames=[firstColumn, UI.Enabled, ""];
		ruleTable = createTable(columnNames, ruleTableData, rulePrefix + "table", true, false, removeRuleCallback);
		
		tableContainer = document.getElementById(rulePrefix + 'table_container');
		
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(ruleTable);

		while(checkElements.length > 0)
		{
			var c = checkElements.shift();
			var b = areChecked.shift();
			c.checked = b;
		}

		setDocumentFromUci(document, new UCIContainer(), "", ruleType, rulePrefix);
		setVisibility(document, rulePrefix);
	}
}


function addNewRule(ruleType, rulePrefix)
{
	var errors = validateRule(document, rulePrefix);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+restStr.ARErr);
	}
	else
	{
		var tableContainer = document.getElementById(rulePrefix + 'table_container');
		var table = tableContainer.firstChild;
		var tableData = getTableDataArray(table);
		
		var newIndex = tableData.length+1;
		var newId = rulePrefix + "" + newIndex;
		while( uci.get(pkg, newId, "") != "" )
		{
			newIndex++;
			newId = rulePrefix + "" + newIndex;
		}
		
		setUciFromDocument(document, newId, ruleType, rulePrefix);

		var description = uci.get(pkg, newId, "description");
		description = description == "" ? newId : description;

		var enabledCheck = createEnabledCheckbox(true);
		enabledCheck.id = newId; //save section id as checkbox name (yeah, it's kind of sneaky...)
		
		addTableRow(table, [description, enabledCheck, createEditButton(true)], true, false, removeRuleCallback);	

		setDocumentFromUci(document, new UCIContainer(), "", ruleType, rulePrefix);

		enabledCheck.checked = true;
	}
}

function setVisibility(controlDocument, rulePrefix)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	
	setInvisibleIfAnyChecked([rulePrefix + "all_access"], rulePrefix + "resources", "block", controlDocument);
	setInvisibleIfAnyChecked([rulePrefix + "all_day"], rulePrefix + "hours_active_container", "block", controlDocument);
	setInvisibleIfAnyChecked([rulePrefix + "every_day"], rulePrefix + "days_active", "block", controlDocument);
	setInvisibleIfAnyChecked([rulePrefix + "all_day", rulePrefix + "every_day"], rulePrefix + "days_and_hours_active_container", "block", controlDocument);
	setInvisibleIfAnyChecked([rulePrefix + "all_day", rulePrefix + "every_day"], rulePrefix + "schedule_repeats", "inline", controlDocument);


	var scheduleRepeats = controlDocument.getElementById(rulePrefix + "schedule_repeats");
	if(scheduleRepeats.style.display != "none")
	{
		setInvisibleIfIdMatches(rulePrefix + "schedule_repeats", "daily", rulePrefix + "days_and_hours_active_container", "block", controlDocument);
		setInvisibleIfIdMatches(rulePrefix + "schedule_repeats", "weekly", rulePrefix + "days_active", "block", controlDocument);
		setInvisibleIfIdMatches(rulePrefix + "schedule_repeats", "weekly", rulePrefix + "hours_active_container", "block", controlDocument);
	}


	setInvisibleIfIdMatches(rulePrefix + "applies_to", "all", rulePrefix + "applies_to_container", "block", controlDocument);
	setInvisibleIfIdMatches(rulePrefix + "remote_ip_type", "all", rulePrefix + "remote_ip_container", "block", controlDocument);
	setInvisibleIfIdMatches(rulePrefix + "remote_port_type", "all", rulePrefix + "remote_port", "inline", controlDocument);
	setInvisibleIfIdMatches(rulePrefix + "local_port_type", "all", rulePrefix + "local_port", "inline", controlDocument);
	setInvisibleIfIdMatches(rulePrefix + "app_protocol_type", "all", rulePrefix + "app_protocol", "inline", controlDocument);
	setInvisibleIfIdMatches(rulePrefix + "url_type", "all", rulePrefix + "url_match_list", "block", controlDocument);
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
	defaultDisplayMode = defaultDisplayMode == null ? "restriction_rule" : defaultDisplayMode;
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

function createEditButton(enabled, ruleType, rulePrefix)
{
	editButton = createInput("button");
	editButton.value = UI.Edit;
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

	uci.set(pkg, enabledId, "enabled", enabled);
}
function removeRuleCallback(table, row)
{
	var ruleId = row.childNodes[1].firstChild.id;
	uci.removeSection(pkg, ruleId);
}

function editRule()
{
	editRow = this.parentNode.parentNode;
	editTable = editRow.parentNode.parentNode;
	if(editTable.id.match("rule_"))
	{
		editRuleType = "restriction_rule";
		editRulePrefix = "rule_";
	}
	else
	{
		editRuleType = "whitelist_rule";
		editRulePrefix = "exception_";
	}

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


	editRuleWindow = window.open(editRuleType == "restriction_rule" ? "restriction_edit_rule.sh" : "whitelist_edit_rule.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editRuleWindow.document);
	closeButton = createInput("button", editRuleWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
	closeButton.className = "default_button";

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
		
				setDocumentFromUci(editRuleWindow.document, uci, editRuleSectionId, editRuleType, editRulePrefix);
				setVisibility(editRuleWindow.document, editRulePrefix);

				closeButton.onclick = function()
				{
					editRuleWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = validateRule(editRuleWindow.document, editRulePrefix);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n"+restStr.ARErr);
					}
					else
					{
						setUciFromDocument(editRuleWindow.document, editRuleSectionId, editRuleType, editRulePrefix);
						if(uci.get(pkg, editRuleSectionId, "description") != "")
						{
							editRow.childNodes[0].firstChild.data = uci.get(pkg, editRuleSectionId, "description");
						}
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

function addAddressesToTable(controlDocument, textId, tableContainerId, tableId, macsValid)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var newAddrs = controlDocument.getElementById(textId).value;
	var valid = macsValid ?  validateMultipleIpsOrMacs(newAddrs) : validateMultipleIps(newAddrs);
	if(valid == 0)
	{
		var tableContainer = controlDocument.getElementById(tableContainerId);
		var table = tableContainer.childNodes.length > 0 ? tableContainer.firstChild : createTable([""], [], tableId, true, false);
		newAddrs = newAddrs.replace(/^[\t ]*/, "");
		newAddrs = newAddrs.replace(/[\t ]*$/, "");
		var addrs = newAddrs.split(/[\t ]*,[\t ]*/);
		
		while(addrs.length > 0)
		{
			addTableRow(table, [ addrs.shift() ], true, false);
		}
		
		if(tableContainer.childNodes.length == 0)
		{
			tableContainer.appendChild(table);
		}
		controlDocument.getElementById(textId).value = "";
	}
	else
	{
		alert(restStr.IAErr+"\n");
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
		var table = tableContainer.childNodes.length > 0 ? tableContainer.firstChild : createTable([restStr.UPrt, restStr.MTyp, restStr.MTExp], [], tableId, true, false);
		addTableRow(table, [(urlType.match("domain") ? "domain" : "full"), urlType.substring(urlType.lastIndexOf("_")+1), urlSpan ], true, false);
		if(tableContainer.childNodes.length == 0)
		{
			tableContainer.appendChild(table);
		}
		controlDocument.getElementById(textId).value = "";
	}
	else
	{
		if( newUrl.length == 0)
		{
			alert(restStr.UMZErr);
		}
		else
		{
			alert(restStr.UChErr+"\n");
		}
	}
}

function validateRule(controlDocument, rulePrefix)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var inputIds = [rulePrefix + "hours_active", rulePrefix + "days_and_hours_active", rulePrefix + "remote_port", rulePrefix + "local_port"];
	var labelIds = [rulePrefix + "hours_active_label", rulePrefix + "days_and_hours_active_label", rulePrefix + "remote_port_label", rulePrefix + "local_port_label"];
	var functions = [validateHours, validateWeeklyRange, validateMultiplePorts, validateMultiplePorts];
	var validReturnCodes = [0,0,0,0];
	var visibilityIds = [rulePrefix + "hours_active_container", rulePrefix + "days_and_hours_active_container", rulePrefix + "remote_port", rulePrefix + "local_port"];
	if(controlDocument.getElementById(rulePrefix + "all_access").checked)
	{
		visibilityIds[2] = rulePrefix + "resources";
		visibilityIds[3] = rulePrefix + "resources";
	}
	
	return proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );
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
function proofreadMultipleIpsOrMacs(input)
{
	proofreadText(input, validateMultipleIpsOrMacs, 0);
}
function validateMultipleIpsOrMacs(addresses)
{
	var addr = addresses.replace(/^[\t ]+/g, "");
	addr = addr.replace(/[\t ]+$/g, "");
	var splitAddr = addr.split(/[\t ]*,[\t ]*/);
	var valid = splitAddr.length > 0 ? 0 : 1;
	while(valid == 0 && splitAddr.length > 0)
	{
		var nextAddr = splitAddr.pop();
		if(nextAddr.match(/-/))
		{
			var nextSplit = nextAddr.split(/[\t ]*-[\t ]*/);
			valid = nextSplit.length==2 && validateIP(nextSplit[0]) == 0 && validateIP(nextSplit[1]) == 0 ? 0 : 1;
		}
		else if(nextAddr.match(/:/))
		{
			valid = validateMac(nextAddr);
		}
		else
		{
			valid = validateIpRange(nextAddr);
		}
	}
	return valid;

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
	var valid = url.match(/[\n\r\"\']/) || url.length == 0 ? 1 : 0;
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


function setDocumentFromUci(controlDocument, sourceUci, sectionId, ruleType, rulePrefix)
{
	controlDocument = controlDocument == null ? document : controlDocument;

	var description = sourceUci.get(pkg, sectionId, "description");
	description = description == "" ? sectionId : description;	
	controlDocument.getElementById(rulePrefix + "name").value = description;

	setIpTableAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "local_addr", rulePrefix + "applies_to_table_container", rulePrefix + "applies_to_table", rulePrefix + "applies_to", rulePrefix + "applies_to_addr");


	var daysAndHours = sourceUci.get(pkg, sectionId, "active_weekly_ranges");
	var hours = sourceUci.get(pkg, sectionId, "active_hours");
	var allDay = (daysAndHours == "" && hours == "");
	controlDocument.getElementById(rulePrefix + "hours_active").value = hours;
	controlDocument.getElementById(rulePrefix + "all_day").checked = allDay;
	controlDocument.getElementById(rulePrefix + "days_and_hours_active").value = weekly_i18n(daysAndHours,"uci");
	setSelectedValue(rulePrefix + "schedule_repeats", (daysAndHours == "" ? "daily" : "weekly"), controlDocument);

	var allDays = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
	var dayStr =  sourceUci.get(pkg, sectionId, "active_weekdays");
	var days = [];
	if(dayStr == "")
	{
		days = allDays;
	}
	else
	{
		days = dayStr.split(/,/);
	}
	
	var everyDay = (daysAndHours == "");
	var dayIndex=0;
	for(dayIndex = 0; dayIndex < allDays.length; dayIndex++)
	{
		var nextDay = allDays[dayIndex];
		var dayFound = false;
		var testIndex=0;
		for(testIndex=0; testIndex < days.length && !dayFound; testIndex++)
		{
			dayFound = days[testIndex] == nextDay;
		}
		everyDay = everyDay && dayFound;
		controlDocument.getElementById(rulePrefix + allDays[dayIndex]).checked = dayFound;
	}
	controlDocument.getElementById(rulePrefix + "every_day").checked = everyDay;


	setIpTableAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "remote_addr", rulePrefix + "remote_ip_table_container", rulePrefix + "remote_ip_table", rulePrefix + "remote_ip_type", rulePrefix + "remote_ip");
	setTextAndSelectFromUci(controlDocument, sourceUci,  pkg, sectionId, "remote_port", rulePrefix + "remote_port", rulePrefix + "remote_port_type");
	setTextAndSelectFromUci(controlDocument, sourceUci, pkg, sectionId, "local_port", rulePrefix + "local_port", rulePrefix + "local_port_type");

	var proto = sourceUci.get(pkg, sectionId, "proto");
	proto = proto != "tcp" && proto != "udp" ? "both" : proto;
	setSelectedValue(rulePrefix + "transport_protocol", proto, controlDocument);

	var app_proto = sourceUci.get(pkg, sectionId, "app_proto");
	var app_proto_type = app_proto == "" ? "except" : "only";
	app_proto = app_proto == "" ? sourceUci.get(pkg, sectionId, "not_app_proto") : app_proto;
	app_proto_type = app_proto == "" ? "all" : app_proto_type;
	setSelectedValue(rulePrefix + "app_protocol_type", app_proto_type, controlDocument);
	setSelectedValue(rulePrefix + "app_protocol", app_proto, controlDocument);
	




	var urlTypes = [ "url_contains", "url_regex", "url_exact", "url_domain_contains", "url_domain_regex", "url_domain_exact" ];
	var urlExprTypes = [ "contains", "regex", "exact", "contains", "regex", "exact" ];
	var urlPartTypes = [ "full", "full", "full", "domain", "domain", "domain" ];
	var urlPrefix = "";
	var urlDefinitions = [];
	var urlDefFound = false;
	var urlTypeIndex = 0;
	var urlMatchType = "all";
	for(urlTypeIndex=0; urlTypeIndex < urlTypes.length; urlTypeIndex++)
	{
		urlDefinitions[urlTypeIndex] = sourceUci.get(pkg, sectionId, urlTypes[urlTypeIndex]);
		urlDefFound = urlDefinitions[urlTypeIndex] != "" ? true : urlDefFound;
		urlMatchType = urlDefinitions[urlTypeIndex] != "" ? "only" : urlMatchType;
	}
	if(!urlDefFound)
	{
		urlPrefix = "not_";
		for(urlTypeIndex=0; urlTypeIndex < urlTypes.length; urlTypeIndex++)
		{
			urlDefinitions[urlTypeIndex] = sourceUci.get(pkg, sectionId, urlPrefix + urlTypes[urlTypeIndex]);
			urlDefFound = urlDefinitions[urlTypeIndex] != "" ? true : urlDefFound;
			urlMatchType = urlDefinitions[urlTypeIndex] != "" ? "except" : urlMatchType;
		}
	}
	setSelectedValue(rulePrefix + "url_type", urlMatchType, controlDocument);
	
	var urlTableContainer = controlDocument.getElementById(rulePrefix + "url_match_table_container");
	if(urlTableContainer.childNodes.length > 0)
	{
		urlTableContainer.removeChild(urlTableContainer.firstChild);
	}


	if(urlDefFound)
	{
		var table = createTable([restStr.UPrt, restStr.MTyp, restStr.MTExp], [], rulePrefix + "url_match_table", true, false, null, null, controlDocument);
		for(urlTypeIndex=0; urlTypeIndex < urlTypes.length; urlTypeIndex++)
		{
			var defStr = urlDefinitions[urlTypeIndex];
			if(defStr != "")
			{
				defStr = defStr.replace(/^[\t ]*\"/, "");
				defStr = defStr.replace(/\"[\t ]*$/, "");
				def = defStr.match(/\".*\"/) ? defStr.split(/\"[\t, ]*\"/) : [ defStr ];
				var defIndex=0;
				for(defIndex=0; defIndex < def.length; defIndex++)
				{
					addTableRow(table, [ urlPartTypes[urlTypeIndex], urlExprTypes[urlTypeIndex], createUrlSpan(def[defIndex], controlDocument) ], true, false, null, null, controlDocument);
				}
			}
		}
		urlTableContainer.appendChild(table);
	}


	controlDocument.getElementById(rulePrefix + "url_match").value = "";
	
	var allResourcesBlocked = true;
	var resourceTypeIds = ["remote_ip_type", "remote_port_type", "local_port_type", "transport_protocol", "app_protocol_type", "url_type" ];
	for(typeIndex=0; typeIndex < resourceTypeIds.length; typeIndex++)
	{
		var type = getSelectedValue(rulePrefix + resourceTypeIds[typeIndex], controlDocument);
		allResourcesBlocked = allResourcesBlocked && (type == "all" || type == "both");
	}
	controlDocument.getElementById(rulePrefix + "all_access").checked = allResourcesBlocked;

	setVisibility(controlDocument, rulePrefix);
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


	var tableContainer = controlDocument.getElementById(tableContainerId);
	if(tableContainer.childNodes.length > 0)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}	
	if(optionValue != "")
	{
		optionValue = optionValue.replace(/^[\t ]*/, "");
		optionValue = optionValue.replace(/[\t ]*$/, "");
		var ips = optionValue.split(/[\t ]*,[\t ]*/);


		var table = createTable([""], [], tableId, true, false, null, null, controlDocument);
		while(ips.length > 0)
		{
			addTableRow(table, [ ips.shift() ], true, false, null, null, controlDocument);
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


function setUciFromDocument(controlDocument, sectionId, ruleType, rulePrefix)
{
	// note: we assume error checking has already been done 
	uci.removeSection(pkg, sectionId);
	uci.set(pkg, sectionId, "", ruleType);
	uci.set(pkg, sectionId, "is_ingress", "0");


	controlDocument = controlDocument == null ? document : controlDocument;
	
	uci.set(pkg, sectionId, "", ruleType);
	uci.set(pkg, sectionId, "description", controlDocument.getElementById(rulePrefix + "name").value);
	
	setFromIpTable(controlDocument, pkg, sectionId, "local_addr", rulePrefix + "applies_to_table_container", rulePrefix + "applies_to");

	var daysActive = controlDocument.getElementById(rulePrefix + "days_active");
	if(daysActive.style.display != "none")
	{
		var daysActive = [];
		var dayIds = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
		
		for(dayIndex =0; dayIndex < dayIds.length; dayIndex++)
		{
			if(controlDocument.getElementById(rulePrefix + dayIds[dayIndex]).checked)
			{
				daysActive.push(dayIds[dayIndex]);
			}
		}
		daysActiveStr = daysActive.join(",");
		uci.set(pkg, sectionId, "active_weekdays", daysActiveStr);
	}
	setIfVisible(controlDocument, pkg, sectionId, "active_hours",         rulePrefix + "hours_active",          rulePrefix + "hours_active_container" );
	var weekly_ranges=controlDocument.getElementById(rulePrefix + "days_and_hours_active");
	weekly_ranges.value=weekly_i18n(weekly_ranges.value, "table");
	setIfVisible(controlDocument, pkg, sectionId, "active_weekly_ranges", rulePrefix + "days_and_hours_active", rulePrefix + "days_and_hours_active_container");

	if(!controlDocument.getElementById(rulePrefix + "all_access").checked)
	{
		setFromIpTable(controlDocument, pkg, sectionId, "remote_addr", rulePrefix + "remote_ip_table_container", rulePrefix + "remote_ip_type");
		setIfVisible(controlDocument, pkg, sectionId, "remote_port", rulePrefix + "remote_port", rulePrefix + "remote_port", rulePrefix + "remote_port_type");
		setIfVisible(controlDocument, pkg, sectionId, "local_port",  rulePrefix + "local_port",  rulePrefix + "local_port",  rulePrefix + "local_port_type");
		
		uci.set(pkg, sectionId, "proto", getSelectedValue(rulePrefix + "transport_protocol", controlDocument));

		var appProtocolType = getSelectedValue(rulePrefix + "app_protocol_type", controlDocument);
		if(appProtocolType != "all")
		{
			var prefix = appProtocolType == "except" ? "not_" : "";
			uci.set(pkg, sectionId, prefix + "app_proto", getSelectedValue(rulePrefix + "app_protocol", controlDocument));
		}




		var urlMatchType = getSelectedValue(rulePrefix + "url_type", controlDocument);
		var urlTable = controlDocument.getElementById(rulePrefix + "url_match_table_container").firstChild;
		if(urlMatchType != "all" && urlTable != null)
		{
			var urlData = getTableDataArray(urlTable, true, false);
			var urlPrefix = urlMatchType == "except" ? "not_" : "";
			var urlDefStrings = [];
			var urlIndex;
			for(urlIndex = 0; urlIndex < urlData.length; urlIndex++)
			{
				var urlId = urlData[urlIndex][0];
				urlId = (urlId.match("domain") ? "url_domain_" : "url_") + urlData[urlIndex][1];
				urlStr = parseUrlSpan(urlData[urlIndex][2]);
				if(urlDefStrings[urlId] != null)
				{
					urlDefStrings[urlId] = urlDefStrings[urlId] + ",\"" + urlStr + "\"";
				}
				else
				{
					urlDefStrings[urlId] = "\"" + urlStr + "\""
				}
			}

			var parts = ["url_", "url_domain_"];
			var exprs = ["exact", "contains", "regex"];
			var partIndex=0;
			var exprIndex=0;
			for(partIndex=0; partIndex < 2; partIndex++)
			{
				for(exprIndex=0; exprIndex < 3; exprIndex++)
				{
					var id = parts[partIndex] + exprs[exprIndex];
					if(urlDefStrings[id] != null)
					{
						uci.set(pkg, sectionId, urlPrefix + id, urlDefStrings[id]);
					}
				}
			}

		}
	}
}



function setIfVisible(controlDocument, pkg, sectionId, optionId, textId, visId, prefixSelectId)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	visElement = controlDocument.getElementById(visId);
	if(visElement.style.display != "none")
	{
		var textElement = controlDocument.getElementById(textId);
		if(prefixSelectId != null)
		{
			prefixValue = getSelectedValue(prefixSelectId, controlDocument);
			optionId = prefixValue == "except" ? "not_" + optionId : optionId;
		}
		uci.set(pkg, sectionId, optionId, textElement.value);
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

function weekly_i18n(weekly_schd, source) { //this is part of i18n; TODO: best to have an uci get language to see if absent to just return daystrings
	if (weekly_schd.length < 6) return weekly_schd;
	var localdays=[UI.Sun, UI.Mon, UI.Tue, UI.Wed, UI.Thu, UI.Fri, UI.Sat];
	var fwdays=["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
	var indays, outdays, splits, idx;
	var joiner=[];

	if (source == "uci") {
		indays=fwdays;
		outdays=localdays;
	} else { // from the browser
		indays=localdays;
		outdays=fwdays;
	}

	splits=weekly_schd.split(" ");
	for (idx=0; idx < splits.length; idx++) {
		var pos= indays.indexOf(splits[idx]);
		if (pos >= 0) {
			joiner[idx]=outdays[pos];
		} else {
			joiner[idx]=splits[idx];
		}
	}
	return joiner.join(" ");
}
