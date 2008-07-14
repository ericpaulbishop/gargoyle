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

	document.body.style.cursor="wait";
	document.getElementById("save_button").style.display="none";
	document.getElementById("reset_button").style.display="none";
	document.getElementById("update_container").style.display="block";


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
	var param = getParameterDefinition("commands", commands);
	

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
	var param = getParameterDefinition("commands", commands);

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
			document.getElementById("update_container").style.display="none";		
			document.getElementById("save_button").style.display="inline";
			document.getElementById("reset_button").style.display="inline";
			document.body.style.cursor='auto';
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function resetData()
{
	// setup providers in add section
	removeAllOptionsFromSelectElement(document.getElementById("ddns_provider"));
	serviceProviders=parseProviderData();
	for(providerIndex=0; providerIndex < serviceProviders.length; providerIndex++)
	{
		provider = serviceProviders[providerIndex];
		addOptionToSelectElement("ddns_provider", provider["name"], provider["name"]);
	}
	
	setProvider();
	document.getElementById("ddns_check").value = "";
	document.getElementById("ddns_force").value = "";

	
	// setup table of existing providers
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
		
		
		uci.set("ddns_gargoyle", sectionName, "", "service");
		uci.set("ddns_gargoyle", sectionName, "enabled", "1");
		uci.set("ddns_gargoyle", sectionName, "service_provider", providerName);
		uci.set("ddns_gargoyle", sectionName, "ip_source", "internet");
		uci.set("ddns_gargoyle", sectionName, "force_interval", document.getElementById("ddns_force").value  );
		uci.set("ddns_gargoyle", sectionName, "force_unit", "days");
		uci.set("ddns_gargoyle", sectionName, "check_interval", document.getElementById("ddns_check").value );
		uci.set("ddns_gargoyle", sectionName, "check_unit", "minutes");
		
		provider = null;
		for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
		{
			provider = serviceProviders[providerIndex]["name"] == selected ? serviceProviders[providerIndex] : null;
		}
		if(provider == null)
		{
			alert("ERROR: specified provider is invalid"); //should never get here, but let's have an error message just in case
			return;
		}
		variables=provider["variables"];
		for(variableIndex=0; variableIndex < variables.length; variableIndex++)
		{
			variable = variables[variableIndex];
			value = document.getElementById(variable).value;
			if(value != "")
			{
				uci.set("ddns_gargoyle", sectionName, variable, value);
			}
		}

		
		domain = uci.get("ddns_gargoyle", sectionName, "domain");
		enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = true; 
		newTableRow =  [domain, "Never", enabledCheckbox, createEditButton(), createForceUpdateButton()];

		ddnsTable = document.getElementById('ddns_table_container').firstChild;
		addTableRow(ddnsTable, newTableRow, true, false, removeServiceProviderCallback);


		for(variableIndex=0; variableIndex < variables.length; variableIndex++)
		{
			document.getElementById(variables[variableIndex]).value = "";
		}
		document.getElementById("ddns_check").value = "";
		document.getElementById("ddns_force").value = "";

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


function setProvider()
{
	selected = getSelectedValue("ddns_provider")
	provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = serviceProviders[providerIndex]["name"] == selected ? serviceProviders[providerIndex] : null;
	}
	if(provider != null) //should NEVER be null, but test just in case
	{
		variables = provider["variables"];
		variableNames = provider["variable_names"];
		newElements = new Array();
		
		for(variableIndex = 0; variableIndex < variables.length; variableIndex++)
		{
			div= document.createElement("div");
			div.className="indent";
			
			label = document.createElement("label");
			label.className="leftcolumn";
			label.id=variables[variableIndex] + "_label";
			label.appendChild( document.createTextNode(variableNames[variableIndex] + ":" ));
			div.appendChild(label);
			
			input = createInput("text");
			input.className = "rightcolumn";
			input.id = variables[variableIndex];
			div.appendChild(input);
			
			newElements.push(div);
		}
		
		container = document.getElementById("ddns_variable_container");
		while(container.childNodes.length > 0)
		{
			container.removeChild( container.firstChild );
		}
		for(newElementIndex = 0; newElementIndex < newElements.length; newElementIndex++)
		{
			container.appendChild(newElements[newElementIndex]);
		}
	}
}

