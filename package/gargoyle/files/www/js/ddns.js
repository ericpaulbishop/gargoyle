/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


var serviceProviders;
var uci;
var newSections;
var updatedSections;

function saveChanges()
{
	setControlsEnabled(false, true);
	
	//completely re-build config data
	deleteCommands = [];
	sections = uciOriginal.getAllSections("ddns_gargoyle");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		deleteCommands.push("uci del ddns_gargoyle." + sections[sectionIndex]);
	}
	deleteCommands.push("uci commit");

	

		
	createCommands = uci.getScriptCommands(new UCIContainer());
	testCommands = ["/etc/init.d/ddns_gargoyle stop", "/etc/init.d/ddns_gargoyle test_config"];


	
	commands =  deleteCommands.join("\n") + "\n" + createCommands + "\n" + testCommands.join("\n");
	//document.getElementById("output").value = commands;
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			saveChangesPart2(req.responseText);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);	

}


function saveChangesPart2(response)
{
	responseLines=response.split(/[\r\n]+/);
	names=responseLines[0].split(/[\t ]+/);
	success=responseLines[1].split(/[\t ]+/);

	
	newFailedDomains = [];
	deleteCommands = [];
	
	//if config didn't change success.length == 0, and we won't run this loop
	for(nameIndex=0; nameIndex < names.length && names.length == success.length; nameIndex++)
	{
		if(success[nameIndex] == "0")
		{
			found=false;
			failedName = names[nameIndex];
			for(newIndex=0; newIndex < newSections.length && !found; newIndex++)
			{
				if(failedName == newSections[newIndex])
				{
					found = true;
					failedDomain = uci.get("ddns_gargoyle", failedName, "domain");
					newFailedDomains.push(failedDomain);
					deleteCommands.push("uci del ddns_gargoyle." + failedName);
					uci.removeSection("ddns_gargoyle", failedName);
				}
			}
		}
	}
	deleteCommands.push("uci commit");
	
	if(newFailedDomains.length > 0)
	{
		alert("Update of new dynamic DNS service configuration(s) failed:\n" + newFailedDomains.join("\n") + "\n\nService(s) could not be updated properly and have therefore been removed.");
	}
	
	getUpdateTimeCommands = [];
	sections = uci.getAllSections("ddns_gargoyle");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		getUpdateTimeCommands.push("echo " + sections[sectionIndex] + " $(cat /var/last_ddns_updates/" + sections[sectionIndex] + ")" );
	}

	commands =  deleteCommands.join("\n") + "\n" + "/etc/init.d/ddns_gargoyle enable\n" + "/etc/init.d/ddns_gargoyle restart\n" + getUpdateTimeCommands.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
