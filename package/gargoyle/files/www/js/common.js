/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
function setControlsEnabled(enabled, showWaitMessage, waitText)
{
	var dark = document.getElementById("darken");
	var msg  = document.getElementById("wait_msg");
	if (!enabled)
	{

		var totalHeight="100%";
		var totalWidth="100%";
		if(document.body.parentNode.scrollHeight)
		{
			totalHeight = document.body.parentNode.scrollHeight + "px";
		}
		else if(document.height)
		{
			totalHeight = document.height + "px";
		}
		if(document.body.parentNode.scrollWidth)
		{
			totalWidth  = document.body.parentNode.scrollWidth + "px";
		}
		else if(document.width)
		{
			totalWidth = document.width + "px";
		}

		var viewportHeight;
		var vewportWidth;
		if(self.innerHeight)
		{
			viewportHeight = window.innerHeight;
			viewportWidth = window.innerWidth;
		}
		else if(document.documentElement && document.documentElement.clientHeight)
		{
			viewportHeight = document.documentElement.clientHeight;
			viewportWidth = document.documentElement.clientWidth;
		}
		else if(document.body)
		{
			viewportHeight = document.body.clientHeight;
			viewportWidth = document.body.clientWidth;
		}

		var leftOffset = Math.floor((viewportWidth-300)/2);
		var topOffset  = Math.floor((viewportHeight-150)/2); 
		var is_ie = false;
		if(document.all)
		{
			is_ie = true;
		}
		if(is_ie)
		{
			var di = document.getElementById("d_iframe");
			di.style.display="block";
			di.style.width=totalWidth;
			di.style.height=totalHeight;

			var dm = document.getElementById("m_iframe");
			dm.style.display="block";
			dm.style.width="300px";
			dm.style.height="150px";

			msg.style.position="absolute";
			topOffset  = topOffset  + document.documentElement.scrollTop;
			leftOffset = leftOffset + document.documentElement.scrollLeft;
		}

		dark.style.height=totalHeight;
		dark.style.width=totalWidth;



		msg.style.left = leftOffset >= 0 ? leftOffset+"px" : "0px";
		msg.style.top  = topOffset >=0 ? topOffset+"px" : "0px";

		
		dark.style.display="block";

		if(showWaitMessage)
		{
			msg.style.display= "block";
			if(waitText != null)
			{
				document.getElementById("wait_txt").firstChild.data = waitText;
			}
		}
	}
	else
	{
		dark.style.display="none";
		msg.style.display= "none";
	}
}


