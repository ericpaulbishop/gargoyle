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
	setInterval("updateTableData()", 1500);
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
	return [hours,days,weekly,active];
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
				quotaTableData.push( [ textListToSpanElement(hostList, true, document), timeParamsToTableSpan(timeParameters), total, down, up ] );
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