;

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			responseLines=req.responseText.split(/[\r\n]+/);
			for(responseIndex=0; responseIndex < responseLines.length-1; responseIndex++)
			{
				lineParts=responseLines[responseIndex].split(/[\t ]+/);
				updateTimes[ lineParts[0] ] = lineParts[1];	
			}

			uciOriginal = uci.clone();
			resetData();
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function resetData()
{
	//set description visibility
	initializeDescriptionVisibility(uciOriginal, "ddns_1");
	uciOriginal.removeSection("gargoyle", "help"); //necessary, or we over-write the help settings when we save

	// setup providers in add section
	removeAllOptionsFromSelectElement(document.getElementById("ddns_provider"));
	serviceProviders=parseProviderData();
	for(providerIndex=0; providerIndex < serviceProviders.length; providerIndex++)
	{
		provider = serviceProviders[providerIndex];
		addOptionToSelectElement("ddns_provider", provider["name"], provider["name"]);
	}
	
	setDocumentFromUci( new UCIContainer(), "", "", document);

	
	// setup table of existing domains configured for ddns
	uci = uciOriginal.clone();
	newSections = [];
	updatedSections = [];
	
	sections = uci.getAllSections("ddns_gargoyle");
	columnNames=["Domain", "Last Update", "Enabled", "", "" ];
	ddnsTableData = new Array();
	ddnsEnabledData = new Array();
	for (sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		section = sections[sectionIndex];
		domain = uciOriginal.get("ddns_gargoyle", section, "domain");
		
		last_date = new Date();
		if(updateTimes[section] != null)
		{
			last_date.setTime(1000*updateTimes[section]);
		}
		
		seconds = last_date.getSeconds() < 10 ? "0" + last_date.getSeconds() : last_date.getSeconds();
		minutes = last_date.getMinutes() < 10 ? "0" + last_date.getMinutes() : last_date.getMinutes();
		lastUpdate = updateTimes[section] == null ? "Never" : (last_date.getMonth()*1+1*1) + "/" + last_date.getDate() + " " + last_date.getHours() + ":" + minutes + ":" + seconds;
		

		enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = uciOriginal.get("ddns_gargoyle", section, "enabled") == "1" ? true : false;
		ddnsTableData.push( [domain, lastUpdate, enabledCheckbox, createEditButton(), createForceUpdateButton()]);
		ddnsTableData[ ddnsTableData.length-1][4].disabled = enabledCheckbox.checked ? false : true;
		ddnsTableData[ ddnsTableData.length-1][4].className = enabledCheckbox.checked ? "default_button" : "default_button_disabled" ;
		ddnsEnabledData.push(enabledCheckbox.checked);
 	}
	ddnsTable=createTable(columnNames, ddnsTableData, "ddns_table", true, false, removeServiceProviderCallback);
	tableContainer = document.getElementById('ddns_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(ddnsTable);

	// Because IE6 was designed by programmers whose only qualification was participation in the Special Olympics,
	// checkboxes become unchecked when added to table.  We need to reset checked status here.
	for(deIndex = 0; deIndex < ddnsEnabledData.length; deIndex++)
	{
		ddnsTableData[deIndex][2].checked = ddnsEnabledData[deIndex];
	}

}

function addDdnsService()
{
	
	errorList = proofreadServiceProvider(document);
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		//update uci (NOT uciOriginal)
		sections = uci.getAllSections("ddns_gargoyle");
		sectionNum = 1+sections.length;
		while( uci.get("ddns_gargoyle", "ddns_" + sectionNum) != '')
		{
			sectionNum++;
		}
		sectionName = "ddns_" + sectionNum;
		providerName = getSelectedValue("ddns_provider");
		
		setUciFromDocument(uci, "ddns_gargoyle", sectionName, document);
		
		domain = uci.get("ddns_gargoyle", sectionName, "domain");
		enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = true; 
		newTableRow =  [domain, "Never", enabledCheckbox, createEditButton(), createForceUpdateButton()];

		ddnsTable = document.getElementById('ddns_table_container').firstChild;
		addTableRow(ddnsTable, newTableRow, true, false, removeServiceProviderCallback);

		setDocumentFromUci( new UCIContainer(), "", "", document);

		newSections.push(sectionName);
		updatedSections.push(sectionName);
	}
}
function removeServiceProviderCallback(table, row)
{
	removedDomain=row.firstChild.firstChild.data;
	sections = uci.getAllSections("ddns_gargoyle");
	sectionName = "";
	for(sectionIndex = 0; sectionIndex < sections.length && sectionName == ""; sectionIndex++)
	{
		testDomain= uci.get("ddns_gargoyle", sections[sectionIndex], "domain");
		sectionName = testDomain == removedDomain ? sections[sectionIndex] : sectionName;
	}
	uci.removeSection("ddns_gargoyle", sectionName);
}

function proofreadServiceProvider(controlDocument)
{
	//there will always be domain variable as well as check interval & force interval
	//if there's a problem with other variables we'll learn when we try to save changes
	//and initial update test will fail
	//
	ddnsIds = ['domain', 'ddns_check', 'ddns_force'];
	labelIds= ['domain_label', 'ddns_check_label', 'ddns_force_label'];
	validateNotNull=function(text){ return validateLengthRange(text, 1, 999); };
	functions = [validateNotNull, validateNumeric, validateNumeric];
	returnCodes = [0,0,0];
	errors = proofreadFields(ddnsIds, labelIds, functions, returnCodes, ddnsIds, controlDocument);


	//we don't have a proofread function on domain input element
	//so we need to make sure class is set to default (not error)
	controlDocument.getElementById("domain").className="";


	//verify domain name is not duplicate
	if(errors.length == 0)
	{
		domain = controlDocument.getElementById("domain").value;
		tableData = getTableDataArray(document.getElementById("ddns_table_container").firstChild, true, false);
		domainMatches = false;
		for(rowIndex = 0; rowIndex < tableData.length && domainMatches == false; rowIndex++)
		{
			domainMatches = tableData[rowIndex][0] == domain ? true : false;
		}
		if(domainMatches)
		{
			errors.push("Duplicate domain name.");
		}
	}

	return errors;
}


