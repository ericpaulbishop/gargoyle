/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var bndwS=new Object(); //part of i18n
 
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

var expandedWindows = [];
var expandedFunctions = [];

var updateInterval = null;

function stopInterval()
{
	if(updateInterval != null)
	{
		clearInterval(updateInterval);
	}
}
window.onbeforeunload=stopInterval;



function trim(str)
{
	if ( !str )
	{
		return str;
	}
	// TODO is this the best way to trim strings in JS?
	return str.replace(/^\s\s*/, '').replace(/\s\s*$/, '');
}

function BandwidthCookieContainer()
{
	this.prefix = "gargoyle.bandwidth_display.";
	
	this.set = function(key, value)
	{
		var expires = new Date( new Date().getTime() + ( 86400 * 100 * 1000 /*100 days in mills*/) );
		document.cookie = this.prefix + key + "=" + escape(value) + ";expires=" + expires.toUTCString();
	}

	this.remove = function(key)
	{
		var gone = new Date( 0 );
		document.cookie = this.prefix + key + ";expires=" + gone.toUTCString();
	}

	this.get = function(key, defvalue)
	{
		var cookiearray = document.cookie.split( ';' );
		for( var i=0; i < cookiearray.length; i++ )
		{
			var cookie = cookiearray[i].split('=');
			if ( trim(cookie[0]) == this.prefix + key )
			{
				return trim(cookie[1]);
			}
		}
		return defvalue;
	}
}
bandwidthSettings = new BandwidthCookieContainer();


function initializePlotsAndTable()
{
	var plotTimeFrame = bandwidthSettings.get("plot_time_frame", "1");
	var tableTimeFrame = bandwidthSettings.get("table_time_frame", "1");
	setSelectedValue("plot_time_frame", plotTimeFrame);
	setSelectedValue("table_time_frame", tableTimeFrame);
	setSelectedValue("table_units", "mixed");

	document.getElementById("use_high_res_15m").checked = uciOriginal.get("gargoyle", "bandwidth_display", "high_res_15m") == "1" ? true : false;

	var haveQosUpload = false;
	var haveQosDownload = false;
	var haveTor = false;
	var haveOpenvpn = false;
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
		haveTor = monId.match(/tor/) ? true : haveTor;
		haveOpenvpn = monId.match(/openvpn/) ? true : haveOpenvpn;
	}
	var plotIdNames = ["plot1_type", "plot2_type", "plot3_type", "table_type"];
	var idIndex;
	for(idIndex=0; idIndex < plotIdNames.length; idIndex++)
	{
		var plotIdName = plotIdNames[idIndex];
		if(haveQosUpload)
		{
			addOptionToSelectElement(plotIdName, "QoS "+bndwS.UpCl, "qos-upload");
		}
		if(haveQosDownload)
		{
			addOptionToSelectElement(plotIdName, "QoS "+bndwS.DlCl, "qos-download");
		}
		if(haveTor)
		{
			addOptionToSelectElement(plotIdName, "Tor", "tor");
		}
		if(haveOpenvpn)
		{
			addOptionToSelectElement(plotIdName, "OpenVPN", "openvpn");
		}

		addOptionToSelectElement(plotIdName, UI.HsNm, "hostname");
		addOptionToSelectElement(plotIdName, "IP", "ip");


		var plotType = bandwidthSettings.get(plotIdName, "none");
		setSelectedValue(plotIdName, plotType);
	}
	plotsInitializedToDefaults = false;


	uploadMonitors = ["","",""];
	downloadMonitors = ["","",""];
	updateInProgress = false;
	setTimeout(resetPlots, 150); //for some reason Opera 10.50 craps out if we try to load plot functions immediately
	updateInterval = setInterval(doUpdate, 2000);
}


function getEmbeddedSvgPlotFunction(embeddedId, controlDocument)
{
	if(controlDocument == null) { controlDocument = document; }

	windowElement = getEmbeddedSvgWindow(embeddedId, controlDocument);
	if( windowElement != null)
	{
		return windowElement.plotAll;
	}
	return null;
}


