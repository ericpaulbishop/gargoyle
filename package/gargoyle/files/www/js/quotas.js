/*
 * This program is copyright © 2008,2009-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var quotasStr=new Object(); //part of i18n 

var pkg = "firewall";
var changedIds = [];
var rowCheckIndex = 3;

var downQosClasses = [];
var downQosMarks = [];
var upQosClasses = [];
var upQosMarks = [];

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
	var idToNewSection = [];
	while(allNewQuotas.length > 0)
	{
		//if ip has changed, reset saved data
		var section = allNewQuotas.shift()
		var newId = uci.get(pkg,section,"id");
		idToNewSection[newId] = section;
		if( changedIds[ newId ] == 1 )
		{
			uci.set(pkg, section, "ignore_backup_at_next_restore", "1");
		}
	}

	//set enabled / disabled	
	var quotaTable = document.getElementById('quota_table_container').firstChild;
	var quotaTableData = getTableDataArray(quotaTable, true, false);
	var qtIndex=0;
	for(qtIndex=0; qtIndex < quotaTableData.length; qtIndex++)
	{
		var enabledCheck = quotaTableData[qtIndex][rowCheckIndex];
		var enabledSection = idToNewSection[ enabledCheck.id ];
		if(enabledSection != null)
		{
			uci.set(pkg, enabledSection, "enabled", (enabledCheck.checked ? "1" : "0") )
			if(enabledCheck.checked)
			{
				quotaUseVisibleCommand = "\nuci set gargoyle.status.quotause=\"225\" ; uci commit ;\n"
			}
		}
	}

	var postCommands = [];
	postCommands.push("sh /usr/lib/gargoyle/restart_firewall.sh");
	postCommands.push("if [ -d \"/usr/data/quotas/\" ] ; then rm -rf /usr/data/quotas/* ; fi ;");
	postCommands.push("backup_quotas");
	var commands = preCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + quotaUseVisibleCommand + "\n" + postCommands.join("\n");

	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
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
	//initialize qos mark lists, if full qos is active
	var qmIndex=0;
	upQosClasses = [];
	upQosMarks = [];
	downQosClasses = [];
	downQosMarks = [];
	for(qmIndex=0; qmIndex < qosMarkList.length; qmIndex++)
	{
		var className = qosMarkList[qmIndex][1];
		var classDisplay = uciOriginal.get("qos_gargoyle", className, "name");
		className = classDisplay == "" ? className : classDisplay;
		if(qosMarkList[qmIndex][0] == "upload")
		{
			upQosClasses.push(className);
			upQosMarks.push(qosMarkList[qmIndex][2]);
		}
		else
		{
			downQosClasses.push(className);
			downQosMarks.push(qosMarkList[qmIndex][2]);
		}
	}

	//table columns: ip, percent upload used, percent download used, percent combined used, enabled, edit, remove
	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var quotaTableData = [];
	var checkElements = []; //because IE is a bitch and won't register that checkboxes are checked/unchecked unless they are part of document
	var areChecked = [];
	changedIds = [];
	for(sectionIndex = 0; sectionIndex < quotaSections.length; sectionIndex++)
	{
		var ip = uciOriginal.get(pkg, quotaSections[sectionIndex], "ip").toUpperCase();
		var id = uciOriginal.get(pkg, quotaSections[sectionIndex], "id");
		if(id == "")
		{
			id = getIdFromIp(ip);
			uci.set(pkg, quotaSections[sectionIndex], "id", id);
		}


		
		var timeParameters = getTimeParametersFromUci(uci, quotaSections[sectionIndex], 1);
		var limitStr = getLimitStrFromUci(uci, quotaSections[sectionIndex]);
		var enabled = uciOriginal.get(pkg, quotaSections[sectionIndex], "enabled");
		enabled = enabled != "0" ? true : false;
		
		
		var enabledCheck = createEnabledCheckbox(enabled);
		enabledCheck.id= id;
		checkElements.push(enabledCheck);
		areChecked.push(enabled);

		quotaTableData.push( [ ipToTableSpan(ip), timeParamsToTableSpan(timeParameters), limitStr, enabledCheck, createEditButton(enabled) ] );
	}

	
	columnNames=[quotasStr.IPs, quotasStr.Active, textListToSpanElement([quotasStr.Limits,quotasStr.Totals], false), UI.Enabled, "" ];
	
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

function ipToTableSpan(ip)
{
	var ipStr = ip;
	if(ipStr == "ALL_OTHERS_INDIVIDUAL")
	{
		ipStr=quotasStr.OthersOne;
	}
	else if(ipStr == "ALL_OTHERS_COMBINED")
	{
		ipStr=quotasStr.OthersAll;
	}
	else if(ipStr == "ALL" || ipStr == "")
	{
		ipStr=quotasStr.All;
	}
	return textListToSpanElement(ipStr.split(/[\t ]*,[\t ]*/), true, document);
}