function getRequestObj()
{
	var req;
	try
	{

		// standards compliant browsers
		req = new XMLHttpRequest();
	}
	catch (ex)
	{
		// MicroShit Browsers
		try
		{
			req = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (ex) 
		{
			try
			{
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} 
			catch (ex)
			{
				// Browser is not Ajax compliant
				return false;
			}
		}
	}
	return req;
}

function runAjax(method, url, params, stateChangeFunction)
{
	var req = getRequestObj();
	if(req)
	{
		req.onreadystatechange = function()
		{
			stateChangeFunction(req);
		}

		if(method == "POST")
		{
			//for some reason we need at least one character of data, so use a space if params == null
			params = (params == null) ? " " : params;
			
			req.open("POST", url, true);
			req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			req.setRequestHeader("Content-length", params.length);
			req.setRequestHeader("Connection", "close");
			req.send(params);
		}
		else if(method == "GET")
		{
			req.open("GET", url + "?" + params, true);
			req.send(null);
		}
	}
}

// The way we store keys is massively inefficient, but
// there are generally few enough keys defined that
// the O(N) time we're using doesn't impact performance.
//
// This implementation is necessary so we can control order
// of sections, i.e. sections will be added in the order
// they are defined with set method
function UCIContainer()
{
	this.keys = new Array();
	this.values = new Array();

	this.set = function(pkg, section, option, value)
	{
		var next_key = pkg + "\." + section;
	       	if(option != null && option != "" )
		{
			next_key = next_key + "\." + option;
		}
		if(this.values[next_key] != null)
		{
			this.values[next_key] = value;
		}
		else
		{
			this.keys.push(next_key);
			this.values[next_key] = value;
		}
	}

	this.get = function(pkg, section, option)
	{
		
		var next_key = pkg + "\." + section;
		if(option != null && option != '')
		{
			next_key = next_key + "\." + option;
		}
		var value = this.values[next_key];
		return value != null ? value : '';
	}
	this.removeAllSectionsOfType = function(pkg, type)
	{
		var removeSections = this.getAllSectionsOfType(pkg, type);
		var rmIndex=0;
		for(rmIndex=0; rmIndex < removeSections.length; rmIndex++)
		{
			this.removeSection(pkg, removeSections[rmIndex]);
		}
	}
	this.getAllSectionsOfType = function(pkg, type)
	{
		var matches = new Array();
		for (keyIndex in this.keys)
		{
			key = this.keys[keyIndex];
			if(key.match(pkg) && key.match(/^[^\.]+\.[^\.]+$/))
			{
				if(this.values[key] == type)
				{
					var section = key.match(/^[^\.]+\.([^\.]+)$/)[1];
					matches.push(section);
				}
			}
		}
		return matches;
	}
	this.getAllSections = function(pkg)
	{
		var matches = new Array();
		for (keyIndex in this.keys)
		{
			key = this.keys[keyIndex];
			if(key.match(pkg) && key.match(/^[^\.]+\.[^\.]+$/))
			{
				var section = key.match(/^[^\.]+\.([^\.]+)$/)[1];
				matches.push(section);
			}
		}
		return matches;
	}

	this.remove = function(pkg, section, option)
	{
		var removeKey = pkg + "\." + section;
	       	if(option != "")
		{
			removeKey = removeKey + "\." + option;
		}
		var value = this.values[removeKey] == null ? '' : this.values[removeKey] ;
		if(value != null)
		{
			this.values[removeKey] = null;
			var newKeys = [];
			while(this.keys.length > 0)
			{
				var nextKey = this.keys.shift();
				if(nextKey != removeKey){ newKeys.push(nextKey); }
			}
			this.keys = newKeys;
		}
		return value;
	}
	this.removeSection = function(pkg, section)
	{
		removeKeys = new Array();
		sectionDefined = false;
		for (keyIndex in this.keys)
		{
			key = this.keys[keyIndex];
			testExp = new RegExp(pkg + "\\." + section + "\\.");
			if(key.match(testExp))
			{
				var splitKey = key.split("\.");
				removeKeys.push(splitKey[2]);
			}
			if(key == pkg + "." + section)
			{
				sectionDefined = true;
			}

		}
		for (rkIndex in removeKeys)
		{
			this.remove(pkg, section, removeKeys[rkIndex]);
		}
		if(sectionDefined)
		{
			this.remove(pkg, section, "");
		}
	}

	this.clone = function()
	{
		var copy = new UCIContainer();
		var keyIndex = 0;
		for(keyIndex = 0; keyIndex < this.keys.length; keyIndex++)
		{
			var key = this.keys[keyIndex];
			var splitKey = key.match(/^([^\.]+)\.([^\.]+)\.([^\.]+)$/);
			if(splitKey == null)
			{
				splitKey = key.match(/^([^\.]+)\.([^\.]+)$/);
				if(splitKey != null)
				{
					splitKey.push("");
				}
				else
				{
					//should never get here -- if problems put debugging code here
				}
			}
			copy.set(splitKey[1], splitKey[2], splitKey[3], this.values[key]);
		}
		return copy;
	}

	this.print = function()
	{
		var str="";
		var keyIndex=0;
		for(keyIndex=0; keyIndex < this.keys.length; keyIndex++)
		{
			var key = this.keys[keyIndex]
			str=str+ "\n" + key + " = \"" + this.values[key] + "\"";
		}
		return str;
	}

	// sections are printed in the same order they were added (with the set method)
	this.getScriptCommands = function(oldSettings)
	{
		var commandArray = new Array();
		
		var keyIndex=0;	
		for(keyIndex=0; keyIndex < oldSettings.keys.length; keyIndex++)
		{
			var key = oldSettings.keys[keyIndex];
			var oldValue = oldSettings.values[key];
			var newValue = this.values[key];
			if((newValue == null || newValue == '') && (oldValue != null && oldValue !=''))
			{
				commandArray.push( "uci del " + key);
			}
		}

		for(keyIndex=0; keyIndex < this.keys.length; keyIndex++)
		{
			var key = this.keys[keyIndex];
			var oldValue = oldSettings.values[key];
			var newValue = this.values[key];
			if(oldValue != newValue && (newValue != null && newValue !=''))
			{
				commandArray.push( "uci set " + key + "=\'" + newValue + "\'" );
			}
		}

		commandArray.push("uci commit");
		return commandArray.join("\n");
	}
}




function getParameterDefinition(parameter, definition)
{
	return(fullEscape(parameter) + "=" + fullEscape(definition));
}


function fullEscape(str)
{
	str = escape(str);
	var otherEscape = [ '*', '@', '-', '_', '+', '.', '/' ];
	var otherEscaped= [ '2A','40','2D','5F','2B','2E','2F'];
	for(oeIndex=0; oeIndex < otherEscape.length; oeIndex++)
	{
		var splitStr = str.split( otherEscape[oeIndex] );
		if(splitStr.length > 1)
		{
			str = splitStr.join( "%" + otherEscaped[oeIndex] );
		}
	}
	return str;
}




function setChildText(parentId, text)
{
	parentElement = document.getElementById(parentId);
	if(parentElement.firstChild != null)
	{
		parentElement.removeChild(parentElement.firstChild);
	}
	parentElement.appendChild(document.createTextNode(text));
}

function createInput(type, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	try
	{
		inp = controlDocument.createElement('input');
		inp.type = type;
	}
	catch(e)
	{
		inp = controlDocument.createElement('<input type="' + type + '" />');
	}
	return inp;
}

function trueAndVisible(elementId, visibilityId)
{
	return (document.getElementById(elementId).checked && document.getElementById(visibilityId).style.display != 'none');
}

function getDhcpSection(uciData)
{
	var allDhcpSections = uciData.getAllSections("dhcp");
	var dhcpSection = allDhcpSections.length > 0 ? allDhcpSections[0] : "cfg1";
	for(dsecIndex=0; dsecIndex < allDhcpSections.length; dsecIndex++)
	{
		if(uciData.get("dhcp", allDhcpSections[dsecIndex], "interface") == "lan")
		{
			dhcpSection = allDhcpSections[dsecIndex];
		}
	}
	return dhcpSection;
}


function getWirelessMode(uciTest)
{
	var wifiSections = uciTest.getAllSections("wireless");
	var deviceSections = uciTest.getAllSectionsOfType("wireless", "wifi-device");
	var wifiDevice = deviceSections[0];

	var wifiCfg2="";
	var wifiCfg3="";
	if(wifiSections.length >= 2)
	{
		wifiCfg2 = wifiSections[1];
	}
	if(wifiSections.length >= 3)
	{
		wifiCfg3 = wifiSections[2];
	}
	var cfg2mode=uciTest.get("wireless", wifiCfg2, "mode");
	var cfg3mode=uciTest.get("wireless", wifiCfg3, "mode");
	var p = cfg2mode != '' && cfg3mode != '' ? '+' : '';
	var cfgMode= cfg3mode == 'ap' && cfg2mode != 'ap' ? cfg3mode + p + cfg2mode : cfg2mode + p + cfg3mode;

	var wirelessIsDisabled= uciTest.get("wireless", wifiDevice, "disabled") == "1" || cfg2mode == '';
	
	var wirelessMode= wirelessIsDisabled ? 'disabled' : cfgMode;
	return wirelessMode;
}


function setDescriptionVisibility(descriptionId, defaultDisplay, displayText, hideText)
{
	defaultDisplay = (defaultDisplay == null) ? "inline" : defaultDisplay;
	displayText = (displayText == null) ? "More Info" : displayText;
	hideText = (hideText == null) ? "Hide Text" : hideText;

	var ref = document.getElementById( descriptionId + "_ref" );
	var txt = document.getElementById( descriptionId + "_txt" );
	var command = "uci set gargoyle.help." + descriptionId + "=";
	if(ref.firstChild.data == displayText)
	{
		txt.style.display=defaultDisplay;
		ref.firstChild.data = hideText;
		command = command + "1\n";
	}
	else
	{
		txt.style.display="none";
		ref.firstChild.data = displayText;
		command = command + "0\n";
	}

	// we don't wait/notify user on completion so update seems instant
	var param = getParameterDefinition("commands", command);
	runAjax("POST", "utility/run_commands.sh", param, function(){ return 0; }); 
}

function initializeDescriptionVisibility(testUci, descriptionId, defaultDisplay, displayText, hideText)
{
	defaultDisplay = (defaultDisplay == null) ? "inline" : defaultDisplay;
	displayText = (displayText == null) ? "More Info" : displayText;
	hideText = (hideText == null) ? "Hide Text" : hideText;

	var descLinkText = displayText;
	var descDisplay = "none";
	if(testUci.get("gargoyle", "help", descriptionId) == "1")
	{
		descLinkText = hideText
		descDisplay = defaultDisplay;
	}
	document.getElementById(descriptionId + "_ref").firstChild.data = descLinkText;
	document.getElementById(descriptionId + "_txt").style.display = descDisplay;
}


function getSubnetRange(mask, ip)
{
	//first we need to get size of subnet
	var splitMask = mask.split(".");
	if(splitMask.length != 4)
	{
		return [];
	}
	var masks = [ "255","254","252","248","240","224","192","128", "0" ];
	var bits  = [ 0,  1,  2,  3,  4,  5,  6,  7,   8 ];
	var subnetBits = 0;
	while(splitMask.length > 0)
	{
		var nextPart = splitMask.shift();
		var nextBits = -1;
		for(testIndex=0 ; testIndex < 9 && nextBits < 0; testIndex++)
		{
			nextBits = masks[testIndex] == nextPart ? bits[testIndex] : nextBits;
		}
		if(nextBits < 0)
		{
			return [];
		}
		subnetBits = subnetBits + nextBits;
	}
	
	var subnetLength = Math.pow(2, subnetBits);
	var testIpEnd = parseInt( (ip.split("."))[3] );

	var subnetStart = 0;
	while(subnetStart + subnetLength < testIpEnd)
	{
		subnetStart = subnetStart + subnetLength;
	}
	return [subnetStart, (subnetStart + subnetLength - 1) ];
}

function rangeInSubnet(mask, ip, start, end)
{
	var range = getSubnetRange(mask, ip);
	var subnetStart = range[0];
	var subnetEnd = range[1];

	if(subnetStart != null && subnetEnd != null)
	{
		if(subnetStart <= start && subnetEnd >= end)
		{
			return true;
		}
	}
	return false;
}


function proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, fieldDocument )
{
	fieldDocument = fieldDocument == null ? document : fieldDocument;

	var errorArray= new Array();
	for (idIndex in inputIds)
	{
		isVisible = true;
		if(visibilityIds != null)
		{
			if(visibilityIds[idIndex] != null)
			{
				visibilityElement = fieldDocument.getElementById(visibilityIds[idIndex]);
				isVisible = visibilityElement.style.display == 'none' || visibilityElement.disabled == true ? false : true;
			}
		}
		if(isVisible)
		{
			input = fieldDocument.getElementById(inputIds[idIndex]);
			
			f = functions[idIndex];
			proofreadText(input, f, validReturnCodes[idIndex]);

			if(f(input.value) != validReturnCodes[idIndex])
			{
				labelStr = fieldDocument.getElementById(labelIds[idIndex]).firstChild.data;
				labelStr = labelStr.replace(/:/, "");
				errorArray.push("There is an error in " + labelStr);
			}
		}
	}
	return errorArray;
}

