var pkg = "restricter_gargoyle";
var allIps = new Array();

function saveChanges()
{
	setControlsEnabled(false, true);

	//set enabled status to corrospond with checked in table
	enabledQuotaFound = false;
	var runCommands = [];
	var quotaTableContainer = document.getElementById('quota_table_container');
	var quotaTable = quotaTableContainer.firstChild;
	var quotaTableData = getTableDataArray(quotaTable);
	for(quotaIndex =0; quotaIndex < quotaTableData.length; quotaIndex++)
	{
		var check = quotaTableData[quotaIndex][3];
		enabledQuotaFound = enabledQuotaFound || check.checked;
		var sections = check.id.split(".");
		if( uci.get(pkg, sections[0]) == "quota" )
		{
			uci.set(pkg, sections[0], "enabled", check.checked ? "1" : "0");
		}
		if( uci.get(pkg, sections[1]) == "quota" )
		{
			uci.set(pkg, sections[1], "enabled", check.checked ? "1" : "0");
		}
	}
	if(enabledQuotaFound || restricterEnabled)
	{
		runCommands.push("/etc/init.d/restricter_gargoyle enable");
		runCommands.push("/etc/init.d/restricter_gargoyle restart");
		runCommands.push("q=$(uci show restricter_gargoyle | grep \"=quota\" | sed \" s/^.*\\.//g \" | sed \"s/=.*\\$//g\" )")
		runCommands.push("quotas=$(echo $q)");
		runCommands.push("echo $quotas");
		runCommands.push("sleep 3");
		runCommands.push("restricter_gargoyle -m $quotas");

	}
	else
	{
		runCommands.push("q=$(uci show restricter_gargoyle | grep \"=quota\" | sed \" s/^.*\.//g \" | sed \"s/=.*\$//g\" )")
		runCommands.push("quotas=$(echo $q)");
		runCommands.push("echo $quotas");
		runCommands.push("-");
	}
	
	//delete all quota sections in uciOriginal & remove them from uciOriginal
	var deleteSectionCommands = [];
	var originalQuotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	for(quotaIndex=0; quotaIndex < originalQuotaSections.length; quotaIndex++)
	{
		uciOriginal.removeSection(pkg, originalQuotaSections[quotaIndex]);
		deleteSectionCommands.push("uci del " + pkg + "." + originalQuotaSections[quotaIndex]);
	}
	deleteSectionCommands.push("uci commit");

	//create/initialize all quota sections in uci
	var createSectionCommands = [];
	var newQuotaSections = uci.getAllSectionsOfType(pkg, "quota");
	for(quotaIndex=0; quotaIndex < newQuotaSections.length; quotaIndex++)
	{
		createSectionCommands.push("uci set " + pkg + "." + newQuotaSections[quotaIndex] + "='quota'");
	}
	createSectionCommands.push("uci commit");



	var commands = deleteSectionCommands.join("\n") + "\n" + createSectionCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + runCommands.join("\n") + "\n";

	var param = getParameterDefinition("commands", commands);
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{	
			restricterEnabled = restricterEnabled || enabledQuotaFound;
			uciOriginal = uci.clone();
			
			
			var outLines = req.responseText.split(/[\r\n]+/);
			var popLine = outLines.pop();
			while(outLines.length > 0 && (!popLine.match(/Success/)))
			{
				popLine = outLines.pop();
			}
			if(popLine.match(/Success/))
			{
				quotaData = outLines.pop();
				quotaNames = outLines.pop();
				if(quotaData == "-")
				{
					quotaData = "";
				}
			}
			
			resetData();
			setControlsEnabled(true);	
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}


function resetData()
{
	
	var splitQuotaNames = quotaNames != null ? quotaNames.split(/[\t ]+/) : [];
	var splitQuotaData = quotaData != null ? quotaData.split(/[\t ]+/) : [];
	var quotaPercentages = new Array();
	for(quotaIndex=0; quotaIndex < splitQuotaNames.length && quotaIndex < splitQuotaData.length; quotaIndex++)
	{
		if(splitQuotaNames[quotaIndex] != "" && splitQuotaData[quotaIndex] != "")
		{
			var splitPart = splitQuotaData[quotaIndex].split(/,/);
			var used = Math.round((100*100*parseInt(splitPart[0]))/parseInt(splitPart[1]));
			var usedStr = (used/100.0) + "%";
			quotaPercentages[ splitQuotaNames[quotaIndex] ] = usedStr;
		
		}
	}


	//table columns: ip, percent upload used, percent download used, enabled, edit, remove
	allIps = new Array();
	var quotaSections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex = 0; sectionIndex < quotaSections.length; sectionIndex++)
	{
		ip = uciOriginal.get(pkg, quotaSections[sectionIndex], "ip");
		ip = ip.toUpperCase();
		ip = (ip == "" ? "ALL" : ip);
		allIps[ip] = 1;
	}

	var quotaTableData = [];
	var checkElements = []; //because IE is a bitch and won't register that checkboxes are checked/unchecked unless they are part of document
	var areChecked = [];

	for(ip in allIps)
	{
		var uploadSection = "";
		var downloadSection = "";
		for(sectionIndex=0; sectionIndex < quotaSections.length; sectionIndex++)
		{
			if(uci.get(pkg, quotaSections[sectionIndex], "ip") == ip || (ip == "ALL" && uci.get(pkg, quotaSections[sectionIndex], "ip") == ""))
			{

				var is_ingress = uci.get(pkg, quotaSections[sectionIndex], "is_ingress") == "1" ? true : false;
				if(is_ingress)
				{
					downloadSection = quotaSections[sectionIndex];
				}
				else
				{
					uploadSection = quotaSections[sectionIndex];
				}
			}
		}
		
		var upEnabledStr = uciOriginal.get("restricter_gargoyle", uploadSection, "enabled");
		var upEnabledBool =  (upEnabledStr == "" || upEnabledStr == "1" || upEnabledStr == "true") && (uploadSection != "") && restricterEnabled;
		var downEnabledStr = uciOriginal.get("restricter_gargoyle", uploadSection, "enabled");
		var downEnabledBool =  (downEnabledStr == "" || downEnabledStr == "1" || downEnabledStr == "true") && (downloadSection != "") && restricterEnabled;
		var enabledCheck = createEnabledCheckbox(upEnabledBool || downEnabledBool);
		enabledCheck.id = uploadSection + "." + downloadSection;


		var upPercentage = quotaPercentages[ uploadSection ] == null || (!upEnabledBool) ? "N/A" : quotaPercentages[ uploadSection ];
		var downPercentage = quotaPercentages[ downloadSection ] == null || (!downEnabledBool) ? "N/A" : quotaPercentages[ downloadSection ];

		checkElements.push(enabledCheck);
		areChecked.push(upEnabledBool || downEnabledBool);


		quotaTableData.push( [ ip, upPercentage, downPercentage, enabledCheck, createEditButton(upEnabledBool || downEnabledBool) ] );
	}
	
	columnNames=["IP", "% Upload Used", "% Download Used", "", "" ];
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

function addNewQuota()
{
	var errors = validateQuota(document, "none");
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\nCould not add rule.");
	}
	else
	{
		setUciFromDocument(document);
		var ip = getSelectedValue("applies_to_type", document) == "all" ? "ALL" : document.getElementById("applies_to").value;
		
		var uploadSection = getSelectedValue("max_up_type", document) == "limited" ? "quota_up_" + ip.replace(/\./g, "_") : "";
		var downloadSection = getSelectedValue("max_down_type", document) == "limited" ? "quota_down_" + ip.replace(/\./g, "_") : "";
		var enabledCheck = createEnabledCheckbox(true);
		enabledCheck.id = uploadSection + "." + downloadSection; 

		var tableContainer = document.getElementById("quota_table_container");
		var table = tableContainer.firstChild;	
		addTableRow(table, [ip, "N/A", "N/A", enabledCheck, createEditButton(true)], true, false, removeQuotaCallback);	
		
		setDocumentFromUci(document, new UCIContainer(), "");

		enabledCheck.checked = true;
	}
}