function createEnabledCheckbox()
{
	enabledCheckbox = createInput('checkbox');
	enabledCheckbox.onclick = setRowEnabled;
	return enabledCheckbox;
}


function createEditButton()
{
	editButton = createButton();
	editButton.value = "Edit";
	editButton.className="default_button";
	editButton.onclick = editServiceTableRow;
	return editButton;
}

function createForceUpdateButton()
{
	updateButton = createButton();
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

			line = "";
			providerLineIndex++;
			while(providerLineIndex < providerData.length && line.match(/^[\t ]*service[\t ]+/) == null)
			{
				line = providerData[providerLineIndex];
				if(line.match(/^[\t ]*variables[\t ]+/))
				{
					variablePart = (line.match(/ariables[\t ]+(.*)$/))[1];
					if(variablePart.match(/domain/)) //domain variable MUST exist to display properly
					{
						provider["variables"]=variablePart.split(/[\t ]+/);
					}
				}
				else if(line.match(/^[\t ]*variable_names[\t ]+/))
				{
					variablePart = (line.match(/ariable_names[\t ]+(.*)$/))[1];
					provider["variable_names"] = variablePart.split(/,/);
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
		document.body.style.cursor="wait";
		commands = "/usr/bin/ddns_gargoyle -P /etc/ddns_providers.conf -C /etc/ddns_gargoyle.conf -m -f " + sectionName;
		commands = commands + "\n" + "echo $(cat /var/last_ddns_updates/" + sectionName + ") ";
		var param = getParameterDefinition("commands", commands);

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				responseLines=req.responseText.split(/[\r\n]+/);
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
				document.body.style.cursor='auto';
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
			

				editServiceWindow.document.getElementById("ddns_provider").appendChild( editServiceWindow.document.createTextNode(providerName));
				variableContainer = editServiceWindow.document.getElementById("variable_container");
				for(variableIndex=0; variableIndex < providerVariables.length; variableIndex++)
				{
					div= editServiceWindow.document.createElement("div");
			
					label = editServiceWindow.document.createElement("label");
					label.className="leftcolumn";
					label.id=providerVariables[variableIndex] + "_label";
					label.appendChild( editServiceWindow.document.createTextNode(providerVariableNames[variableIndex] + ":" ));
					div.appendChild(label);
			
					input = createInput("text", editServiceWindow.document);
					input.className = "rightcolumn";
					input.id = providerVariables[variableIndex];
					div.appendChild(input);
					
					variableContainer.appendChild(div);
					editServiceWindow.document.getElementById( providerVariables[variableIndex]).value = uci.get("ddns_gargoyle", sectionName, providerVariables[variableIndex] );
				}
				
				checkMinutes = (getMultipleFromUnit( uci.get("ddns_gargoyle", sectionName, "check_unit") ) * uci.get("ddns_gargoyle", sectionName, "check_interval"))/(60);
				checkMinutes = (checkMinutes > 0) ? checkMinutes : 1;
				editServiceWindow.document.getElementById( "ddns_check" ).value = checkMinutes;


				forceDays = (getMultipleFromUnit( uci.get("ddns_gargoyle", sectionName, "force_unit") ) * uci.get("ddns_gargoyle", sectionName, "force_interval"))/(24*60*60);
				forceDays = (forceDays > 0) ? forceDays : 1;
				editServiceWindow.document.getElementById( "ddns_force" ).value = forceDays;
				
			
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
						for(variableIndex=0; variableIndex < providerVariables.length; variableIndex++)
						{
							value = editServiceWindow.document.getElementById(variables[variableIndex]).value;
							if(value != "")
							{
								uci.set("ddns_gargoyle", sectionName, variables[variableIndex], value);
							}
							else
							{
								uci.remove("ddns_gargoyle", sectionName, variables[variableIndex]);
							}
						}
						uci.set("ddns_gargoyle", sectionName, "check_interval", editServiceWindow.document.getElementById("ddns_check").value);
						uci.set("ddns_gargoyle", sectionName, "check_unit", "minutes");
						uci.set("ddns_gargoyle", sectionName, "force_interval", editServiceWindow.document.getElementById("ddns_force").value);
						uci.set("ddns_gargoyle", sectionName, "force_unit", "days");


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