function timeParamsToTableSpan(timeParameters)
{
	var hours = timeParameters[0];
	var days = timeParameters[1];
	var weekly = timeParameters[2];
	var active = timeParameters[3];
	
		
	var textList = [];
	if(active == "always")
	{
		textList.unshift(UI.Always);
	}
	else
	{
		if(weekly != "")
		{
			textList = weekly.match(",") ? weekly.split(/[\t ]*,[\t ]*/) : [ weekly ];
		}
		else
		{
			if(hours != ""){ textList = hours.match(",") ? hours.split(/[\t ]*,[\t ]*/) : [ hours ]; }
			if(days  != ""){ textList.unshift(days); }
		}
		textList.unshift( active == "only" ? quotasStr.Only+":" : quotasStr.AllExcept+":" );
	}
	return textListToSpanElement(textList, false, document);
}
function getLimitStrFromUci(srcUci, section)
{
	var totalLimit = uci.get(pkg, section, "combined_limit");
	var downLimit  = uci.get(pkg, section, "ingress_limit");
	var upLimit    = uci.get(pkg, section, "egress_limit");

	var parseLimit = function(limStr){ return limStr == "" ? quotasStr.NA : parseBytes(parsePaddedInt(limStr), null,true).replace(/\.[\d]+/,"").replace(/[\t ]+/, ""); }
	return parseLimit(totalLimit) + "/" + parseLimit(downLimit) + "/" + parseLimit(upLimit);
}

function getIdFromIp(ip)
{
	id = ip == "" ? "ALL" : ip.replace(/[\t, ]+.*$/, "");
	id = id.replace(/\//, "_");
			
	var idPrefix = id;
	var found = true;
	var suffixCount = 0;

	var quotaSections = uci.getAllSectionsOfType(pkg, "quota");

	while(found)
	{
		found = false;
		var sectionIndex;
		for(sectionIndex=0; sectionIndex < quotaSections.length && (!found); sectionIndex++)
		{
			found = found || uci.get(pkg, quotaSections[sectionIndex], "id") == id;
		}
		if(found)
		{
			var letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			var suffix = suffixCount < 26 ? "_" + letters.substr(suffixCount,1) : "_Z" + (suffixCount-25);
			id = idPrefix + suffix;
		}
		suffixCount++;
	}
	return id;

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
		
		var table = controlDocument.getElementById("quota_ip_table_container").firstChild;
		var ipData = table != null ? getTableDataArray(table, true, false) : [];
		var ipList = [];
		var rowIndex;
		for(rowIndex=0; rowIndex < ipData.length; rowIndex++)
		{
			ipList.push( ipData[rowIndex][0] );
		}
		ip = ipList.join(",");
	}
	return ip;
}

function setDocumentIp(ip, controlDocument)
{
	ip = ip== ""  ? "ALL" : ip;
	controlDocument = controlDocument == null ? document : controlDocument;
	controlDocument.getElementById("add_ip").value = "";

	/* clear ip table */
	var tableContainer = controlDocument.getElementById("quota_ip_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}


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
		controlDocument.getElementById("add_ip").value = ip;
		var valid = addAddressesToTable(controlDocument,"add_ip","quota_ip_table_container","quota_ip_table",false, 3, false,250);
		if(!valid)
		{
			controlDocument.getElementById("add_ip").value = "";
		}
	}
}


function addNewQuota()
{
	var errors = validateQuota(document, "", "none");
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+quotasStr.AddError);
	}
	else
	{
		var quotaNum = 1;
		while( uci.get(pkg, "quota_" + quotaNum, "") != "") { quotaNum++; }

		setUciFromDocument(document, "");

		
		var enabledCheck = createEnabledCheckbox(true);
		enabledCheck.id = uci.get(pkg, "quota_" + quotaNum, "id");

		var tableContainer = document.getElementById("quota_table_container");
		var table = tableContainer.firstChild;
		
		
		var ip = getIpFromDocument(document);
		var timeParameters = getTimeParametersFromUci(uci, "quota_" + quotaNum);
		var limitStr = getLimitStrFromUci(pkg, "quota_" + quotaNum);
		addTableRow(table, [ ipToTableSpan(ip), timeParamsToTableSpan(timeParameters), limitStr, enabledCheck, createEditButton(true)], true, false, removeQuotaCallback);

		setDocumentFromUci(document, new UCIContainer(), "");

		enabledCheck.checked = true;
	}
}

