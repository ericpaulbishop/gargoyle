/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
 
var bndwS=new Object(); //part of i18n - reused from bandwidth.js

var plotsInitializedToDefaults = false;
var updateInProgress = false;
var pieChart = null;
var initialized = false;

// data for current time frame
//at each index, are 3 more arrays (combined, down, up) each of which is an array of values for that interval
var timeFrameIntervalData = [];
var idList = [];
var resetColors = false;

function initializePlotsAndTable()
{
	updateInProgress = false;
	initFunction();
}

function initFunction()
{	
	pieChart = document.getElementById("pie_chart");
	if(pieChart != null)
	{
		doUpdate(); 
		setInterval( doUpdate, 2000);
	}
	else
	{
		setTimeout(initFunction, 50); 
	} 
}


function getEmbeddedSvgPlotFunction(embeddedId)
{
	windowElement = getEmbeddedSvgWindow(embeddedId);
	if( windowElement != null)
	{
		return windowElement.setPieChartData;
	}
	return null;
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


function doUpdate()
{
	if(!updateInProgress && pieChart != null)
	{
		var bdistId = getSelectedValue("time_frame");

		// get names of monitors to query (those that match bdistId)
		var downloadName = "";
		var uploadName = "";
		var mIndex=0;
		for(mIndex=0; mIndex < monitorNames.length; mIndex++)
		{
			var m = monitorNames[mIndex];
			if(m.indexOf(bdistId) >= 0)
			{
				if(m.indexOf("upload") >= 0)
				{
					uploadName = "" + m;
				}
				if(m.indexOf("download") >= 0)
				{
					downloadName = "" + m;
				}
			}
		}

		//query monitor data
		var queryNames = downloadName + " " + uploadName;
		var param = getParameterDefinition("monitor", queryNames)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			var monitors=null;
			if(req.readyState == 4)
			{
				if(!req.responseText.match(/ERROR/))
				{
					var parsed = parseMonitors(req.responseText);
					
					
					//calculate max intervals (we make everything this length by adding zeros)
					//also, get a list of all ids, in case up/down don't have same set of ips
					var numIntervals = 1;
					var dirIndex = 0;
					var latestTime = 0;
					var allIds = [];
					for(dirIndex=0; dirIndex < parsed.length; dirIndex++)
					{
						var dirData = parsed[dirIndex];
						for (id in dirData)
						{
							var idPoints = dirData[id][0];
							latestTime = parseInt(dirData[id][1]);
							numIntervals = idPoints.length > numIntervals ? idPoints.length : numIntervals;
							allIds[id] = 1;
						}
					}
					
					idList = [];
					for (id in allIds)
					{
						idList.push(id);
					}


					var currentIntervalIndex = getSelectedValue("time_interval");
					var currentIntervalText = getSelectedText("time_interval");
					var timeIntervalValues = [];
					var timeIntervalNames = [];

					var nextDate = new Date();
					nextDate.setTime(latestTime*1000);
					nextDate.setUTCMinutes( nextDate.getUTCMinutes()+tzMinutes );
					var nextIntervalStart = nextDate.valueOf()/1000;

					timeFrameIntervalData = [];
					var intervalNames = [];
					var intervalIndex;
					for (intervalIndex=0; intervalIndex < numIntervals; intervalIndex++)
					{
						var nextIntervalData = [];
						var dirIndex;
						var combinedData = [];
						for(dirIndex=0; dirIndex < parsed.length; dirIndex++)
						{
							var dirData = parsed[dirIndex];
							var nextDirData = [];
							var idIndex;
							for(idIndex=0; idIndex < idList.length; idIndex++)
							{
								var id = idList[idIndex];
								id =  id == currentWanIp ? currentLanIp : id ;	
								var value = 0;
								if(dirData[id] != null)
								{
									var idPoints = dirData[id][0];
									if(idPoints != null)
									{
										value = idPoints[idPoints.length-1-intervalIndex];
										value = value==null? 0 : parseFloat(value);
									}
								}
								nextDirData.push(value);
								combinedData[idIndex] = combinedData[idIndex] == null ? value : combinedData[idIndex] + value;
							}
							nextIntervalData.push(nextDirData);
						}
						nextIntervalData.unshift(combinedData);
						timeFrameIntervalData.push(nextIntervalData);
						
						
						
						var monthNames = bndwS.FMonths;
						var twod = function(num) { var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; return nstr; }
						
						nextDate.setTime(parseInt(nextIntervalStart)*1000);
						var intervalName = "";
						if(uploadName.match("minute"))
						{
							intervalName = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
							nextDate.setUTCMinutes( nextDate.getUTCMinutes()-1);
							
						}
						else if(uploadName.match("hour"))
						{
							intervalName = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
							nextDate.setUTCHours(nextDate.getUTCHours()-1);
						}
						else if(uploadName.match("day"))
						{
							intervalName = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCDate();
							nextDate.setUTCDate(nextDate.getUTCDate()-1);
						}
						else if(uploadName.match("month"))
						{
							intervalName = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCFullYear();
							nextDate.setUTCMonth(nextDate.getUTCMonth()-1);
						}
						else
						{
							var splitName = uploadName.split(/-/);
							var numIntervals = splitName.pop();
							var interval = splitName.pop();
							if(parseInt(interval) >= 28*24*60*60)
							{
								intervalName = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCFullYear() + " " + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
							}
							else if(parseInt(interval) >= 24*60*60)
							{
								intervalName = monthNames[nextDate.getUTCMonth()] + " " + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
							}
							else
							{
								intervalName = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
							}
							nextDate.setTime(nextDate.getTime()-(parseInt(interval)*1000));
						}
						timeIntervalNames.push(intervalName);
						timeIntervalValues.push(""+intervalIndex);
						nextIntervalStart = nextDate.valueOf()/1000;
					}
					setAllowableSelections("time_interval", timeIntervalValues, timeIntervalNames);
					if(currentIntervalIndex == null || currentIntervalIndex == 0)
					{
						setSelectedValue("time_interval", "0");
					}	
					else
					{
						setSelectedText("time_interval", currentIntervalText);
					}
				}
				updateInProgress = false;
				resetDisplayInterval();
			}
		}
		runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
	}
}

