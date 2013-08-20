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

	refreshTableData();
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


function refreshTableData()
{
	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var quotaTableData = [];
	var idIndex;
	for(idIndex=0; idIndex < allQuotaIds.length; idIndex++)
	{
		var ipIndex;
		var id =  allQuotaIds[idIndex];
		var quotaIpList = allQuotaIps[ id ];
		var timeParameters = idToTimeParams[ id ];
		for(ipIndex=0; ipIndex < quotaIpList.length; ipIndex++)
		{
			var ip       = quotaIpList[ipIndex];
			var up       = "N/A";
			var down     = "N/A";
			var total    = "N/A";
			if(allQuotaPercents[id] != null)
			{
				if(allQuotaPercents[id][ip] != null)
				{
					var usds = allQuotaUsed[id][ip];
					var lims = allQuotaLimits[id][ip];
					var pcts = allQuotaPercents[id][ip];

					total = pcts[0] >= 0 ? pcts[0] + "%" : total;
					down = pcts[1] >= 0 ? pcts[1] + "%" : down;
					up = pcts[2] >= 0 ? pcts[2] + "%" : up;
				}
			}
			ipList = ip.split(/[\t ]*,[\t ]*/);
			hostList = getHostList(ipList);
			quotaTableData.push( [ textListToSpanElement(hostList, true, document), timeParamsToTableSpan(timeParameters), total, down, up ] );
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
				refreshTableData();
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}

}	