function getMonitorId(isUp, graphTimeFrameIndex, plotType, plotId, graphLowRes)
{
	var nameIndex;
	var selectedName = null;
	
	var match1 = "";
	var match2 = "";


	var hr15m = uciOriginal.get("gargoyle", "bandwidth_display", "high_res_15m");
	graphTimeFrameIndex = graphTimeFrameIndex == 1 && plotType != "total" && (!plotType.match(/tor/)) && (!plotType.match(/openvpn/)) && hr15m == "1" && (!graphLowRes) ? 0 : graphTimeFrameIndex;



	if(plotType == "total")
	{
		match1 = graphLowRes ? "bdist" + graphTimeFrameIndex : "total" + graphTimeFrameIndex;
	}
	else if(plotType.match(/qos/))
	{
		if( (isUp && plotType.match(/up/)) || ( (!isUp) && plotType.match(/down/)) )
		{
			match1 = "qos" + graphTimeFrameIndex;
			match2 = plotId;
		}
		else
		{
			plotType = "none"; //forces us to return null
		}
	}
	else if(plotType.match(/tor/))
	{
		match1 = graphLowRes ? "tor-lr" + graphTimeFrameIndex : "tor-hr" + graphTimeFrameIndex;
	}
	else if(plotType.match(/openvpn/))
	{
		match1 = graphLowRes ? "openvpn-lr" + graphTimeFrameIndex : "openvpn-hr" + graphTimeFrameIndex;
	}
	else if(plotType == "ip" || plotType == "hostname")
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


function getHostnameList(ipList)
{
	var hostnameList = [];
	var ipIndex =0;
	for(ipIndex=0; ipIndex < ipList.length; ipIndex++)
	{
		var ip = ipList[ipIndex];
		var host = ipToHostname[ip] == null ? ip : ipToHostname[ip];
		host = host.length < 25 ? host : host.substr(0,22)+"...";
		hostnameList.push(host);
	}
	return hostnameList;
}

function resetPlots()
{
	if( (!updateInProgress) && updateTotalPlot != null && updateUploadPlot != null && updateDownloadPlot != null)
	{
		updateInProgress = true;
		var oldTableDownloadMonitor = tableDownloadMonitor;
		var oldTableUploadMonitor = tableUploadMonitor;
		var oldDownloadMonitors = downloadMonitors.join("\n") + "\n";
		var oldUploadMonitors = uploadMonitors.join("\n") ;

		uploadMonitors = [];
		downloadMonitors = [];

		var graphTimeFrameIndex = getSelectedValue("plot_time_frame");
		var tableTimeFrameIndex = getSelectedValue("table_time_frame");
		var graphLowRes = false;
		var plotNum;
		for(plotNum=1; plotNum<=3; plotNum++)
		{
			var t = getSelectedValue("plot" + plotNum + "_type");
			var is15MHighRes = graphTimeFrameIndex == 1 && uciOriginal.get("gargoyle", "bandwidth_display", "high_res_15m") == "1";
			graphLowRes = graphLowRes || (t != "total" && t != "none" && t != "tor" && t != "openvpn" && (!is15MHighRes));
		}
		for(plotNum=1; plotNum<=4; plotNum++)
		{
			var plotIdName = plotNum < 4 ? "plot" + plotNum + "_id" : "table_id";
			var plotIdVisName = plotNum < 4 ? plotIdName : plotIdName + "_container";
			var plotTypeName = plotNum < 4 ? "plot" + plotNum + "_type" : "table_type";
			var plotType = getSelectedValue(plotTypeName);
			var plotId= getSelectedValue(plotIdName);
			plotId = plotId == null ? "" : plotId;

			if(plotType == "ip" || plotType == "hostname")
			{
				if(plotId.match(/^[0-9]+\./) == null)
				{
					if(plotType == "hostname")
					{
						setAllowableSelections(plotIdName, ipsWithData, getHostnameList(ipsWithData));
					}
					else
					{
						setAllowableSelections(plotIdName, ipsWithData, ipsWithData);

					}
					setSelectedValue(plotIdName, ipsWithData[0]);
					plotId = ipsWithData[0] == null ? "" : ipsWithData[0];
				}
				document.getElementById(plotIdVisName).style.display = "block";
			}
			else if(plotType == "qos-upload")
			{
				if(definedUploadClasses[plotId] == null)
				{
					setAllowableSelections(plotIdName, qosUploadClasses, qosUploadNames);
					plotId = qosUploadClasses[0]
				}
				document.getElementById(plotIdVisName).style.display="block";
			}
			else if(plotType == "qos-download")
			{
				if(definedDownloadClasses[plotId] == null)
				{
					setAllowableSelections(plotIdName, qosDownloadClasses, qosDownloadNames);
					plotId = qosDownloadClasses[0];
				}
				document.getElementById(plotIdVisName).style.display="block";
			}
			else
			{
				document.getElementById(plotIdVisName).style.display="none";
			}

			if(!plotsInitializedToDefaults)
			{
				if(plotType != "" && plotType != "none" && plotType != "total" && plotType != "tor" && plotType != "openvpn" )
				{
					var idValue = bandwidthSettings.get(plotIdName, "none");
					if(idValue != "" && (plotType == "ip" || plotType == "hostname") )
					{
						setAllowableSelections(plotIdName, [idValue], [idValue]);
					}
					setSelectedValue(plotIdName, idValue);
					plotId = idValue;
				}
			}
			
			if(plotNum != 4)
			{
				uploadMonitors[plotNum-1]  = getMonitorId(true, graphTimeFrameIndex, plotType, plotId, graphLowRes);
				downloadMonitors[plotNum-1] = getMonitorId(false, graphTimeFrameIndex, plotType, plotId, graphLowRes);
				uploadMonitors[plotNum-1] = uploadMonitors[plotNum-1] == null ? "" : uploadMonitors[plotNum-1];
				downloadMonitors[plotNum-1] = downloadMonitors[plotNum-1] == null ? "" : downloadMonitors[plotNum-1];
			}
			else
			{
				//for interval=days, display total, query high-res total 1 year monitor, otherwise the low res monitor
				var lowRes = plotType == "total" && tableTimeFrameIndex == 4 ? false : true;
				tableTimeFrameIndex =  lowRes ? tableTimeFrameIndex : 5;
				tableUploadMonitor   = getMonitorId(true,  tableTimeFrameIndex, plotType, plotId, lowRes);
				tableDownloadMonitor = getMonitorId(false, tableTimeFrameIndex, plotType, plotId, lowRes);
				tableUploadMonitor = tableUploadMonitor == null ? "" : tableUploadMonitor;
				tableDownloadMonitor = tableDownloadMonitor == null ? "" : tableDownloadMonitor;
			}
		}
		plotsInitializedToDefaults = true;
		
		updateInProgress = false;
		if(oldUploadMonitors != uploadMonitors.join("\n") || oldDownloadMonitors != downloadMonitors.join("\n") || oldTableUploadMonitor != tableUploadMonitor || oldTableDownloadMonitor != tableDownloadMonitor )
		{
			doUpdate();
		}

		bandwidthSettings.set('plot_time_frame', getSelectedValue("plot_time_frame"));
		bandwidthSettings.set('table_time_frame', getSelectedValue("table_time_frame"));

		for(plotNum=1; plotNum <= 4; plotNum++)
		{
			var plotIdName = plotNum < 4 ? "plot" + plotNum + "_id" : "table_id";
			var plotTypeName = plotNum < 4 ? "plot" + plotNum + "_type" : "table_type";
			var plotType = getSelectedValue(plotTypeName);
			bandwidthSettings.set(plotTypeName, plotType);

			if(plotType != "" && plotType != "none" && plotType != "total")
			{
				bandwidthSettings.set(plotIdName, getSelectedValue(plotIdName));
			}
			else
			{
				bandwidthSettings.remove(plotIdName);
				bandwidthSettings.remove(plotTypeName);
			}
		}
	}
	else
	{
		setTimeout(resetPlots, 25); //try again in 25 milliseconds
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
	var dataLines = outputData.split(/[\r\n]+/);
	var currentTime = parseInt(dataLines.shift());
	if(""+currentTime == "NaN")
	{
		return monitors;
	}


	var lineIndex;
	for(lineIndex=0; lineIndex < dataLines.length; lineIndex++)
	{
		if(dataLines[lineIndex] != null && dataLines[lineIndex].length > 0)
		{
			if(dataLines[lineIndex].match(/ /))
			{
				var monitorId = (dataLines[lineIndex].split(/[\t ]+/))[0];
				var monitorIp = (dataLines[lineIndex].split(/[\t ]+/))[1];
				lineIndex++; 
				var firstTimeStart = dataLines[lineIndex];
				lineIndex++;
				var firstTimeEnd = dataLines[lineIndex];
				lineIndex++; 
				var lastTimePoint = dataLines[lineIndex];
				if(dataLines[lineIndex+1] != null)
				{
					if(dataLines[lineIndex+1].match(/,/) || dataLines[lineIndex+1].match(/^[0-9]+$/))
					{
						lineIndex++;
						var points = dataLines[lineIndex].split(",");
						monitors[monitorId] = monitors[monitorId] == null ? [] : monitors[monitorId];
						monitors[monitorId][monitorIp] = [points, lastTimePoint, currentTime ];
						found = 1
					}
				}
			}
		}
	}
	return monitors;
}

function getDisplayIp(realIp)
{
	var dip = realIp
	if(dip != null && currentWanIp != null && currentLanIp != null && dip != "")
	{
		dip = dip == currentWanIp ? currentLanIp : dip;
	}
	return dip
}
function getRealIp(displayIp)
{
	var rip = displayIp
	if(rip != null && currentWanIp != null && currentLanIp != null && currentWanIp != "" && currentLanIp != "" && rip != "")
	{
		rip = rip == currentLanIp ? currentWanIp : rip;
	}
	return rip

}


var updateReq = null;
var updateTimeoutId = null;
function doUpdate()
{
	if(!updateInProgress && updateUploadPlot != null && updateDownloadPlot != null && updateTotalPlot != null)
	{
		updateInProgress = true;
		var monitorQueryNames = uploadMonitors.join(" ") + " " + downloadMonitors.join(" ") + " " + tableDownloadMonitor + " " + tableUploadMonitor ;
		var param = getParameterDefinition("monitor", monitorQueryNames)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				try{ clearTimeout(updateTimeoutId); }catch(e){}
				updateReq = null;
				
				if(  req.responseText.length > 0 && (!req.responseText.match(/ERROR/)) )
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
					var plotLastTimePoint = Math.floor( (new Date()).getTime()/1000 );
					var plotCurrentTimePoint = plotLastTimePoint;
					var tableLastTimePoint = plotLastTimePoint;
					
					
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
							
							var plotTypeName = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_type" : "table_type";
							var selectedPlotType = getSelectedValue(plotTypeName);
							var monitorData = monitorName == "" ? null : monitors[monitorName];
							if(monitorData != null)
							{
								var selectedIp = "";


								//get list of available ips
								var ipList = [];
								var ip;
								for (ip in monitorData)
								{
									if( ((selectedPlotType == "total" || selectedPlotType.match("qos") || selectedPlotType.match("tor") || selectedPlotType.match("openvpn") ) && ip == "COMBINED") || (selectedPlotType != "total" && ip != "COMBINED") )
									{
										ipList.push(getDisplayIp(ip));
									}
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
									if(monitorName.match("bdist") && selectedPlotType != "total")
									{
										var plotIdName   = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_id"   : "table_id";
										ip = getSelectedValue(plotIdName);
										ip = ip == null ? "" : getRealIp(ip);
										
										
										ip = monitorData[ip] != null ? ip : ipList[0];
										
									
										//if new ip list differs from allowable selections, update
										if(selectedPlotType == "hostname")
										{
											setAllowableSelections(plotIdName, ipList, getHostnameList(ipList));
										}
										else
										{
											setAllowableSelections(plotIdName, ipList, ipList);
										}
										ipsWithData = ipList;
									}
									else
									{
										ip = ipList[0];
									}
									
									ip = ip == null ? "" : getRealIp(ip);
									var points = monitorData[ip][0]
									if(monitorIndex < 3)
									{
										plotLastTimePoint = monitorData[ip][1];
										plotCurrentTimePoint = monitorData[ip][2];
									}
									else
									{
										tableLastTimePoint = monitorData[ip][1];
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
								else if(monitorName.match("bdist") && monitorIndex < 3 )
								{
									//no ips defined
									var plotTypeName = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_type" : "table_type";
									var plotIdName   = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_id"   : "table_id";
									monitorList[monitorIndex] = "";
									setSelectedValue(plotTypeName, "none");
									document.getElementById(plotIdName).display = "none";


								}
							}
							else if(monitorName.match("bdist") && selectedPlotType != "total" && monitorIndex < 3 )
							{
								//monitor data null because no ips have been seen
								var plotIdName   = monitorIndex < 3 ? "plot" + (monitorIndex+1) + "_id"   : "table_id";
								monitorList[monitorIndex] = ""
								setSelectedValue(plotTypeName, "none");
								document.getElementById(plotIdName).style.display = "none";


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
					updateTotalPlot(totalPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI);
					updateDownloadPlot(downloadPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI );
					updateUploadPlot(uploadPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI );


					if(expandedFunctions[bndwS.Totl] != null)
					{
						var f = expandedFunctions[bndwS.Totl] ;
						f(totalPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI);
					}
					if(expandedFunctions[bndwS.Dnld] != null)
					{
						var f = expandedFunctions[bndwS.Dnld] ;
						f(downloadPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI);
					}
					if(expandedFunctions[bndwS.Upld] != null)
					{
						var f = expandedFunctions[bndwS.Upld] ;
						f(uploadPointSets, plotNumIntervals, plotIntervalLength, plotLastTimePoint, plotCurrentTimePoint, tzMinutes, UI);
					}



					updateBandwidthTable(tablePointSets, tableIntervalLength, tableLastTimePoint);
				}
				updateInProgress = false;
			}
		}
		var timeoutFun = function()
		{
			updateInProgress = false; 
		}
		updateReq = runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
		updateTimeoutId = setTimeout(timeoutFun, 5000);
	}
}

function twod(num)
{
	var nstr = "" + num; nstr = nstr.length == 1 ? "0" + nstr : nstr; 
	return nstr; 
}


function updateBandwidthTable(tablePointSets, interval, tableLastTimePoint)
{
	var rowData = [];
	var rowIndex = 0;
	var displayUnits = getSelectedValue("table_units");
	var timePoint = tableLastTimePoint;
	var nextDate = new Date();
	nextDate.setTime(timePoint*1000);
	nextDate.setUTCMinutes( nextDate.getUTCMinutes()+tzMinutes );
	if((parseInt(interval) == "NaN") && (interval.match(/month/) || interval.match(/day/)))
	{
		// When interval is month or day, the transition is always at beginning of day/month, so adding just a few hours will never change the day or month
		// However, when an hour gets subtracted for DST, there are problems.
		// So, always add three hours, so when DST shifts an hour back in November date doesn't get pushed back to previous month and wrong month is displayed
		nextDate = new Date( nextDate.getTime() + (3*60*60*1000))
	}
	var monthNames = UI.EMonths;
	
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

		var timeStr = "";
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
			//nextDate.setDate(2) //set second day of month, so when DST shifts hour back in November we don't push it back to previous month
			timeStr = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCFullYear();
			nextDate.setUTCMonth( nextDate.getUTCMonth()-1);
		}
		else if(parseInt(interval) != "NaN")
		{
			if(parseInt(interval) >= 28*24*60*60)
			{
				timeStr = monthNames[nextDate.getUTCMonth()] + " " + nextDate.getUTCFullYear() + " " + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
			}
			else if(parseInt(interval) >= 24*60*60)
			{
				timeStr = monthNames[nextDate.getUTCMonth()] + " " + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
			}
			else
			{
				timeStr = "" + twod(nextDate.getUTCHours()) + ":" + twod(nextDate.getUTCMinutes());
			}
			nextDate.setTime(nextDate.getTime()-(parseInt(interval)*1000));
		}
		vals.unshift(timeStr);
		rowData.push(vals);
		timePoint = nextDate.getTime()/1000;
	}

	var columnNames = [bndwS.Time, bndwS.Totl, bndwS.Dnld, bndwS.Upld];
	var bwTable=createTable(columnNames , rowData, "bandwidth_table", false, false);
	tableContainer = document.getElementById('bandwidth_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(bwTable);
}



function expand(name)
{
	var expWindow = expandedWindows[name];
	var xCoor = 0;
	var yCoor = 0;
	if(typeof(expWindow) != "undefined")
	{
		try { expWindow.close(); } catch(e){}
		expWindow = null;
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
	expWindow= window.open("bandwidth_expand.sh", name + " "+bndwS.BPlot, "width=850,height=650,left=" + xCoor + ",top=" + yCoor );
	expandedWindows[name] = expWindow;

	var runOnWindowLoad = function(name)
	{
		var loaded = false;
		var loadWin = expandedWindows[name];
		if(loadWin.document != null)
		{
			if(loadWin.document.getElementById("bandwidth_plot") != null)
			{
				var plotTitle = loadWin.document.getElementById("plot_title");
				if(plotTitle != null)
				{
					expandedFunctions[name] = getEmbeddedSvgPlotFunction("bandwidth_plot", loadWin.document);
					if(expandedFunctions[name] != null)
					{
						plotTitle.appendChild(loadWin.document.createTextNode(name + " "+bndwS.BUsag));
						loadWin.onbeforeunload=function(){ expandedFunctions[name] = null; expandedWindows[name] = null; }
						loaded = true;
					}
				}
			}
		}
		if(!loaded)
		{
			var rerun=function(){ runOnWindowLoad(name); }
			setTimeout(rerun,250);
		}
	}
	runOnWindowLoad(name);
}

function highResChanged()
{
	setControlsEnabled(false, true, bndwS.RstGr);

	var useHighRes15m = document.getElementById("use_high_res_15m").checked;
	var commands = [];
	commands.push("uci set gargoyle.bandwidth_display=bandwidth_display");
	commands.push("uci set gargoyle.bandwidth_display.high_res_15m=" + (useHighRes15m ? "1" : "0"));
	commands.push("uci commit");
	commands.push("/etc/init.d/bwmon_gargoyle restart");

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			window.location = window.location;
			setControlsEnabled(true);
		}
	}	
	var param = getParameterDefinition("commands", commands.join("\n"))  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}

function deleteData()
{
	if (confirm(bndwS.DelAD) == false)
	{
		return;
	}

	setControlsEnabled(false, true, bndwS.DelDW);

	var commands = [];
	commands.push("/etc/init.d/bwmon_gargoyle stop");
	commands.push("rm /tmp/data/bwmon/*");
	commands.push("rm /usr/data/bwmon/*");
	commands.push("/etc/init.d/bwmon_gargoyle start");

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
		}
	}
	var param = getParameterDefinition("commands", commands.join("\n"))  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