function resetTimeFrame()
{
	resetColors = true;
	doUpdate();
}




function resetDisplayInterval()
{
	var plotFunction = getEmbeddedSvgPlotFunction("pie_chart");
	if(plotFunction != null && pieChart != null && (!updateInProgress) && timeFrameIntervalData.length > 0 )
	{
		updateInProgress = true;
		//first, update pie chart
		var intervalIndex = getSelectedValue("time_interval");
		intervalIndex = intervalIndex == null ? 0 : intervalIndex;
		
		var data = timeFrameIntervalData[intervalIndex];
		var pieTotals = plotFunction(idList, [bndwS.Totl, bndwS.Dnld, bndwS.Upld ], getHostList(idList), data, 0, 9, resetColors);
		resetColors = false;

		//then update table, sorting ids alphabetically so order is consistant
		var sortedIdIndices = [];
		var idIndex;
		for(idIndex=0; idIndex < idList.length; idIndex++) { sortedIdIndices.push(idIndex) };
		var idSort = function(a,b) { return idList[a] < idList[b] ? 1 : -1; }	
		sortedIdIndices.sort( idSort );
		
		var pieNames = [bndwS.Totl, bndwS.Down, bndwS.Up];
		var tableRows = [];

		var pieIndex;
		zeroPies = [];
		for(pieIndex=0; pieIndex<pieNames.length; pieIndex++)
		{
			idIndex=0;
			var pieIsZero = true;
			for(idIndex=0; idIndex < idList.length; idIndex++)
			{
				pieIsZero = pieIsZero && data[pieIndex][idIndex] == 0;
			}
			zeroPies.push(pieIsZero);
		}

		var sum = [0,0,0];
		for(idIndex=0; idIndex < sortedIdIndices.length; idIndex++)
		{
			var index = sortedIdIndices[idIndex]; 
			var id = idList[ index ];
			id =  id == currentWanIp ? currentLanIp : id ;	
			
			var tableRow = [getHostDisplay(id)];
			var pieIndex;
			var allZero = true;
			for(pieIndex=0;pieIndex < pieNames.length; pieIndex++)
			{
				var value = parseBytes(data[pieIndex][index]);
				value = value.replace("ytes", "");
				tableRow.push(value);
				sum[pieIndex] = sum[pieIndex] + data[pieIndex][index];
			}
			for(pieIndex=0;pieIndex < pieNames.length; pieIndex++)
			{
				var percent = zeroPies[pieIndex] ? 100/idList.length : data[pieIndex][index]*100/pieTotals[pieIndex];
				var pctStr = "" + (parseInt(percent*10)/10) + "%";
				tableRow.push(pctStr);
			}
			tableRows.push(tableRow);
		}
		tableRows.push([bndwS.Sum,parseBytes(sum[0]),parseBytes(sum[1]),parseBytes(sum[2]),"","",""]);
		
		var columnNames = [bndwS.Host];
		for(pieIndex=0;pieIndex < pieNames.length; pieIndex++){ columnNames.push(pieNames[pieIndex]); }
		for(pieIndex=0;pieIndex < pieNames.length; pieIndex++){ columnNames.push(pieNames[pieIndex] + " %"); }
	
		var distTable=createTable(columnNames, tableRows, "bandwidth_distribution_table", false, false);
		var tableContainer = document.getElementById('bandwidth_distribution_table_container');
		if(tableContainer.firstChild != null)
		{
			tableContainer.removeChild(tableContainer.firstChild);
		}
		tableContainer.appendChild(distTable);
		updateInProgress = false;
	}
}






// data for current time frame
// at each index, are 3 more arrays (combined, down, up) each of which is an array of values for that interval
// var timeFrameIntervalData = [];
// var idList = [];

function parseMonitors(outputData)
{
	var monitors = [ [],[] ];
	var dataLines = outputData.split("\n");
	var currentDate = parseInt(dataLines.shift());
	var lineIndex;
	for(lineIndex=0; lineIndex < dataLines.length; lineIndex++)
	{
		if(dataLines[lineIndex].length > 0)
		{
			var monitorType = (dataLines[lineIndex].split(/[\t ]+/))[0];
			monitorType = monitorType.match(/download/) ? 0 : 1;
			var monitorIp = (dataLines[lineIndex].split(/[\t ]+/))[1];

			lineIndex++; 
			var firstTimeStart = dataLines[lineIndex];
			lineIndex++;
			var firstTimeEnd = dataLines[lineIndex];
			lineIndex++; 
			var lastTimePoint = dataLines[lineIndex];
			lineIndex++;
			var points = dataLines[lineIndex].split(",");
			if(monitorIp != "COMBINED")
			{
				monitors[monitorType][monitorIp] = [points, lastTimePoint ];
			}
		}
	}
	return monitors;
}

