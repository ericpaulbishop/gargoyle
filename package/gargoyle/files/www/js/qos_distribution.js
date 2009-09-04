/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var uploadClassIds;
var downloadClassIds;

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
	
	uploadClassIds = new Array();
	downloadClassIds = new Array();

	upIdHash = [];
	downIdHash = [];
	for(monitorIndex=0; monitorIndex < monitorNames.length; monitorIndex++)
	{
		if(monitorNames[monitorIndex].match(/^qos\-upload\-/))
		{
			
			classId=monitorNames[monitorIndex].match(/os\-upload\-(.*)-[^\-]+$/)[1];
			upIdHash[classId] = 1;
		}
		else if(monitorNames[monitorIndex].match(/^qos\-download\-/))
		{
			classId=monitorNames[monitorIndex].match(/os\-download\-(.*)-[^\-]+$/)[1];
			downIdHash[classId] = 1;
		}
	}
	for(id in upIdHash)
	{
		uploadClassIds.push(id);
	}
	for(id in downIdHash)
	{
		downloadClassIds.push(id);
	}

	initializePies = function()
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
	}
	initializePies();


	setQosTimeframes();

	if(uciOriginal.get("qos_gargoyle", "upload", "total_bandwidth") == "")
	{
		document.getElementById("upload_container").style.display="none";
	}
	if(uciOriginal.get("qos_gargoyle", "download", "total_bandwidth") == "")
	{
		document.getElementById("download_container").style.display="none";
	}
	setInterval( 'updatePieCharts()', 2000);
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

function updatePieCharts()
{

	if(!updateInProgress)
	{
		updateInProgress=true;
		
		var directions = ["upload", "download" ];
		var monitorNames = [];
		for(directionIndex = 0; directionIndex < directions.length; directionIndex++)
		{
			var direction = directions[directionIndex];
			var classIdList = direction == "upload" ? uploadClassIds : downloadClassIds;
			var timeFrame = getSelectedValue(direction + "_timeframe");
			for(classIndex=0; classIndex < classIdList.length; classIndex++)
			{
				monitorNames.push("qos-" + direction + "-" + classIdList[classIndex] + "-" + timeFrame);
			}
		}
		var param = getParameterDefinition("monitor", monitorNames.join(" ")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));


		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var monitors = parseMonitors(req.responseText);
				var directions = ["upload", "download" ];
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
					for(nameIndex=0; nameIndex < monitorNames.length; nameIndex++)
					{
						if(monitorNames[nameIndex].indexOf("qos-" + direction) >= 0)
						{
							var classSum = 1;
							var monitor = monitors[ monitorNames[nameIndex] ];
							if( monitor != null)
							{

								var points = monitor[0];
								for(pointIndex=0; pointIndex < points.length; pointIndex++)
								{
									classSum = (1*classSum) + (1*(points[pointIndex] >= 0 ? points[pointIndex] : 0));
								}
								classData.push(classSum);
								directionMonitorNames.push( monitorNames[nameIndex] );
								totalSum = totalSum + classSum;
							}
						}
					}
					totalSum = totalSum + classData.length;
					for(nameIndex=0; nameIndex < directionMonitorNames.length; nameIndex++)
					{
						var classSum = 1;
						var monitor = monitors[ directionMonitorNames[nameIndex] ];
						var classIdList = direction == "upload" ? uploadClassIds : downloadClassIds;
						var className = uciOriginal.get("qos_gargoyle", classIdList[nameIndex], "name");
						className = className == ""  ? classIdList[nameIndex] : className;
						var percentage = "(" + truncateDecimal( 100*(classData[nameIndex]-1)/totalSum ) + "%)";
						classLabels.push( className + " - " + parseBytes((classData[nameIndex]-1)) + " " + percentage);
					}
					if(direction == "upload")
					{
						uploadClassData = classData;
						uploadClassLabels = classLabels;
					}
					else
					{
						downloadClassData = classData;
						downloadClassLabels = classLabels;
					}
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
	monitors = new Array();
	dataLines = outputData.split("\n");
	
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

function parseBytes(bytes)
{
	var parsed;
	if(bytes > 1024*1024*1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024*1024)) + "TB";
	}
	else if(bytes > 1024*1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024*1024)) + "GB";
	}
	else if(bytes > 1024*1024)
	{
		parsed = truncateDecimal(bytes/(1024*1024)) + "MB";
	}
	else if(bytes > 1024)
	{
		parsed = truncateDecimal(bytes/(1024)) + "KB";
	}
	else
	{
		parsed = bytes + "B";
	}
	return parsed;
}

function truncateDecimal(dec)
{
	result = "" + ((Math.ceil(dec*10))/10);
		return result;
}