function setUciFromDocument(dstUci, pkg, section, controlDocument)
{
	if(controlDocument == null)
	{
		controlDocument = document;
	}

	var providerName;
	if(controlDocument.getElementById("ddns_provider_text") != null)
	{
		providerName = controlDocument.getElementById("ddns_provider_text").firstChild.data;
	}
	else
	{
		providerName = getSelectedValue("ddns_provider", controlDocument);
	}

	dstUci.removeSection("ddns_gargoyle", section); 
	dstUci.set("ddns_gargoyle", section, "", "service");
	dstUci.set("ddns_gargoyle", section, "enabled", "1");
	dstUci.set("ddns_gargoyle", section, "service_provider", providerName);
	dstUci.set("ddns_gargoyle", section, "ip_source", "internet");
	dstUci.set("ddns_gargoyle", section, "force_interval", controlDocument.getElementById("ddns_force").value  );
	dstUci.set("ddns_gargoyle", section, "force_unit", "days");
	dstUci.set("ddns_gargoyle", section, "check_interval", controlDocument.getElementById("ddns_check").value );
	dstUci.set("ddns_gargoyle", section, "check_unit", "minutes");
		
	var provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = serviceProviders[providerIndex]["name"] == providerName ? serviceProviders[providerIndex] : null;
	}
	if(provider == null)
	{
		alert("ERROR: specified provider is invalid"); //should never get here, but let's have an error message just in case
		return;
	}
	var variables=provider["variables"];
	var optionalVariables = provider["optional_variables"];
	var allBooleanVariables = [];
	var variableIndex=0;
	for(variableIndex=0; variableIndex < provider["boolean_variables"].length; variableIndex++)
	{
		allBooleanVariables[ provider["boolean_variables"][variableIndex] ] = 1;
	}

	for(variableIndex=0; variableIndex < variables.length; variableIndex++)
	{
		var el = controlDocument.getElementById(variables[variableIndex]);
		var value = allBooleanVariables[ el.id ] != 1 ? el.value : (el.checked ? "1" : "0");
		if(value != "")
		{
			dstUci.set("ddns_gargoyle", section, el.id, value);
		}
	}
	for(variableIndex=0; variableIndex < optionalVariables.length; variableIndex++)
	{
		var el = controlDocument.getElementById(variables[variableIndex]);
		if( allBooleanVariables[ el.id ] != 1)
		{
			if(controlDocument.getElementById( el.id + "_enabled").checked)
			{
				dstUci.set("ddns_gargoyle", section, el.id, el.value);
			}
		}
		else
		{
			dstUci.set("ddns_gargoyle", section, el.id, el.checked ? "1" : "0");
		}
	}
}


function setDocumentFromUci(srcUci, pkg, section, controlDocument)
{
	if(controlDocument == null)
	{
		controlDocument = document;
	}

	var providerName = srcUci.get(pkg, section, "service_provider");
	if(controlDocument.getElementById("ddns_provider_text") != null)
	{	
		controlDocument.getElementById("ddns_provider_text").appendChild( controlDocument.createTextNode(providerName));
	}
	else
	{
		setSelectedValue("ddns_provider", providerName, controlDocument);
	}
	var provider = setProvider(controlDocument);
	var variables = provider["variables"];
	var optionalVariables = provider["optional_variables"];
	var allBooleanVariables = [];
	var variableIndex=0;
	for(variableIndex=0; variableIndex < provider["boolean_variables"].length; variableIndex++)
	{
		allBooleanVariables[ provider["boolean_variables"][variableIndex] ] = 1;
	}
	for(variableIndex = 0; variableIndex < variables.length; variableIndex++)
	{
		var el = controlDocument.getElementById( variables[variableIndex] );
		if( allBooleanVariables[ el.id ] != 1)
		{
			el.value = srcUci.get(pkg, section, el.id);
		}
		else
		{
			el.checked = srcUci.get(pkg, section, el.id) == "1" ? true : false;
		}
	}
	for(variableIndex = 0; variableIndex < optionalVariables.length; variableIndex++)
	{
		var el = controlDocument.getElementById( optionalVariables[variableIndex] );
		if( allBooleanVariables[ el.id ] != 1)
		{
			var check = controlDocument.getElementById( optionalVariables[variableIndex] + "_enabled" );
			check.checked = srcUci.get(pkg, section, el.id) != "";
			if(check.checked)
			{
				el.value = srcUci.get(pkg, section, el.id);
			}
		}
		else
		{
			el.checked = srcUci.get(pkg, section, el.id) == "1" ? true : false;
		}
	}

	var checkMinutes = (getMultipleFromUnit( srcUci.get("ddns_gargoyle", section, "check_unit") ) * srcUci.get("ddns_gargoyle", section, "check_interval"))/(60);
	checkMinutes = (checkMinutes > 0) ? checkMinutes : 15;
	controlDocument.getElementById( "ddns_check" ).value = checkMinutes;


	var forceDays = (getMultipleFromUnit( srcUci.get("ddns_gargoyle", section, "force_unit") ) * srcUci.get("ddns_gargoyle", section, "force_interval"))/(24*60*60);
	forceDays = (forceDays > 0) ? forceDays : 3;
	controlDocument.getElementById( "ddns_force" ).value = forceDays;
}

