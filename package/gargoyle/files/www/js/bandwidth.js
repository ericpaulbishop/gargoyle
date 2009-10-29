/*
 * This program is copyright 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var ipMonitorIds;
var qosUploadMonitorIds;
var qosDownloadMonitorIds;


var uploadMonitors;
var downloadMonitors;
var updateInProgress;
var graphType;

var ipsWithData = [];
var qosDownloadIds  = [];
var qosDownloadNames = [];
var qosUploadIds  = [];
var qosUploadNames = [];

var updateTotalPlot = null;
var updateUploadPlot = null;
var updateDownloadPlot = null;

var plotsInitializedToDefaults = false;



function initializePlotsAndTable()
{
	// we are going to assume we know what graph types (i.e. time frames) are available for each
	// type of monitor.  We might want to go back and make this configurable somehow later
	// by parameterizing the init script but this gets complicated and ugly really fast
	// so for now we assume all 4 types for total and 2 for the others (15h & 15d)
	
	addOptionToSelectElement("plot1_type", "IP", "ip");
	addOptionToSelectElement("plot2_type", "IP", "ip");
	addOptionToSelectElement("plot3_type", "IP", "ip");

	var haveQosUpload = false;
	var haveQosDownload = false;
	var monitorIndex;
	for(monitorIndex=0; monitorIndex < monitorNames.length; monitorIndex++)
	{
		var isQosUpload = (monitorNames[monitorIndex]).match(/qos/) && (monitorNames[monitorIndex]).match(/upload/);
		var isQosDownload = (monitorNames[monitorIndex]).match(/qos/) && (monitorNames[monitorIndex]).match(/download/);
		haveQosUpload =   haveQosUpload   || isQosUpload;
		haveQosDownload = haveQosDownload || isQosDownload;
		
		var getQosName = function(idStr)
		{
			var splitId = idStr.split("-");
			splitId.shift();
			splitId.pop();
			splitId.pop();
			var qosClass = splitId.join("-");
			var qosName = uciOriginal.get("qos_gargoyle", downClass, "name");
			return  qosName == "" ? qosClass : qosName;
		}	
		if(isQosUpload)
		{
			qosUploadIds.push(monitorNames[monitorIndex]);
			qosUploadNames.push(getQosName(monitorNames[monitorIndex]));
		}
		if(isQosDownload)
		{
			qosDownloadIds.push(monitorNames[monitorIndex]);
			qosDownloadNames.push(getQosName(monitorNames[monitorIndex]));
		}
	}
	if(haveQosUpload)
	{
		addOptionToSelectElement("plot1_type", "QoS Upload Class", "qos-upload");
		addOptionToSelectElement("plot2_type", "QoS Upload Class", "qos-upload");
		addOptionToSelectElement("plot3_type", "QoS Upload Class", "qos-upload");
	}
	if(haveQosDownload)
	{
		addOptionToSelectElement("plot1_type", "QoS Download Class", "qos-download");
		addOptionToSelectElement("plot2_type", "QoS Download Class", "qos-download");
		addOptionToSelectElement("plot3_type", "QoS Download Class", "qos-download");
	}
	

	/* set defaults, then set loaded values.  That way if loaded
	 * values are invalid we get the defaults */
	var timeFrame = uciOriginal.get("gargoyle", "bandwidth_display", "time_frame");
	var plotIndex;
	for(plotIndex=1; plotIndex <= 3; plotIndex++)
	{
		var plotTypeElement = "plot" + plotIndex + "_type";
		var plotType = uciOriginal.get("gargoyle", "bandwidth_display", plotTypeElement);
		setSelectedValue(plotTypeElement, plotType);
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
	else if(plotType == "qos")
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
			if(	((name.match("upload") && isUp) || (name.match("download") && !isUp)) &&
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

		var graphTimeFrameIndex = getSelectedValue("time_frame");
		var graphNonTotal = false;
		var plotNum;
		for(plotNum=1; plotNum<=3; plotNum++)
		{
			var t = getSelectedValue("plot" + plotNum + "_type");
			graphNonTotal = graphNonTotal || (t != "total" && t != "none");
		}
		for(plotNum=1; plotNum<=3; plotNum++)
		{
			var plotType = getSelectedValue("plot" + plotNum + "_type");
			var plotIdName = "plot" + plotNum + "_id";
			var plotId= getSelectedValue(plotIdName);
			plotId = plotId == null ? "" : plotId;


			if(plotType == "ip")
			{
				if(plotId.match(/^[0-9]+\./) == null)
				{
					removeAllOptionsFromSelectElement(document.getElementById(plotIdName));
					var ipIndex=0;
					for(ipIndex=0; ipIndex < ipsWithData.length; ipIndex++)
					{
						addOptionToSelectElement(plotIdName, ipsWithData[ipIndex], ipsWithData[ipIndex]);
					}
					setSelectedText(plotIdName, ipsWithData[0]);
				}
				document.getElementById(plotIdName).style.display="block";
			}
			else if(plotType == "qos-upload")
			{
				if(selectedId.match(/^qos\-upload\-/) == null)
				{
					removeAllOptionsFromSelectElement(document.getElementById(plotIdName));
					var upIndex=0;
					for(upIndex=0; upIndex < qosUploadIds.length; upIndex++)
					{
						addOptionToSelectElement(plotIdName, qosUploadIds[upIndex], qosUploadNames[upIndex]);
					}
				}
				document.getElementById(plotIdName).style.display="block";
			}
			else if(plotType == "qos-download")
			{
				if(selectedId.match(/^qos\-download\-/) == null)
				{
					removeAllOptionsFromSelectElement(document.getElementById(plotIdName));
					var downIndex=0;
					for(downIndex=0; downIndex < qosDownloadMonitorIds.length; downIndex++)
					{
						addOptionToSelectElement(plotIdName, qosUploadIds[upIndex], qosUploadNames[upIndex]);
					}
				}
				document.getElementById(plotIdName).style.display="block";
			}
			else
			{
				document.getElementById(plotIdName).style.display="none";
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

			uploadMonitors[plotNum-1]  = getMonitorId(true, graphTimeFrameIndex, plotType, plotId, graphNonTotal);
			downloadMonitors[plotNum-1] = getMonitorId(false, graphTimeFrameIndex, plotType, plotId, graphNonTotal);
		}
		plotsInitializedToDefaults = true;
		

		if(oldUploadMonitors != uploadMonitors.join("\n") || oldDownloadMonitors != downloadMonitors.join("\n") )
		{
			doUpdate();
		}
		
		/* update plot variables via ajax, so they "stick" */
		var command = "";
		command = command + "uci set gargoyle.bandwidth_display=bandwidth_display\n";
		command = command + "uci set gargoyle.bandwidth_display.time_frame=\"" + getSelectedValue("time_frame") + "\"\n";
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
		var monitorNames = uploadMonitors.join(" ") + " " + downloadMonitors.join(" ");
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
					var numIntervals = 0;
					var intervalLength = 2;
					var lastTimePoint = Math.floor( (new Date()).getTime()/1000 );
					
					for(monitorIndex=0; monitorIndex < uploadMonitors.length; monitorIndex++)
					{
						var ipsInitialized = false;
						var dirIndex;
						for(dirIndex = 0; dirIndex < 2; dirIndex++)
						{
							var dataLoaded = false;
							var monitorList = dirIndex == 0 ? uploadMonitors : downloadMonitors;
							var pointSets = dirIndex == 0 ? uploadPointSets : downloadPointSets;
							var monitorName = monitorList[monitorIndex];
							
							
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
									numIntervals = splitName.pop();
									intervalLength = splitName.pop();


									//select ip based on selected value in plot (or first available if none selected)
									if(monitorName.match("bdist"))
									{
										var plotIdName = "plot" + (monitorIndex+1) + "_id";
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
									lastTimePoint = monitorData[ip][1];


									//update total point set, assume differences in length
									//indicate more/less points at BEGINNING, not end
									var totalSet = totalPointSets[monitorIndex] == null ? [] : totalPointSets[monitorIndex];
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
									totalPointSets[monitorIndex] = totalSet;
									pointSets.push(points);
									dataLoaded=true;
								}
							}
							if(!dataLoaded)
							{
								pointSets.push(null);
							}
						}
						totalPointSets[monitorIndex] = totalPointSets[monitorIndex] == null ? null : (totalPointSets[monitorIndex]).reverse()
					}
					updateTotalPlot(totalPointSets, numIntervals, intervalLength, lastTimePoint );
					updateDownloadPlot(downloadPointSets, numIntervals, intervalLength, lastTimePoint );
					updateUploadPlot(uploadPointSets, numIntervals, intervalLength, lastTimePoint );
				}
				
				//get daily download/upload data for table if we haven't tried yet
				//if we've tried and failed these will be set to "no_monitor"
				//if(dailyDownloadData == null || dailyUploadData == null)
				//{
				//	loadTotalTableData();
				//}
				updateInProgress = false;
			}
		}
		runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
	}
}