function setVisibility(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	setInvisibleIfIdMatches("applies_to_type", "all", "applies_to", "inline", controlDocument);
	setInvisibleIfIdMatches("max_up_type", "unlimited", "max_up_container", "inline", controlDocument);
	setInvisibleIfIdMatches("max_down_type", "unlimited", "max_down_container", "inline", controlDocument);
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

function validateQuota(controlDocument, originalQuotaIp)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	var inputIds = ["applies_to", "max_up", "max_down"];
	var labelIds = ["appies_to_label", "max_up_label", "max_down_label"];
	var functions = [validateIP, validateNumeric, validateNumeric];
	var validReturnCodes = [0,0,0];
	var visibilityIds = ["applies_to", "max_up_container","max_down_container"];
	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );

	//also validate 1) ip is either currentQuotaIp or is unique in table, 2) up & down aren't both unlimited
	if(errors.length == 0)
	{
		if( getSelectedValue("max_up_type", controlDocument) == "unlimited" && getSelectedValue("max_down_type", controlDocument) == "unlimited")
		{
			errors.push("Upload and download bandwidth limits cannot both be unlimited");
		}
		ip = getSelectedValue("applies_to_type", controlDocument) == "all" ? "ALL" : controlDocument.getElementById("applies_to").value;
		if(ip != originalQuotaIp && allIps[ip] == 1)
		{
			errors.push("Duplicate IP -- only one quota per IP is allowed");
		}
	}
	return errors;
}
function setDocumentFromUci(controlDocument, srcUci, ip)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	ip = ip.toUpperCase();

	var uploadSection = "";
	var downloadSection = "";
	var sections = srcUci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "ip") == ip || (ip == "ALL" && uci.get(pkg, sections[sectionIndex], "ip") == ""))
		{
			var is_ingress = uci.get(pkg, sections[sectionIndex], "is_ingress") == "1" ? true : false;
			if(is_ingress)
			{
				downloadSection = sections[sectionIndex];
			}
			else
			{
				uploadSection = sections[sectionIndex];
			}
		}
	}

	var resetInterval = ""
	var uploadBandwidth = "";
	var downloadBandwidth = "";
	if(uploadSection != "")
	{
		resetInterval = srcUci.get(pkg, uploadSection, "reset_interval");
		uploadBandwidth = srcUci.get(pkg, uploadSection, "max_bandwidth");
	}
	if(downloadSection != "")
	{
		resetInterval = resetInterval == "" || resetInterval == "minutely" ? srcUci.get(pkg, downloadSection, "reset_interval") : resetInterval;
		downloadBandwidth = srcUci.get(pkg, downloadSection, "max_bandwidth");
	}	
	resetInterval = resetInterval == "" || resetInterval == "minutely" ? "daily" : resetInterval;

	setSelectedValue("applies_to_type", ip=="" || ip=="ALL" ? "all" : "only", controlDocument);
	setSelectedValue("quota_reset", resetInterval, controlDocument);
	setSelectedValue("max_up_type", uploadBandwidth == "" ? "unlimited" : "limited", controlDocument );
	setSelectedValue("max_down_type", downloadBandwidth == "" ? "unlimited" : "limited", controlDocument );

	controlDocument.getElementById("applies_to").value = (ip == "" || ip == "ALL" ? "" : ip);
	controlDocument.getElementById("max_up").value = Math.round(uploadBandwidth/(1024*1024)) > 0 ? Math.round(uploadBandwidth/(1024*1024)) : 1;
	controlDocument.getElementById("max_down").value = Math.round(downloadBandwidth/(1024*1024)) > 0 ? Math.round(downloadBandwidth/(1024*1024)) : 1;

	

	setVisibility(controlDocument);
}


