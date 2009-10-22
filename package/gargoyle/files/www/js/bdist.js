/*
 * This program is copyright 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

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
	setInterval( 'doUpdate()', 2000);
}

function initFunction()
{	
	pieChart = document.getElementById("pie_chart");
	if(pieChart != null)
	{
		doUpdate(); 
	}
	else
	{
		setTimeout("initFunction()", 50); 
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
							latestTime = dirData[id][1];
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
					removeAllOptionsFromSelectElement(document.getElementById("time_interval"));


					timeFrameIntervalData = [];
					var intervalNames = [];
					var nextIntervalStart = latestTime;
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
								var value = 0;
								if(dirData[id] != null)
								{
									var idPoints = dirData[id][0];
									if(idPoints != null)
									{
										value = idPoints[idPoints.length-intervalIndex];
										value = value==null? 0 : value;
									}
								}
								nextDirData.push(value);
								combinedData[idIndex] = combinedData[idIndex] == null ? value : combinedData[idIndex] + value;
							}
							nextIntervalData.push(nextDirData);
						}
						nextIntervalData.unshift(combinedData);
						timeFrameIntervalData.push(nextIntervalData);

						
						
						var monthNames = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
						var twod = function(num) { var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; return nstr; }
						var nextDate = new Date();
						nextDate.setTime(nextIntervalStart*1000);
						var intervalName = "";
						if(uploadName.match("minute"))
						{
							intervalName = "" + twod(nextDate.getHours()) + ":" + twod(nextDate.getMinutes());
							nextDate.setMinutes( nextDate.getMinutes()-1);
							
						}
						else if(uploadName.match("hour"))
						{
							intervalName = "" + twod(nextDate.getHours()) + ":" + twod(nextDate.getMinutes());
							nextDate.setHours(nextDate.getHours()-1);
						}
						else if(uploadName.match("day"))
						{
							intervalName = monthNames[nextDate.getMonth()] + " " + nextDate.getDate();
							nextDate.setDate(nextDate.getDate()-1);
						}
						else if(uploadName.match("month"))
						{
							intervalName = monthNames[nextDate.getMonth()] + " " + nextDate.getFullYear();
							nextDate.setMonth(nextDate.getMonth()-1);
						}
						addOptionToSelectElement("time_interval", intervalName, ""+intervalIndex);
						nextIntervalStart = nextDate.valueOf()/1000;
					}
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
	if(pieChart != null && (!updateInProgress) && timeFrameIntervalData.length > 0 )
	{
		var plotFunction = getEmbeddedSvgPlotFunction("pie_chart");
		var intervalIndex = getSelectedValue("time_interval");
		intervalIndex = intervalIndex == null ? 0 : intervalIndex;
		
		var data = timeFrameIntervalData[intervalIndex];
		plotFunction(idList, ["Combined", "Download", "Upload" ], idList, data, 0, 9, resetColors);
		resetColors = false;
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
			monitors[monitorType][monitorIp] = [points, lastTimePoint ];
		}
	}
	return monitors;
}