function loadTotalTableData()
{
	//setup table
	var param = getParameterDefinition("monitor", "total-upload-1y  total-download-1y" )  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(dailyReq)
	{
		var monitors=null;
		if(dailyReq.readyState == 4)
		{
			if(!dailyReq.responseText.match(/ERROR/))
			{
				monitors = parseMonitors(dailyReq.responseText);
				if(monitors["total-upload-1y"] != null)
				{
					document.getElementById("total_bandwidth_use").style.display="block";
					
					removeAllOptionsFromSelectElement(document.getElementById("total_time_frame"));
					addOptionToSelectElement("total_time_frame", "Daily", "daily"); 
					addOptionToSelectElement("total_time_frame", "Monthly", "monthly");
					setSelectedValue("total_time_frame", "daily");
					
					removeAllOptionsFromSelectElement(document.getElementById("total_table_units"));
					addOptionToSelectElement("total_table_units", "Auto (Mixed)", "mixed"); 
					addOptionToSelectElement("total_table_units", "KBytes", "KBytes"); 
					addOptionToSelectElement("total_table_units", "MBytes", "MBytes");
					addOptionToSelectElement("total_table_units", "GBytes", "GBytes");
					setSelectedValue("total_table_units", "mixed");


				
					dailyUploadData=monitors["total-upload-1y"];
					dailyDownloadData=monitors["total-download-1y"];
					updateTotalTable();
				}
				else
				{
					dailyUploadData = "no_monitor";
					dailyDownloadData = "no_monitor";
				}
			}
		}
	}
	runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
}