function setVisibility(controlDocument)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	setInvisibleIfIdMatches("applies_to_type", ["all","others_combined", "others_individual"], "quota_ip_container", "inline", controlDocument);
	setInvisibleIfIdMatches("quota_reset", ["hour", "day"], "quota_day_container", "block", controlDocument);
	setInvisibleIfIdMatches("quota_reset", ["hour"], "quota_hour_container", "block", controlDocument);
	setInvisibleIfIdMatches("max_up_type", ["unlimited"], "max_up_container", "inline", controlDocument);
	setInvisibleIfIdMatches("max_down_type", ["unlimited"], "max_down_container", "inline", controlDocument);
	setInvisibleIfIdMatches("max_combined_type", ["unlimited"], "max_combined_container", "inline", controlDocument);
	
	setInvisibleIfIdMatches("quota_active", ["always"], "quota_active_type", "inline", controlDocument);
	setInvisibleIfIdMatches("quota_active", ["always"], "quota_active_controls_container", "block", controlDocument);
	if(getSelectedValue("quota_active", controlDocument) != "always")
	{
		setInvisibleIfIdMatches("quota_active_type", ["days", "weekly_range"], "active_hours_container", "block", controlDocument);
		setInvisibleIfIdMatches("quota_active_type", ["hours", "weekly_range"], "active_days_container", "block", controlDocument);
		setInvisibleIfIdMatches("quota_active_type", ["hours", "days", "days_and_hours"], "active_weekly_container", "block", controlDocument);
	}
	else
	{
		//individual control divs need to be set invisible as well as enclosing div, because this lets validation function know whether to test them
		setInvisibleIfIdMatches("quota_active", ["always"], "active_hours_container", "block", controlDocument);
		setInvisibleIfIdMatches("quota_active", ["always"], "active_days_container", "block", controlDocument);
		setInvisibleIfIdMatches("quota_active", ["always"], "active_weekly_container", "block", controlDocument);
	}
	
	setInvisibleIfIdMatches("quota_exceeded", ["hard_cutoff"], "quota_only_qos_container", "block", controlDocument);
	setInvisibleIfIdMatches("quota_exceeded", ["hard_cutoff"], "quota_full_qos_container", "block", controlDocument);
	if(fullQosEnabled)
	{
		controlDocument.getElementById("quota_only_qos_container").style.display = "none";
	}
	else
	{
		controlDocument.getElementById("quota_full_qos_container").style.display = "none";
	}


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
			var suffix=quotasStr.Digs
			if( day % 100  != 11 && lastDigit == "1")
			{
				suffix=quotasStr.LD1s
			}
			if( day % 100 != 12 && lastDigit == "2")
			{
				suffix=quotasStr.LD2s
			}
			if( day %100 != 13 && lastDigit == "3")
			{
				suffix=quotasStr.LD3s
			}
			names.push(dayStr + suffix);
			vals.push( ((day-1)*60*60*24) + "" );
		}
		setAllowableSelections("quota_day", vals, names, controlDocument);
	}
	if(qri == "week")
	{
		var names = [UI.Sunday, UI.Monday, UI.Tuesday, UI.Wednesday, UI.Thursday, UI.Friday, UI.Saturday];
		var vals = [];
		var dayIndex;
		for(dayIndex=0; dayIndex < 7; dayIndex++)
		{
			vals.push( (dayIndex*60*60*24) + "")
		}
		setAllowableSelections("quota_day", vals, names, controlDocument);
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


function timeVariablesToWeeklyRanges(hours, days, weekly, invert)
{
	var hours = hours == null ? "" : hours;
	var days = days == null ? "" : days;
	var weekly = weekly == null ? "" : weekly;
	
	var dayToIndex = [];
	dayToIndex[UI.Sun.toUpperCase()] = 0;
	dayToIndex[UI.Mon.toUpperCase()] = 1;
	dayToIndex[UI.Tue.toUpperCase()] = 2;
	dayToIndex[UI.Wed.toUpperCase()] = 3;
	dayToIndex[UI.Thu.toUpperCase()] = 4;
	dayToIndex[UI.Fri.toUpperCase()] = 5;
	dayToIndex[UI.Sat.toUpperCase()] = 6;


	var splitRangesAtEnd = function(rangeList, max)
	{
		var startEndPairs = [];
		var rangeIndex;
		for(rangeIndex=0;rangeIndex < rangeList.length; rangeIndex=rangeIndex+2)
		{
			if(rangeList[rangeIndex+1] < rangeList[rangeIndex])
			{
				var oldEnd = rangeList[rangeIndex+1];
				rangeList[rangeIndex+1] = max;
				rangeList.push(0);
				rangeList.push(oldEnd);
			}
			var s = rangeList[rangeIndex];
			var e = rangeList[rangeIndex+1];
			startEndPairs.push( [s,e] );
		}
		
		//sort based on starts
		var sortPairs = function(a,b){ return a[0] - b[0]; }
		var sortedPairs = startEndPairs.sort(sortPairs);
		var newRanges = [];
		for(rangeIndex=0;rangeIndex < sortedPairs.length; rangeIndex++)
		{
			newRanges.push( sortedPairs[rangeIndex][0] );
			newRanges.push( sortedPairs[rangeIndex][1] );
		}
		return newRanges;
	}


	var ranges = [];
	if(hours == "" && days == "" && weekly == "")
	{
		ranges = [0, 7*24*60*60];
		invert = false;
	}
	else if(weekly != "")
	{
		var parsePiece = function(piece)
		{
			var splitPiece = piece.split(/[:\t ]+/);
			var dayName = (splitPiece[0]).substr(0,3).toUpperCase();
			splitPiece[0] = dayToIndex[dayName] != null ? dayToIndex[dayName]*24*60*60 : 0;
			splitPiece[1] = parsePaddedInt(splitPiece[1]) + "" != "NaN" ? parsePaddedInt(splitPiece[1])*60*60 : 0;
			splitPiece[2] = parsePaddedInt(splitPiece[2]) + "" != "NaN" ? parsePaddedInt(splitPiece[2])*60 : 0;
			splitPiece[3] = splitPiece[3] != null ? ( parsePaddedInt(splitPiece[3]) + "" != "NaN" ? parsePaddedInt(splitPiece[3]) : 0) : 0;
			return splitPiece[0] + splitPiece[1] + splitPiece[2] + splitPiece[3];
		}
		var pairs = weekly.split(/[\t ]*,[\t ]*/);
		var pairIndex;
		for(pairIndex=0; pairIndex < pairs.length; pairIndex++)
		{

			var pieces = (pairs[pairIndex]).split(/[\t ]*\-[\t ]*/);
			ranges.push(parsePiece(pieces[0]));
			ranges.push(parsePiece(pieces[1]));
		}
		ranges = splitRangesAtEnd(ranges, 7*24*60*60);
	}
	else
	{
		var validDays= [1,1,1,1,1,1,1];
		var hourRanges = [];
		if(days != "")
		{
			validDays= [0,0,0,0,0,0,0];
			var splitDays = days.split(/[\t ]*,[\t ]*/);
			var dayIndex;
			for(dayIndex=0; dayIndex < splitDays.length; dayIndex++)
			{
				var dayName = (splitDays[dayIndex]).substr(0,3).toUpperCase();
				if(dayToIndex[dayName] != null)
				{
					validDays[ dayToIndex[dayName] ] = 1;
				}
			}
		}
		if(hours != "")
		{
			var parsePiece = function(piece)
			{
				var splitPiece = piece.split(/[:\t ]+/);
				splitPiece[0] = parsePaddedInt(splitPiece[0]) + "" != "NaN" ? parsePaddedInt(splitPiece[0])*60*60 : 0;
				splitPiece[1] = parsePaddedInt(splitPiece[1]) + "" != "NaN" ? parsePaddedInt(splitPiece[1])*60 : 0;
				splitPiece[2] = splitPiece[2] != null ? ( parsePaddedInt(splitPiece[2]) + "" != "NaN" ? parsePaddedInt(splitPiece[2]) : 0) : 0;


				return splitPiece[0] + splitPiece[1] + splitPiece[2]; 
			}
			var pairs = hours.split(/[\t ]*,[\t ]*/);
			var pairIndex;
			for(pairIndex=0; pairIndex < pairs.length; pairIndex++)
			{
				var pair = (pairs[pairIndex]).replace(/^[\t ]*/, "").replace(/[\t ]*$/, "");
				var pieces = pair.split(/[\t ]*\-[\t ]*/);
				hourRanges.push(parsePiece(pieces[0]));
				hourRanges.push(parsePiece(pieces[1]));
			}
			hourRanges = splitRangesAtEnd(hourRanges, 24*60*60);
		}
		hourRanges = hourRanges.length == 0 ? [0,24*60*60] : hourRanges;

		var dayIndex;
		for(dayIndex=0; dayIndex < validDays.length; dayIndex++)
		{
			if(validDays[dayIndex] != 0)
			{
				var hourIndex;
				for(hourIndex=0; hourIndex < hourRanges.length; hourIndex++)
				{
					ranges.push( (dayIndex*24*60*60) + hourRanges[hourIndex] )
				}
			}
		}
	}

	if(invert)
	{
		if(ranges[0] == 0)
		{
			ranges.shift();
		}
		else
		{
			ranges.unshift(0);
		}

		if(ranges[ ranges.length-1 ] == 7*24*60*60)
		{
			ranges.pop();
		}
		else
		{
			ranges.push(7*24*60*60);
		}
	}
	return ranges;
}


function rangesOverlap(t1, t2)
{
	//alert("testing overlap for:\n" + t1.join(",") + "\n" + t2.join(",") );
	var ranges1 = timeVariablesToWeeklyRanges(t1[0], t1[1], t1[2], t1[3]);
	var ranges2 = timeVariablesToWeeklyRanges(t2[0], t2[1], t2[2], t2[3]);

	var r1Index = 0;
	var r2Index = 0;
	var overlapFound = false;
	for(r1Index=0; r1Index < ranges1.length && (!overlapFound); r1Index=r1Index+2)
	{
		var r1Start = ranges1[r1Index];
		var r1End   = ranges1[r1Index+1];
		var r2Start = ranges2[r2Index];
		var r2End   = ranges2[r2Index+1];
		overlapFound = overlapFound || (r1End > r2Start && r1Start < r2End);

		while( (!overlapFound) && r2Start < r1Start && r2Index < ranges2.length)
		{
			r2Index = r2Index+2;
			if(r2Index < ranges2.length)
			{
				var r2Start = ranges2[r2Index];
				var r2End   = ranges2[r2Index+1];
				overlapFound = overlapFound || (r1End > r2Start && r1Start < r2End);
			}
		}
		/*
		if(overlapFound)
		{
			alert("overlapFound: r1=[" + r1Start + "," + r1End + "], r2=[" + r2Start + "," + r2End + "]");
		}
		*/
	}
	return overlapFound;
}



function validateQuota(controlDocument, originalQuotaId, originalQuotaIp)
{
	originalQuotaId = originalQuotaId == null ? "" : originalQuotaId;
	originalQuotaIp = originalQuotaIp == null ? "none" : originalQuotaIp; //null is not the same as "" -- the latter gets interpretted as "ALL"

	controlDocument = controlDocument == null ? document : controlDocument;


	var inputIds = ["max_up", "max_down", "max_combined", "active_hours", "active_weekly"];
	var labelIds = ["max_up_label", "max_down_label", "max_combined_label", "quota_active_label", "quota_active_label"];
	var functions = [validateDecimal, validateDecimal, validateDecimal, validateHours, validateWeeklyRange];
	var validReturnCodes = [0,0,0,0,0];
	var visibilityIds = ["max_up_container","max_down_container","max_combined_container", "active_hours_container", "active_weekly_container"];
	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );

	//add any ips in add_ip box, if it is visible and isn't empty
	if(errors.length == 0 && getSelectedValue("applies_to_type", controlDocument) == "only" && controlDocument.getElementById("add_ip").value != "")
	{
		var valid = addAddressesToTable(controlDocument,"add_ip","quota_ip_table_container","quota_ip_table",false, 3, false,250);
		if(!valid)
		{
			errors.push("\"" + controlDocument.getElementById("add_ip").value  + "\" is not a valid IP or IP range");
		}
	}

	// check that ip is not empty (e.g. that we are matching based on IP(s) and no ips are defined)
	// thw getIpFromDocument function will always return ALL in the case where uci had no ip originallly, 
	// so we don't have to worry about empty ip meaning ALL vs null here
	var ip = "";
	if(errors.length == 0)
	{
		ip = getIpFromDocument(controlDocument);
		if(ip == "")
		{
			errors.push(quotasStr.IPError);
		}
	}

	//check that up,down,total aren't all unlimited 
	if(errors.length == 0)
	{
		if( 	getSelectedValue("max_up_type", controlDocument) == "unlimited" && 
			getSelectedValue("max_down_type", controlDocument) == "unlimited" && 
			getSelectedValue("max_combined_type", controlDocument) == "unlimited"
			)
		{
			errors.push(quotasStr.AllUnlimitedError);
		}
	}

	//check that any quota with overlapping ips with this one doesn't have overlapping time ranges
	if(errors.length == 0)
	{
		if(ip != originalQuotaIp)
		{
			var quotaSections = uci.getAllSectionsOfType(pkg, "quota");
			var sectionIndex;
			var overlapFound = false;
			for(sectionIndex=0; sectionIndex < quotaSections.length && (!overlapFound); sectionIndex++)
			{
				var sectionId = uci.get(pkg, quotaSections[sectionIndex], "id");
				if(sectionId != originalQuotaId)
				{
					var sectionIp = uci.get(pkg, quotaSections[sectionIndex], "ip");
					var ipOverlap = testAddrOverlap(sectionIp, ip);
					if(ipOverlap)
					{
						//test time range overlap
						var sectionTime = getTimeParametersFromDocument(controlDocument);
						var testTime = getTimeParametersFromUci(uci, quotaSections[sectionIndex]);
						sectionTime[3] = sectionTime[3] == "except" ? true : false;
						testTime[3] = testTime[3] == "except" ? true : false;
						overlapFound = rangesOverlap(sectionTime, testTime);
					}
				}
			}
			
			if(overlapFound)
			{	
				if(!ip.match(/ALL/))
				{
					errors.push(quotasStr.DuplicateRange);
				}
				else if(ip.match(/OTHER/))
				{
					errors.push(quotasStr.OneTimeQuotaError);
				}
				else
				{
					errors.push(quotasStr.OneNetworkQuotaError);
				}
			}
		}
	}
	return errors;
}