function parseBytes(bytes, units)
{
	var parsed;
	units = units != "KBytes" && units != "MBytes" && units != "GBytes" && units != "TBytes" ? "mixed" : units;
	if( (units == "mixed" && bytes > 1024*1024*1024*1024) || units == "TBytes")
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024*1024)) + " TBytes";
	}
	else if( (units == "mixed" && bytes > 1024*1024*1024) || units == "GBytes")
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024)) + " GBytes";
	}
	else if( (units == "mixed" && bytes > 1024*1024) || units == "MBytes" )
	{
		parsed = truncateDecimal(bytes/(1024*1024)) + " MBytes";
	}
	else
	{
		parsed = truncateDecimal(bytes/(1024)) + " KBytes";
	}
	
	return parsed;
}
function truncateDecimal(dec)
{
	result = "" + ((Math.floor(dec*1000))/1000);
	
	//make sure we have exactly three decimal places so 
	//results line up properly in table presentation
	decMatch=result.match(/.*\.(.*)$/);
	if(decMatch == null)
	{
		result = result + ".000"
	}
	else 
	{
		if(decMatch[1].length==1)
		{
			result = result + "00";
		}
		else if(decMatch[1].length==2)
		{
			result = result + "0";
		}
	}
	return result;
}

function enableAssociatedField(checkbox, associatedId, defaultValue, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;

	element=controlDocument.getElementById(associatedId);
	setElementEnabled(element, checkbox.checked, defaultValue);
}