function updateTotalTable()
{
	if(dailyUploadData == null || dailyUploadData == "no_monitor" || dailyDownloadData == null || dailyDownloadData == "no_monitor")
	{
		return;
	}

	var systemDateFormat = uciOriginal.get("gargoyle",  "global", "dateformat");
	var selectedTotalTimeFrame=getSelectedValue("total_time_frame");
	var displayUnits = getSelectedValue("total_table_units");
	var totalTableData = new Array();
	var columnNames = ["", "Total Upload Bandwidth", "Total Download Bandwidth"];
	if(selectedTotalTimeFrame == "daily")
	{
		columnNames[0] = "Date";
		var nextDate=new Date();
		nextDate.setTime(dailyUploadData[1]*1000);
		for(dailyIndex=dailyUploadData[0].length-1; dailyIndex >=0; dailyIndex--)
		{
			dailyUploadData[0][dailyIndex] = dailyUploadData[0][dailyIndex] > 0 ? 1.0*dailyUploadData[0][dailyIndex] : 0.0;
			dailyDownloadData[0][dailyIndex] = dailyDownloadData[0][dailyIndex] > 0 ? 1.0*dailyDownloadData[0][dailyIndex] : 0.0;

			var twod = function(num) { var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; return nstr; }
			var y2 = twod(nextDate.getFullYear()%100)
			var y4 = nextDate.getFullYear();
			var m = twod(nextDate.getMonth()+1);
			var d = twod(nextDate.getDate());


			var outputDate = "";
			if(systemDateFormat == "iso")
			{
				outputDate = y4 + "/" + m + "/" + d;
			}
			else if(systemDateFormat == "australia")
			{
				outputDate = d + "/" + m + "/" + y2;
			}
			else
			{
				outputDate = m + "/" + d + "/" + y2;
			}

			totalTableData.push([ outputDate, parseBytes(dailyUploadData[0][dailyIndex], displayUnits), parseBytes(dailyDownloadData[0][dailyIndex], displayUnits) ]);
			newTime = nextDate.getTime() - (24*60*60*1000);
			nextDate.setTime(newTime);
		}
	}
	else if(selectedTotalTimeFrame == "monthly")
	{
		columnNames[0] = "Month";
		var monthNames = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
		var nextDate=new Date();
		nextDate.setTime(dailyUploadData[1]*1000);
		var dailyIndex = dailyUploadData[0].length-1;
		while( dailyIndex >= 0 )
		{
			//make sums floating points to deal with 64bit / 32bit conversion which can be an issue
			dailyUploadData[0][dailyIndex] = dailyUploadData[0][dailyIndex] > 0 ? 1.0*dailyUploadData[0][dailyIndex] : 0.0;
			dailyDownloadData[0][dailyIndex] = dailyDownloadData[0][dailyIndex] > 0 ? 1.0*dailyDownloadData[0][dailyIndex] : 0.0;
			monthUploadSum = dailyUploadData[0][dailyIndex];
			monthDownloadSum = dailyDownloadData[0][dailyIndex];
			month = nextDate.getMonth();
			year = nextDate.getFullYear();
			newTime = nextDate.getTime() - (24*60*60*1000);
			nextDate.setTime(newTime);
			dailyIndex--;
			while(nextDate.getMonth() == month &&  dailyIndex >= 0)
			{
				dailyUploadData[0][dailyIndex] = dailyUploadData[0][dailyIndex] > 0 ? 1.0*dailyUploadData[0][dailyIndex] : 0.0;
				dailyDownloadData[0][dailyIndex] = dailyDownloadData[0][dailyIndex] > 0 ? 1.0*dailyDownloadData[0][dailyIndex] : 0.0;

				monthUploadSum = monthUploadSum + dailyUploadData[0][dailyIndex];
				monthDownloadSum = monthDownloadSum + dailyDownloadData[0][dailyIndex];

				newTime = nextDate.getTime() - (24*60*60*1000);
				nextDate.setTime(newTime);
				dailyIndex--;
			}
			totalTableData.push([ monthNames[month] + " " + year, parseBytes(monthUploadSum, displayUnits), parseBytes(monthDownloadSum, displayUnits) ]);
		}
	}
	totalTable=createTable(columnNames, totalTableData, "total_bandwidth_table", false, false);
	tableContainer = document.getElementById('total_bandwidth_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(totalTable);
}


