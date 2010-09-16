/*
 * This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */



function saveChanges()
{


	uci = uciOriginal.clone();
	var commands = "";
	var errors = "";

	var stopbwmon = "/etc/init.d/bwmon_gargoyle stop ;\n";
	var startbwmon = "/etc/init.d/bwmon_gargoyle start ;\n";
	var bwmonCleanCommand = "\nif [ -d /usr/data/bwmon/ ] ; then rm /usr/data/bwmon/qos-" + direction + "-* >/dev/null 2>&1 ; fi ;\n";
	var bwmonCleanCommand = bwmonCleanCommand + "if [ -d /tmp/data/bwmon/ ] ; then rm /tmp/data/bwmon/qos-" + direction + "-* >/dev/null 2>&1 ; fi ;\n";


       //Save the setting of the qos_monenable flag
	if (direction == "download") {
		uci.set("qos_gargoyle", direction, "qos_monenabled", document.getElementById("qos_monenabled").checked);

		if (document.getElementById("use_ptarget_ip").checked == true) {
			uci.set("qos_gargoyle", direction, "ptarget_ip", document.getElementById("ptarget_ip").value);
		} else {
			uci.remove("qos_gargoyle", direction, "ptarget_ip");
		}
     	}


	var disabled = document.getElementById("qos_enabled").checked == false;
	var otherDirection = direction == "upload" ? "download" : "upload";
	var alternateBandwidth = uciOriginal.get("qos_gargoyle", otherDirection, "total_bandwidth");

	var fullQosWillBeEnabled = ( (!disabled) || alternateBandwidth != "");
	var switchingFullQosEnabled = (fullQosWillBeEnabled != qosEnabled);

	//Is the user requesting disable of this direction of QoS?
	if(disabled)
	{
		//If this page was enabled before and the other was not then stop and disable QoS
		if(qosEnabled && alternateBandwidth == "" )
		{
			//delete qos distribution section
			uci.remove("gargoyle", "status", "qos");
			commands = "/etc/init.d/qos_gargoyle stop\n/etc/init.d/qos_gargoyle disable\n";
			qosEnabled = false;
		}
		else //Otherwise just restart QOS.  This will delete this direction.
		{
			commands = "/etc/init.d/qos_gargoyle start\n";
		}

		uci.remove("qos_gargoyle", direction, "total_bandwidth");
		if(switchingFullQosEnabled &&  qosQuotasExist())
		{
			commands =  uci.getScriptCommands(uciOriginal) + "\n" + commands + "sh /usr/lib/gargoyle/restart_firewall.sh ;\n";
		}
		else
		{
			commands =  uci.getScriptCommands(uciOriginal) + "\n" +  stopbwmon + commands + bwmonCleanCommand + startbwmon;
		}
	}
	else if(validateNumeric(document.getElementById("total_bandwidth").value) != 0)
	{
		errors="There is an error in Total Bandwidth field.\n\nCould not update QoS.";
	}
	else
	{

		qosEnabled = true;
		preCommands = [];

		//ensure qos distribution section is enabled
		uci.set("gargoyle", "status", "qos", "300");

		//delete all existing direction_rule sections and all existing direction_class sections
		//go backwards through sections so that any time we delete cfgX we do not change X!
		//note: no longer a problem with new UCI, but I am too lazy to change it back for no good reason
		directionRule=direction + "_rule";
		oldRuleSections = uciOriginal.getAllSectionsOfType("qos_gargoyle", directionRule);
		for(sectionIndex = oldRuleSections.length-1; sectionIndex >= 0; sectionIndex--)
		{
			uciOriginal.removeSection("qos_gargoyle", oldRuleSections[sectionIndex]);
			uci.removeSection("qos_gargoyle", oldRuleSections[sectionIndex]);
			preCommands.push("uci del qos_gargoyle." + oldRuleSections[sectionIndex]);
		}
		directionClass=direction + "_class";
		oldClassSections = uciOriginal.getAllSectionsOfType("qos_gargoyle", directionClass);
		for(sectionIndex = oldClassSections.length-1; sectionIndex >= 0; sectionIndex--)
		{
			uciOriginal.removeSection("qos_gargoyle", oldClassSections[sectionIndex]);
			uci.removeSection("qos_gargoyle", oldClassSections[sectionIndex]);
			preCommands.push("uci del qos_gargoyle." + oldClassSections[sectionIndex]);
		}
		preCommands.push("uci commit");


		uci.set("qos_gargoyle", direction, "total_bandwidth", document.getElementById("total_bandwidth").value);

		classTable = document.getElementById('qos_class_table_container').firstChild;
		classData = getTableDataArray( classTable, true, false);

		//normalize bandwidth percentages
		percentSum = 0;
		for(classIndex = 0; classIndex < classData.length; classIndex++)
		{
			percent = classData[classIndex][1];
			percent = percent.substr(0,percent.length-1);
			classData[classIndex][1] = percent;
			percentSum = percentSum + (1*percent);
		}

		classIds = [];
		for(classIndex = 0; classIndex < classData.length; classIndex++)
		{
			classId = direction.substr(0,1).toLowerCase() + "class_" + (classIndex+1);
			className = classData[classIndex][0];
			classIds[className] = classId;

			uci.set("qos_gargoyle", classId, "", directionClass);
			uci.set("qos_gargoyle", classId, "name", className);
			uci.set("qos_gargoyle", classId, "percent_bandwidth", Math.round(100*(classData[classIndex][1]/percentSum)) );

			minBandwidth = classData[classIndex][2];
			if(minBandwidth != "zero")
			{
				minBandwidth = minBandwidth.substr(0,minBandwidth.length-7);
				uci.set("qos_gargoyle", classId, "min_bandwidth", minBandwidth);
			}
			maxBandwidth = classData[classIndex][3];
			if(maxBandwidth != "unlimited")
			{
				maxBandwidth = maxBandwidth.substr(0,maxBandwidth.length-7);
				uci.set("qos_gargoyle", classId, "max_bandwidth", maxBandwidth);
			}
		}
		uci.set("qos_gargoyle", direction, "default_class", classIds[getSelectedText("default_class")]);

		ruleTable = document.getElementById('qos_rule_table_container').firstChild;
		ruleData = getTableDataArray( ruleTable, true, true);
		for(ruleIndex = 0; ruleIndex < ruleData.length; ruleIndex++)
		{
			rulePriority = (ruleIndex+1)*100;
			ruleId = direction + "_rule_" + rulePriority;

			classId = classIds[ ruleData[ruleIndex][1] ];
			uci.set("qos_gargoyle", ruleId, "", directionRule);
			uci.set("qos_gargoyle", ruleId, "class", classId);
			uci.set("qos_gargoyle", ruleId, "test_order", rulePriority);

			optionList = ["source", "srcport", "destination", "dstport", "max_pkt_size","min_pkt_size",  "proto", "connbytes"];

			matchCriteria = parseRuleMatchCriteria(ruleData[ruleIndex][0]);
			for(criteriaIndex = 0; criteriaIndex < optionList.length; criteriaIndex++)
			{
				if  (matchCriteria[criteriaIndex] != "")
				{
					value = matchCriteria[criteriaIndex];
					value = optionList[criteriaIndex] == "proto" ? value.toLowerCase() : value;
					uci.set("qos_gargoyle", ruleId, optionList[criteriaIndex], value);
				}
			}
			if(matchCriteria[matchCriteria.length-1] != "")
			{

                            protocolMap.getL7ID = function (desc) {
                                 for(key in this) {if (this[key] == desc) return key;}
                            }

				appProtocol = matchCriteria[matchCriteria.length-1];
                            appProtocolName=protocolMap.getL7ID(appProtocol);
				if(appProtocolName != null) {
					uci.set("qos_gargoyle", ruleId, "layer7", appProtocolName);
                            }
			}
		}
		if(switchingFullQosEnabled &&  qosQuotasExist())
		{
			commands = "\n/etc/init.d/qos_gargoyle enable ;\n";
			commands =  preCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + commands + "sh /usr/lib/gargoyle/restart_firewall.sh ;\n";
		}
		else
		{
			commands = "\n/etc/init.d/qos_gargoyle start ;\n/etc/init.d/qos_gargoyle enable ;\n";
			commands = preCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" +stopbwmon + commands + bwmonCleanCommand + startbwmon;
		}
	}

	if(errors != "")
	{
		alert(errors);
	}
	else if(commands.length > 0)
	{
		setControlsEnabled(false, true);

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);
			}
		}

		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function resetData()
{
	//set description visibility
	initializeDescriptionVisibility(uciOriginal, "qos_" + (direction == "upload" ? "up" : "down") + "_1");
	initializeDescriptionVisibility(uciOriginal, "qos_" + (direction == "upload" ? "up" : "down") + "_2");

	if (direction == "download") {
		initializeDescriptionVisibility(uciOriginal, "qos_down_3");
		initializeDescriptionVisibility(uciOriginal, "qos_down_4");
	}
	
	uciOriginal.removeSection("gargoyle", "help"); //necessary, or we over-write the help settings when we save


	totalBandwidth = uciOriginal.get("qos_gargoyle", direction, "total_bandwidth");
	totalBandwidth = totalBandwidth == "" ? -1 : totalBandwidth;
	document.getElementById("qos_enabled").checked = qosEnabled && totalBandwidth > 0;

	defaultBandwidth = direction == "upload" ? 8*40 : 8*400;  //default upload bandwidth = 40 kilobytes/s, download = 400 kilobytes/s
	document.getElementById("total_bandwidth").value = totalBandwidth > 0 ? totalBandwidth : defaultBandwidth;

	directionClass = direction + "_class";
	directionRule  = direction + "_rule";




	var classSections = uciOriginal.getAllSectionsOfType("qos_gargoyle", directionClass);



	removeAllOptionsFromSelectElement( document.getElementById("default_class"));
	removeAllOptionsFromSelectElement( document.getElementById("classification"));

	classTableColumns = ["Service Class Name", "Percent Bandwidth At Capacity", "Minimum Bandwidth", "Maximum Bandwidth", ""];
	classTableData = new Array();
	classNames = new Array();
	totalPercent = 0;
	defaultClassName = "";
	for (classIndex=0; classIndex < classSections.length; classIndex++)
	{
		classSection = classSections[classIndex];


		nextName = uciOriginal.get("qos_gargoyle", classSection, "name");
		nextPercent = uciOriginal.get("qos_gargoyle", classSection, "percent_bandwidth");
		nextMax = uciOriginal.get("qos_gargoyle", classSection, "max_bandwidth");
		nextMin = uciOriginal.get("qos_gargoyle", classSection, "min_bandwidth");


		nextName = nextName == "" ? classSection : nextName;
		nextMin = nextMin != "" && nextMin > 0 ? nextMin + " kbit/s" : "zero";
		nextMax = nextMax != "" && nextMax > 0 ? nextMax + " kbit/s" : "unlimited";


		addOptionToSelectElement("default_class", nextName, nextName, null);
		addOptionToSelectElement("classification", nextName, nextName, null);
		classNames[classSection] = nextName;

		defaultClassName = uciOriginal.get("qos_gargoyle", direction, "default_class") == classSection ? nextName : defaultClassName;

		classTableData.push( [nextName, nextPercent, nextMin, nextMax, createClassTableEditButton()] );
		totalPercent = totalPercent + nextPercent;
	}
	setSelectedText("default_class", defaultClassName);



	for (classIndex=0; classIndex < classSections.length; classIndex++)
	{
		classTableDataRow = classTableData[classIndex];
		classTableDataRow[1] = totalPercent == 100 ? Math.round(100 * (classTableDataRow[1]/totalPercent)) + "%" : classTableDataRow[1] + "%";
	}

	classTable=createTable(classTableColumns, classTableData, "qos_class_table", true, false, removeServiceClassCallback);


	classTableContainer = document.getElementById('qos_class_table_container');
	if(classTableContainer.firstChild != null)
	{
		classTableContainer.removeChild(classTableContainer.firstChild);
	}
	classTableContainer.appendChild(classTable);



	var ruleSections = uciOriginal.getAllSectionsOfType("qos_gargoyle", directionRule);
	ruleTableColumns = ["Match Criteria", "Classification", ""];
	ruleTableData = new Array();
	rulePriorities = [];
	for(ruleIndex = 0; ruleIndex < ruleSections.length; ruleIndex++)
	{
		ruleSection = ruleSections[ruleIndex];

		rulePriority = uciOriginal.get("qos_gargoyle", ruleSection, "test_order");
		rulePriority = rulePriority == "" ? 9999999 : rulePriority;
		rulePriorities.push(rulePriority + "," + ruleIndex);


		optionList = ["source", "srcport", "destination", "dstport", "max_pkt_size", "min_pkt_size", "proto", "connbytes"];
		displayOptionList = ["Source: $", "Source Port: $", "Destination: $", "Destination Port: $", "Maximum Packet Length: $ bytes", "Minimum Packet Length: $ bytes", "Transport Protocol: $", "Connection bytes: $ MBytes"];

		ruleText = "";
		for (optionIndex = 0; optionIndex < optionList.length; optionIndex++)
		{

			optionValue = uciOriginal.get("qos_gargoyle", ruleSection, optionList[optionIndex])
			if(optionList[optionIndex] == "proto")
			{
				if(optionValue == "both")
				{
					optionValue =""
				}
				else
				{
					optionValue = optionValue.toUpperCase();
				}
			}
			if(optionValue != "")
			{
				substitution = displayOptionList[optionIndex];
				substitution = substitution.replace(/\$/, optionValue);
				ruleText = ruleText == "" ? ruleText +substitution : ruleText + ", " + substitution;
			}
		}

		if(uciOriginal.get("qos_gargoyle", ruleSection, "layer7") != "")
		{

			app_protocol="Application Protocol: " + protocolMap[uciOriginal.get("qos_gargoyle", ruleSection, "layer7")];
			ruleText = ruleText == "" ? ruleText + app_protocol : ruleText + ", " + app_protocol;
		}

		classification = classNames[ uciOriginal.get("qos_gargoyle", ruleSection, "class") ];
		ruleTableData.push( [ruleText, classification, createRuleTableEditButton()] );
	}

	//sort rules
	sortedRulePriorities = rulePriorities.sort(rulePriorityCompareFunction);
	sortedRuleTableData = new Array();
	for(rowIndex = 0; rowIndex < sortedRulePriorities.length; rowIndex++)
	{
		sortedRuleTableData.push( ruleTableData[ sortedRulePriorities[rowIndex].split(",")[1] ] );
	}

	//create table
	ruleTable=createTable(ruleTableColumns, sortedRuleTableData, "qos_rule_table", true, true);
	ruleTableContainer = document.getElementById('qos_rule_table_container');
	if(ruleTableContainer.firstChild != null)
	{
		ruleTableContainer.removeChild(ruleTableContainer.firstChild);
	}
	ruleTableContainer.appendChild(ruleTable);

	setQosEnabled();

       if (direction == "download") { 
           monenabled= uciOriginal.get("qos_gargoyle", direction, "qos_monenabled");
           if (monenabled == "true") document.getElementById("qos_monenabled").checked = true;

           ptarget_ip = uciOriginal.get("qos_gargoyle", direction, "ptarget_ip");
           if (ptarget_ip == "") {
                document.getElementById("use_ptarget_ip").checked = false;
		setElementEnabled(document.getElementById("ptarget_ip"), false, currentWanGateway)

           } else {
                document.getElementById("use_ptarget_ip").checked = true;
                document.getElementById("ptarget_ip").value=ptarget_ip;
		setElementEnabled(document.getElementById("ptarget_ip"), true, "")

           }

       }
}