function setDocumentFromUci(controlDocument, srcUci, id)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	

	var quotaSection = "";
	var sections = srcUci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length && quotaSection == ""; sectionIndex++)
	{
		if(srcUci.get(pkg, sections[sectionIndex], "id") == id )
		{
			quotaSection = sections[sectionIndex];
		}
	}

	var ip = srcUci.get(pkg, quotaSection, "ip");
	ip = ip == "" ? "ALL" : ip;

	var resetInterval = srcUci.get(pkg, quotaSection, "reset_interval");
	var uploadLimit = srcUci.get(pkg, quotaSection, "egress_limit");
	var downloadLimit = srcUci.get(pkg, quotaSection, "ingress_limit");
	var combinedLimit = srcUci.get(pkg, quotaSection, "combined_limit");

	resetInterval = resetInterval == "" || resetInterval == "minute" ? "day" : resetInterval;
	var offset = srcUci.get(pkg, quotaSection, "reset_time");
	offset = offset == "" ? 0 : parseInt(offset);
	var resetDay = getDaySeconds(offset);
	var resetHour = getHourSeconds(offset);

	var exceededUpSpeed = srcUci.get(pkg, quotaSection, "exceeded_up_speed");
	var exceededDownSpeed = srcUci.get(pkg, quotaSection, "exceeded_down_speed");
	var upMark = srcUci.get(pkg, quotaSection, "exceeded_up_class_mark");
	var downMark = srcUci.get(pkg, quotaSection, "exceeded_down_class_mark");


	setDocumentIp(ip, controlDocument);
	setSelectedValue("quota_reset", resetInterval, controlDocument);



	var timeParameters = getTimeParametersFromUci(srcUci, quotaSection);
	var days = timeParameters[1];

	var allDays = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
	var dayList = [];
	if(days == "")
	{
		dayList = allDays;
	}
	else
	{
		dayList = days.split(/,/);
	}
	var dayIndex=0;
	for(dayIndex = 0; dayIndex < allDays.length; dayIndex++)
	{
		var nextDay = allDays[dayIndex];
		var dayFound = false;
		var testIndex=0;
		for(testIndex=0; testIndex < dayList.length && !dayFound; testIndex++)
		{
			dayFound = dayList[testIndex] == nextDay;
		}
		controlDocument.getElementById("quota_" + allDays[dayIndex]).checked = dayFound;
	}

	controlDocument.getElementById("active_hours").value = timeParameters[0];
	controlDocument.getElementById("active_weekly").value = timeParameters[2];

	var active = timeParameters[3];
	setSelectedValue("quota_active", active, controlDocument);
	if(active != "always")
	{
		var activeTypes = [];
		activeTypes["000"] = "hours";
		activeTypes["100"] = "hours";
		activeTypes["010"] = "days";
		activeTypes["110"] = "days_and_hours";
		var activeTypeId = (timeParameters[0] != "" ? "1" : "0") + (timeParameters[1] != "" ? "1" : "0") + (timeParameters[2] == "" ? "0" : "1");
		var activeType = activeTypes[activeTypeId] != null ? activeTypes[activeTypeId] : "weekly_range";
		setSelectedValue("quota_active_type", activeType, controlDocument);
	}
	

	setSelectedValue("max_up_type", uploadLimit == "" ? "unlimited" : "limited", controlDocument );
	setSelectedValue("max_down_type", downloadLimit == "" ? "unlimited" : "limited", controlDocument );
	setSelectedValue("max_combined_type", combinedLimit == "" ? "unlimited" : "limited", controlDocument );

	
	setDocumentLimit(uploadLimit,   "max_up",       "max_up_unit", controlDocument);
	setDocumentLimit(downloadLimit, "max_down",     "max_down_unit", controlDocument);
	setDocumentLimit(combinedLimit, "max_combined", "max_combined_unit", controlDocument);

	//setAllowableSelections("quota_exceeded", (fullQosEnabled ? ["hard_cutoff"] : ["hard_cutoff", "throttle"]), (fullQosEnabled ? ["Shut Down All Internet Access"] : ["Shut Down All Internet Access", "Throttle Bandwidth"]), controlDocument);
	var exceededType = (exceededUpSpeed != "" && exceededDownSpeed != "" && (!fullQosEnabled)) || (upMark != "" && downMark != "" && fullQosEnabled) ? "throttle" : "hard_cutoff";
	setSelectedValue("quota_exceeded", exceededType, controlDocument);
	setDocumentSpeed(exceededUpSpeed, "quota_qos_up",   "quota_qos_up_unit", controlDocument);
	setDocumentSpeed(exceededDownSpeed, "quota_qos_down", "quota_qos_down_unit", controlDocument);

	

	setVisibility(controlDocument);
	setSelectedValue("quota_day", resetDay + "", controlDocument);
	setSelectedValue("quota_hour", resetHour + "", controlDocument);

	if(fullQosEnabled)
	{
		setAllowableSelections("quota_full_qos_up_class", upQosMarks, upQosClasses, controlDocument);
		setAllowableSelections("quota_full_qos_down_class", downQosMarks, downQosClasses, controlDocument);
		if(upMark != "" && downMark != "")
		{
			setSelectedValue("quota_full_qos_up_class", upMark, controlDocument);
			setSelectedValue("quota_full_qos_down_class", downMark, controlDocument);
		}
	}
}

