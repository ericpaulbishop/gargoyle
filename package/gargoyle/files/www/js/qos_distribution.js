/*
 * This program is copyright © 2008,2009 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var qosStr=new Object(); //part of i18n

var uploadClassIds = [];
var downloadClassIds = [];
var uploadClassNames = [];
var downloadClassNames = [];

var uploadUpdateInProgress = false;
var downloadUpdateInProgress = false;
var updateInProgress = false;

var setUploadPie = null;
var setDownloadPie = null;



function getEmbeddedSvgSetFunction(embeddedId)
{
	windowElement = getEmbeddedSvgWindow(embeddedId);
	if( windowElement != null)
	{
		return windowElement.setPieChartData;
	}
	return null;
}

function initializePieCharts()
{
	
	uploadClassIds = [];
	downloadClassIds = [];
	uploadClassNames = [];
	downloadClassNames = [];

	var definedUploadClasses = [];
	var definedDownloadClasses = [];
	for(monitorIndex=0; monitorIndex < monitorNames.length; monitorIndex++)
	{
		var monId = monitorNames[monitorIndex];
		if(monId.match(/qos/))
		{
			var isQosUpload = monId.match(/up/);
			var isQosDownload = monId.match(/down/);
		
			var splitId = monId.split("-");
			splitId.shift();
			splitId.shift();
			splitId.pop();
			splitId.pop();
			var qosClass = splitId.join("-");
			var qosName = uciOriginal.get("qos_gargoyle", qosClass, "name");
			
			if(isQosUpload && definedUploadClasses[qosClass] == null)
			{
				uploadClassIds.push(qosClass);
				uploadClassNames.push(qosName);
				definedUploadClasses[qosClass] = 1;
			}
			if(isQosDownload && definedDownloadClasses[qosClass] == null)
			{
				downloadClassIds.push(qosClass);
				downloadClassNames.push(qosName);
				definedDownloadClasses[qosClass] = 1;
			}
		}
	}




	if(uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") == "")
	{
		document.getElementById("upload_container").style.display="none";
	}
	if(uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") == "")
	{
		document.getElementById("download_container").style.display="none";
	}
	
	
	setTimeout(initializePies, 150); //for some reason Opera 10.50 craps out if we try to load plot functions immediately
	setInterval( 'updatePieCharts()', 2000);
}

function initializePies()
{
	if(setUploadPie == null)
	{
		setUploadPie = getEmbeddedSvgSetFunction("upload_pie");
	}
	if(setDownloadPie == null)
	{
		setDownloadPie = getEmbeddedSvgSetFunction("download_pie");	
	}
	if(setUploadPie == null || setDownloadPie == null)
	{
		setTimeout( "initializePies()", 100);
	}
	else
	{
		setQosTimeframes();

	}
}




function setQosTimeframes()
{
	if( 	(!updateInProgress) && 
		(setUploadPie != null || uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") == "") && 
		(setDownloadPie != null || uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") == "") 
	)
	{
		updatePieCharts();
	}
	else
	{
		setTimeout( "setQosTimeframes()", 100);
		if(setUploadPie == null || setDownloadPie == null)
		{
			setUploadPie = getEmbeddedSvgSetFunction("upload_pie");
			setDownloadPie = getEmbeddedSvgSetFunction("download_pie");	
		}
	}
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


function updatePieCharts()
{
	if(!updateInProgress)
	{
		updateInProgress=true;
		
		var directions = ["up", "down" ];
		var monitorQueryNames = [];
		for(directionIndex = 0; directionIndex < directions.length; directionIndex++)
		{
			var direction = directions[directionIndex];
			var classIdList = direction == "up" ? uploadClassIds : downloadClassIds;
			var timeFrameIndex = parseInt(getSelectedValue(direction + "_timeframe"));
			for(classIndex=0; classIndex < classIdList.length; classIndex++)
			{
				monitorQueryNames.push( getMonitorId((direction == "up" ? true : false), timeFrameIndex, "qos", classIdList[classIndex], true) );
			}
		}
		var param = getParameterDefinition("monitor", monitorQueryNames.join(" ")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));


		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var monitors = parseMonitors(req.responseText);
				var directions = ["up", "down" ];
				var uploadClassData = [];
				var uploadClassLabels = [];
				var downloadClassData = [];
				var downloadClassLabels = [];
				for(directionIndex = 0; directionIndex < directions.length; directionIndex++)
				{
					var direction = directions[directionIndex];	
					var classData = [];
					var totalSum = 0;
					var classLabels = [];
					var directionMonitorNames = [];
					for(nameIndex=0; nameIndex < monitorQueryNames.length; nameIndex++)
					{
						if(monitorQueryNames[nameIndex].match(direction))
						{
							var classSum = 0;
							var monitor = monitors[ monitorQueryNames[nameIndex] ];
							if( monitor != null)
							{
								var points = monitor[0];
								classSum = parseInt(points[points.length-1]);
							}
							classData.push(classSum);
							totalSum = totalSum + classSum;
						}
					}
					var sumIsZero = totalSum == 0 ? true : false;
					if(sumIsZero)
					{
						var classIndex; 
						for(classIndex=0; classIndex < classData.length; classIndex++)
						{
							classData[classIndex] = 1;
							totalSum++;
						}
					}
					for(nameIndex=0; nameIndex < classData.length; nameIndex++)
					{
						var classNameList = direction.match("up") ? uploadClassNames : downloadClassNames;
						className = classNameList[nameIndex];
						if(sumIsZero)
						{
							var percentage = "(" + truncateDecimal( 100*(1/classData.length) ) + "%)";
							classLabels.push( className + " - " + parseBytes((classData[nameIndex]-1),null,true) + " " + percentage);
						}
						else
						{
							var percentage = "(" + truncateDecimal( 100*(classData[nameIndex])/totalSum ) + "%)";
							classLabels.push( className + " - " + parseBytes(classData[nameIndex],null,true) + " " + percentage);
						}
					}
					uploadClassData = direction.match("up") ? classData : uploadClassData;
					uploadClassLabels = direction.match("up") ? classLabels : uploadClassLabels;
					downloadClassData = direction.match("down") ? classData : downloadClassData;
					downloadClassLabels = direction.match("down") ? classLabels : downloadClassLabels;
				}
				
				if(uploadClassData.length > 0 && setUploadPie != null && uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") != "")
				{
					setUploadPie(uploadClassData, uploadClassLabels);
				}
				if(downloadClassData.length > 0 && setDownloadPie != null && uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") != "")
				{
					setDownloadPie(downloadClassData, downloadClassLabels);
				}
				updateInProgress = false;	
			}
		}
		runAjax("POST", "utility/load_bandwidth.sh", param, stateChangeFunction);
	}
}

function parseMonitors(outputData)
{
	var monitors = new Array();
	var dataLines = outputData.split("\n");
	var currentDate = parseInt(dataLines.shift());	
	for(lineIndex=0; lineIndex < dataLines.length; lineIndex++)
	{
		if(dataLines[lineIndex].length > 0)
		{
			monitorName = dataLines[lineIndex];
			monitorName = monitorName.replace(/[\t ]+.*$/, "");
			lineIndex++; 
			lineIndex++; //ignore first interval start
			lineIndex++; //ignore first interval end
			lastTimePoint = dataLines[lineIndex];
			lineIndex++;
			points = dataLines[lineIndex].split(",");
			monitors[monitorName] = [points, lastTimePoint];
		}
	}
	return monitors;
}

function truncateDecimal(dec)
{
	result = "" + ((Math.ceil(dec*10))/10);
	return result;
}