function qosQuotasExist()
{
	var quotaSections = uciOriginal.getAllSectionsOfType("firewall", "quota");
	var qsi;
	var found = false;
	for(qsi=0; qsi < quotaSections.length && (!found); qsi++)
	{
		var testVals = [ "exceeded_up_speed", "exceeded_down_speed", "exceeded_up_class_mark", "exceeded_down_class_mark" ];
		while(testVals.length > 0 && (!found) )
		{
			found = found || (uciOriginal.get("firewall", quotaSections[qsi], testVals.shift()) != "");
		}
	}
	return found;
}



function setQosEnabled()
{
	enabled = document.getElementById("qos_enabled").checked;
	controlIds = ["default_class", "source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol", "connbytes", "app_protocol", "use_source_ip", "use_source_port", "use_dest_ip", "use_dest_port", "use_max_pktsize", "use_min_pktsize", "use_transport_protocol", "use_connbytes", "use_app_protocol", "classification", "add_rule_button", "class_name", "percent_bandwidth", "min_radio1", "min_radio2", "min_bandwidth", "max_radio1", "max_radio2", "max_bandwidth", "add_class_button"];

	setElementEnabled( document.getElementById("total_bandwidth"), enabled, uciOriginal.get("qos_gargoyle", direction, "total_bandwidth") );
	for(controlIndex = 0; controlIndex < controlIds.length; controlIndex++)
	{
		setElementEnabled( document.getElementById(controlIds[controlIndex]), enabled, "");
	}

	setRowClasses( document.getElementById('qos_rule_table_container').firstChild, enabled);
	setRowClasses( document.getElementById('qos_class_table_container').firstChild, enabled);

	if(enabled && document.getElementById("total_bandwidth").value == "")
	{
		defaultBandwidth = direction == "upload" ? 8*40 : 8*400;  //default upload bandwidth = 40 kilobytes/s, download = 400 kilobytes/s
		document.getElementById("total_bandwidth").value = totalBandwidth > 0 ? totalBandwidth : defaultBandwidth;
	}


	if (direction == "download")
	{
	setElementEnabled( document.getElementById("qos_monenabled"), enabled, "");
	qmenabled = (enabled && document.getElementById("qos_monenabled").checked);
	setElementEnabled( document.getElementById("use_ptarget_ip"), qmenabled,"");
	setElementEnabled( document.getElementById("ptarget_ip"), qmenabled &&	document.getElementById("use_ptarget_ip").checked,document.getElementById("ptarget_ip").value);
	}

	resetRuleControls();
	resetServiceClassControls(document);
}