function setDocumentLimit(bytes, textId, unitSelectId, controlDocument)
{
	bytes = bytes == "" ? 0 : parseInt(bytes);
	var textEl = controlDocument.getElementById(textId);
	var defaultUnit = UI.MB;
	var defaultMultiple = 1024*1024;
	if(bytes <= 0)
	{
		setSelectedValue(unitSelectId, defaultUnit, controlDocument);
		textEl.value = "0";
	}
	else
	{
		var pb = parseBytes(bytes);
		var unit = defaultUnit;
		var multiple = defaultMultiple;
		if(pb.match(new RegExp(UI.GBy))) { unit = UI.GB; multiple = 1024*1024*1024; };
		if(pb.match(new RegExp(UI.TBy))) { unit = UI.TB; multiple = 1024*1024*1024*1024; };
		setSelectedValue(unitSelectId, unit, controlDocument);
		var adjustedVal = truncateDecimal(bytes/multiple);
		textEl.value = adjustedVal;
	}
}
function setDocumentSpeed(kbytes, textId, unitSelectId, controlDocument)
{
	var defaultUnit = UI.KBs;
	var textEl = controlDocument.getElementById(textId);
	setSelectedValue(unitSelectId, defaultUnit, controlDocument);
	
	kbytes = kbytes == "" ? 0 : parseInt(kbytes);
	if(kbytes <= 0)
	{
		textEl.value = "0";
	}
	else
	{
		var pb = parseKbytesPerSecond(kbytes);
		var splitParsed = pb.split(/[\t ]+/);
		textEl.value = splitParsed[0];
		switch (splitParsed[1])
		{
		case UI.KBs:
			defaultUnit = 'KBytes/s'; break;
		case UI.MBs:
			defaultUnit = 'MBytes/s'; break;
		}
		setSelectedValue(unitSelectId, defaultUnit, controlDocument);
	}
}


