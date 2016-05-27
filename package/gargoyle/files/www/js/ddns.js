/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var DyDNS=new Object();

var serviceProviders;
var uci;
var newSections;
var updatedSections;
var resettingAfterFailedUpdate = false;

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
	resettingAfterFailedUpdate=false;

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
					resettingAfterFailedUpdate=true;
					// set parameters in form to that of failed section, so they can
					// be edited/corrected
					setDocumentFromUci(uci, "ddns_gargoyle", failedName, document);

					found = true;
					failedDomain = uci.get("ddns_gargoyle", failedName, "domain");
					failedDomain = failedDomain == "" ? uci.get("ddns_gargoyle", failedName, "service_provider") : failedDomain;
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
		alert(DyDNS.UpErr1+":\n" + newFailedDomains.join("\n") + "\n\n"+DyDNS.UpErr2);
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

	//initialize global variables
	uci = uciOriginal.clone();
	newSections = [];
	updatedSections = [];
	serviceProviders=parseProviderData();


	// setup providers in add section
	// also, make sure freedns.afraid.org is at top of list [ DO NOT CHANGE THIS! ]
	var providerNames = [];
	var providerIndex;
	var foundAfraid = false;
	for(providerIndex=0; providerIndex < serviceProviders.length; providerIndex++)
	{
		var p = serviceProviders[providerIndex]["name"]
		if(p != "freedns.afraid.org")
		{
			providerNames.push( serviceProviders[providerIndex]["name"] );
		}
		else
		{
			foundAfraid=true;
		}
	}

	providerNames.sort(function(a, b) {
		a = a.toLowerCase();
		b = b.toLowerCase();
		return a < b ? -1 : a > b ? 1 : 0
	});
	if(foundAfraid)
	{
		providerNames.unshift("freedns.afraid.org")
	}
	setAllowableSelections("ddns_provider", providerNames, providerNames, document);
	setSelectedValue("ddns_provider", 'freedns.afraid.org', document);


	if(!resettingAfterFailedUpdate)
	{
		setDocumentFromUci( new UCIContainer(), "", "", document);
	}
	resettingAfterFailedUpdate = false;
	
	// setup table of existing domains configured for ddns
	
	var sections = uci.getAllSections("ddns_gargoyle");
	var columnNames=DyDNS.cNams;
	var ddnsTableData = new Array();
	var ddnsEnabledData = new Array();
	for (sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		var section = sections[sectionIndex];
		var domain = uciOriginal.get("ddns_gargoyle", section, "domain");
		domain = domain == "" ? uciOriginal.get("ddns_gargoyle", section, "service_provider") : domain;
		domain = domain.length > 20 ? domain.substr(0,17) + "..." : domain;

		var lastDate = new Date();
		if(updateTimes[section] != null)
		{
			lastDate.setTime(1000*updateTimes[section]);
		}
		
		
		var systemDateFormat = uciOriginal.get("gargoyle",  "global", "dateformat");
		var twod = function(num) { var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; return nstr; }

		var m = twod(lastDate.getMonth()+1);
		var d = twod(lastDate.getDate());
		var h = " " + lastDate.getHours() + ":" +  twod(lastDate.getMinutes())  + ":" + twod(lastDate.getSeconds());
		var lastUpdate = (systemDateFormat == "" || systemDateFormat == "usa") ? m + "/" + d + h : d + "/" + m + h;
		lastUpdate = systemDateFormat == "russia" ? d + "." + m + h : lastUpdate;
		lastUpdate = systemDateFormat == "argentina" ? d + "/" + m + h : lastUpdate;
		lastUpdate = systemDateFormat == "iso8601" ? m + "-" + d + h : lastUpdate;
		lastUpdate =  updateTimes[section] == null ? UI.never : lastUpdate;



		var enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = uciOriginal.get("ddns_gargoyle", section, "enabled") == "1" ? true : false;
		enabledCheckbox.id = section;
		ddnsTableData.push( [domain, lastUpdate, enabledCheckbox, createEditButton(), createForceUpdateButton()]);
	
		ddnsTableData[ ddnsTableData.length-1][4].disabled = enabledCheckbox.checked ? false : true;
		ddnsTableData[ ddnsTableData.length-1][4].className = enabledCheckbox.checked ? "default_button" : "default_button_disabled" ;
		ddnsEnabledData.push(enabledCheckbox.checked);
 	}
	var ddnsTable=createTable(columnNames, ddnsTableData, "ddns_table", true, false, removeServiceProviderCallback);
	var tableContainer = document.getElementById('ddns_table_container');
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
	var errorList = proofreadServiceProvider(document);
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		//update uci (NOT uciOriginal)
		var sections = uci.getAllSections("ddns_gargoyle");
		var sectionNum = 1+sections.length;
		while( uci.get("ddns_gargoyle", "ddns_" + sectionNum) != '')
		{
			sectionNum++;
		}
		var section = "ddns_" + sectionNum;
		var providerName = getSelectedValue("ddns_provider");
		
		setUciFromDocument(uci, "ddns_gargoyle", section, document);
	

		var domain = uci.get("ddns_gargoyle", section, "domain");
		domain = domain == "" ? providerName : domain;
		domain = domain.length > 20 ? domain.substr(0,17) + "..." : domain;
		
		var enabledCheckbox = createEnabledCheckbox();
		enabledCheckbox.checked = true;
		enabledCheckbox.id = section;	
		var newTableRow =  [domain, UI.never, enabledCheckbox, createEditButton(), createForceUpdateButton()];

		var ddnsTable = document.getElementById('ddns_table_container').firstChild;
		addTableRow(ddnsTable, newTableRow, true, false, removeServiceProviderCallback);

		setDocumentFromUci( new UCIContainer(), "", "", document);

		newSections.push(section);
		updatedSections.push(section);
	}
}
function removeServiceProviderCallback(table, row)
{
	var section = row.childNodes[2].firstChild.id;
	uci.removeSection("ddns_gargoyle", section);
}