function addClassificationRule()
{
	errors = proofreadClassificationRule(document);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add classification rule.");
	}
	else
	{

		addRuleMatchControls = ["source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol", "connbytes", "app_protocol"];
		displayList = ["Source: $", "Source Port: $", "Destination: $", "Destination Port: $","Maximum Packet Length: $ bytes", "Minimum Packet Length: $ bytes", "Transport Protocol: $", "Connection bytes: $ MBytes", "Application Protocol: $"];

		ruleText = ""
		for (controlIndex = 0; controlIndex <addRuleMatchControls.length; controlIndex++)
		{
			control = document.getElementById(addRuleMatchControls[controlIndex]);
			controlValue = "";
			if(control.type == "text")
			{
				controlValue = control.value;
			}
			if(control.type == "select-one")
			{
				controlValue = control.options[control.selectedIndex].text;
			}
			if(control.disabled == false)
			{
				substitution = displayList[controlIndex];
				substitution = substitution.replace(/\$/, controlValue);
				ruleText = ruleText == "" ? ruleText +substitution : ruleText + ", " + substitution;
			}
		}
		classification = document.getElementById("classification").options[document.getElementById("classification").selectedIndex].text;

		ruleTable = document.getElementById('qos_rule_table_container').firstChild;
		addTableRow(ruleTable,[ruleText,classification, createRuleTableEditButton()],true,true);

		resetRuleControls();
	}
}