function setUciFromDocument(controlDocument, id)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var ip = getIpFromDocument(controlDocument);
	id = id == null ? "" : id;
	id = id == "" ? getIdFromIp(ip) : id;

	var quotaSection = "";
	var sections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "id") == id)
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
		if(!testAddrOverlap(oldIp, ip))
		{
			changedIds[id] = 1;
		}
	}

	
	uci.set(pkg, quotaSection, "ingress_limit",  getDocumentLimit("max_down", "max_down_type", "max_down_unit", controlDocument)  );
	uci.set(pkg, quotaSection, "egress_limit",   getDocumentLimit("max_up", "max_up_type", "max_up_unit", controlDocument) );
	uci.set(pkg, quotaSection, "combined_limit", getDocumentLimit("max_combined", "max_combined_type", "max_combined_unit", controlDocument) );

	uci.set(pkg, quotaSection, "exceeded_up_speed", getDocumentSpeed("quota_only_qos_container", "quota_qos_up", "quota_qos_up_unit", controlDocument) );
	uci.set(pkg, quotaSection, "exceeded_down_speed", getDocumentSpeed("quota_only_qos_container", "quota_qos_down", "quota_qos_down_unit", controlDocument) );

	uci.set(pkg, quotaSection, "exceeded_up_class_mark", getDocumentMark("quota_full_qos_container", "quota_full_qos_up_class", controlDocument) );
	uci.set(pkg, quotaSection, "exceeded_down_class_mark", getDocumentMark("quota_full_qos_container", "quota_full_qos_down_class", controlDocument) );


	uci.set(pkg, quotaSection, "reset_interval", getSelectedValue("quota_reset", controlDocument));
	uci.set(pkg, quotaSection, "ip", ip);
	uci.set(pkg, quotaSection, "id", id);

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


	var timeParameters = getTimeParametersFromDocument(controlDocument);
	var active = timeParameters[3];
	var onoff = ["offpeak", "onpeak"];
	var onoffIndex = 0;
	for(onoffIndex=0; onoffIndex < onoff.length; onoffIndex++)
	{
		var prefix = onoff[onoffIndex];
		var updateFun = function(prefixActive,option,val)
		{ 
			if(prefixActive)
			{
				uci.set(pkg,quotaSection,option,val); 
			}
			else
			{
				uci.remove(pkg,quotaSection,option);
			}
		}
		var prefixActive = (prefix == "offpeak" && active == "except") || (prefix == "onpeak" && active == "only");
		updateFun(prefixActive, prefix + "_hours", timeParameters[0]);
		updateFun(prefixActive, prefix + "_weekdays", timeParameters[1]);
		updateFun(prefixActive, prefix + "_weekly_ranges", timeParameters[2]);
	}
}

