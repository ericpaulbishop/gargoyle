/*
 * This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var UI=new Object(); //part of i18n
var TiZ=new Object(); //i18n timezones

window.onresize = function onresize()
{
	//in case this gets called while initializing page, you may get error if element isnt defined,
	//so use a try/catch
	try
	{
		if(document.getElementById("darken").style.display == "block")
		{
			setControlsEnabled(false, document.getElementById("wait_msg").style.display == "block");
		}
	}
	catch(e){}
}

// should be called by hooks
// main page code can just use regular resetData() function
// which should get called first
function addLoadFunction(func)
{
	var old_load = window.onload;
	window.onload = (typeof window.onload != 'function') ? func : function() { old_load() ; func() ; }
}


function setControlsEnabled(enabled, showWaitMessage, waitText)
{
	var dark = document.getElementById("darken");
	var msg  = document.getElementById("wait_msg");
	document.getElementById("wait_txt").firstChild.data = UI.waitText;
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
			totalWidth  = document.body.parentNode.scrollWidth;
			if(document.width)
			{
				totalWidth = document.width > totalWidth ? document.width : totalWidth;
			}
			totalWidth = totalWidth + "px";
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

	//let's be sneaky -- instead of adding setBrowserTimeCookie() to the saveChanges() function on every page
	//just add it to this function which almost always gets called just before we set parameters
	//This covers all instances not taken care of by the runAjax() function, e.g. when submitting a form
	setBrowserTimeCookie();
}

function setBrowserTimeCookie()
{
	var browserSecondsUtc = Math.floor( ( new Date() ).getTime() / 1000 );
	document.cookie="browser_time=" +browserSecondsUtc + "; path=/"; //don't bother with expiration -- who cares when the cookie was set? It just contains the current time, which the browser already knows
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

	//let's be sneaky -- instead of adding setBrowserTimeCookie() to the saveChanges() function on every page
	//add it to this function, which gets run on every ajax call.  This covers all instances not taken care of
	//by the setControlsEnabled() function
	setBrowserTimeCookie();
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
			//req.setRequestHeader("Content-length", params.length);
			//req.setRequestHeader("Connection", "close");
			req.send(params);
		}
		else if(method == "GET")
		{
			req.open("GET", url + "?" + params, true);
			req.send(null);
		}
	}
	return req;
}


function execute(cmd)
{
	var commands = cmd.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, UI.WaitSettings);
	
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			window.location.href=window.location.href;
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
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
	this.listOptions = new Array();


	this.createListOption = function(pkg,section,option,destroy_existing_nonlist)
	{
		destroy_existing_nonlist = destroy_existing_nonlist == null ? true : false;
		var  list_key = pkg + "\." + section + "\." + option;
		if( this.listOptions[ list_key ] != null )
		{
			return;
		}

		this.listOptions[ list_key ] = 1;
		if( this.values[list_key] != null )
		{
			var old = this.values[list_key];
			this.values[list_key] = (!destroy_existing_nonlist) && old != null ? [old] : [] ; 
		}
		else
		{
			this.keys.push(list_key);
			this.values[list_key] = [];
		}
	}
	this.set = function(pkg, section, option, value, preserveExistingListValues)
	{
		preserveExistingListValues = preserveExistingListValues == null ? false : preserveExistingListValues;
		var next_key = pkg + "\." + section;
	       	if(option != null && option != "" )
		{
			next_key = next_key + "\." + option;
		}
		if(this.values[next_key] != null)
		{
			if (this.listOptions[ next_key ] != null)
			{
				var set = this.values[next_key];
				while(set.length > 0 && (!preserveExistingListValues))
				{
					set.pop();
				}
				if( value instanceof Array )
				{
					var vi;
					for(vi=0; vi<value.length; vi++)
					{
						set.push( value[vi] );
					}
				}
				else
				{
					set.push(value);
				}
				this.values[next_key] = set;
			}
			else
			{
				this.values[next_key] = value;
			}
		}
		else
		{
			this.keys.push(next_key);
			if (this.listOptions[ next_key ] != null)
			{
				var set = [];
				if(value instanceof Array)
				{
					var setIndex;
					for(setIndex=0;setIndex < value.length; setIndex++)
					{
						set.push( value[setIndex] )
					}
				}
				else
				{
					set = [ value ]
				}
				this.values[next_key] = set
			}
			else
			{
				this.values[next_key] = value;
			}
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
	this.getAllOptionsInSection = function(pkg, section, includeLists)
	{
		includeLists = includeLists == null ? false : includeLists;
		var matches = new Array();
		for (keyIndex in this.keys)
		{
			var key = this.keys[keyIndex];
			var test = pkg + "." + section;
			if(key.match(test) && key.match(/^[^\.]+\.[^\.]+\.[^\.]+/) && (includeLists || this.listOptions[key] == null) )
			{
				var option = key.match(/^[^\.]+\.[^\.]+\.([^\.]+)$/)[1];
				matches.push(option);
			}
		}
		return matches;
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
		if( this.listOptions[ removeKey ] != null )
		{
			this.listOptions[ removeKey ] = null;
		}

		var value = this.values[removeKey];
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
		else
		{
			value = ''
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
			var val = this.values[key]
			if( this.listOptions[ key ] != null )
			{
				copy.listOptions[ key ] = 1;
			}

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
			copy.set(splitKey[1], splitKey[2], splitKey[3], val, true);
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
			if(this.values[key] instanceof Array )
			{
				str=str+ "\n" + key + " = \"" + this.values[key].join(",") + "\"";
			}
			else
			{
				str=str+ "\n" + key + " = \"" + this.values[key] + "\"";
			}
		}
		return str;
	}

	// sections are printed in the same order they were added (with the set method)
	this.getScriptCommands = function(oldSettings)
	{
		var commandArray = new Array();
		
		var listsWithoutUpdates = [];

		var keyIndex=0;	
		for(keyIndex=0; keyIndex < oldSettings.keys.length; keyIndex++)
		{
			var key = oldSettings.keys[keyIndex];
			var oldValue = oldSettings.values[key];
			var newValue = this.values[key];

			if( (oldValue instanceof Array && !(newValue instanceof Array)) || (newValue instanceof Array   && !(oldValue instanceof Array))  ) 
			{
				commandArray.push( "uci del " + key);
			}
			else if (oldValue instanceof Array && newValue instanceof Array)
			{
				var matches = oldValue.length == newValue.length;
				if(matches)
				{
					var sortedOld = oldValue.sort()
					var sortedNew = newValue.sort()
					var sortedIndex;
					for(sortedIndex=0; matches && sortedIndex <sortedOld.length; sortedIndex++)
					{
						matches = sortedOld[sortedIndex] == sortedNew[sortedIndex] ? true : false
					}
				}
				if(matches)
				{
					listsWithoutUpdates[key] = 1
				}
				else
				{
					commandArray.push( "uci del " + key);
				}

			}
			else if((newValue == null || newValue == '') && (oldValue != null && oldValue !=''))
			{
				commandArray.push( "uci del " + key);
			}
		}

		for(keyIndex=0; keyIndex < this.keys.length; keyIndex++)
		{
			var key = this.keys[keyIndex];
			var oldValue = oldSettings.values[key];
			var newValue = this.values[key];
			try
			{

				if( (oldValue instanceof Array) || (newValue instanceof Array) )
				{
					if(newValue instanceof Array)
					{
						if(listsWithoutUpdates[key] == null)
						{
							var vi;
							for(vi=0; vi< newValue.length ; vi++)
							{
								var nv = "" + newValue[vi] + "";
								commandArray.push( "uci add_list " + key + "=\'" + nv.replace(/'/, "'\\''") + "\'" );
							}
						}
					}
					else
					{
						newValue = "" + newValue + ""
						commandArray.push( "uci set " + key + "=\'" + newValue.replace(/'/, "'\\''") + "\'" );
					}
				}
				else if(oldValue != newValue && (newValue != null && newValue !=''))
				{		
					newValue = "" + newValue + ""
					commandArray.push( "uci set " + key + "=\'" + newValue.replace(/'/, "'\\''") + "\'" );
				}
			}
			catch(e)
			{
				alert("bad key = " + key + "\n");
			}
		}

		commandArray.push("uci commit");
		
		return commandArray.join("\n");
	}
}




function getParameterDefinition(parameter, definition)
{
	return(encodeURIComponent(parameter) + "=" + encodeURIComponent(definition));
}


function removeStringFromArray(arr, str)
{
	var arrIndex;
	var newArr = [];
	for(arrIndex=0;arrIndex<arr.length; arrIndex++)
	{
		var elFound = false;
		if(typeof(arr[arrIndex]) == "string" )
		{
			elFound = (arr[arrIndex] == str)
		}
		if(!elFound)
		{
			newArr.push(arr[arrIndex]);
		}
	}
	return newArr;
}


function setChildText(parentId, text, color, isBold, fontSize, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	parentElement = controlDocument.getElementById(parentId);
	if(parentElement != null)
	{
		if(color != null)
		{
			parentElement.style.color = color;
		}
		if(isBold != null)
		{
			parentElement.style.fontWeight = isBold ? "bold" : "normal";
		}
		if(fontSize != null)
		{
			parentElement.style.fontSize = fontSize;

		}
		while(parentElement.firstChild != null)
		{
			parentElement.removeChild(parentElement.firstChild);
		}

		text = text == null ? "" : text;
		var textParts = text.replace(/\&amp;/g, '&').split("\n");
		while(textParts.length > 0)
		{
			var txt = textParts.shift()
			parentElement.appendChild(controlDocument.createTextNode(txt));
			if(textParts.length > 0)
			{
				parentElement.appendChild(controlDocument.createElement('br'));

			}
		}
	}
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
	var deviceSections = uciTest.getAllSectionsOfType("wireless", "wifi-device");
	var validDevices = [];
	var di;
	for(di=0; di < deviceSections.length; di++)
	{
		var disabled = uciTest.get("wireless", deviceSections[di], "disabled");
		if(disabled == "0" || disabled == "")
		{
			validDevices[ deviceSections[di] ] = 1
		}
	}

	var ap = '';
	var other = '';
	var ifSections =  uciTest.getAllSectionsOfType("wireless", "wifi-iface");
	var ifi;
	for(ifi=0; ifi < ifSections.length; ifi++)
	{
		var dev  = uciTest.get("wireless", ifSections[ifi], "device");
		if( validDevices[ dev ]  == 1)
		{
			var mode = uciTest.get("wireless", ifSections[ifi], "mode");
			ap    = mode == "ap" ? mode : ap;
			other = mode == "ap" ? (uciTest.get("wireless",  ifSections[ifi], "wds") ? "wds" : other) : mode;
		}
	}

	
	var p = ap != '' && other != '' ? '+' : '';
	var wirelessMode = ap + p + other;
	var wirelessMode= wirelessMode == '' ? 'disabled' : wirelessMode;
	return wirelessMode;
}


function setDescriptionVisibility(descriptionId, defaultDisplay, displayText, hideText)
{
	defaultDisplay = (defaultDisplay == null) ? "inline" : defaultDisplay;
	displayText = (displayText == null) ? UI.MoreInfo : displayText;
	hideText = (hideText == null) ? UI.Hide : hideText;

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
	command = command + "\nuci commit\n";

	// we don't wait/notify user on completion so update seems instant
	var param = getParameterDefinition("commands", command)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	runAjax("POST", "utility/run_commands.sh", param, function(){ return 0; }); 
}

function initializeDescriptionVisibility(testUci, descriptionId, defaultDisplay, displayText, hideText)
{
	defaultDisplay = (defaultDisplay == null) ? "inline" : defaultDisplay;
	displayText = (displayText == null) ? UI.MoreInfo : displayText;
	hideText = (hideText == null) ? UI.Hide : hideText;

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
				labelStr = labelIds[idIndex] + "";
				if( fieldDocument.getElementById(labelIds[idIndex]) != null)
				{
					labelStr = fieldDocument.getElementById(labelIds[idIndex]).firstChild.data;
					labelStr = labelStr.replace(/:/, "");
				}
				else
				{
					alert("error in proofread: label with id " +  labelIds[idIndex] + " is not defined");
				}
				errorArray.push(UI.prfErr+" " + labelStr);
			}
		}
	}
	return errorArray;
}

function parseBytes(bytes, units, abbr, dDgt)
{
	var parsed;
	units = units != "KBytes" && units != "MBytes" && units != "GBytes" && units != "TBytes" ? "mixed" : units;
	spcr = abbr==null||abbr==0 ? " " : "";
	if( (units == "mixed" && bytes > 1024*1024*1024*1024) || units == "TBytes")
	{
		parsed = (bytes/(1024*1024*1024*1024)).toFixed(dDgt||3) + spcr + (abbr?UI.TB:UI.TBy);
	}
	else if( (units == "mixed" && bytes > 1024*1024*1024) || units == "GBytes")
	{
		parsed = (bytes/(1024*1024*1024)).toFixed(dDgt||3) + spcr + (abbr?UI.GB:UI.GBy);
	}
	else if( (units == "mixed" && bytes > 1024*1024) || units == "MBytes" )
	{
		parsed = (bytes/(1024*1024)).toFixed(dDgt||3) + spcr + (abbr?UI.MB:UI.MBy);
	}
	else
	{
		parsed = (bytes/(1024)).toFixed(dDgt||3) + spcr + (abbr?UI.KB:UI.KBy);
	}
	
	return parsed;
}

function parseKbytesPerSecond(kbytes, units)
{
	var parsed;
	units = units != "bytes/s" && units != "KBytes/s" && units != "MBytes/s" ? "mixed" : units;
	
	if( (units == "mixed" && kbytes > 1024) || units == "MBytes/s")
	{
		parsed = (kbytes/(1024)).toFixed(3) + " "+UI.MBs;
	}
	else
	{
		parsed = kbytes + " "+UI.KBs;
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
		if(element.type == "text" || element.type == "textarea")
		{
			element.style.color="#000000";
			element.className="" ;
		}
		else if(element.type == "select-one" || element.type == "select-multiple" || element.type == "select" )
		{
			element.className="";
		}
		else if(element.type == "button")
		{
			var activeClassName = element.className.replace(/_button.*$/, "_button");
			element.className=activeClassName;
		}
	}
	else
	{
		element.disabled=true;
		if(element.type == "text" || element.type == "textarea")
		{
			element.value=defaultValue;
			element.style.color="#AAAAAA";
			element.className="text_disabled";
		}
		else if(element.type == "select-one" || element.type == "select-multiple" || element.type == "select" )
		{
			setSelectedValue(element.id, defaultValue, element.ownerDocument);
			element.className="select_disabled";
		}
		else if(element.type == "button")
		{
			var activeClassName = element.className.replace(/_button.*$/, "_button");
			element.className= activeClassName + "_disabled";
		}
		else if(element.type == "file")
		{
			element.value=defaultValue;
		}
	}
}



function getSelectedValue(selectId, controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	if(controlDocument.getElementById(selectId) == null)
	{
		alert(UI.Err+": " + selectId + " "+UI.nex);
		return;
	}

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
	var controlDocument = controlDocument == null ? document : controlDocument;
	
	var selectElement = controlDocument.getElementById(selectId);
	if(selectElement == null){ alert(UI.Err+": " + selectId + " "+UI.nex); }

	var selectionFound = false;
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

function setAllowableSelections(selectId, allowableValues, allowableNames, controlDocument)
{
	if(controlDocument == null) { controlDocument = document; }

	var selectElement = controlDocument.getElementById(selectId);
	if(allowableNames != null && allowableValues != null && selectElement != null)
	{

		var doReplace = true;
		if(allowableValues.length == selectElement.options.length)
		{
			doReplace = false;
			for(optionIndex = 0; optionIndex < selectElement.options.length && (!doReplace); optionIndex++)
			{
				doReplace = doReplace || (selectElement.options[optionIndex].text != allowableNames[optionIndex]) || (selectElement.options[optionIndex].value != allowableValues[optionIndex]) ;
			}
		}
		if(doReplace)
		{
			currentSelection=getSelectedValue(selectId, controlDocument);
			removeAllOptionsFromSelectElement(selectElement);
			for(addIndex=0; addIndex < allowableValues.length; addIndex++)
			{
				addOptionToSelectElement(selectId, allowableNames[addIndex], allowableValues[addIndex], null, controlDocument);
			}
			setSelectedValue(selectId, currentSelection, controlDocument); //restore original settings if still valid
		}
	}
}

function setSingleChild(container, child)
{
	while(container.firstChild != null)
	{
		container.removeChild( container.firstChild);
	}
	container.appendChild(child);
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

function setVisibility(ids, visibility, defaultDisplays, controlDocument)
{
	if(controlDocument == null)
	{
		controlDocument = document;
	}
	for (index in ids)
	{
		element = controlDocument.getElementById(ids[index]);
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
	//3 = ends with 255 (actually, broadcast address can end with other value if subnet smaller than 255... but let's not worry about that)
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
				if(ipFields[field] == 255 && field==4)
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

function validateMultipleIps(ips)
{
	ips = ips.replace(/^[\t ]+/g, "");
	ips = ips.replace(/[\t ]+$/g, "");
	var splitIps = ips.split(/[\t ]*,[\t ]*/);
	var valid = splitIps.length > 0 ? 0 : 1; //1= error, 0=true
	while(valid == 0 && splitIps.length > 0)
	{
		var nextIp = splitIps.pop();
		if(nextIp.match(/-/))
		{
			var nextSplit = nextIp.split(/[\t ]*-[\t ]*/);
			if( nextSplit.length==2 && validateIP(nextSplit[0]) == 0 && validateIP(nextSplit[1]) == 0)
			{
				var ipInt1 = getIpInteger(nextSplit[0]);
				var ipInt2 = getIpInteger(nextSplit[1]);
				valid = ipInt1 <= ipInt2 ? 0 : 1;
			}
			else
			{
				valid = 1;
			}		
		}
		else
		{
			valid = validateIpRange(nextIp);
		}
	}
	return valid;
}
function validateMultipleIpsOrMacs(addresses)
{
	var addr = addresses.replace(/^[\t ]+/g, "");
	addr = addr.replace(/[\t ]+$/g, "");
	var splitAddr = addr.split(/[\t ]*,[\t ]*/);
	var valid = splitAddr.length > 0 ? 0 : 1; //1= error, 0=true
	while(valid == 0 && splitAddr.length > 0)
	{
		var nextAddr = splitAddr.pop();
		if(nextAddr.match(/-/))
		{
			var nextSplit = nextAddr.split(/[\t ]*-[\t ]*/);
			if( nextSplit.length==2 && validateIP(nextSplit[0]) == 0 && validateIP(nextSplit[1]) == 0)
			{
				var ipInt1 = getIpInt(nextSplit[0]);
				var ipInt2 = getIpInt(nextSplit[1]);
				valid = ipInt1 <= ipInt2 ? 0 : 1;
			}
			else
			{
				valid = 1;
			}		
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

function validateDecimal(num)
{
	var errorCode = num.match(/^[\d]*\.?[\d]+$/) != null || num.match(/^[\d]+\.?[\d]*$/) != null ? 0 : 1;
	return errorCode;
}

function validateNumeric(num)
{
	var errorCode = num.match(/^[\d]+$/) == null ? 1 : 0;
	return errorCode;
}

function validatePort(port)
{
	return validateNumericRange(port, 1, 65535)
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
			nextValid = nextValid && splitStr[0].match(/^[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5][0-9])?[\t ]*$/)
			nextValid = nextValid && splitStr[1].match(/^[\t ]*([0-1]?[0-9]|2[0-3])(:[0-5][0-9])?[\t ]*$/)
		}
		valid = valid && nextValid;
	}
	return valid ? 0 : 1;
}