function proofreadClassificationRule(controlDocument)
{
	addRuleIds = ["source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol", "connbytes", "app_protocol"];
	validatePktSize = function(text){ return validateNumericRange(text, 1, 1500); };
	validateCBSize = function(text){ return validateNumericRange(text, 0, 4194393); };
	alwaysValid = function(text){return 0;};
	ruleValidationFunctions = [ validateIpRange, validatePortOrPortRange, validateIpRange, validatePortOrPortRange, validatePktSize, validatePktSize, alwaysValid, validateCBSize ,alwaysValid ];
	labelIds = new Array();
	returnCodes = new Array();
	toggledMatchCriteria = 0;
	for(ruleIndex = 0; ruleIndex < addRuleIds.length; ruleIndex++)
	{
		toggledMatchCriteria = toggledMatchCriteria + ( controlDocument.getElementById(addRuleIds[ruleIndex]).disabled == true ? 0 : 1);
		labelIds.push( addRuleIds[ruleIndex] + "_label");
		returnCodes.push(0);
	}
	errors = [];
	if(toggledMatchCriteria == 0)
	{
		errors.push("No match criteria have been selected.");
	}
	else
	{
		errors = proofreadFields(addRuleIds, labelIds, ruleValidationFunctions, returnCodes, addRuleIds, controlDocument);
	}
	return errors;
}