function setProvider(controlDocument)
{
	if(controlDocument == null)
	{
		controlDocument = document;
	}

	var selected;
	if(controlDocument.getElementById("ddns_provider_text") != null)
	{
		selected = controlDocument.getElementById("ddns_provider_text").firstChild.data;
	}
	else
	{
		selected = getSelectedValue("ddns_provider", controlDocument);
	}
	var provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = serviceProviders[providerIndex]["name"] == selected ? serviceProviders[providerIndex] : null;
	}
	if(provider != null) //should NEVER be null, but test just in case
	{
		var variables = provider["variables"];
		var variableNames = provider["variable_names"];
		var newElements = new Array();
		
		var allBooleanVariables = [];
		var variableIndex=0;
		for(variableIndex=0; variableIndex < provider["boolean_variables"].length; variableIndex++)
		{
			allBooleanVariables[ provider["boolean_variables"][variableIndex] ] = 1;
		}

		for(variableIndex = 0; variableIndex < variables.length; variableIndex++)
		{
			var div= controlDocument.createElement("div");
			div.className="indent";
			
			var label = controlDocument.createElement("label");
			label.className="leftcolumn";
			label.id=variables[variableIndex] + "_label";
			label.appendChild( controlDocument.createTextNode(variableNames[variableIndex] + ":" ));
			div.appendChild(label);
			
			var input;
			if(allBooleanVariables[ variables[variableIndex] ] != 1)
			{
				input = createInput("text", controlDocument);
			}
			else
			{
				input = createInput("checkbox", controlDocument);
			}
			input.className = "rightcolumn";
			input.id = variables[variableIndex];
			div.appendChild(input);
			newElements.push(div);
		}

		var optionalVariables = provider["optional_variables"];
		var optionalVariableNames = provider["optional_variable_names"];
		for(variableIndex = 0; variableIndex < optionalVariables.length; variableIndex++)
		{
			var div= controlDocument.createElement("div");
			div.className="indent";
			
			var label = controlDocument.createElement("label");
			label.className="leftcolumn";
			label.id=optionalVariables[variableIndex] + "_label";
			label.appendChild( controlDocument.createTextNode(optionalVariableNames[variableIndex] + ":" ));
			div.appendChild(label);
			if(allBooleanVariables[ optionalVariables[variableIndex] ] != 1)
			{
				var span = controlDocument.createElement("span");
				span.className = "rightcolumn";
				
				var check = createInput("checkbox", controlDocument);
				check.style.position="relative";
				check.style.top="3px";
				var text  = createInput("text", controlDocument);
				check.id = optionalVariables[variableIndex] + "_enabled";
				text.id  = optionalVariables[variableIndex];
				check.onclick= function() 
				{
					var textId = this.id.replace("_enabled", "");
			     		setElementEnabled( controlDocument.getElementById(textId), this.checked, "");
				}
				span.appendChild(check);
				span.appendChild(text);
				div.appendChild(span);
			}
			else
			{
				var input = createInput("checkbox", controlDocument);
				input.className = "rightcolumn";
				input.id = optionalVariables[variableIndex];
				div.appendChild(input);
			}
			newElements.push(div);
		}

		
		container = controlDocument.getElementById("ddns_variable_container");
		while(container.childNodes.length > 0)
		{
			container.removeChild( container.firstChild );
		}
		for(newElementIndex = 0; newElementIndex < newElements.length; newElementIndex++)
		{
			container.appendChild(newElements[newElementIndex]);
		}
	}
	return provider;
}

