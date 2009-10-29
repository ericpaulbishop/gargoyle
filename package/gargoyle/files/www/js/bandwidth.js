/*
 * This program is copyright 2008,2009 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var ipMonitorIds;
var qosUploadMonitorIds;
var qosDownloadMonitorIds;


var uploadMonitors = null;
var downloadMonitors = null;
var tableUploadMonitor = null;
var tableDownloadMonitor = null;


var ipsWithData = [];
var qosDownloadClasses  = [];
var qosDownloadNames = [];
var qosUploadClasses  = [];
var qosUploadNames = [];

var definedUploadClasses = [];
var definedDownloadClasses = [];

var updateTotalPlot = null;
var updateUploadPlot = null;
var updateDownloadPlot = null;

var updateInProgress = false;
var plotsInitializedToDefaults = false;



function initializePlotsAndTable()
{
	var plotTimeFrame = uciOriginal.get("gargoyle", "bandwidth_display", "plot_time_frame");
	var tableTimeFrame = uciOriginal.get("gargoyle", "bandwidth_display", "table_time_frame");
	setSelectedValue("plot_time_frame", plotTimeFrame);
	setSelectedValue("table_time_frame", tableTimeFrame);
	setSelectedValue("table_units", "mixed");
	
	var haveQosUpload = false;
	var haveQosDownload = false;
	var monitorIndex;
	for(monitorIndex=0; monitorIndex < monitorNames.length; monitorIndex++)
	{
		var monId = monitorNames[monitorIndex];
		if(monId.match(/qos/))
		{
			var isQosUpload = monId.match(/up/);
			var isQosDownload = monId.match(/down/);
			haveQosUpload =   haveQosUpload   || isQosUpload;
			haveQosDownload = haveQosDownload || isQosDownload;
		
			var splitId = monId.split("-");
			splitId.shift();
			splitId.shift();
			splitId.pop();
			splitId.pop();
			var qosClass = splitId.join("-");
			var qosName = uciOriginal.get("qos_gargoyle", qosClass, "name");
			
			if(isQosUpload && definedUploadClasses[qosClass] == null)
			{
				qosUploadClasses.push(qosClass);
				qosUploadNames.push(qosName);
				definedUploadClasses[qosClass] = 1;
			}
			if(isQosDownload && definedDownloadClasses[qosClass] == null)
			{
				qosDownloadClasses.push(qosClass);
				qosDownloadNames.push(qosName);
				definedDownloadClasses[qosClass] = 1;
			}
		}
	}
	var plotIdNames = ["plot1_type", "plot2_type", "plot3_type", "table_type"];
	var idIndex;
	for(idIndex=0; idIndex < plotIdNames.length; idIndex++)
	{
		var plotIdName = plotIdNames[idIndex];
		if(haveQosUpload)
		{	
			addOptionToSelectElement(plotIdName, "QoS Upload Class", "qos-upload");
		}
		if(haveQosDownload)
		{
			addOptionToSelectElement(plotIdName, "QoS Download Class", "qos-download");
		}
		addOptionToSelectElement(plotIdName, "IP", "ip");
		
		var plotType = uciOriginal.get("gargoyle", "bandwidth_display", plotIdName);
		setSelectedValue(plotIdName, plotType);
	}
	plotsInitializedToDefaults = false;


	uploadMonitors = ["","",""];
	downloadMonitors = ["","",""];
	updateInProgress = false;
	resetPlots();
	setInterval( 'doUpdate()', 2000);
}


function getEmbeddedSvgPlotFunction(embeddedId)
{
	windowElement = getEmbeddedSvgWindow(embeddedId);
	if( windowElement != null)
	{
		return windowElement.plotAll;
	}
	return null;
}
function getMonitorId(isUp, graphTimeFrameIndex, plotType, plotId, graphNonTotal)
{
	var nameIndex;
	var selectedName = null;
	
	var match1 = "";
	var match2 = "";

	if(plotType == "total")
	{
		match1 = graphNonTotal ? "total" + graphTimeFrameIndex + "B" : "total" + graphTimeFrameIndex + "A";
	}
	else if(plotType.match(/qos/))
	{
		match1 = "qos" + graphTimeFrameIndex;
		match2 = plotId;
	}
	else if(plotType == "ip")
	{
		match1 = "bdist" + graphTimeFrameIndex;
	}
	
	if(plotType != "none")
	{
		for(nameIndex=0;nameIndex < monitorNames.length && selectedName == null; nameIndex++)
		{
			var name = monitorNames[nameIndex];
			if(	((name.match("up") && isUp) || (name.match("down") && !isUp)) &&
				(match1 == "" || name.match(match1)) &&
				(match2 == "" || name.match(match2)) 
			)
			{
				selectedName = name;
			}
		}
	}
	return selectedName;
}


function resetPlots()
{
	if(!updateInProgress && updateUploadPlot != null && updateDownloadPlot != null)
	{
		var oldUploadMonitors = uploadMonitors.join("\n");
		var oldDownloadMonitors = downloadMonitors.join("\n");

		uploadMonitors = [];
		downloadMonitors = [];

		var graphTimeFrameIndex = getSelectedValue("plot_time_frame");
		var tableTimeFrameIndex = getSelectedValue("table_time_frame");
		var graphNonTotal = false;
		var plotNum;
		for(plotNum=1; plotNum<=3; plotNum++)
		{
			var t = getSelectedValue("plot" + plotNum + "_type");
			graphNonTotal = graphNonTotal || (t != "total" && t != "none");
		}
		for(plotNum=1; plotNum<=4; plotNum++)
		{
			var plotIdName = plotNum < 4 ? "plot" + plotNum + "_id" : "table_id";
			var plotIdVisName = plotNum < 4 ? plotIdName : plotIdName + "_container";
			var plotTypeName = plotNum < 4 ? "plot" + plotNum + "_type" : "table_type";
			var plotType = getSelectedValue(plotTypeName);
			var plotId= getSelectedValue(plotIdName);
			plotId = plotId == null ? "" : plotId;

			if(plotType == "ip")
			{
				if(plotId.match(/^[0-9]+\./) == null)
				{
					setAllowableSelections(plotIdName, ipsWithData, ipsWithData);
					setSelectedText(plotIdName, ipsWithData[0]);
				}
				document.getElementById(plotIdVisName).style.display="block";
			}
			else if(plotType == "qos-upload")
			{
				if(definedUploadClasses[plotId] == null)
				{
					setAllowableSelections(plotIdName, qosUploadClasses, qosUploadNames);
				}
				document.getElementById(plotIdVisName).style.display="block";
			}
			else if(plotType == "qos-download")
			{
				if(definedDownloadClasses[plotId] == null)
				{
					setAllowableSelections(plotIdName, qosDownloadClasses, qosDownloadNames);
				}
				document.getElementById(plotIdVisName).style.display="block";
			}
			else
			{
				document.getElementById(plotIdVisName).style.display="none";
			}

			if(!plotsInitializedToDefaults)
			{
				if(plotType != "" && plotType != "none" && plotType != "total")
				{
					var idValue = uciOriginal.get("gargoyle", "bandwidth_display", plotIdName);
					setSelectedValue(plotIdName, idValue);
					plotId = idValue;
				}
			}
			
			if(plotNum != 4)
			{
				uploadMonitors[plotNum-1]  = getMonitorId(true, graphTimeFrameIndex, plotType, plotId, graphNonTotal);
				downloadMonitors[plotNum-1] = getMonitorId(false, graphTimeFrameIndex, plotType, plotId, graphNonTotal);
			}
			else
			{
				var lowRes = plotType == "total" && tableTimeFrameIndex == 3 ? false : true;
				tableTimeFrameIndex =  lowRes ? tableTimeFrameIndex : 4;
				tableUploadMonitor   = getMonitorId(true,  tableTimeFrameIndex, plotType, plotId, lowRes);
				tableDownloadMonitor = getMonitorId(false, tableTimeFrameIndex, plotType, plotId, lowRes);
			}
		}
		plotsInitializedToDefaults = true;
		

		if(oldUploadMonitors != uploadMonitors.join("\n") || oldDownloadMonitors != downloadMonitors.join("\n") )
		{
			doUpdate();
		}
		
		/* update plot variables via ajax, so they "stick" */
		var command = "";
		command = command + "uci set gargoyle.bandwidth_display=bandwidth_display\n";
		command = command + "uci set gargoyle.bandwidth_display.plot_time_frame=\"" + getSelectedValue("plot_time_frame") + "\"\n";
		command = command + "uci set gargoyle.bandwidth_display.table_time_frame=\"" + getSelectedValue("table_time_frame") + "\"\n";
		var plotIndex;
		for(plotIndex=1; plotIndex <= 3; plotIndex++)
		{
			var plotTypeElement = "plot" + plotIndex + "_type";
			var plotType = getSelectedValue(plotTypeElement);
			command = command + "uci set gargoyle.bandwidth_display." + plotTypeElement + "=\"" + plotType + "\"\n";
			
			var plotIdElement = "plot" + plotIndex + "_id";
			if(plotType != "" && plotType != "none" && plotType != "total")
			{
				command = command + "uci set gargoyle.bandwidth_display." + plotIdElement + "=\"" + getSelectedValue(plotIdElement) + "\"\n";
			}
			else
			{
				command = command + "uci del gargoyle.bandwidth_display." + plotIdElement + "\n"
			}
		}
		command = command + "uci commit\n";
		
		var param = getParameterDefinition("commands", command)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

		runAjax("POST", "utility/run_commands.sh", param, function(){ return 0; });
	}
	else
	{
		setTimeout( "resetPlots()", 100); //try again in 100 milliseconds
		if(  updateTotalPlot == null || updateDownloadPlot == null ||  updateUploadPlot == null   )
		{
			updateTotalPlot = getEmbeddedSvgPlotFunction("total_plot");
			updateDownloadPlot = getEmbeddedSvgPlotFunction("download_plot");
			updateUploadPlot = getEmbeddedSvgPlotFunction("upload_plot");
		}
	}
}