function resetRuleControls()
{
	ruleControlIds = ["source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol","connbytes", "app_protocol"];
	document.getElementById("classification").selectedIndex = document.getElementById("default_class").selectedIndex;

	for(ruleControlIndex=0; ruleControlIndex < ruleControlIds.length; ruleControlIndex++)
	{
		checkbox =  document.getElementById( "use_" + ruleControlIds[ruleControlIndex]);
		checkbox.checked =false;
		enableAssociatedField( checkbox, ruleControlIds[ruleControlIndex], "");
	}
}

function addServiceClass()
{
	errors = proofreadServiceClass(document);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n\nCould not add new service class.");
	}
	else
	{
		newRowData = new Array();
		newRowData.push( document.getElementById("class_name").value );
		newRowData.push( document.getElementById("percent_bandwidth").value + "%" );
		newRowData.push( document.getElementById("min_radio1").checked == true ? "zero" : document.getElementById("min_bandwidth").value + " kbit/s" );
		newRowData.push( document.getElementById("max_radio1").checked == true ? "unlimited" : document.getElementById("max_bandwidth").value + " kbit/s" );
		newRowData.push( createClassTableEditButton() );

		//select element refers to main document, but this is fine since thats where these controls are
		addOptionToSelectElement("default_class", document.getElementById("class_name").value, document.getElementById("class_name").value, null);
		addOptionToSelectElement("classification", document.getElementById("class_name").value, document.getElementById("class_name").value, null);

		classTable = document.getElementById('qos_class_table_container').firstChild;
		addTableRow(classTable,newRowData,true,false,removeServiceClassCallback);


		//see if were adding a class that matches existing rules (i.e. that was previously deleted) and change color back to black from red if so
		ruleTable = document.getElementById('qos_rule_table_container').firstChild
		ruleTableData = getTableDataArray(ruleTable, true,true);
		for(ruleIndex = 0; ruleIndex < ruleTableData.length; ruleIndex++)
		{
			if(ruleTableData[ruleIndex][1] == document.getElementById("class_name").value)
			{
				ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[0].style.color = "black";
				ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[1].style.color = "black";
			}
		}

		resetServiceClassControls(document);


	}
}
function proofreadServiceClass(controlDocument)
{
	addClassIds=[ "class_name","percent_bandwidth","max_bandwidth" ];
	labelIds=[ "class_name_label", "percent_bandwidth_label", "max_bandwidth_label" ];
	classValidationFunctions = [function(text){ return validateLengthRange(text,1,10);}, function(text){return validateNumericRange(text,1,100);}, validateNumeric ];
	returnCodes= [0,0,0];
	errors = proofreadFields(addClassIds, labelIds, classValidationFunctions, returnCodes, addClassIds, controlDocument);
	if(errors.length == "")
	{
		classTableData = getTableDataArray( document.getElementById('qos_class_table_container').firstChild, true, false );
		classNameMatches =false;
		newClassName = controlDocument.getElementById("class_name").value;
		for(rowIndex=0; rowIndex < classTableData.length && (!classNameMatches); rowIndex++)
		{
			existingClassName = classTableData[rowIndex][0];
			if(existingClassName == newClassName)
			{
				classNameMatches = true;
				errors.push("Duplicate class name.");
			}
		}

	}

	return errors;
}