function getTimeParametersFromDocument(controlDocument)
{
	var hours = controlDocument.getElementById("active_hours_container").style.display != "none" ? controlDocument.getElementById("active_hours").value : "";
	var weekly = controlDocument.getElementById("active_weekly_container").style.display != "none" ? controlDocument.getElementById("active_weekly").value : "";
	
	var dayList = [];
	if(controlDocument.getElementById("active_days_container").style.display != "none")
	{
		var allDays = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
		var dayIndex;
		for(dayIndex=0; dayIndex < allDays.length; dayIndex++)
		{
			if( controlDocument.getElementById("quota_" + allDays[dayIndex]).checked )
			{
				dayList.push( allDays[dayIndex]);
			}
		}
	}
	var days = "" + dayList.join(",");

	var active = getSelectedValue("quota_active", controlDocument);
	
	return [hours,days,weekly_i18n(weekly, "page"),active];

}
function getTimeParametersFromUci(srcUci, quotaSection, i18ndays)
{
	var hours = srcUci.get(pkg, quotaSection, "offpeak_hours");
	var days = srcUci.get(pkg, quotaSection, "offpeak_weekdays");
	var weekly = srcUci.get(pkg, quotaSection, "offpeak_weekly_ranges");
	var active = hours != "" || days != "" || weekly != "" ? "except" : "always";
	if(active == "always")
	{
		hours = srcUci.get(pkg, quotaSection, "onpeak_hours");
		days = srcUci.get(pkg, quotaSection, "onpeak_weekdays");
		weekly = srcUci.get(pkg, quotaSection, "onpeak_weekly_ranges");
		active = hours != "" || days != "" || weekly != "" ? "only" : "always";

	}
	return [hours,(i18ndays==1?dayToi18n(days):days),weekly_i18n(weekly, "uci"),active];
}


/* returns a number if there is a limit "" if no limit defined */
function getDocumentLimit(textId, unlimitedSelectId, unitSelectId, controlDocument)
{
	var ret = "";
	if(getSelectedValue(unlimitedSelectId, controlDocument) != "unlimited")
	{
		var unit = getSelectedValue(unitSelectId, controlDocument);
		var multiple = 1024*1024;
		if(unit == "MB") { multiple = 1024*1024; }
		if(unit == "GB") { multiple = 1024*1024*1024; }
		if(unit == "TB") { multiple = 1024*1024*1024*1024; }
		var bytes = Math.round(multiple * parseFloat(controlDocument.getElementById(textId).value));
		ret =  "" + bytes;
	}
	return ret;
}