function setElementEnabled(element, enabled, defaultValue)
{
	if(enabled)
	{
		element.readonly=false;
		element.disabled=false;
		if(element.type == "text")
		{
			element.style.color="#000000";
			element.className="" ;
		}
		if(element.type == "select-one" || element.type == "select-multiple")
		{
			element.className="";
		}
		if(element.type == "button")
		{
			element.className="default_button";
		}
	}
	else
	{
		element.disabled=true;
		if(element.type == "text")
		{
			element.value=defaultValue;
			element.style.color="#AAAAAA";
			element.className="text_disabled";
		}
		if(element.type == "select-one" || element.type == "select-multiple")
		{
			setSelectedValue(element.id, defaultValue, element.ownerDocument);
			element.className="select_disabled";
		}
		if(element.type == "button")
		{
			element.className="default_button_disabled";
		}
	}
}



function getSelectedValue(selectId, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	selectedIndex = controlDocument.getElementById(selectId).selectedIndex;
	selectedValue = "";
	if(selectedIndex >= 0)
	{
		selectedValue= controlDocument.getElementById(selectId).options[ controlDocument.getElementById(selectId).selectedIndex ].value;
	}
	return selectedValue;

}

function getSelectedText(selectId, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	selectedIndex = controlDocument.getElementById(selectId).selectedIndex;
	selectedText = "";
	if(selectedIndex >= 0)
	{
		selectedText= controlDocument.getElementById(selectId).options[ controlDocument.getElementById(selectId).selectedIndex ].text;
	}
	return selectedText;

}
function setSelectedValue(selectId, selection, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	selectElement = controlDocument.getElementById(selectId);
	selectionFound = false;
	for(optionIndex = 0; optionIndex < selectElement.options.length && (!selectionFound); optionIndex++)
	{
		selectionFound = (selectElement.options[optionIndex].value == selection);
		if(selectionFound)
		{
			selectElement.selectedIndex = optionIndex;
		}
	}
	if(!selectionFound && selectElement.options.length > 0 && selectElement.selectedIndex < 0)
	{
		selectElement.selectedIndex = 0;
	}
}



