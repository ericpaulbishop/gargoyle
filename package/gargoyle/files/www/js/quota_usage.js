/*
 * This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var quotasStr=new Object(); //part of i18n

var pkg = "firewall";
var updateInProgress = false;

var allQuotaIds;
var allQuotaIps;
var allQuotaUsed;
var allQuotaLimits;
var allQuotaPercents;

var idToSection = [];
var idToIpStr = [];
var idToTimeParams = [];

var refreshInterval;

function resetData()
{
	allQuotaIds      = quotaIdList;
	allQuotaIps      = quotaIpLists;
	allQuotaUsed     = quotaUsed;
	allQuotaLimits   = quotaLimits;
	allQuotaPercents = quotaPercents;

	idToSection = [];
	idToIpStr = [];
	idToTimeParams = [];

	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var qIndex;
	for(qIndex=0; qIndex < quotaSections.length; qIndex++)
	{
		var ip = getIpFromUci(uciOriginal, quotaSections[qIndex]);
		var id = uciOriginal.get(pkg, quotaSections[qIndex], "id");
		id = id == "" ? getIdFromIp(ip, uciOriginal) : id;

		idToSection[ id ] = quotaSections[qIndex];
		idToIpStr[ id ] = ip;
		idToTimeParams[ id ] = getTimeParametersFromUci(uciOriginal, quotaSections[qIndex]);
	}

	refreshTableData(document.getElementById("data_display").value);
	refreshInterval = setInterval("updateTableData()", 1500);
}
function getIpFromUci(srcUci, quotaSection)
{
	var ipStr = srcUci.get(pkg, quotaSection, "ip");
	if(ipStr == "ALL_OTHERS_INDIVIDUAL")
	{
		ipStr=quotasStr.OthersOne;
	}
	else if(ipStr == "ALL_OTHERS_COMBINED")
	{
		ipStr = quotasStr.OthersAll;
	}
	else if(ipStr == "ALL" || ipStr == "")
	{
		ipStr = quotasStr.All;
	}
	return ipStr;
}
function getIdFromIp(ip, srcUci)
{
	id = ip == "" ? "ALL" : ip.replace(/[\t, ]+.*$/, "");
	id = id.replace(/\//, "_");

	var idPrefix = id;
	var found = true;
	var suffixCount = 0;

	var quotaSections = srcUci.getAllSectionsOfType(pkg, "quota");

	while(found)
	{
		found = false;
		var sectionIndex;
		for(sectionIndex=0; sectionIndex < quotaSections.length && (!found); sectionIndex++)
		{
			found = found || srcUci.get(pkg, quotaSections[sectionIndex], "id") == id;
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
function getTimeParametersFromUci(srcUci, quotaSection)
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
	return [hours,dayToi18n(days),weekly_i18n(weekly,"uci"),active];
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
		textList.unshift(quotasStr.Alws);
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


function getHostDisplay(ip)
{
	var hostDisplay = getSelectedValue("host_display");
	var host = ip;
	if(hostDisplay == "hostname" && ipToHostname[ip] != null)
	{
		host = ipToHostname[ip];
		host = host.length < 25 ? host : host.substr(0,22)+"...";
	}
	return host;
}


function getHostList(ipList)
{
	var hostList = [];
	var ipIndex =0;
	for(ipIndex=0; ipIndex < ipList.length; ipIndex++)
	{
		hostList.push( getHostDisplay(ipList[ipIndex]));
	}
	return hostList;
}


function refreshTableData(scheme)
{
	scheme = scheme == null ? "pcts" : scheme;
	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var quotaTableData = [];

	var quotaData = [];
	if (scheme.localeCompare("usds") == 0)
	{
		quotaData = allQuotaUsed;
	}
	else if (scheme.localeCompare("lims") == 0)
	{
		quotaData = allQuotaLimits;
	}
	else if (scheme.localeCompare("pcts") == 0)
	{
		quotaData = allQuotaPercents;
	}

	var idIndex;
	for(idIndex=0; idIndex < allQuotaIds.length; idIndex++)
	{
		var ipIndex;
		var id =  allQuotaIds[idIndex];
		var quotaIpList = allQuotaIps[ id ];
		var timeParameters = idToTimeParams[ id ];
		var hide = false;
		for(ipIndex=0; ipIndex < quotaIpList.length; ipIndex++)
		{
			var ip       = quotaIpList[ipIndex];
			var up       = "N/A";
			var down     = "N/A";
			var total    = "N/A";
			if(quotaData[id] != null)
			{
				if(quotaData[id][ip] != null)
				{
					var data = quotaData[id][ip];
					var usedData = allQuotaUsed[id][ip]
					var noData = usedData[0]<=0 && usedData[1]<=0 && usedData[2]<=0 ;
					hide = noData && id.localeCompare("ALL_OTHERS_INDIVIDUAL") == 0;
					if (scheme.localeCompare("pcts") == 0)
					{
						total = data[0] >= 0 ? percentColorSpan(data[0]) : total;
						down = data[1] >= 0 ? percentColorSpan(data[1]) : down;
						up = data[2] >= 0 ? percentColorSpan(data[2]) : up;
					}
					else
					{
						total = data[0] >= 0 ? parseBytes(data[0]) : total;
						down = data[1] >= 0 ? parseBytes(data[1]) : down;
						up = data[2] >= 0 ? parseBytes(data[2]) : up;
					}
				}
			}
			ipList = ip.split(/[\t ]*,[\t ]*/);
			hostList = getHostList(ipList);
			if (!hide)
			{
				quotaTableData.push( [ textListToSpanElement(hostList, true, document), timeParamsToTableSpan(timeParameters), total, down, up, createSetButton(id,ipList) ] );
			}
		}
	}
	var columnNames = quotasStr.ColNms;

	var quotaTable = createTable(columnNames, quotaTableData, "quota_usage_table", false, false);
	var tableContainer = document.getElementById('quota_table_container');
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(quotaTable);
}