function setUciFromDocument(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	ip = getSelectedValue("applies_to_type", controlDocument) == "all" ? "ALL" : controlDocument.getElementById("applies_to").value;
	
	var uploadSection = "";
	var downloadSection = "";
	var sections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "ip") == ip || (ip == "ALL" && uci.get(pkg, sections[sectionIndex], "ip") == ""))
		{
			var is_ingress = uci.get(pkg, sections[sectionIndex], "is_ingress") == "1" ? true : false;
			if(is_ingress)
			{
				downloadSection = sections[sectionIndex];
			}
			else
			{
				uploadSection = sections[sectionIndex];
			}
		}
	}
	uploadSection = uploadSection == "" ? "quota_up_" + ip.replace(/\./g, "_") : uploadSection;
	downloadSection = downloadSection == "" ? "quota_down_" + ip.replace(/\./g, "_") : downloadSection;


	uci.removeSection(uploadSection);
	uci.removeSection(downloadSection);
	
	var uploadBandwidth = getSelectedValue("max_up_type", controlDocument) == "unlimited" ? "" : controlDocument.getElementById("max_up").value;
	var downloadBandwidth = getSelectedValue("max_down_type", controlDocument) == "unlimited" ? "" : controlDocument.getElementById("max_down").value;
	if(uploadBandwidth != "")
	{
		uci.set(pkg, uploadSection,   "", "quota");
		uci.set(pkg, uploadSection,   "is_ingress", "0");
		uci.set(pkg, uploadSection,   "ip", ip);
		uci.set(pkg, uploadSection,   "reset_interval", getSelectedValue("quota_reset", controlDocument));
		uci.set(pkg, uploadSection,   "max_bandwidth", Math.round(uploadBandwidth*1024*1024));
	}
	if(downloadBandwidth != "")
	{
		uci.set(pkg, downloadSection, "", "quota");
		uci.set(pkg, downloadSection, "is_ingress", "1");
		uci.set(pkg, downloadSection, "ip", ip);
		uci.set(pkg, downloadSection, "reset_interval", getSelectedValue("quota_reset", controlDocument));
		uci.set(pkg, downloadSection, "max_bandwidth", Math.round(downloadBandwidth*1024*1024));
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

	enabledRow.childNodes[4].firstChild.disabled  = this.checked ? false : true;
	enabledRow.childNodes[4].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

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
	var idStr = row.childNodes[3].firstChild.id;
	var ids = idStr.split(/\./);
	if(uci.get(pkg, ids[0]) != "")
	{
		uci.removeSection(pkg, ids[0]);
	}
	if(uci.get(pkg, ids[1]) != "")
	{
		uci.removeSection(pkg, ids[1]);
	}
	allIps[ row.childNodes[0].firstChild.data ] = null;
}

function editRule()
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
	editIp = editRow.childNodes[0].firstChild.data;

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
				setVisibility(editQuotaWindow.document);

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
						alert(errors.join("\n") + "\nCould not add rule.");
					}
					else
					{
						setUciFromDocument(editQuotaWindow.document);
						editRow.childNodes[0].firstChild.data = getSelectedValue("applies_to_type", editQuotaWindow.document) == "all" ? "ALL" : editQuotaWindow.document.getElementById("applies_to").value;
						editRow.childNodes[1].firstChild.data = "N/A";
						editRow.childNodes[2].firstChild.data = "N/A";
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