function setSelectedText(selectId, selection, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	selectElement = controlDocument.getElementById(selectId);
	selectionFound = false;
	for(optionIndex = 0; optionIndex < selectElement.options.length && (!selectionFound); optionIndex++)
	{
		selectionFound = (selectElement.options[optionIndex].text == selection);
		if(selectionFound)
		{
			selectElement.selectedIndex = optionIndex;
		}
	}
	if(!selectionFound && selectElement.options.length > 0 && selectElement.selectedIndex < 0)
	{
		selectElement.selectedIndex = 0;
	}
}

function addOptionToSelectElement(selectId, optionText, optionValue, before, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;

	option = controlDocument.createElement("option");
	option.text=optionText;
	option.value=optionValue;
	
	//FUCK M$ IE, FUCK IT UP THE ASS WITH A BASEBALL BAT.  A BIG WOODEN ONE. WITH SPLINTERS.
	try
	{
		controlDocument.getElementById(selectId).add(option, before);
	}
	catch(e)
	{
		if(before == null)
		{
			controlDocument.getElementById(selectId).add(option);
		}
		else
		{
			controlDocument.getElementById(selectId).add(option, before.index);	
		}
	}
}
function removeOptionFromSelectElement(selectId, optionText, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	selectElement = controlDocument.getElementById(selectId);
	selectionFound = false;
	for(optionIndex = 0; optionIndex < selectElement.options.length && (!selectionFound); optionIndex++)
	{
		selectionFound = (selectElement.options[optionIndex].text == optionText);
		if(selectionFound)
		{
			selectElement.remove(optionIndex);
		}
	}
}

function removeAllOptionsFromSelectElement(selectElement)
{
	while(selectElement.length > 0)
	{
		try { selectElement.remove(0); } catch(e){}
	}
}

function setAllowableSelections(selectId, allowableValues, allowableNames)
{
	if(allowableNames != null && allowableValues != null)
	{
		currentSelection=getSelectedValue(selectId);
		removeAllOptionsFromSelectElement(document.getElementById(selectId));
		for(addIndex=0; addIndex < allowableValues.length; addIndex++)
		{
			addOptionToSelectElement(selectId, allowableNames[addIndex], allowableValues[addIndex]);
		}
		setSelectedValue(selectId, currentSelection); //restore original settings if still valid
	}
}

function setVariableFromValue(params)
{
	elementId    = params[0];
	visibilityId = params[1];
	uci          = params[2];
	pkg          = params[3];
	section      = params[4];
	option       = params[5];
	setIfBlank   = params[6];

	var isVisible = true;
	if(visibilityId != null)
	{
		isVisible = document.getElementById(visibilityId).style.display == 'none' ? false : true;
	}


	if(isVisible == true)
	{
		value = document.getElementById(elementId).value;
		if(value != '' || setIfBlank == true)
		{
			uci.set(pkg, section, option, value);
		}
	}
}
function setVariableFromModifiedValue(params)
{
	elementId    = params[0];
	visibilityId = params[1];
	uci          = params[2];
	pkg          = params[3];
	section      = params[4];
	option       = params[5];
	setIfBlank   = params[6];
	modFunction  = params[7];

	isVisible = true;
	if(visibilityId != null)
	{
		isVisible = document.getElementById(visibilityId).style.display == 'none' ? false : true;
	}
	if(isVisible==true)
	{
		value = document.getElementById(elementId).value;
		if(value != '' || setIfBlank == true)
		{
			uci.set(pkg, section, option, modFunction(value));
		}
	}	
}
function setVariableFromCombined(params)
{
	elementIds	 = params[0]
	visibilityId     = params[1];
	uci              = params[2];
	pkg              = params[3];
	section          = params[4];
	option           = params[5];
	setIfBlank       = params[6];
	combineFunction  = params[7];
	
	isVisible = true;
	if(visibilityId != null)
	{
		isVisible = document.getElementById(visibilityId).style.display == 'none' ? false : true;
	}
	if(isVisible==true)
	{
		values = new Array();
		for (idIndex in elementIds)
		{
			values.push(document.getElementById(elementIds[idIndex]).value);
		}
		if(value != '' || setIfBlank == true)
		{
			uci.set(pkg, section, option, combineFunction(values));
		}
	}
}