function resetServiceClassControls(controlDocument)
{
	controlDocument.getElementById("class_name").value = "";
	controlDocument.getElementById("percent_bandwidth").value = "";

	controlDocument.getElementById("min_radio1").checked = true;
	controlDocument.getElementById("min_radio2").checked = false;
	enableAssociatedField(controlDocument.getElementById("min_radio2"), "min_bandwidth", "", controlDocument);

	controlDocument.getElementById("max_radio1").checked = true;
	controlDocument.getElementById("max_radio2").checked = false;
	enableAssociatedField(controlDocument.getElementById("max_radio2"), "max_bandwidth", "", controlDocument);
}


function rulePriorityCompareFunction(a, b)
{
	splitA = a.split(",");
	splitB = b.split(",");
	return splitA[0] - splitB[0];
}


function removeServiceClassCallback(table, row)
{
	//if someone just tried to remove the last row, dont let them
	//since we need to have at least one service class
	if(table.firstChild.childNodes.length == 1)
	{
		table.firstChild.appendChild(row);
		alert("At least one service class is required.\nCannot remove service class.");
	}
	else
	{
		removedClassName = row.childNodes[0].firstChild.data;
		removeOptionFromSelectElement("default_class", removedClassName);
		removeOptionFromSelectElement("classification", removedClassName);

		ruleTable = document.getElementById('qos_rule_table_container').firstChild;
		ruleTableData = getTableDataArray(ruleTable, true,true);
		for(ruleIndex = 0; ruleIndex < ruleTableData.length; ruleIndex++)
		{
			if(ruleTableData[ruleIndex][1] == removedClassName)
			{
				ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[0].style.color="red";
				ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[1].style.color="red";
			}
		}
	}

}

function createRuleTableEditButton()
{
	editRuleButton = createInput("button");
	editRuleButton.value = "Edit";
	editRuleButton.className="default_button";
	editRuleButton.onclick = editRuleTableRow;

	return editRuleButton;

}
function editRuleTableRow()
{
	if( typeof editRuleWindow != "undefined" )
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


	editRuleWindow = window.open("qos_edit_rule.sh", "test", "width=560,height=450,left=" + xCoor + ",top=" + yCoor );
	try
	{

		saveButton = editRuleWindow.document.createElement('input');
		saveButton.type= 'button';
		closeButton = editRuleWindow.document.createElement('input');
		closeButton.type= 'button';
	}
	catch(e)
	{
		saveButton = editRuleWindow.createElement('<input type="button" />');
		closeButton = editRuleWindow.createElement('<input type="button" />');
	}
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";


	editRuleWindowRow=this.parentNode.parentNode;

	// we cant run setup functions until edit window is done loading and there
	// is no sleep function in javascript, so we implement a recursive solution using
	// the setTimeout function which runs a specific function after waiting a specified
	// amount of time
	runOnRuleEditorLoaded = function ()
	{
		update_done=false;
		if(editRuleWindow.document != null)
		{
			if(editRuleWindow.document.getElementById("bottom_button_container") != null)
			{

				editRuleWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editRuleWindow.document.getElementById("bottom_button_container").appendChild(closeButton);

				//setup edit window controls
				serviceClassOptions = document.getElementById("default_class").options;
				for(classIndex = 0; classIndex < serviceClassOptions.length; classIndex++)
				{
					addOptionToSelectElement("classification", serviceClassOptions[classIndex].text, serviceClassOptions[classIndex].text, null, editRuleWindow.document);
				}

				ruleControlIds = ["source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol", "connbytes", "app_protocol"];

				criteria = parseRuleMatchCriteria(editRuleWindowRow.firstChild.firstChild.data);
				for(ruleControlIndex=0; ruleControlIndex < ruleControlIds.length; ruleControlIndex++)
				{
					ruleControlId=ruleControlIds[ruleControlIndex];
					ruleControl = editRuleWindow.document.getElementById(ruleControlId);
					ruleControlCheckbox = editRuleWindow.document.getElementById("use_" + ruleControlId);
					if(criteria[ruleControlIndex] != "")
					{
						if(ruleControl.type == "text")
						{
							ruleControl.value=criteria[ruleControlIndex];
						}
						if(ruleControl.type == "select-one")
						{
							setSelectedText( ruleControlId, criteria[ruleControlIndex], editRuleWindow.document);
						}
						ruleControlCheckbox.checked = true;
					}
					else
					{
						ruleControlCheckbox.checked = false;
					}
					enableAssociatedField(ruleControlCheckbox, ruleControlId, "", editRuleWindow.document);
				}
				setSelectedText("classification", editRuleWindowRow.childNodes[1].firstChild.data, editRuleWindow.document);





				//exit & save functions
				closeButton.onclick = function()
				{
					editRuleWindow.close();
				}

				saveButton.onclick = function()
				{
					errors = proofreadClassificationRule(editRuleWindow.document);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n\nCould not update classification rule.");
						editRuleWindow.focus();
					}
					else
					{
						addRuleMatchControls = ["source_ip", "source_port", "dest_ip", "dest_port", "max_pktsize", "min_pktsize", "transport_protocol", "connbytes", "app_protocol"];
						displayList = ["Source: $", "Source Port: $", "Destination: $", "Destination Port: $",  "Maximum Packet Length: $ bytes", "Minimum Packet Length: $ bytes","Transport Protocol: $", "Connection bytes: $ MBytes", "Application Protocol: $"];
						ruleText = ""
						for (controlIndex = 0; controlIndex <addRuleMatchControls.length; controlIndex++)
						{
							control = editRuleWindow.document.getElementById(addRuleMatchControls[controlIndex]);
							controlValue = "";
							if(control.type == "text")
							{
								controlValue = control.value;
							}
							if(control.type == "select-one")
							{
								controlValue = control.options[control.selectedIndex].text;
							}
							if(control.disabled == false)
							{
								substitution = displayList[controlIndex];
								substitution = substitution.replace(/\$/, controlValue);
								ruleText = ruleText == "" ? ruleText +substitution : ruleText + ", " + substitution;
							}
						}
						classification = editRuleWindow.document.getElementById("classification").options[editRuleWindow.document.getElementById("classification").selectedIndex].text;
						editRuleWindowRow.childNodes[0].firstChild.data = ruleText;
						editRuleWindowRow.childNodes[0].style.color = "black";
						editRuleWindowRow.childNodes[1].firstChild.data = classification;
						editRuleWindowRow.childNodes[1].style.color = "black";

						editRuleWindow.close();
					}
				}
				editRuleWindow.moveTo(xCoor,yCoor);
				editRuleWindow.focus();
				update_done = true;
			}
		}
		if(update_done == false)
		{
			setTimeout( "runOnRuleEditorLoaded()", 250);
		}
	}

	runOnRuleEditorLoaded();


}