function parseMonitors(outputData)
{
	var monitors = [ ];
	var dataLines = outputData.split("\n");
	var lineIndex;
	for(lineIndex=0; lineIndex < dataLines.length; lineIndex++)
	{
		if(dataLines[lineIndex].length > 0)
		{
			var monitorId = (dataLines[lineIndex].split(/[\t ]+/))[0];
			var monitorIp = (dataLines[lineIndex].split(/[\t ]+/))[1];

			lineIndex++; 
			var firstTimeStart = dataLines[lineIndex];
			lineIndex++;
			var firstTimeEnd = dataLines[lineIndex];
			lineIndex++; 
			var lastTimePoint = dataLines[lineIndex];
			lineIndex++;
			var points = dataLines[lineIndex].split(",");
			monitors[monitorId] = monitors[monitorId] == null ? [] : monitors[monitorId];
			monitors[monitorId][monitorIp] = [points, lastTimePoint ];
		}
	}
	return monitors;
}


function doUpdate()
{
	if(updateUploadPlot != null && updateDownloadPlot != null && updateTotalPlot != null)
	{
		updateInProgress = true;
		var monitorNames = uploadMonitors.join(" ") + " " + downloadMonitors.join(" ") + " " + tableDownloadMonitor + " " + tableUploadMonitor ;
		var param = getParameterDefinition("monitor", monitorNames)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			var monitors=null;
			if(req.readyState == 4)
			{
				if(!req.responseText.match(/ERROR/))
				{
					var monitors = parseMonitors(req.responseText);
					var uploadPointSets = [];
					var downloadPointSets = [];
					var totalPointSets = [];
					var tablePointSets = [];
					var tableTotal = [];
					var plotNumIntervals = 0;
					var plotIntervalLength = 2;
					var tableNumIntervals = 0;
					var tableIntervalLength = 2;
					var lastPlotTimePoint = Math.floor( (new Date()).getTime()/1000 );
					var lastTableTimePoint = Math.floor( (new Date()).getTime()/1000 );
					
					for(monitorIndex=0; monitorIndex < 4; monitorIndex++)
					{
						var ipsInitialized = false;
						var dirIndex;
						for(dirIndex = 0; dirIndex < 2; dirIndex++)
						{
							var dataLoaded = false;
							var pointSets;
							var monitorName;
							if(monitorIndex < 3)
							{
								var monitorList = dirIndex == 0 ? downloadMonitors : uploadMonitors;
								pointSets = dirIndex == 0 ? downloadPointSets : uploadPointSets;
								monitorName = monitorList[monitorIndex];
							}
							else
							{
								pointSets = tablePointSets;
								monitorName = dirIndex == 0 ? tableDownloadMonitor : tableUploadMonitor;
							}
							monitorName = monitorName == null ? "" : monitorName;
							
							
							var monitorData = monitorName == "" ? null : monitors[monitorName];
							if(monitorData != null)
							{
								var selectedIp = "";
								
								//get list of available ips
								var ipList = [];
								var ip;
								for (ip in monitorData)
								{
									ipList.push(ip);
								}
								if(ipList.length > 0)
								{
									var splitName = monitorName.split("-");
									if(monitorIndex < 3)
									{
										plotNumIntervals = splitName.pop();
										plotIntervalLength = splitName.pop();
									}
									else
									{
										tableNumIntervals = splitName.pop();
										tableIntervalLength = splitName.pop();

									}

									//select ip based on selected value in plot (or first available if none selected)
									if(monitorName.match("bdist"))
									{
										var plotIdName = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_id" : "table_id";
										ip = getSelectedValue(plotIdName);
										ip = ip == null ? "" : ip;
										ip = monitorData[ip] != null ? ip : ipList[0];
	
										//if new ip list differs from allowable selections, update
										setAllowableSelections(plotIdName, ipList, ipList);
										ipsWithData = ipList;
									}
									else
									{
										ip = ipList[0];
									}
								
								
									var points = monitorData[ip][0]
									if(monitorIndex < 3)
									{
										lastPlotTimePoint = monitorData[ip][1];
									}
									else
									{
										lastTableTimePoint = monitorData[ip][1];
									}

									//update total point set, assume differences in length
									//indicate more/less points at BEGINNING, not end
									var totalSet;
									if(monitorIndex < 3)
									{
										totalSet = totalPointSets[monitorIndex] == null ? [] : totalPointSets[monitorIndex];
									}
									else
									{
										totalSet = tableTotal;
									}
									var updateIndex;
									for(updateIndex=0; updateIndex < points.length; updateIndex++)
									{
										var pointIndex = points.length-(1+updateIndex);
										var totalIndex = totalSet.length < points.length ? updateIndex : (totalSet.length-points.length)+updateIndex;
										if(totalSet[totalIndex] != null)
										{
											totalSet[totalIndex] = parseInt(totalSet[totalIndex]) + parseInt(points[pointIndex])
										}
										else
										{
											totalSet.push( points[pointIndex] );
										}
									}
									if(monitorIndex < 3)
									{
										totalPointSets[monitorIndex] = totalSet;
									}
									pointSets.push(points);
									dataLoaded=true;
								}
							}
							if(!dataLoaded)
							{
								pointSets.push(null);
							}
						}
						if(monitorIndex < 3)
						{
							totalPointSets[monitorIndex] = totalPointSets[monitorIndex] == null ? null : (totalPointSets[monitorIndex]).reverse()
						}
						else
						{
							tableTotal.reverse();
							tablePointSets.unshift(tableTotal);
						}
					}
					updateTotalPlot(totalPointSets, plotNumIntervals, plotIntervalLength, lastPlotTimePoint, tzMinutes );
					updateDownloadPlot(downloadPointSets, plotNumIntervals, plotIntervalLength, lastPlotTimePoint, tzMinutes );
					updateUploadPlot(uploadPointSets, plotNumIntervals, plotIntervalLength, lastPlotTimePoint, tzMinutes );

					updateBandwidthTable(tablePointSets, tableIntervalLength, lastTableTimePoint);
				}
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
	}
}

function twod(num)
{
	var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; 
	return nstr; 
}


function updateBandwidthTable(tablePointSets, interval, lastTableTimePoint)
{
	var rowData = [];
	var rowIndex = 0;
	var displayUnits = getSelectedValue("table_units");
	var timePoint = lastTableTimePoint;
	var nextDate = new Date();
	nextDate.setTime(timePoint*1000);
	nextDate.setUTCMinutes( nextDate.getUTCMinutes()+tzMinutes );
	var monthNames = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
	for(rowIndex=0; rowIndex < (tablePointSets[0]).length; rowIndex++)
	{
		var colIndex = 0;
		var vals = [];
		for(colIndex=0; colIndex < 3; colIndex++)
		{
			var points = tablePointSets[colIndex];
			var val = points == null ? 0 : points[points.length-(1+rowIndex)];
			val = val == null ? 0 : val;
			vals.push(parseBytes(val, displayUnits));
		}

		var timeStr = "" + timeStr;
		if(interval.match(/minute/))
		{
			timeStr = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
			nextDate.setUTCMinutes( nextDate.getUTCMinutes()-1);
		}
		else if(interval.match(/hour/))
		{
			timeStr = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
			nextDate.setUTCHours( nextDate.getUTCHours()-1);
		}
		else if(interval.match(/day/))
		{
			timeStr = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCDate();
			nextDate.setUTCDate( nextDate.getUTCDate()-1);
		}
		else if(interval.match(/month/))
		{
			timeStr = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCFullYear();
			nextDate.setUTCMonth( nextDate.getUTCMonth()-1);
		}
		else if(parseInt(interval) != "NaN")
		{
			nextDate.setTime((timePoint-parseInt(interval))*1000)
		}
		vals.unshift(timeStr);
		rowData.push(vals);
		timePoint = nextDate.getTime()/1000;
	}

	var columnNames = ["Time", "Total", "Download", "Upload"];
	var bwTable=createTable(columnNames , rowData, "bandwidth_table", false, false);
	tableContainer = document.getElementById('bandwidth_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(bwTable);
}