function setVariableFromConcatenation(params)
{
	elementIds   = params[0];
	visibilityIds= params[1];
	uci          = params[2];
	pkg          = params[3];
	section      = params[4];
	option       = params[5];
	setIfBlank   = params[6];

	concat = '';
	nextIdIndex = 0;
	while(nextIdIndex < elementIds.length)
	{
		idVisible = true;
		if(visibilityIds != null)
		{
			nextVisId = visibilityIds[nextIdIndex];
			if(nextVisId!= null)
			{
				idVisible = document.getElementById(nextVisId).style.display == 'none' ? false : true;
			}
		}
		value = document.getElementById(elementIds[nextIdIndex]).value;
		if(idVisible==true && value != '')
		{
			value = document.getElementById(elementIds[nextIdIndex]).value;
			endSpace = nextIdIndex < elementIds.length - 1 ? " " : "";
			concat = concat + value + endSpace;
		}
		nextIdIndex++;
	}
	if(concat != '' || setIfBlank == true)
	{
		uci.set(pkg, section, option, concat);
	}
}
function setVariableConditionally(params)
{
	elementId    = params[0]; 
	visibilityId = params[1];
	uci          = params[2];
	pkg          = params[3];
	section      = params[4];
	option       = params[5];
	testFunction = params[6];
	useValueFromElement = params[7];
	alternateValue =      params[8];
	
	isVisible = true;
	if(visibilityId != null)
	{
		isVisible = document.getElementById(visibilityId).style.display == 'none' ? false : true;
	}
	if(isVisible==true)
	{
		value = useValueFromElement == true ? document.getElementById(elementId).value : alternateValue;
		
		if(testFunction(value))
		{
			uci.set(pkg, section, option, value);
		}
	}
}


function setVariables(inputIds, visibilityIds, uci, pkgs, sections, options, setFunctions, additionalParameters)
{
	for (idIndex in inputIds)
	{
		nextId             = inputIds[idIndex];
		nextVisibilityId   = visibilityIds[idIndex];
		nextPkg            = pkgs[idIndex];
		nextSection        = sections[idIndex];
		nextOption         = options[idIndex];
		nextParams         = additionalParameters[idIndex];
		nextFunction       = setFunctions[idIndex];
		if(isArray(nextParams))
		{
			fullList = [nextId, nextVisibilityId, uci, nextPkg, nextSection, nextOption];
			for (pIndex in nextParams)
			{
				fullList.push(nextParams[pIndex]);
			}
			nextFunction(fullList);

		}
		else
		{
			nextFunction([nextId, nextVisibilityId, uci, nextPkg, nextSection, nextOption, nextParams]);
		}
	}
}

function loadSelectedValueFromVariable(params)
{
	var elementId    = params[0];
	var uci          = params[1];
	var pkg          = params[2];
	var section      = params[3];
	var option       = params[4];
	var defaultValue = params[5];
	
	var v=uci.get(pkg, section, option);
	if(v != null && v != '')
	{
		setSelectedValue(elementId, v);
	}
	else if(defaultValue != null)
	{
		setSelectedValue(elementId, defaultValue);
	}

}

function loadValueFromVariable(params)
{

	var elementId    = params[0];
	var uci          = params[1];
	var pkg          = params[2];
	var section      = params[3];
	var option       = params[4];
	var defaultValue = params[5];
	
	var v=uci.get(pkg, section, option);
	var e=document.getElementById(elementId);
	if(v != null && v != '')
	{
		e.value = v;
	}
	else if(defaultValue != null)
	{
		e.value = defaultValue;
	}
}
function loadValueFromVariableMultiple(params)
{
	var multiple = params[6];
	loadValueFromVariable(params);
	var e=document.getElementById(params[0]);
	e.value=e.value*multiple;
}
function loadValueFromModifiedVariable(params)
{
	var elementId    = params[0];
	var uci          = params[1];
	var pkg          = params[2];
	var section      = params[3];
	var option       = params[4];
	var defaultValue = params[5];
	var modificationFunction = params[6];
	
	var v=modificationFunction(uci.get(pkg, section, option));
	var e=document.getElementById(elementId);
	if(v != null && v != '')
	{
		e.value = v;
	}
	else if(defaultValue != null)
	{
		e.value = defaultValue;
	}	

}
function loadValueFromVariableAtIndex(params)
{
	var elementId    = params[0];
	var uci          = params[1];
	var pkg          = params[2];
	var section      = params[3];
	var option       = params[4];
	var defaultValue = params[5];
	var index        = params[6];
	
	var vStr=uci.get(pkg, section, option);
	var vSplit = vStr.split(/[,\t ]+/);
	

	var v;
	if(index < vSplit.length)
	{
		v=vSplit[index];
	}
	else
	{
		v = '';
	}
	
	var e=document.getElementById(elementId);
	if(v != null && v != '')
	{
		e.value = v;
	}
	else if(defaultValue != null)
	{
		e.value = defaultValue;
	}
}
function loadChecked(params)
{
	var elementId    = params[0];
	var uci          = params[1];
	var pkg          = params[2];
	var section      = params[3];
	var option       = params[4];
	var test         = params[5];
	document.getElementById(params[0]).checked = test(uci.get(pkg,section,option));
}



