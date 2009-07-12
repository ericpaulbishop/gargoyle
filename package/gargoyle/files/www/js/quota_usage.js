/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var pkg = "firewall";
var updateInProgress = false;

function resetData()
{
	refreshTableData();
	setInterval("updateTableData()", 1500);
}

function refreshTableData()
{
	var quotaSections = uciOriginal.getAllSectionsOfType(pkg, "quota");
	var quotaTableData = [];
	var ipIndex;
	for(ipIndex=0; ipIndex < allQuotaIps.length; ipIndex++)
	{
		var ip = allQuotaIps[ipIndex];
		ip = ip.replace(/\//g, "_");
		var up       = "N/A";
		var down     = "N/A";
		var combined = "N/A";
		if(allQuotaPercents[ip] != null)
		{
			var usds = allQuotaUsed[ip];
			var lims = allQuotaLimits[ip];
			var pcts = allQuotaPercents[ip];

			var pkg = "firewall";

			up = pcts[0] >= 0 ? pcts[0] + "%" : up;
			down = pcts[1] >= 0 ? pcts[1] + "%" : down;
			combined = pcts[2] >= 0 ? pcts[2] + "%" : combined;
		}
		quotaTableData.push( [ ip.replace(/_/g, " "), up, down, combined ] );
	}
	var columnNames = ["IP", "% Upload Used", "% Download Used", "% Combined Used" ];
	
	var quotaTable = createTable(columnNames, quotaTableData, "quota_usage_table", false, false);
	var tableContainer = document.getElementById('quota_table_container');
	if(tableContainer.firstChild != null)
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
				allQuotaUsed     = quotaUsed;
				allQuotaLimits   = quotaLimits;
				allQuotaPercents = quotaPercents;
				refreshTableData();
				updateInProgress = false;
			}
		}
	}

	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}	