function createEnabledCheckbox()
{
	enabledCheckbox = createInput('checkbox');
	enabledCheckbox.onclick = setRowEnabled;
	return enabledCheckbox;
}


function createEditButton()
{
	editButton = createInput("button");
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editServiceTableRow;
	return editButton;
}

function createForceUpdateButton()
{
	updateButton = createInput("button");
	updateButton.value = "Force Update";
	updateButton.className="default_button";
	updateButton.onclick = forceUpdateForRow;
	return updateButton;
}

function setRowEnabled()
{
	enabled= this.checked ? "1" : "0";
	enabledRow=this.parentNode.parentNode;
	enabledDomain = enabledRow.firstChild.firstChild.data;

	enabledRow.childNodes[4].firstChild.disabled   = this.checked ? false : true;
	enabledRow.childNodes[4].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

	sections = uci.getAllSections("ddns_gargoyle");
	sectionName = ""
	for(sectionIndex = 0; sectionIndex < sections.length && sectionName == ""; sectionIndex++)
	{
		testDomain = uci.get("ddns_gargoyle", sections[sectionIndex], "domain");
		sectionName = enabledDomain == testDomain ? sections[sectionIndex] : "";
	}
	uci.set("ddns_gargoyle", sectionName, "enabled", enabled);
	updatedSections.push(sectionName);
}


function parseProviderData()
{
	providers = new Array();
	for(providerLineIndex=0; providerLineIndex < providerData.length; providerLineIndex++)
	{
		line = providerData[providerLineIndex];
		if(line.match(/^[\t ]*service[\t ]+/))
		{
			var provider = new Array();
			var splitService = line.split(/ervice[\t ]+/);
			provider["name"] = splitService[1];

			provider["optional_variables"] = [];
			provider["optional_variable_names"] = [];
			provider["boolean_variables"] = [];

			line = "";
			providerLineIndex++;
			while(providerLineIndex < providerData.length && line.match(/^[\t ]*service[\t ]+/) == null)
			{
				line = providerData[providerLineIndex];
				if(line.match(/^[\t ]*required_variables[\t ]+/))
				{
					variablePart = (line.match(/ariables[\t ]+(.*)$/))[1];
					if(variablePart.match(/domain/)) //domain variable MUST exist to display properly
					{
						provider["variables"]=variablePart.split(/[\t ]+/);
					}
				}
				else if(line.match(/^[\t ]*optional_variables[\t ]+/))
				{
					variablePart = (line.match(/ariables[\t ]+(.*)$/))[1];
					provider["optional_variables"]=variablePart.split(/[\t ]+/);
				}
				else if(line.match(/^[\t ]*required_variable_names[\t ]+/))
				{
					variablePart = (line.match(/ariable_names[\t ]+(.*)$/))[1];
					provider["variable_names"] = variablePart.split(/,/);
				}
				else if(line.match(/^[\t ]*optional_variable_names[\t ]+/))
				{
					variablePart = (line.match(/ariable_names[\t ]+(.*)$/))[1];
					provider["optional_variable_names"] = variablePart.split(/,/);
				}
				else if(line.match(/^[\t ]*boolean_variables[\t ]+/))
				{
					variablePart = (line.match(/ariables[\t ]+(.*)$/))[1];
					provider["boolean_variables"] = variablePart.split(/[\t ]+/);
				}
				providerLineIndex++;
			}
			if(provider["name"] != null && provider["variables"] != null && provider["variable_names"] != null)
			{
				providers.push(provider);
			}
			if(line.match(/^[\t ]*service[\t ]+/))
			{
				providerLineIndex = providerLineIndex-2;
			}
		}
	}
	return providers;
}