function isArray(obj)
{
	return (obj.constructor.toString().indexOf('Array') >= 0 || obj instanceof Array ? true : false);
}

function loadVariables(uci, varIds, varPkgs, varSections, varOptions, varParams, varFunctions)
{	
	for (idIndex in varIds)
	{
		nextId      = varIds[idIndex];
		nextPkg     = varPkgs[idIndex];
		nextSection = varSections[idIndex];
		nextOption  = varOptions[idIndex];
		nextParams  = varParams[idIndex];
		nextFunc    = varFunctions[idIndex];
		if(isArray(nextParams))
		{
			fullList = [nextId, uci, nextPkg, nextSection, nextOption]
			for (pIndex in nextParams)
			{
				fullList.push(nextParams[pIndex]);
			}
			nextFunc(fullList);

		}
		else
		{
			nextFunc([nextId, uci, nextPkg, nextSection, nextOption, nextParams]);
		}
	}
}	
function loadValueFromMultipleVariables(params)
{
	var elementId   = params[0];
	var uci          = params[1];
	var pkgs         = params[2];
	var sections     = params[3];
	var options      = params[4];
	var combineFunc   = params[5];
	var defaultValue = params[6];

	var values = new Array();
	for (pkgIndex in pkgs)
	{
		values.push(uci.get(pkgs[pkgIndex], sections[pkgIndex], options[pkgIndex]));
	}
	var combined = combineFunc(values);
	var e=document.getElementById(elementId);
	if(combined != null && combined != '')
	{
		e.value = combined;
	}
	else if(defaultValue != null)
	{
		e.value = defaultValue;
	}

}

function setVisibility(ids, visibility, defaultDisplays)
{
	for (index in ids)
	{
		element = document.getElementById(ids[index]);
		if(visibility[index] == 0)
		{
			element.style.display = "none";
		}
		else
		{
			if(defaultDisplays)
			{
				element.style.display = defaultDisplays[index];
			}
			else
			{
				element.style.display = "block";
			}
		}
	}
}


function validateIP(address)
{
	//return codes:
	//0 == valid IP
	//1 = 0.0.0.0
	//2 = 255.255.255.255
	//3 = some other mixture of 255/0s 
	//4 = value >255 in at least one field
	//5 = improper format

	var errorCode = 0;
	if(address == "0.0.0.0")
	{
		errorCode = 1;
	}
	else if(address == "255.255.255.255")
	{
		errorCode = 2;
	}
	else
	{
		var ipFields = address.match(/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/);
		if(ipFields == null)
		{
			errorCode = 5;
		}
		else
		{
			for(field=1; field <= 4; field++)
			{
				if(ipFields[field] > 255)
				{
					errorCode = 4;
				}
				if(ipFields[field] == 255)
				{
					errorCode = 3;
				}
			}
		}
	}
	return errorCode;
}

function validateMac(mac)
{
	var errorCode = 0;
	var macFields = mac.split(/:/);
	if(macFields.length != 6)
	{
		errorCode = 2;
	}
	else
	{
		for(fieldIndex=0; fieldIndex < 6 && errorCode == 0; fieldIndex++)
		{
			field = macFields[fieldIndex];
			if(field.match(/^[0123456789ABCDEFabcdef]{2}$/) == null)
			{
				errorCode = 1;
			}
		}
	}
	return errorCode;
}

function validateNumeric(num)
{
	var errorCode = num.match(/^[\d]+$/) == null ? 1 : 0;
	return errorCode;
}

function validateNumericRange(num, min, max)
{
	var errorCode = num.match(/^[\d]+$/) == null ? 1 : 0;
	if(errorCode == 0)
	{
		errorCode = num < min ? 2 : 0;
	}
	if(errorCode == 0)
	{
		errorCode = num > max ? 3 : 0;
	}
	return errorCode;
}

function validatePortOrPortRange(ports)
{
	var errorCode = 0;
	if(ports.match(/-/) != null)
	{
		var splitPorts=ports.split(/-/);

		if(splitPorts.length > 2)
		{
			errorCode =  5;
		}
		else
		{
			error1 = validateNumericRange(splitPorts[0], 1, 65535);
			error2 = validateNumericRange(splitPorts[1], 1, 65535);
			errorCode = error1 + (10*error2);
			if(errorCode == 0)
			{
				errorCode = splitPorts[1] - splitPorts[0] >= 0 ? 0 : 4;
			}
		}
	}
	else
	{
		errorCode = validateNumericRange(ports, 1, 65535);
	}
	return errorCode;
}