function createClassTableEditButton()
{
	editClassButton = createInput("button");
	editClassButton.value = "Edit";
	editClassButton.className="default_button";
	editClassButton.onclick = editClassTableRow;

	return editClassButton;
}


function editClassTableRow()
{

	if( typeof editClassWindow != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editClassWindow.close();
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


	editClassWindow = window.open("qos_edit_class.sh", "test", "width=560,height=375,left=" + xCoor + ",top=" + yCoor );
	try
	{
		saveButton = editClassWindow.document.createElement('input');
		saveButton.type= 'button';
		closeButton = editClassWindow.document.createElement('input');
		closeButton.type= 'button';
	}
	catch(e)
	{
		saveButton = editClassWindow.createElement('<input type="button" />');
		closeButton = editClassWindow.createElement('<input type="button" />');
	}
	saveButton.value = "Close and Apply Changes";
	saveButton.className = "default_button";
	closeButton.value = "Close and Discard Changes";
	closeButton.className = "default_button";


	editClassWindowRow=this.parentNode.parentNode;
	runOnClassEditorLoaded = function ()
	{
		update_done=false;
		if(editClassWindow.document != null)
		{
			if(editClassWindow.document.getElementById("bottom_button_container") != null)
			{

				//editClassWindow.document.getElementById("tmp").appendChild( editClassWindow.document.createTextNode("TEST!!!"));
				editClassWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editClassWindow.document.getElementById("bottom_button_container").appendChild(closeButton);

				resetServiceClassControls(editClassWindow.document);

				cells = editClassWindowRow.childNodes;
				editClassWindow.document.getElementById("class_name").value = cells[0].firstChild.data;

				percent = cells[1].firstChild.data;
				editClassWindow.document.getElementById("percent_bandwidth").value = percent.substr(0, percent.length-1);

				if(cells[2].firstChild.data != "zero")
				{
					editClassWindow.document.getElementById("min_radio2").checked = true;
					enableAssociatedField(editClassWindow.document.getElementById("min_radio2"), "min_bandwidth", "", editClassWindow.document);

					minBandwidth = cells[2].firstChild.data;
					editClassWindow.document.getElementById("min_bandwidth").value = minBandwidth.substr(0, minBandwidth.length-7);
				}

				if(cells[3].firstChild.data != "unlimited")
				{
					editClassWindow.document.getElementById("max_radio2").checked = true;
					enableAssociatedField(editClassWindow.document.getElementById("max_radio2"), "max_bandwidth", "", editClassWindow.document);

					maxBandwidth = cells[3].firstChild.data;
					editClassWindow.document.getElementById("max_bandwidth").value = maxBandwidth.substr(0, maxBandwidth.length-7);
				}




				closeButton.onclick = function()
				{
					editClassWindow.close();
				}

				saveButton.onclick = function()
				{
					errors = proofreadServiceClass(editClassWindow.document);
					if(errors.length == 1)
					{
						if(errors[0] == "Duplicate class name." && editClassWindow.document.getElementById("class_name").value == editClassWindowRow.childNodes[0].firstChild.data)
						{
							errors = [];
						}
					}
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n\nCould not update service class.");
						editClassWindow.focus();
					}
					else
					{
						oldClassName =  editClassWindowRow.childNodes[0].firstChild.data;
						newClassName = editClassWindow.document.getElementById("class_name").value;
						if(newClassName != oldClassName)
						{
							//swap class name in select elements for new name
							selectIds = ["default_class", "classification"];
							for(selectIndex = 0; selectIndex < selectIds.length; selectIndex++)
							{
								selectElement = document.getElementById( selectIds[selectIndex] );
								for(optionIndex = 0; optionIndex < selectElement.options.length; optionIndex++)
								{
									if(selectElement.options[optionIndex].text == oldClassName)
									{
										selectElement.options[optionIndex].text = newClassName;
									}
								}
							}


							//swap class name in rules table for new name && if name matches new name make sure text is not red (indicating deleted class)
							ruleTable = document.getElementById('qos_rule_table_container').firstChild;
							ruleTableData = getTableDataArray(ruleTable, true,true);
							for(ruleIndex = 0; ruleIndex < ruleTableData.length; ruleIndex++)
							{
								if(ruleTableData[ruleIndex][1] == oldClassName)
								{
									ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[1].firstChild.data = newClassName;
								}
								if(ruleTableData[ruleIndex][1] == newClassName)
								{
									ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[0].style.color = "black";
									ruleTable.firstChild.childNodes[ruleIndex+1].childNodes[1].style.color = "black";
								}
							}
						}
						editClassWindowRow.childNodes[0].firstChild.data = editClassWindow.document.getElementById("class_name").value;
						editClassWindowRow.childNodes[1].firstChild.data = editClassWindow.document.getElementById("percent_bandwidth").value + "%";
						editClassWindowRow.childNodes[2].firstChild.data = editClassWindow.document.getElementById("min_radio1").checked == true ? "zero" : editClassWindow.document.getElementById("min_bandwidth").value + " kbit/s";
						editClassWindowRow.childNodes[3].firstChild.data = editClassWindow.document.getElementById("max_radio1").checked == true ? "unlimited" : editClassWindow.document.getElementById("max_bandwidth").value + " kbit/s";



						editClassWindow.close();
					}
				}

				editClassWindow.moveTo(xCoor,yCoor);
				editClassWindow.focus();
				update_done = true;
			}
		}
		if(update_done == false)
		{
			setTimeout( "runOnClassEditorLoaded()", 250);
		}
	}

	runOnClassEditorLoaded();


}