function proofreadServiceProvider(controlDocument)
{
	var ddnsIds = ['ddns_check', 'ddns_force'];
	var labelIds= ['ddns_check_label', 'ddns_force_label'];
	var functions = [validateNumeric, validateNumeric];
	var returnCodes = [0,0];

	var validateNotNull=function(text){ return validateLengthRange(text, 1, 999); };




	var providerName;
	if(controlDocument.getElementById("ddns_provider_text") != null)
	{
		providerName = controlDocument.getElementById("ddns_provider_text").firstChild.data;
	}
	else
	{
		providerName = getSelectedValue("ddns_provider", controlDocument);
	}
	var provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = serviceProviders[providerIndex]["name"] == providerName ? serviceProviders[providerIndex] : null;
	}
	if(provider == null)
	{
		alert(DyDNS.InvErr); //should never get here, but let's have an error message just in case
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
		if(allBooleanVariables[ variables[variableIndex] ] != 1)
		{
			ddnsIds.push( variables[variableIndex] );
			labelIds.push( variables[variableIndex] + "_label" );
			functions.push(validateNotNull);
			returnCodes.push(0);
		}
	}
	for(variableIndex=0; variableIndex < optionalVariables.length; variableIndex++)
	{
		if(allBooleanVariables[ optionalVariables[variableIndex] ] != 1)
		{
			if( controlDocument.getElementById( optionalVariables[variableIndex] + "_enabled" ).checked )
			{
				ddnsIds.push( optionalVariables[variableIndex] );
				labelIds.push( optionalVariables[variableIndex] + "_label" );
				functions.push(validateNotNull);
				returnCodes.push(0);
			}
		}
	}



	var errors = proofreadFields(ddnsIds, labelIds, functions, returnCodes, ddnsIds, controlDocument);


	//we don't have a proofread functions on provider elements
	//so we need to make sure class is set to default (not error) for all of them
	for(variableIndex=0; variableIndex < variables.length; variableIndex++)
	{
		if(allBooleanVariables[ variables[variableIndex] ] != 1)
		{
			controlDocument.getElementById( variables[variableIndex] ).className="";
		}
	}
	for(variableIndex=0; variableIndex < optionalVariables.length; variableIndex++)
	{
		if(allBooleanVariables[ optionalVariables[variableIndex] ] != 1)
		{
			if( controlDocument.getElementById( optionalVariables[variableIndex] + "_enabled" ).checked )
			{
				controlDocument.getElementById( optionalVariables[variableIndex] ).className="";
			}
		}
	}
	


	//verify domain name is not duplicate
	if(errors.length == 0)
	{
		var domain;
		if( controlDocument.getElementById("domain") != null)
		{
			domain = controlDocument.getElementById("domain").value;
		}
		else
		{
			domain = providerName;
		}
		var allServices = uci.getAllSectionsOfType("ddns_gargoyle", "service");
		var domainMatches = false;
		for(serviceIndex = 0; serviceIndex < allServices.length && domainMatches == false; serviceIndex++)
		{
			var testDomain = uci.get("ddns_gargoyle", allServices[serviceIndex], "domain");
			testDomain = testDomain == "" ? uci.get("ddns_gargoyle", allServices[serviceIndex], "service_provider") : testDomain;
			domainMatches = testDomain == domain ? true : false;
		}
		if(domainMatches)
		{
			errors.push(DyDNS.DupErr);
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
		alert(DyDNS.InvErr); //should never get here, but let's have an error message just in case
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
		var el = controlDocument.getElementById(optionalVariables[variableIndex]);
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
			setElementEnabled(el, check.checked, "");
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
			
			var label = controlDocument.createElement("label");
			label.className="leftcolumn";
			label.id=variables[variableIndex] + "_label";
			label.appendChild( controlDocument.createTextNode( (ObjLen(DyDNS)==0 ? variableNames[variableIndex] : eval(variableNames[variableIndex])) + ":" ));
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

			label.setAttribute("for", input.id);
		}

		var optionalVariables = provider["optional_variables"];
		var optionalVariableNames = provider["optional_variable_names"];
		for(variableIndex = 0; variableIndex < optionalVariables.length; variableIndex++)
		{
			var div= controlDocument.createElement("div");
			
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

				label.setAttribute("for", check.id);				
			}
			else
			{
				var input = createInput("checkbox", controlDocument);
				input.className = "rightcolumn";
				input.id = optionalVariables[variableIndex];
				div.appendChild(input);

				label.setAttribute("for", input.id);				
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

		for(variableIndex = 0; variableIndex < optionalVariables.length; variableIndex++)
		{
			if(allBooleanVariables[ optionalVariables[variableIndex] ] != 1)
			{
				setElementEnabled( controlDocument.getElementById(optionalVariables[variableIndex]), controlDocument.getElementById(optionalVariables[variableIndex] + "_enabled").checked, "");
			}
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
	editButton.value = UI.Edit;
	editButton.className="default_button";
	editButton.onclick = editServiceTableRow;
	return editButton;
}

function createForceUpdateButton()
{
	updateButton = createInput("button");
	updateButton.value = DyDNS.ForceU;
	updateButton.className="default_button";
	updateButton.onclick = forceUpdateForRow;
	return updateButton;
}

function setRowEnabled()
{
	var enabled= this.checked ? "1" : "0";
	var enabledRow=this.parentNode.parentNode;
	var enabledDomain = enabledRow.firstChild.firstChild.data;

	enabledRow.childNodes[4].firstChild.disabled   = this.checked ? false : true;
	enabledRow.childNodes[4].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

	var section = enabledRow.childNodes[2].firstChild.id;
	uci.set("ddns_gargoyle", section, "enabled", enabled);
	updatedSections.push(section);
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
					provider["variables"]=variablePart.split(/[\t ]+/);
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
	var updateRow=this.parentNode.parentNode;
	var sections = uci.getAllSections("ddns_gargoyle");
	var updateDomain = updateRow.firstChild.firstChild.data;

	var section = updateRow.childNodes[2].firstChild.id;
	var needsUpdate=false;
	for(updatedIndex=0; updatedIndex < updatedSections.length && !needsUpdate; updatedIndex++)
	{
		needsUpdate = section == updatedSections[updatedIndex];
	}
	if(needsUpdate) //should check newSections instead (implement later)
	{
		alert(DyDNS.ModErr);
	}
	else
	{
		setControlsEnabled(false, true);
		var commands = "/usr/bin/ddns_gargoyle -P /etc/ddns_providers.conf -C /etc/ddns_gargoyle.conf -m -f " + section;
		commands = commands + "\n" + "echo $(cat /var/last_ddns_updates/" + section + ") ";
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));


		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var responseLines=req.responseText.split(/[\r\n]+/);
				setControlsEnabled(true);
				if(responseLines[0] == "0")
				{
					alert(DyDNS.UpFErr);
				}
				else
				{
					alert(DyDNS.UpOK);
					updateTimes[section] = responseLines[1];
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

	var xCoor;
	var yCoor;
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

	//editServiceWindow is global so we can close it above if it is left open
	editServiceWindow = window.open("ddns_edit_service.sh", "edit", "width=560,height=500,left=" + xCoor + ",top=" + yCoor );
	
	var saveButton = createInput("button", editServiceWindow.document);
	var closeButton = createInput("button", editServiceWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
	closeButton.className = "default_button";

	//load provider data for this row
	var editServiceWindowRow=this.parentNode.parentNode;
	var section = editServiceWindowRow.childNodes[2].firstChild.id;
	var providerName = uci.get("ddns_gargoyle", section, "service_provider");
	var selectedDomain = uci.get("ddns_gargoyle", section, "domain");
	selectedDomain = selectedDomain == "" ? providerName : selectedDomain;


	var provider = null;
	for(providerIndex=0; providerIndex < serviceProviders.length && provider == null; providerIndex++)
	{
		provider = providerName == serviceProviders[providerIndex]["name"] ? serviceProviders[providerIndex] : null;
	}
	var providerVariables = provider["variables"];
	var providerVariableNames = provider["variable_names"];

	runOnServiceEditorLoaded = function () 
	{
		update_done=false;
		if(editServiceWindow.document != null)
		{
			if(editServiceWindow.document.getElementById("bottom_button_container") != null)
			{
				editServiceWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editServiceWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
			

				setDocumentFromUci(uci, "ddns_gargoyle", section, editServiceWindow.document);
								
				
			
				closeButton.onclick = function()
				{
					editServiceWindow.close();
				}	
				
				saveButton.onclick = function()
				{
					var newDomain = editServiceWindow.document.getElementById("domain") != null ?
								editServiceWindow.document.getElementById("domain").value :
								providerName;

					var errors = proofreadServiceProvider(editServiceWindow.document);
					if(errors.length == 1)
					{
						var dupRegEx=new RegExp(DyDNS.DupErr);
						//if(errors[0].match(/Duplicate/) && newDomain == selectedDomain)
						if(errors[0].match(dupRegEx) && newDomain == selectedDomain)
						{
							errors = [];
						}
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n\n"+DyDNS.UpSrvErr);
						editServiceWindow.focus();
					}
					else
					{
						var truncatedDomain = newDomain.length > 20 ? newDomain.substr(0,17) + "..." : newDomain;	
						editServiceWindowRow.firstChild.firstChild.data = truncatedDomain;
						setUciFromDocument(uci, "ddns_gargoyle", section, editServiceWindow.document);
						updatedSections.push(section);
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