function validateNetMask(mask)
{
	//return codes:
	//0 = valid mask
	//1 = invalid digit
	//2 = invalid field order 
	//3 = fields > 255
	//4 = invalid format

	var errorCode = 0;
	var ipFields = mask.match(/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/);
	if(ipFields == null)
	{
		errorCode = 4;
	}
	else
	{
		previousField = 255;
		for(field=1; field <= 4; field++)
		{
			if(ipFields[field] > 255)
			{
				errorCode = 3;
			}
			if(previousField < 255 && ipFields[field] != 0 && errorCode < 2)
			{
				errorCode = 2;
			}
			if(	ipFields[field] != 255 && 
				ipFields[field] != 254 && 
				ipFields[field] != 252 && 
				ipFields[field] != 248 &&
				ipFields[field] != 240 && 
				ipFields[field] != 224 && 
				ipFields[field] != 192 &&
				ipFields[field] != 128 &&
				ipFields[field] != 0 &&
				errorCode <  1
			)
			{
				errorCode = 1;
			}

			previousField = ipFields[field];
		}
	}
	return errorCode;
}

//this is for source/destination in iptables
//netmask can optionally be specified with / after ip to indicate range
function validateIpRange(range)
{
	var valid = 1; //initially invalid, 0=valid, 1=invalid
	if(range.indexOf("/") > 0)
	{
		var split=range.split("/");
		if(split.length == 2)
		{
			var ipValid = validateIP(split[0]);
			var maskValid = validateNetMask(split[1]) == 0 || validateNumericRange(split[1],1,31) == 0 ? 0 : 1;
			valid = ipValid == 0 && maskValid == 0 ? 0 : 1;
		}	
	}
	else 
	{
		valid = validateIP(range);
	}
	return valid;
}


function validateLengthRange(text,min,max)
{
	var errorcode = 0;
	if(text.length < min)
	{
		errorcode = 1;
	}
	if(text.length > max)
	{
		errorcode = 2;
	}
	return errorcode;
}
function validateHex(text)
{
	var errorcode = 0;
	if(!text.match(/^[0123456789AaBbCcDdEeFf]*$/))
	{
		errorcode = 1;
	}
	return errorcode;
}

function proofreadLengthRange(input,min,max)
{
	var vlr = function(text){return validateLengthRange(text,min,max);};
	proofreadText(input, vlr, 0);
}
function proofreadIp(input)
{
	proofreadText(input, validateIP, 0);
}
function proofreadMask(input)
{
	proofreadText(input, validateNetMask, 0);
}
function proofreadIpRange(input)
{
	proofreadText(input, validateIpRange, 0);
}
function proofreadMac(input)
{
	proofreadText(input, validateMac, 0);
}
function proofreadNumeric(input)
{
	proofreadText(input, validateNumeric, 0);
}
function proofreadNumericRange(input, min, max)
{
	proofreadText(input, function(text){return validateNumericRange(text,min,max)}, 0);
}
function proofreadPortOrPortRange(input)
{
	proofreadText(input, validatePortOrPortRange, 0);
}
function proofreadText(input, proofFunction, validReturnCode)
{
	if(input.disabled != true)
	{
		input.style.color = (proofFunction(input.value) == validReturnCode) ? "black" : "red";
	}
}



function getEmbeddedSvgWindow(embeddedId)
{
	var embedElement = document.getElementById( embeddedId );
	var windowElement = null;

	try
	{
		var docElement = embedElement.getSVGDocument();
		windowElement = docElement.defaultView;
	}
	catch(ex1){}
	if(windowElement == null)
	{
		try { windowElement = embedElement.window; } catch(ex2){}
		if( windowElement == null)
		{
			try { windowElement = embedElement.getWindow(); } catch(ex3){}
		}
	}
	return windowElement;
}


function getBridgeSection(testUci)
{
	//all bridges will have either option wds=1, option mode=wds, or option client_bridge=1 in one of the wireless sections
	//in the case of broadcom, the client_bridge=1 doesn't do anything, but we put it there for convenience anyway.

	var allWirelessSections = uciOriginal.getAllSections("wireless");
	var wanDef = uciOriginal.get("network", "wan", "");
	var bridgeSection = "";
	var sectionIndex;
	for(sectionIndex=0; sectionIndex < allWirelessSections.length && bridgeSection == ""; sectionIndex++)
	{
		if( testUci.get("wireless", allWirelessSections[sectionIndex], "mode").toLowerCase() == "wds" && wanDef == "")
		{
			bridgeSection = allWirelessSections[sectionIndex];
		}
		else if( testUci.get("wireless", allWirelessSections[sectionIndex], "wds") == "1" && wanDef == "")
		{
			bridgeSection = allWirelessSections[sectionIndex];
		}
		else if( testUci.get("wireless", allWirelessSections[sectionIndex], "client_bridge") == "1")
		{
			bridgeSection = allWirelessSections[sectionIndex];
		}
	}
	return bridgeSection;
}

function isBridge(testUci)
{
	//is it a router or a bridge configuration?
	var bridgeTest = getBridgeSection(testUci) == "" ? false : true;
	return bridgeTest;
}