function parseRuleMatchCriteria(matchText)
{
	splitText = matchText.split(/[\t ]*,[\t ]*/);
	criteria = [];

	possibleCriteria = ["Source: (.*)", "Source Port: (.*)", "Destination: (.*)", "Destination Port: (.*)", "Maximum Packet Length: (.*) bytes", "Minimum Packet Length: (.*) bytes",  "Transport Protocol: (.*)", "Connection bytes: (.*) MBytes", "Application Protocol: (.*)"];

	for(possibleCriteriaIndex=0; possibleCriteriaIndex < possibleCriteria.length; possibleCriteriaIndex++)
	{
		criterionRegExp = new RegExp(possibleCriteria[possibleCriteriaIndex]);
		found = false;
		for(splitIndex = 0; splitIndex < splitText.length && (!found); splitIndex++)
		{
			test=splitText[splitIndex].match(criterionRegExp);

			if(test !=null)
			{
				found = true;
				criteria.push(test[1]);
			}
		}
		if(!found)
		{
			criteria.push("");
		}
	}

	return criteria;
}

/* qosmon congestion monitor status */

var updateInProgress;
function initializeqosmon()
{
	updateInProgress = false;
	setInterval("updateqosmon()", 500);
}


function updateqosmon()
{

	if(!updateInProgress)
	{
		updateInProgress = true;
		var commands="cat /tmp/qosmon.status"
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var Lines = req.responseText.split("\n");
                             
                            if (Lines[0].substr(0,6) == "State:") {
                              document.getElementById("qstate").innerHTML = Lines[0];
   		                document.getElementById("qllimit").innerHTML = Lines[1];
   		                document.getElementById("qollimit").innerHTML = Lines[2];
   		                document.getElementById("qload").innerHTML = Lines[3];
   		                document.getElementById("qpinger").innerHTML = Lines[4];
   		                document.getElementById("qpingtime").innerHTML = Lines[5];
   		                document.getElementById("qpinglimit").innerHTML = Lines[6];
   		                document.getElementById("qactivecnt").innerHTML = Lines[7];
                            } else {

                              if (Lines[0].substr(0,25) == "cat: can't open '/tmp/qos") {
                                 document.getElementById("qstate").innerHTML = "State: Disabled*";
   		                   document.getElementById("qpinger").innerHTML = "Ping: Off";
                              }
                            }

				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}