function forceUpdateForRow()
{
	updateRow=this.parentNode.parentNode;
	sections = uci.getAllSections("ddns_gargoyle");
	updateDomain = updateRow.firstChild.firstChild.data;

	sectionName = ""
	for(sectionIndex = 0; sectionIndex < sections.length && sectionName == ""; sectionIndex++)
	{
		testDomain = uci.get("ddns_gargoyle", sections[sectionIndex], "domain");
		sectionName = testDomain == updateDomain ? sections[sectionIndex] : "";
	}

	needsUpdate=false;
	for(updatedIndex=0; updatedIndex < updatedSections.length && !needsUpdate; updatedIndex++)
	{
		needsUpdate = sectionName == updatedSections[updatedIndex];
	}
	if(needsUpdate) //should check newSections instead (implement later)
	{
		alert("This service has been added/modified and therefore you must save your changes before an update can be performed.  Click \"Save Changes\" and try again.");
	}
	else
	{
		setControlsEnabled(false, true);
		commands = "/usr/bin/ddns_gargoyle -P /etc/ddns_providers.conf -C /etc/ddns_gargoyle.conf -m -f " + sectionName;
		commands = commands + "\n" + "echo $(cat /var/last_ddns_updates/" + sectionName + ") ";
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));


		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				responseLines=req.responseText.split(/[\r\n]+/);
				setControlsEnabled(true);
				if(responseLines[0] == "0")
				{
					alert("Update failed.  Ensure your configuration is valid and that you are connected to the internet.");
				}
				else
				{
					alert("Update successful.");
					updateTimes[sectionName] = responseLines[1];
					resetData();
				}
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	
	}
}


function editServiceTableRow()
{

	if( typeof(editServiceWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editServiceWindow.close();
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


	editServiceWindow = window.open("ddns_edit_service.sh", "edit", "width=560,height=500,left=" + xCoor + ",top=" + yCoor );
	
	saveButton = createInput("button", editServiceWindow.document);
	closeButton = createInput("button", editServiceWindow.document);
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";

	//load provider data for this row
	editServiceWindowRow=this.parentNode.parentNode;
	selectedDomain = editServiceWindowRow.firstChild.firstChild.data;
	sections = uci.getAllSections("ddns_gargoyle");
	sectionName = ""
	providerName = ""
	for(sectionIndex = 0; sectionIndex < sections.length; sectionIndex++)
	{
		testDomain = uci.get("ddns_gargoyle", sections[sectionIndex], "domain");
		if(selectedDomain == testDomain)
		{
			sectionName = sections[sectionIndex];
			providerName = uci.get("ddns_gargoyle", sectionName, "service_provider");
		}
	}
	provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = providerName == serviceProviders[providerIndex]["name"] ? serviceProviders[providerIndex] : null;
	}
	providerVariables = provider["variables"];
	providerVariableNames = provider["variable_names"];

	runOnServiceEditorLoaded = function () 
	{
		update_done=false;
		if(editServiceWindow.document != null)
		{
			if(editServiceWindow.document.getElementById("bottom_button_container") != null)
			{
				editServiceWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editServiceWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
			

				editServiceWindow.document.getElementById("ddns_provider_text").appendChild( editServiceWindow.document.createTextNode(providerName));
				setProvider(editServiceWindow.document);
				setDocumentFromUci(uci, "ddns_gargoyle", sectionName, editServiceWindow.document);
								
				
			
				closeButton.onclick = function()
				{
					editServiceWindow.close();
				}	
				
				saveButton.onclick = function()
				{
					errors = proofreadServiceProvider(editServiceWindow.document);
					if(errors.length == 1)
					{
						if(errors[0] == "Duplicate domain name." && editServiceWindow.document.getElementById("domain").value == selectedDomain)
						{
							errors = [];
						}
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n\nCould not update service class.");
						editServiceWindow.focus();
					}
					else
					{
						editServiceWindowRow.firstChild.firstChild.data = editServiceWindow.document.getElementById("domain").value;
						setUciFromDocument(uci, "ddns_gargoyle", sectionName, editServiceWindow.document);


						updatedSections.push(sectionName);
						editServiceWindow.close();

					}
				}
				
				editServiceWindow.moveTo(xCoor,yCoor);
				editServiceWindow.focus();
				update_done = true;
			}
		}
		if(update_done == false)
		{
			setTimeout( "runOnServiceEditorLoaded()", 250);
		}
	}
	
	runOnServiceEditorLoaded();
}

function getMultipleFromUnit(unit)
{
	multiple = 1;
	if(unit == "minutes")
	{
		multiple = 60;
	}
	else if(unit == "hours")
	{
		multiple = 60*60;
	}
	else if(unit == "days")
	{
		multiple = 24*60*60;
	}
	else if(unit == "weeks")
	{
		multiple = 7*24*60*60;
	}
	else
	{
		multiple = 1;
	}
	return multiple;
}