function validateWeeklyRange(weeklyStr)
{
	var commaSplit = weeklyStr.match(/,/) ? weeklyStr.split(/,/) : [ weeklyStr ] ;
	var valid = true;
	for(commaIndex = 0; commaIndex < commaSplit.length && valid; commaIndex++)
	{
		var splitStr = commaSplit[commaIndex].split(/-/);
		var nextValid = splitStr.length == 2;
		if(nextValid)
		{
			var dayReg=new RegExp("^[\\t ]*("+UI.Sun+"|"+UI.Mon+"|"+UI.Tue+"|"+UI.Wed+"|"+UI.Thu+"|"+UI.Fri+"|"+UI.Sat+")[\\t ]*([0-1]?[0-9]|2[0-3])(:[0-5]?[0-9])?(:[0-5]?[0-9])?[\\t ]*$");
			nextValid = nextValid && splitStr[0].match(dayReg);
			nextValid = nextValid && splitStr[1].match(dayReg);
		}
		valid = valid && nextValid;
	}
	return valid ? 0 : 1;
}

function proofreadHours(input)
{
	proofreadText(input, validateHours, 0);
}

function proofreadWeeklyRange(input)
{
	proofreadText(input, validateWeeklyRange, 0);
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
function proofreadMultipleIps(input)
{
	proofreadText(input, validateMultipleIps, 0);
}
function proofreadMultipleIpsOrMacs(input)
{
	proofreadText(input, validateMultipleIpsOrMacs, 0);
}

function proofreadDecimal(input)
{
	proofreadText(input, validateDecimal, 0);
}
function proofreadNumeric(input)
{
	proofreadText(input, validateNumeric, 0);
}
function proofreadNumericRange(input, min, max)
{
	proofreadText(input, function(text){return validateNumericRange(text,min,max)}, 0);
}
function proofreadPort(input)
{
	proofreadText(input, validatePort, 0);
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


function getEmbeddedSvgWindow(embeddedId, controlDocument)
{
	if(controlDocument == null) { controlDocument = document; }

	var embedElement = controlDocument.getElementById( embeddedId );
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
	//however, if wds is in an AP section, it isn't a bridge

	var allWirelessSections = uciOriginal.getAllSections("wireless");
	var wanDef = uciOriginal.get("network", "wan", "");
	var bridgeSection = "";
	var sectionIndex;
	
	
	for(sectionIndex=0; sectionIndex < allWirelessSections.length && bridgeSection == ""; sectionIndex++)
	{
		var getWirelessVar = function(varName) 
		{
			return testUci.get("wireless", allWirelessSections[sectionIndex], varName).toLowerCase() 
		}
		
		if( getWirelessVar("mode") == "wds" && wanDef == "")
		{
			bridgeSection = allWirelessSections[sectionIndex];
		}
		else if( getWirelessVar("mode") == "sta" &&  getWirelessVar("wds") == "1" && wanDef == "")
		{
			bridgeSection = allWirelessSections[sectionIndex];
		}
		else if(getWirelessVar("mode") == "sta" && getWirelessVar("client_bridge") == "1" )
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

//cnv_LocaleTime takes a "21:09" 24hr timestamp
//  returns a locale-based timestamp (using 12h/24h time prefs & pre/post AM/PM notations): 21:09, 午前 9:09 or 9:09 PM
//NOTE: requires uciOriginal to have the 'gargoyle' section; use 'gargoyle_header_footer [options] gargoyle'
function cnv24hToLocal(timestamp) {
	style=uciOriginal.get("gargoyle", "global", "hour_style");
	if (style == 24) return timestamp;
	locale_time=""
	
	h_m_stamp=timestamp.split(":");
	curr_hour=eval(h_m_stamp[0]);
	if (UI.pAM.length > 0 && UI.pPM.length > 0) {
		locale_time+=(curr_hour < 12 ? UI.pAM : UI.pPM)+" "
		locale_time+=(curr_hour == 0 ? "12" : (curr_hour <= 12 ? curr_hour : curr_hour-12) )+":" + h_m_stamp[1]
	} else {
		locale_time+=(curr_hour == 0 ? "12" : (curr_hour <= 12 ? curr_hour : curr_hour-12) )+":" + h_m_stamp[1]
		locale_time+=" "+(curr_hour < 12 ? UI.hAM : UI.hPM);
	}
	return locale_time
}

//cnv_LocaleTime takes a full "09/16/13 21:09 EDT" date/time stamp
//  returns (based on hour prefs & pre/post AM/PM notations): "09/16/13 21:09 EDT", "09/16/13 午前 9:09 UTC" or "09/16/13 9:09 PM EDT"
//NOTE: requires uciOriginal to have the 'gargoyle' section; use 'gargoyle_header_footer [options] gargoyle'
function cnv_LocaleTime(full_date) { // 
	style=uciOriginal.get("gargoyle", "global", "hour_style");
	if (style == 24) return full_date;
	tcomponents=full_date.split(" ");
	return tcomponents[0]+" "+cnv24hToLocal(tcomponents[1])+" "+tcomponents[2]
}

function parseTimezones(timezoneLines)
{
	timezoneList = [];
	timezoneRegions = [];
	timezoneDefinitions = [];
	definitionTimezones = [];
	for(lineIndex = 0; lineIndex < timezoneLines.length; lineIndex++)
	{
		line = timezoneLines[lineIndex];
		if(!line.match(/^[\t ]*#/) && line.length > 0)
		{
			splitLine = line.split(/[\t]+/);
			region = stripQuotes( splitLine.pop() );
			definition = stripQuotes( splitLine.pop() );
			timezone = stripQuotes( splitLine.pop() );
			if (ObjLen(TiZ) > 0) { //localized firmware will = 0
				tz_parts=timezone.split(" ");
				if (tz_parts.length == 2 && tz_parts[1].search("TiZ.") == 0) {
					timezone=tz_parts[0]+" "+eval(tz_parts[1]);
				}
			}

			timezoneList.push(timezone);
			timezoneDefinitions[timezone] = definition;
			definitionTimezones[definition] = timezone;
			timezoneRegions[timezone] = region;
		}
	}
	return [timezoneList, timezoneRegions, timezoneDefinitions, definitionTimezones];
}
function stripQuotes(str)
{
	if(str.match(/\".*\"/))
	{
		str = str.match(/^[^\"]*\"([^\"]*)\"/)[1];
	}
	return str;
}

function textListToSpanElement(textList, addCommas, controlDocument)
{
	addCommas = addCommas == null ? false : addCommas;
	controlDocument = controlDocument == null ? document : controlDocument;

	var spanEl = controlDocument.createElement("span");
	var tlIndex;
	for(tlIndex=0; tlIndex < textList.length ; tlIndex++)
	{
		if(tlIndex > 0)
		{
			spanEl.appendChild( controlDocument.createElement("br") );
		}
		
		spanEl.appendChild(controlDocument.createTextNode(  textList[tlIndex] + (tlIndex < textList.length-1 && addCommas ? "," : "")  ));
	}
	return spanEl;
}

function addAddressStringToTable(controlDocument, newAddrs, tableContainerId, tableId, macsValid, ipValidType, alertOnError, tableWidth)
{
	//ipValidType: 0=none, 1=ip only, 2=ip or ip subnet, 3>=ip, ip subnet or ip range
	macsValid = macsValid == null ? true : false;
	ipValidType = ipValidType == null ? 3 : ipValidType;
	var ipValidFunction;
	if(ipValidType == 0)
	{
		ipValidFunction = function(){ return 1; };
	}
	else if(ipValidType == 1)
	{
		ipValidFunction = validateIP;
	}
	else if(ipValidType == 2)
	{
		ipValidFunction = validateIpRange;
	}
	else
	{
		ipValidFunction = validateMultipleIps;
	}

	var allCurrentMacs = [];
	var allCurrentIps = [];
	var tableContainer = controlDocument.getElementById(tableContainerId);
	if(tableContainer.firstChild != null)
	{
		var table = tableContainer.firstChild;
		var data = getTableDataArray(table, true, false);
		var rowIndex;
		for(rowIndex=0; rowIndex < data.length; rowIndex++)
		{
			var addr = data[rowIndex][0];
			if(validateMac(addr) == 0)
			{
				allCurrentMacs.push(addr);
			}
			else
			{
				allCurrentIps.push(addr);
			}
		}
	}

	controlDocument = controlDocument == null ? document : controlDocument;
	alertOnError = alertOnError == null ? true : alertOnError;

	var splitAddrs = newAddrs.split(/[\t ]*,[\t ]*/);
	var valid = splitAddrs.length > 0 ? 0 : 1; //1=error, 0=valid
	var splitIndex;
	for(splitIndex=0; splitIndex < splitAddrs.length && valid == 0; splitIndex++)
	{
		var addr = splitAddrs[splitIndex];
		var macValid = (macsValid && validateMac(addr) == 0);
		var ipValid = (ipValidFunction(addr) == 0);
		if(macValid || ipValid)
		{
			var currAddrs = macValid ? allCurrentMacs : allCurrentIps;
			valid = currAddrs.length == 0 || (!testAddrOverlap(addr, currAddrs.join(","))) ? 0 : 1;
			if(valid == 0)
			{
				currAddrs.push(addr); //if we're adding multiple addrs and there's overlap, this will allow us to detect it
			}
		}
		else 
		{
			valid = 1;
		}
	}


	if(valid == 0)
	{
		var table = tableContainer.childNodes.length > 0 ? tableContainer.firstChild : createTable([""], [], tableId, true, false, null, null, controlDocument);
		newAddrs = newAddrs.replace(/^[\t ]*/, "");
		newAddrs = newAddrs.replace(/[\t ]*$/, "");
		var addrs = newAddrs.split(/[\t ]*,[\t ]*/);
		
		while(addrs.length > 0)
		{
			addTableRow(table, [ addrs.shift() ], true, false, null, null, controlDocument);
		}
		
		if(tableContainer.childNodes.length == 0)
		{
			tableContainer.appendChild(table);
		}
		if(tableWidth != null)
		{
			table.style.width = "" + tableWidth + "px";
		}
	}
	else if(alertOnError)
	{
		alert(UI.InvAdd+"\n");
	}

	return valid == 0 ? true : false;

}


function addAddressesToTable(controlDocument, textId, tableContainerId, tableId, macsValid, ipValidType, alertOnError, tableWidth)
{
	
	var newAddrs = controlDocument.getElementById(textId).value;
	var valid = addAddressStringToTable(controlDocument, newAddrs, tableContainerId, tableId, macsValid, ipValidType, alertOnError, tableWidth)
	if(valid)
	{
		controlDocument.getElementById(textId).value = "";
	}
	return valid;
}


function parsePaddedInt(intStr)
{
	intStr = intStr == null ? "" : intStr;
	intStr = intStr.replace(/[\t ]+/, "");
	while( (intStr.length > 1 && intStr.match(/^0/)) || (intStr.length > 2 && intStr.match(/^\-0/)) )
	{
		intStr = intStr.replace(/^0/, "");
		intStr = intStr.replace(/^\-0/, "-");
	}
	return parseInt(intStr);
}

function getIpInteger(ipStr)
{
	ipStr = ipStr == null ? "" : ipStr;
	var ip = ipStr.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/);
	if(ip)
	{
		return (+parsePaddedInt(ip[1])<<24) + (+parsePaddedInt(ip[2])<<16) + (+parsePaddedInt(ip[3])<<8) + (+parsePaddedInt(ip[4]));
	}
	return parseInt(""); //will return NaN
}
function getMaskInteger(maskSize)
{
	return -1<<(32-parsePaddedInt(maskSize))
}

function getIpRangeIntegers(ipStr)
{
	var startInt = 0;
	var endInt = 0;
	if(ipStr.match(/\//))
	{
		var split = ipStr.split(/[\t ]*\/[\t ]*/);
		var ipInt = getIpInteger(split[0]);
		var ipMaskInt = (split[1]).match(/\./) ? getIpInteger(split[1]) : getMaskInteger(split[1]);
		startInt = ipInt & ipMaskInt;
		endInt = startInt | ( ~ipMaskInt );
	}
	else if(ipStr.match(/-/))
	{
		var split = ipStr.split(/[\t ]*\-[\t ]*/);
		startInt = getIpInteger(split[0]);
		endInt = getIpInteger(split[1]);
	}
	else
	{
		startInt = getIpInteger(ipStr);
		endInt = startInt;
	}
	return [startInt, endInt];

}


function testSingleAddrOverlap(addrStr1, addrStr2)
{
	/* 
	 * this adjustment is useful in multiple places, particularly quotas
	 * if you don't want these conversions, just validate quota BEFORE you
	 * try calling this function
	 */
	var adj = function(addrStr)
	{
		addrStr = addrStr == "" ? "ALL" : addrStr.toUpperCase();	
		if(addrStr == "ALL_OTHERS_COMBINED" || addrStr == "ALL_OTHERS_INDIVIDUAL")
		{
			addrStr = "ALL_OTHERS_COMBINED";
		}
		return addrStr;
	}
	addrStr1 = adj(addrStr1);
	addrStr2 = adj(addrStr2);

	var matches = false;
	if(addrStr1 == addrStr2) //can test MAC addr equality as well as ALL/OTHER variables we use sometimes
	{
		matches = true;
	}
	else //assume we're dealing with an actual IP / IP subnet / IP Range
	{
		if(validateMultipleIps(addrStr1) > 0 || validateMultipleIps(addrStr2) > 0 || addrStr1.match(",") || addrStr2.match(",") )
		{
			matches = false;
		}
		else
		{
			var parsed1 = getIpRangeIntegers(addrStr1);
			var parsed2 = getIpRangeIntegers(addrStr2);
			matches = parsed1[0] <= parsed2[1] && parsed1[1] >= parsed2[0]; //test range overlap, inclusive
		}
	}
	return matches;
}

function testAddrOverlap(addrStr1, addrStr2)
{
	addrStr1 = addrStr1.replace(/^[\t ]+/, "");
	addrStr1 = addrStr1.replace(/[\t ]+$/, "");
	addrStr2 = addrStr2.replace(/^[\t ]+/, "");
	addrStr2 = addrStr2.replace(/[\t ]+$/, "");

	var split1 = addrStr1.split(/[,\t ]+/);
	var split2 = addrStr2.split(/[,\t ]+/);
	var index1;
	var overlapFound = false;
	for(index1=0; index1 < split1.length && (!overlapFound); index1++)
	{
		var index2;
		for(index2=0; index2 <split2.length && (!overlapFound); index2++)
		{
			overlapFound = overlapFound || testSingleAddrOverlap(split1[index1], split2[index2]);
		}
	}
	return overlapFound;
}

//validateIP -- tests IP only, and that it's not a broadcast address
//validateIpRange -- tests if it's a valid Ip or a valid Ip/netmask combination
//validateMultipleIps -- tests whether we have a comma separated list of valid Ips or Ip/mask or Ip-Ip range definitions
//validateMac -- test if arg is a valid MAC



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

function arrToHash(arr)
{
	var h = []
	var i
	for(i=0; i < arr.length; i++) { h[ arr[i] ] = 1; }
	return h
}


function confirmPassword(confirmText, validatedFunc, invalidFunc)
{
	confirmText = confirmText == null ? UI.CPass+":" : confirmText;
	if( typeof(confirmWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			confirmWindow.close();
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
	var wlocation = "password_confirm.sh";
	confirmWindow = window.open(wlocation, "password", "width=560,height=260,left=" + xCoor + ",top=" + yCoor );
	
	var okButton = createInput("button", confirmWindow.document);
	var cancelButton = createInput("button", confirmWindow.document);
	
	okButton.value         = UI.OK;
	okButton.className     = "default_button";
	cancelButton.value     = UI.Cancel;
	cancelButton.className = "default_button";


	runOnEditorLoaded = function () 
	{
		updateDone=false;
		if(confirmWindow.document != null)
		{
			if(confirmWindow.document.getElementById("bottom_button_container") != null)
			{
				confirmWindow.document.getElementById("bottom_button_container").appendChild(okButton);
				confirmWindow.document.getElementById("bottom_button_container").appendChild(cancelButton);
				setChildText("confirm_text", confirmText, null, null, null, confirmWindow.document);
			
				cancelButton.onclick = function()
				{
					confirmWindow.close();
				}
				okButton.onclick = function()
				{
					setControlsEnabled(false, true, UI.VPass);
	
					var commands = "gargoyle_session_validator -p \"" + confirmWindow.document.getElementById("password").value + "\" -a \"dummy.browser\" -i \"127.0.0.1\""
					var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
					var stateChangeFunction = function(req)
					{
						if(req.readyState == 4)
						{
							confirmWindow.close();
							var result = req.responseText.split("\n")[0]
							if(result.match(/^echo \"invalid\"/))
							{
								invalidFunc.call(null);
							}
							else
							{
								validatedFunc.call(null);
							}
						}
					}
					runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

				}
				confirmWindow.moveTo(xCoor,yCoor);
				confirmWindow.focus();
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



function getUsedPorts()
{
	var dropbearSections = uciOriginal.getAllSections("dropbear"); 
	var sshPort   = uciOriginal.get("dropbear", dropbearSections[0], "Port")
	var httpPort  = uciOriginal.get("httpd_gargoyle", "server", "http_port")
	var httpsPort = uciOriginal.get("httpd_gargoyle", "server", "https_port")

	var foundPorts = []
	foundPorts["tcp"] = []
	foundPorts["udp"] = []

	var portDefs=[]
	portDefs.push( [ sshPort, "tcp", UI.sprt ] )
	foundPorts["tcp"][sshPort] = 1
	if(httpPort != "")
	{
		portDefs.push( [ httpPort, "tcp", UI.wsprt ] )
		foundPorts["tcp"][httpPort] = 1
	}
	if(httpsPort != "")
	{
		portDefs.push( [ httpsPort, "tcp", UI.wsprt ] )
		foundPorts["tcp"][httpsPort] = 1
	}



	var remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept")
	var acceptIndex;
	for(acceptIndex=0; acceptIndex < remoteAcceptSections.length; acceptIndex++)
	{
		var section    = remoteAcceptSections[acceptIndex];
		var localPort  = uciOriginal.get("firewall", section, "local_port");
		var remotePort = uciOriginal.get("firewall", section, "remote_port");
		var startPort  = uciOriginal.get("firewall", section, "start_port");
		var endPort    = uciOriginal.get("firewall", section, "end_port");
		var proto      = uciOriginal.get("firewall", section, "proto").toLowerCase();
		var zone       = uciOriginal.get("firewall", section, "zone").toLowerCase();
		if(zone == "wan" || zone == "")
		{
			var defIndex;
			remotePort = remotePort == "" ? localPort : remotePort;
			for(defIndex=0; defIndex<portDefs.length ; defIndex++)
			{
				if(defIndex[0] == localPort && (defIndex[1] == proto || proto == "" || proto == "tcpudp"))
				{
					if(localport != remotePort && localport != "")
					{
						if(proto == "" || proto == "tcpudp")
						{	
							portDefs.push([remotePort, "tcp", defIndex[2] ])
							portDefs.push([remotePort, "udp", defIndex[2] ])
							foundPorts["tcp"][remotePort] = 1;
							foundPorts["udp"][remotePort] = 1;

						}
						else
						{
							portDefs.push([remotePort, proto, defIndex[2] ])
							foundPorts[proto][remotePort]= 1
						}
					}
				}
			}
			if( localPort != "" ) //handle ports other than ssh, http, https
			{
				var protoList = proto == "" || proto == "tcpudp" ? [ "tcp", "udp" ] : [ proto ];
				while(protoList.length > 0)
				{
					var nextProto = protoList.shift();
					if(foundPorts[nextProto][remotePort] == null) //bypasses adding second instance defs if already defined above as part of ssh/http/https
					{
						portDefs.push( [ remotePort, nextProto, UI.prdr ])
						foundPorts[nextProto][remotePort] = 1;
						if(foundPorts[nextProto][localPort] == null) //implies localPort != remotePort, since we just set this for remotePort
						{
							portDefs.push([localPort, nextProto, UI.puse ])
							foundPorts[nextProto][localPort] = 1
						}
					}
				}
			}
			if(localPort == "" && startPort != "" && endPort != "")
			{
				var protoList = proto == "" || proto == "tcpudp" ? [ "tcp", "udp" ] : [ proto ];
				while(protoList.length > 0)
				{
					var nextProto = protoList.shift();
					portDefs.push([startPort + "-" + endPort, nextProto, UI.prdr ])
				}
			}
		}
	}

	var redirectSections = uciOriginal.getAllSectionsOfType("firewall", "redirect")
	var redirectIndex;
	for(redirectIndex=0; redirectIndex < redirectSections.length; redirectIndex++)
	{
		var section    = remoteAcceptSections[acceptIndex];
		var localPort  = uciOriginal.get("firewall", section, "local_port");
		var remotePort = uciOriginal.get("firewall", section, "remote_port");
		var proto      = uciOriginal.get("firewall", section, "proto").toLowerCase();
		var srcZone    = uciOriginal.get("firewall", section, "src").toLowerCase();
		var dstZone    = uciOriginal.get("firewall", section, "dest").toLowerCase();
		var port       = uciOriginal.get("firewall", section, "src_dport")
		var ip         = uciOriginal.get("firewall", section, "dest_ip");

		//note range notation already part of remotePort here, so range is handled properly by this code
		portDefs.push([remotePort, proto, UI.pfwd+" " + ip ])
	}

	//for debugging only
	/*
	var portStr = [];
	var pi;
	for(pi=0; pi< portDefs.length; pi++)
	{
		portStr.push( (portDefs[pi]).join(",") );
	}
	alert(portStr.join("\n"));
	*/


	return portDefs;

}

// ignorePorts should be 2d-associateive array with first dimension being proto, second port, eg. ignorePorts[proto][port or port range] = 1 if port is to be ignored
function checkForPortConflict(port, proto, ignorePorts)
{
	var portStart = null
	var portEnd = null
	var isRange = false
	if(port.match(/-/))
	{
		var splitPort = port.split(/-/)
		portStart = parseInt(splitPort[0])
		portEnd   = parseInt(splitPort[1])
		isRange = true;
	}
	else
	{
		port = parseInt(port)
	}


	if(ignorePorts != null)
	{
		ignorePorts["tcp"] = ignorePorts["tcp"] == null ? [] : ignorePorts["tcp"]
		ignorePorts["udp"] = ignorePorts["udp"] == null ? [] : ignorePorts["udp"]
		if(ignorePorts[proto][port] != null && ignorePorts[proto][(""+port)] != null)
		{
			return ""
		}
		else
		{
			var range
			for(range in ignorePorts[proto])
			{
				if(range.match(/-/))
				{
					var splitRange = range.split(/-/);
					if((!isRange) && port >= parseInt(splitRange[0]) && port <= parseInt(splitRange[1]))
					{
						return ""
					}
					if(isRange && portStart <= parseInt(splitRange[1]) && portEnd >= parseInt(splitRange[0]))
					{
						return ""
					}
				}
			}
		}
	}


	var usedPorts = getUsedPorts()

	var portIndex;
	var portConflict = ""
	for(portIndex=0; portIndex < usedPorts.length && portConflict == ""; portIndex++)
	{
		var portDef = usedPorts[portIndex];
		if(proto == portDef[1])
		{
			var portStr = portDef[0]
			if(portStr.match(/\-/))
			{
				var splitPort = portStr.split(/\-/)
				if((!isRange) && port >= parseInt(splitPort[0]) && port <= parseInt(splitPort[1]))
				{
					portConflict = portDef[2]
				}
				if(isRange && portStart <= parseInt(splitPort[1]) && portEnd >= parseInt(splitPort[0]))
				{
					portConflict = portDef[2]
				}
			}
			else
			{
				if((!isRange) && port == parseInt(portStr))
				{
					portConflict = portDef[2]
				}
				if(isRange && parseInt(portStr) >= portStart && parseInt(portStr) <= portEnd)
				{
					portConflict = portDef[2]
				}
			}
		}
	}
	return portConflict;
}



function query(queryHeader, queryText, buttonNameList, continueFunction )
{
	document.getElementById("wait_icon").style.display="none"
	document.getElementById("wait_txt").style.display="none"

	wmOldTxt    = document.getElementById("wait_txt").style == "none" ? false : true;
	wmOldBack   = document.getElementById("wait_msg").style.background
	wmOldWidth  = document.getElementById("wait_msg").style.width;
	wmOldHeight = document.getElementById("wait_msg").style.height
	wmOldTop    = document.getElementById("wait_msg").style.top
	document.getElementById("wait_msg").style.background="white"
	document.getElementById("wait_msg").style.width="585px"
	document.getElementById("wait_msg").style.height="500px"
	setControlsEnabled(false,false)
	document.getElementById("wait_msg").style.top="20px"

	queryFieldset = document.createElement("fieldset");
	queryFieldset.innerHTML='<legend class="sectionheader" id="query_header">' + queryHeader + '</legend><div style="clear:both;display:block"><span class="nocolumn" id="query_text">' + queryText + '</span></div><div id="spacer_div" style="display:block; mrgin:8px;">&nbsp;</div><div id="query_button_container"></div>'
	
	document.getElementById("wait_msg").appendChild(queryFieldset)
	
	var buttonList = [];
	var bIndex;
	for(bIndex=0; bIndex < buttonNameList.length ; bIndex++)
	{
		b           = createInput("button", document);
		b.value     = buttonNameList[bIndex];
		b.className = "default_button"
		b.onclick   = function()
		{
			document.getElementById("wait_msg").removeChild(queryFieldset)
			document.getElementById("wait_msg").style.background=wmOldBack
			document.getElementById("wait_msg").style.width=wmOldWidth
			document.getElementById("wait_msg").style.height=wmOldHeight
			document.getElementById("wait_msg").style.top=wmOldTop
			document.getElementById("wait_icon").style.display="block"
			document.getElementById("wait_txt").style.display="block"
			setControlsEnabled(false,wmOldTxt)
			continueFunction(this.value)
		}
		document.getElementById("query_button_container").appendChild(b);
		document.getElementById("query_button_container").appendChild( document.createElement("br") )
	}
}

//apparently these var fbS=new Object(); //part of i18n  -> objects do not have a length (it is undefined)
function ObjLen(an_obj) {
	var len=0;
	for (item in an_obj) {
		len++;
	}
	return len
}