function percentColorSpan(percent)
{
	span = document.createElement("span");
	text = null;
	color = 'black';
	if (percent != null)
	{
		text = document.createTextNode(percent + "%");

		var toHexTwo = function(num) { var ret = parseInt(num).toString(16).toUpperCase(); ret= ret.length < 2 ? "0" + ret : ret.substr(0,2); return ret; }

		color = percent >= 100  ? "#AA0000" : "";
		color = percent >= 50 && percent < 100 ? "#AA" + toHexTwo(170-(170*((percent-50)/50.0))) + "00" : color;
		color = percent >= 0 && percent < 50 ? "#" + toHexTwo(170*(percent)/50.0) + "AA00" : color;
		color = percent <= 0 ? "#00AA00" : color;
		span.style.color = color;
	}
	span.appendChild(text);
	return span;
}

function updateTableData()
{
	if(!updateInProgress)
	{
		updateInProgress = true;

		var command = "print_quotas\n";
		var param = getParameterDefinition("commands", command) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var text = req.responseText.split(/[\r\n]+/);
				var next = "";
				while(text.length > 0 && next != "Success")
				{
					next = text.pop();
				}
				eval(text.join("\n"));
				allQuotaIds      = quotaIdList;
				allQuotaIps      = quotaIpLists;
				allQuotaUsed     = quotaUsed;
				allQuotaLimits   = quotaLimits;
				allQuotaPercents = quotaPercents;
				refreshTableData(document.getElementById("data_display").value);
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}

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

function createSetButton(id,iplist)
{
	setButton = createInput("button");
	setButton.textContent = quotasStr.SetQuota;
	setButton.className = "btn btn-default btn-edit";
	setButton.onclick = function(){setQuotaModal(id,iplist);};

	setElementEnabled(setButton, true);

	return setButton;
}

function doSetQuota(id,iplist)
{
	var createCmdStr = function(id,quotaId,ipList,quotaVal) {
		// Beware escaping things properly here
		var cmdLines = [];
		cmdLines.push("bw_get -i " + quotaId + " -f /tmp/" + quotaId + ".quota");
		cmdLines.push("sed -i 's/\\(.*\t" + (id == "ALL_OTHERS_INDIVIDUAL" ? ipList : "0.0.0.0") + "\\)[\t ].*/\\1\t" + quotaVal + "/' /tmp/" + quotaId + ".quota");
		cmdLines.push("bw_set -i " + quotaId + " -f /tmp/" + quotaId + ".quota");
		cmdLines.push("rm /tmp/" + quotaId + ".quota");
		
		return cmdLines.join("\n");
	};
	var setUpCheck = byId("use_set_up").checked;
	var setDownCheck = byId("use_set_down").checked;
	var setCombinedCheck = byId("use_set_combined").checked;
	
	var cmd = [];
	if(setUpCheck)
	{
		var quotaID = id + "_egress";
		var setVal = byId("set_up").value;
		var setUnit = getSelectedValue("set_up_unit");
		var quotaVal = toBytes(setVal,setUnit);
		cmd.push(createCmdStr(id,quotaID,iplist,quotaVal));
	}
	if(setDownCheck)
	{
		var quotaID = id + "_ingress";
		var setVal = byId("set_down").value;
		var setUnit = getSelectedValue("set_down_unit");
		var quotaVal = toBytes(setVal,setUnit);
		cmd.push(createCmdStr(id,quotaID,iplist,quotaVal));
	}
	if(setCombinedCheck)
	{
		var quotaID = id + "_combined";
		var setVal = byId("set_combined").value;
		var setUnit = getSelectedValue("set_combined_unit");
		var quotaVal = toBytes(setVal,setUnit);
		cmd.push(createCmdStr(id,quotaID,iplist,quotaVal));
	}

	if(cmd.length > 0)
	{
		var command = cmd.join("\n");
		var param = getParameterDefinition("commands", command) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var text = req.responseText.split(/[\r\n]+/);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}

	closeQuotaModal();
}

function closeQuotaModal()
{
	refreshInterval = setInterval("updateTableData()", 1500); // restart GUI updates
	closeModalWindow('quotas_modal');
}

function setQuotaModal(id,iplist)
{
	var unitOpts = {};
	unitOpts['KB'] = UI.KBy;
	unitOpts['MB'] = UI.MBy;
	unitOpts['GB'] = UI.GBy;
	unitOpts['TB'] = UI.TBy;
	joinIpList = iplist.join(",");

	upCurrent = allQuotaUsed[id][joinIpList][2];
	downCurrent = allQuotaUsed[id][joinIpList][1];
	combinedCurrent = allQuotaUsed[id][joinIpList][0];
	upCurrent = (upCurrent == -1 ? " " : parseBytes(upCurrent)).split(" ");
	downCurrent = (downCurrent == -1 ? " " : parseBytes(downCurrent)).split(" ");
	combinedCurrent = (combinedCurrent == -1 ? " " : parseBytes(combinedCurrent)).split(" ");
	upCurrentUnit = getKeyByValue(unitOpts,upCurrent[1]);
	upCurrent = upCurrent[0];
	downCurrentUnit = getKeyByValue(unitOpts,downCurrent[1]);
	downCurrent = downCurrent[0];
	combinedCurrentUnit = getKeyByValue(unitOpts,combinedCurrent[1]);
	combinedCurrent = combinedCurrent[0];

	upLimit = allQuotaLimits[id][joinIpList][2];
	downLimit = allQuotaLimits[id][joinIpList][1];
	combinedLimit = allQuotaLimits[id][joinIpList][0];
	upLimit = upLimit == -1 ? "N/A" : parseBytes(upLimit);
	downLimit = downLimit == -1 ? "N/A" : parseBytes(downLimit);
	combinedLimit = combinedLimit == -1 ? "N/A" : parseBytes(combinedLimit);

	upDisabled = (upCurrent == "");
	downDisabled = (downCurrent == "");
	combinedDisabled = (combinedCurrent == "");

	enableBothQuotaFields = function(triggerEl,fieldName,data,unit) {
		enableAssociatedField(triggerEl,fieldName,data);
		enableAssociatedField(triggerEl,fieldName+'_unit',unit);
	};

	byId("use_set_up").onclick = function() {
		enableBothQuotaFields(this,'set_up',upCurrent,upCurrentUnit);
	};
	byId("use_set_down").onclick = function() {
		enableBothQuotaFields(this,'set_down',downCurrent,downCurrentUnit);
	};
	byId("use_set_combined").onclick = function() {
		enableBothQuotaFields(this,'set_combined',combinedCurrent,combinedCurrentUnit);
	};

	clearInterval(refreshInterval); // pause GUI updates

	modalButtons = [
		{"title" : UI.CApplyChanges, "classes" : "btn btn-primary", "function" : function(){doSetQuota(id,joinIpList);}},
		{"title" : UI.CDiscardChanges, "classes" : "btn btn-warning", "function" : function(){closeQuotaModal();}}
	];

	modalElements = [
		{"id" : "set_up", "value" : upCurrent, "disable" : true},
		{"id" : "set_down", "value" : downCurrent, "disable" : true},
		{"id" : "set_combined", "value" : combinedCurrent, "disable" : true},
		{"id" : "use_set_up", "checked" : false, "disable" : upDisabled},
		{"id" : "use_set_down", "checked" : false, "disable" : downDisabled},
		{"id" : "use_set_combined", "checked" : false, "disable" : combinedDisabled},
		{"id" : "set_up_unit", "options" : unitOpts, "value" : upCurrentUnit, "disable" : true},
		{"id" : "set_down_unit", "options" : unitOpts, "value" : downCurrentUnit, "disable" : true},
		{"id" : "set_combined_unit", "options" : unitOpts, "value" : combinedCurrentUnit, "disable" : true},
		{"id" : "set_up_limit", "innertext" : upLimit},
		{"id" : "set_down_limit", "innertext" : downLimit},
		{"id" : "set_combined_limit", "innertext" : combinedLimit}
	];

	modalPrepare('quotas_modal', quotasStr.SetQuota, modalElements, modalButtons);
	openModalWindow('quotas_modal');
}