function getDocumentSpeed(containerId, textId, unitSelectId, controlDocument)
{
	var ret = "";
	if(controlDocument.getElementById(containerId).style.display != "none")
	{
		var unit = getSelectedValue(unitSelectId, controlDocument);
		if(unit == "MBytes/s") { multiple = 1024; }
		if(unit == "KBytes/s") { multiple = 1; }
		var kbits = Math.round(multiple * parseFloat(controlDocument.getElementById(textId).value));
		ret = "" + kbits;
	}
	return ret;
}

function getDocumentMark(containerId, selectId, controlDocument)
{
	var ret = "";
	if(controlDocument.getElementById(containerId).style.display != "none")
	{
		ret = getSelectedValue(selectId, controlDocument);
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
	editButton.value = UI.Edit;
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

	enabledRow.childNodes[rowCheckIndex+1].firstChild.disabled  = this.checked ? false : true;
	enabledRow.childNodes[rowCheckIndex+1].firstChild.className = this.checked ? "default_button" : "default_button_disabled" ;

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
	var id = row.childNodes[rowCheckIndex].firstChild.id;
	var sections = uci.getAllSectionsOfType(pkg, "quota");                                                               
	for(sectionIndex=0; sectionIndex < sections.length; sectionIndex++)                             
	{                                                                                                                    
		if(uci.get(pkg, sections[sectionIndex], "id") == id)
		{
			uci.removeSection(pkg, sections[sectionIndex]);
		}
	}
	changedIds [ id ] = 1;
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
	
	var saveButton = createInput("button", editQuotaWindow.document);
	var closeButton = createInput("button", editQuotaWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
	closeButton.className = "default_button";

	var editRow=this.parentNode.parentNode;
	var editId          = editRow.childNodes[rowCheckIndex].firstChild.id;
	
	var editIp;

	var editSection = "";
	var sections = uci.getAllSectionsOfType(pkg, "quota");
	for(sectionIndex=0; sectionIndex < sections.length && editSection == ""; sectionIndex++)
	{
		if(uci.get(pkg, sections[sectionIndex], "id") == editId)
		{
			editSection = sections[sectionIndex];
			editIp = uci.get(pkg, editSection, "ip");
		}
	}

	var editUpMax       = uci.get(pkg, editSection, "egress_limit");
	var editDownMax     = uci.get(pkg, editSection, "ingress_limit");
	var editCombinedMax = uci.get(pkg, editSection, "combined_limit");

	var runOnEditorLoaded = function () 
	{
		var updateDone=false;
		if(editQuotaWindow.document != null)
		{
			if(editQuotaWindow.document.getElementById("bottom_button_container") != null)
			{
				editQuotaWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editQuotaWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
				setDocumentFromUci(editQuotaWindow.document, uci, editId);

				closeButton.onclick = function()
				{
					editQuotaWindow.close();
				}
				saveButton.onclick = function()
				{
					// error checking goes here
					var errors = validateQuota(editQuotaWindow.document, editId, editIp);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n"+quotasStr.QuotaAddError);
					}
					else
					{
						var newIp = getIpFromDocument(editQuotaWindow.document);
						setUciFromDocument(editQuotaWindow.document, editId);

						var setElementAtColumn = function(newEl, cellIndex)
						{
							var cell = editRow.childNodes[cellIndex];
							while(cell.firstChild != null){ cell.removeChild(cell.firstChild); }
							cell.appendChild(newEl);
						}

						if(newIp != editIp)
						{
							var newId = getIdFromIp(newIp);
							uci.set(pkg, editSection, "id", newId);
							if(!testAddrOverlap(newIp, editIp))
							{
								changedIds[newId] = 1;
							}
							
							
							setElementAtColumn(ipToTableSpan(newIp), 0);
							editRow.childNodes[rowCheckIndex].firstChild.id = newId;
						}
						setElementAtColumn(timeParamsToTableSpan(getTimeParametersFromUci(uci, editSection, 1)), 1);
						editRow.childNodes[2].firstChild.data =getLimitStrFromUci(uci, editSection);
						
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
			setTimeout(runOnEditorLoaded, 250);
		}
	}
	runOnEditorLoaded();
}

function dayToi18n(daystrings) { //this is part of i18n; TODO: best to have an uci get language to see if absent to just return daystrings
	var days=daystrings.split(",");
	for (var i = 0; i < days.length; i++) {
		if (days[i] == "sun") { days[i] = UI.Sun; }
		if (days[i] == "mon") { days[i] = UI.Mon; }
		if (days[i] == "tue") { days[i] = UI.Tue; }
		if (days[i] == "wed") { days[i] = UI.Wed; }
		if (days[i] == "thu") { days[i] = UI.Thu; }
		if (days[i] == "fri") { days[i] = UI.Fri; }
		if (days[i] == "sat") { days[i] = UI.Sat; }
	}
	return days.join();
}

function weekly_i18n(weekly_schd, direction) { //this is part of i18n; TODO: best to have an uci get language to see if absent to just return daystrings
	if (weekly_schd.length < 6) return weekly_schd;
	var localdays=[UI.Sun, UI.Mon, UI.Tue, UI.Wed, UI.Thu, UI.Fri, UI.Sat];
	var fwdays=["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
	var indays, outdays, splits, idx;
	var joiner=[];

	if (direction == "uci") {
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
