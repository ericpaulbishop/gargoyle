/*
	This program is copyright 2008,2009,2013 Eric Bishop and is distributed under the terms of the GNU GPL 
	version 2.0. 
	See http://gargoyle-router.com/faq.html#qfoss for more information
*/

var svgNs = "http://www.w3.org/2000/svg"
var svgDoc;
var topCoor=1;
var leftCoor=1;
var rightCoor=800;
var bottomCoor=600;
var minStrokeWidth=1;


var tzMinutes = 0;

function init(evt)
{
	svgDoc = evt.target.ownerDocument;
	topCoor=1;
	leftCoor=1;
	rightCoor=800;
	bottomCoor=600;
	
	var graphRightCoor=Math.floor(rightCoor*85/100);
	var graphBottomCoor=Math.floor(bottomCoor*85/100);
	var graphHeight = (graphBottomCoor-topCoor)+1;

	var scaleFraction = 1;
	var inh = window.innerHeight;
	if(inh != null)
	{
		scaleFraction = inh/bottomCoor;
	}
	var minDisplay = Math.ceil(1/scaleFraction);
	minStrokeWidth = Math.ceil(.005*graphHeight);
	minStrokeWidth = minStrokeWidth < minDisplay ? minDisplay : minStrokeWidth;
}

//note: numDisplay intervals does NOT include last point, which is not a complete interval, so (max) number
//of points plotted is numDisplayIntervals+1
function plotAll(pointSets, numDisplayIntervals, intervalLength, lastIntervalStart, lastTime, tzm, UI)
{
	if(svgDoc == null)
	{
		return;
	}
	topCoor=1;
	leftCoor=1;
	rightCoor=800;
	bottomCoor=600;
	
	numDisplayIntervals = parseInt(numDisplayIntervals);
	lastIntervalStart = parseInt(lastIntervalStart);
	lastTime = parseInt(lastTime);
	if(tzm != null)
	{
		tzMinutes = parseInt(tzm);
	}

	var graphRightCoor=Math.floor(rightCoor*85/100);
	var graphBottomCoor=Math.floor(bottomCoor*85/100);
	var graphHeight = (graphBottomCoor-topCoor)+1;
	var graphWidth = (graphRightCoor-leftCoor)+1;

	var borderEl = svgDoc.getElementById("graph-border");
	borderEl.setAttribute("width", graphRightCoor);
	borderEl.setAttribute("height", graphBottomCoor);
	borderEl.setAttribute("stroke", "black");
	borderEl.setAttribute("stroke-width", minStrokeWidth )

	
	var maxLastIntervalSeconds = getNextTick(intervalLength, 1, lastIntervalStart, 1) - lastIntervalStart;
	var lastIntervalSeconds = lastTime-lastIntervalStart;
	var intervalLengths = [ lastIntervalSeconds ];
	var firstIntervalStart = lastIntervalStart;

	while(intervalLengths.length < numDisplayIntervals+1)
	{
		var previousTime = getNextTick(intervalLength, 1, firstIntervalStart, -1) ;
		intervalLengths.unshift( firstIntervalStart - previousTime );
		firstIntervalStart = previousTime;
	}
	
	//convert everything into bytes/s and get max
	//this solves problem of last interval being different length
	//as well as problem when dealing with months -- they aren't all the same length!
	var maxPoint=0;
	var adjPointSets = [];
	for(plotIndex=0; plotIndex < 3; plotIndex++)
	{
		if(pointSets[plotIndex] != null)
		{
			var points=pointSets[plotIndex];
			var adjPoints = [];
			for(pointIndex=points.length-1; pointIndex >= 0; pointIndex--)
			{
				var intervalIndex = intervalLengths.length-[points.length-pointIndex];
				adjPoints[pointIndex] = (intervalLengths[intervalIndex] > 0 && ""+parseFloat(points[pointIndex]) != "NaN") ? parseFloat(points[pointIndex])/intervalLengths[intervalIndex] : 0;
				maxPoint = adjPoints[pointIndex] > maxPoint ? adjPoints[pointIndex] : maxPoint;
			}
			while(adjPoints.length < numDisplayIntervals+1) { adjPoints.unshift(0); }
			adjPointSets[plotIndex] = adjPoints;
		}
	}

	//now that we've normalized y data by actual interval lengths, shorten first interval to amount we want to display
	var realFirstIntervalLength = intervalLengths[0];
	var firstTime = realFirstIntervalLength > lastIntervalSeconds ? firstIntervalStart + lastIntervalSeconds : firstIntervalStart + Math.floor(lastIntervalSeconds*.95);
	intervalLengths[0] = realFirstIntervalLength > lastIntervalSeconds ? realFirstIntervalLength - lastIntervalSeconds : realFirstIntervalLength -  Math.floor(lastIntervalSeconds*.95);

	var minIntervalSeconds = getMinIntervalSeconds(intervalLength);
	var xTickUnit = createXTicks(numDisplayIntervals, minIntervalSeconds, firstTime, lastTime, topCoor, graphBottomCoor, leftCoor, graphRightCoor, UI)
	var graphYMax = createYTicks(xTickUnit, maxPoint, leftCoor, graphRightCoor, topCoor, graphBottomCoor, UI)

	for(plotIndex=0; plotIndex < 3; plotIndex++)
	{
		if(adjPointSets[plotIndex] != null)
		{
			var adjPoints= adjPointSets[plotIndex];
			var plotPoints = [ ];
			var cumulativeTime = 0;
			var intervalIndex;
			for(intervalIndex=0; intervalIndex < intervalLengths.length; intervalIndex++)
			{
				
				var nextY = adjPoints[intervalIndex];
				nextY = nextY == null ? adjPoints[intervalIndex-1] : nextY;
				var yOffset = Math.floor(graphHeight*nextY/graphYMax);
				yOffset = yOffset == null || yOffset+"" == "NaN" ? 0 : yOffset;
				yOffset = graphBottomCoor - yOffset;

				if(intervalIndex == 0)
				{
					plotPoints.push( leftCoor + "," + graphBottomCoor );
					plotPoints.push( leftCoor + "," + yOffset );
					if(intervalLengths[0] > realFirstIntervalLength/2)
					{
						var midpointTime = (firstIntervalStart-firstTime) + (realFirstIntervalLength/2);
						var x = Math.floor(graphWidth*(midpointTime/(lastTime-firstTime)));
						if(x != "NaN")
						{
							plotPoints.push( (leftCoor + x) + "," + yOffset );
						}
					}
					cumulativeTime = intervalLengths.length > 1 ? intervalLengths[0] + (intervalLengths[1]/2) : intervalLengths[0];
				}
				else if(intervalIndex == intervalLengths.length-1)
				{
					if(intervalLengths[intervalIndex] > maxLastIntervalSeconds/2)
					{
						var midpointTime = (lastIntervalStart-firstTime) + (maxLastIntervalSeconds/2);
						var x = Math.floor(graphWidth*(midpointTime/(lastTime-firstTime)));
						plotPoints.push( (leftCoor + x) + "," + yOffset );
					}
					plotPoints.push( graphRightCoor + "," + yOffset );
					plotPoints.push( graphRightCoor + "," + graphBottomCoor );
					plotPoints.push( leftCoor + "," + graphBottomCoor );
				}
				else
				{
					var x = Math.floor(graphWidth*(cumulativeTime/(lastTime-firstTime)));
					plotPoints.push( (leftCoor + x) + "," + yOffset );
					cumulativeTime = cumulativeTime +  intervalLengths[intervalIndex]/2 + intervalLengths[intervalIndex+1]/2;
				}
			}
			svgDoc.getElementById("plot" + (plotIndex+1)).setAttribute("points", plotPoints.join(" "));
			svgDoc.getElementById("plot" + (plotIndex+1)).setAttribute("stroke-width", minStrokeWidth );
		}
		else
		{
			svgDoc.getElementById("plot" + (plotIndex+1)).setAttribute("points", "");
		}
	}
}

function getMinIntervalSeconds(intervalLength)
{
	var intervalSeconds = 60;
	if(intervalLength == "second")
	{
		intervalSeconds = 1;
	}
	else if(intervalLength == "minute")
	{
		intervalSeconds = 60;
	}
	else if(intervalLength == "hour")
	{
		intervalSeconds = 60*60;
	}
	else if(intervalLength == "day")
	{
		intervalSeconds = 60*60*24;
	}
	else if(intervalLength == "month")
	{
		intervalSeconds = 60*60*24*28;
	}
	else
	{
		intervalSeconds = intervalLength;
	}
	return intervalSeconds;
}

//max point must be in bytes/s
function createYTicks(xTickUnit, maxPoint, graphLeft, graphRight, graphTop, graphBottom, UI)
{
	var timePoints;
	var yTimeUnit;
	var rateMultiple = 1024; //report in at least kilobytes

	if(xTickUnit == "minute")
	{
		yTimeUnit=UI.sc; //bytes/s
	}
	else if(xTickUnit == "hour") 
	{
		yTimeUnit=UI.sc;  //bytes/s
	}
	else if(xTickUnit == "day") 
	{
		rateMultiple=rateMultiple/(60*60);
		yTimeUnit=UI.hr; //bytes/hr
	}
	else if(xTickUnit == "month") 
	{
		rateMultiple=rateMultiple/(60*60*24);
		yTimeUnit=UI.day; //bytes/day
	}

	maxRate = maxPoint/rateMultiple;

	maxLog = Math.floor(Math.log(maxRate)/Math.log(10));
	maxLogMultiple = 1+Math.floor(maxRate/Math.pow(10, maxLog));
	if(maxLogMultiple == 10)
	{
		maxLogMultiple = 1;
		maxLog = maxLog+1;
	}
	else if( maxLogMultiple == 2 && (2 - (maxRate/Math.pow(10, maxLog))) >= 0.5)
	{
		maxLogMultiple = 1.5;
	}
	if(maxLog < 1 && (maxLog <0 || maxLogMultiple < 5))
	{
		maxLog = 0;
		maxLogMultiple = 5;
	}

	var unit;
	var unitScaleFactor;
	if(maxLog < 3)
	{
		unit=UI.KB1;
		unitScaleFactor=1;
	}
	else if(maxLog < 6)
	{
		unit=UI.MB1;
		unitScaleFactor=Math.pow(10,3);
	}
	else if(maxLog < 9)
	{
		unit=UI.GB1;
		unitScaleFactor=Math.pow(10,6);
	}
	else
	{
		unit=UI.TB1;
		unitScaleFactor=Math.pow(10,9);
	}
	unit = unit + " / " + yTimeUnit;

	var tickSize;
	if(maxLogMultiple > 5)
	{
		tickSize = 2*Math.pow(10,maxLog);
	}
	else if(maxLogMultiple > 2)
	{
		tickSize = Math.pow(10,maxLog);
	}
	else
	{
		tickSize = 0.5 * Math.pow(10,maxLog);
	}

	yMax = 	maxLogMultiple * Math.pow(10,maxLog);

	var graphHeight = graphBottom-graphTop;
	svgDoc.getElementById("y-unit-container").setAttribute("font-size", .07*graphHeight);
	svgDoc.getElementById("y-unit-path").setAttribute("d", " M " + (graphRight+(.20*graphHeight) ) + " " + (Math.floor(.6*graphHeight)) + " L " + (graphRight+(.20*graphHeight)) + " " + 0);

	yUnitEl = svgDoc.getElementById("y-units")
	yUnitEl.firstChild.data = " "
	yUnitEl.firstChild.data = unit

	nextTick=0;
	tickNum=1;
	tickPath = "";
	while(nextTick < yMax)
	{
		yCoor= graphBottom - Math.floor((graphBottom-graphTop)*nextTick/yMax);
		if(nextTick != 0)
		{
			tickPath = tickPath + "M " + graphLeft + " " + yCoor + " L " + graphRight + " " + yCoor + " ";
		}

		tickLabel = (nextTick/unitScaleFactor);
		labelElement = svgDoc.getElementById("ytick-label" + tickNum);
		labelElement.style.display = "block";
		labelElement.setAttribute("x", graphRight+ (.02*graphHeight)   );
		labelElement.setAttribute("y", yCoor);
		labelElement.setAttribute("font-size", (.05*graphHeight) + "px" )
		labelElement.firstChild.data = ""; //safari shits itself if label doesn't change
		labelElement.firstChild.data = tickLabel;

		nextTick = nextTick + tickSize;
		tickNum++;
	}
	while(tickNum <= 7)
	{
		labelElement = svgDoc.getElementById("ytick-label" + tickNum);
		labelElement.style.display = "none";
		tickNum++
	}

	svgDoc.getElementById("yticks").setAttribute("d", tickPath);
	svgDoc.getElementById("yticks").setAttribute("stroke-width", minStrokeWidth );

	return rateMultiple * maxLogMultiple * Math.pow(10,maxLog); //return raw value of max value on graph
}

function createXTicks(numDisplayIntervals, intervalSeconds, firstTime, lastTime, graphTop, graphBottom, graphLeft, graphRight, UI)
{
	var minTotalIntervalLength = intervalSeconds*numDisplayIntervals;
	var timeUnit = "minute";
	var majorTickMultiple;
	function getMajorTickMultiple( unitSeconds )
	{
		var m=1;
		while( minTotalIntervalLength/(m*unitSeconds) > 6 ) { m++; } 
		return m;
	}
	if( minTotalIntervalLength < 4*60*60 )
	{
		timeUnit="minute";
		majorTickMultiple = getMajorTickMultiple(60);
	}
	else if( minTotalIntervalLength < 4*60*60*24)
	{
		timeUnit="hour";
		majorTickMultiple = getMajorTickMultiple(60*60);
	}
	else if( minTotalIntervalLength < 4*60*60*24*28)
	{
		timeUnit="day";
		majorTickMultiple = getMajorTickMultiple(60*60*24);
	}
	else if( minTotalIntervalLength < 4*60*60*24*28*12)
	{
		timeUnit="month";
		majorTickMultiple = getMajorTickMultiple(60*60*24*28);
	}

	var minorTickMultiple = (1/3);
	if(majorTickMultiple == 2)
	{
		minorTickMultiple = 1;
	}
	if(majorTickMultiple >= 3)
	{
		minorTickMultiple = Math.floor(majorTickMultiple/3); 
	}

	var graphHeight = graphBottom-graphTop;
	var nextMinorTick = getNextTick(timeUnit, minorTickMultiple, firstTime, 1);
	var minorPathString= "";
	while( nextMinorTick < lastTime )
	{
		xCoor = graphLeft + Math.floor((graphRight-graphLeft)*(nextMinorTick - firstTime)/( lastTime-firstTime ));
		minorPathString= minorPathString + "M " + xCoor + " " + graphTop + " L " + xCoor + " " + graphBottom + " ";
		nextMinorTick = getNextTick(timeUnit, minorTickMultiple, nextMinorTick, 1);
	}
	svgDoc.getElementById("x-minor-ticks").setAttribute("d", minorPathString);
	svgDoc.getElementById("x-minor-ticks").setAttribute("stroke-width", minStrokeWidth );
	

	majorPathString= "";
	tickNum = 1;
	var nextMajorTick = getNextTick(timeUnit, majorTickMultiple, firstTime, 1);
	while(nextMajorTick < lastTime && tickNum <= 10)
	{
		xCoor = graphLeft + Math.floor((graphRight-graphLeft)*(nextMajorTick - firstTime)/(lastTime - firstTime));
		majorPathString= majorPathString + "M " + xCoor + " " + (graphBottom+1) + " L " + xCoor + " " + (graphBottom+Math.floor(.02*(graphBottom-graphTop))) + " ";

		tickLabel = "";
		tickDate = new Date();
		tickDate.setTime(nextMajorTick*1000);
		tickDate.setUTCMinutes( tickDate.getUTCMinutes()+tzMinutes )
		monthAbbreviations=UI.EMonths;
		if(timeUnit == "minute" || timeUnit == "hour")
		{
			tickLabel = tickDate.getUTCHours() + ":" + (tickDate.getUTCMinutes() >= 10 ? tickDate.getUTCMinutes() : "0" + tickDate.getUTCMinutes());
		}
		else if(timeUnit == "day")
		{
			tickLabel = monthAbbreviations[ tickDate.getUTCMonth() ] + " " + tickDate.getUTCDate();
		}
		else
		{
			tickLabel = monthAbbreviations[ tickDate.getUTCMonth() ];
		}
		labelElement = svgDoc.getElementById("major-xtick-label" + tickNum);
		labelElement.style.display = "block";
		labelElement.setAttribute("x", xCoor);
		labelElement.setAttribute("y", graphBottom + Math.ceil(.07*(graphBottom-graphTop))  );
		labelElement.setAttribute("font-size", Math.ceil(.05*graphHeight) + "px" )
		labelElement.firstChild.data = ""; //safari shits itself if label doesn't change
		labelElement.firstChild.data = tickLabel;

		var nt = getNextTick(timeUnit, majorTickMultiple, nextMajorTick, 1);
		nextMajorTick = nt;
		tickNum++;
	}
	while(tickNum <= 10)
	{
		tickLabelId = "major-xtick-label" + tickNum;
		svgDoc.getElementById(tickLabelId).style.display = "none";
		svgDoc.getElementById(tickLabelId).firstChild.data = "";
		tickNum++;
	}
	svgDoc.getElementById("x-major-ticks").setAttribute("d", majorPathString);
	svgDoc.getElementById("x-major-ticks").setAttribute("stroke-width", minStrokeWidth );

	return timeUnit;
}

function getNextTick(unit, multiple, currentTime, incrementDirection)
{
	//direction is either -1 or 1, -1 for previous tick, 1 for next tick
	incrementDirection = incrementDirection != -1 ? 1 : -1;

	var currentDate = new Date();
	var nextDate = new Date();
	currentDate.setTime(currentTime*1000);
	var nextTime = currentTime;
	nextDate.setTime(nextTime*1000);
	nextDate.setUTCMinutes(nextDate.getUTCMinutes()+tzMinutes)
	currentDate.setUTCMinutes(currentDate.getUTCMinutes()+tzMinutes)
	var incFunction = null;
	var multMatchFunction = null;
	if(unit == "second")
	{
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCSeconds( nextDate.getUTCSeconds() + 1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return (nextDate.getUTCSeconds() % multiple == 0) ? true : false; }
	}
	else if(unit == "minute")
	{
		nextDate.setUTCSeconds(0);
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCMinutes( nextDate.getUTCMinutes()+1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return ( Math.floor(nextDate.getTime()/(60*1000)) % multiple == 0) ? true : false; }
	}
	else if(unit == "hour")
	{
		nextDate.setUTCSeconds(0);
		nextDate.setUTCMinutes(0);
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCHours(nextDate.getUTCHours()+1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return ( Math.floor(nextDate.getTime()/(60*60*1000)) % multiple == 0) ? true : false; }

	}
	else if(unit == "day")
	{
		nextDate.setUTCSeconds(0);
		nextDate.setUTCMinutes(0);
		nextDate.setUTCHours(0);
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCDate(nextDate.getUTCDate()+1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return ( Math.floor(nextDate.getTime()/(24*60*60*1000)) % multiple == 0) ? true : false; }
	}
	else if(unit == "month")
	{
		nextDate.setUTCSeconds(0);
		nextDate.setUTCMinutes(0);
		nextDate.setUTCHours(0);
		nextDate.setUTCDate(1);
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCMonth(nextDate.getUTCMonth()+1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){  return ((nextDate.getUTCMonth() + (12*parseInt(nextDate.getUTCFullYear()))) % multiple == 0) ? true : false; }
	}
	else if (unit == "year")
	{
		nextDate.setUTCSeconds(0);
		nextDate.setUTCMinutes(0);
		nextDate.setUTCHours(0);
		nextDate.setUTCDate(1);
		nextDate.setUTCMonth(0);
		
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime(nextDate.getTime());
			incDate.setUTCFullYear(nextDate.getUTCFullYear()+1*incrementDirection);
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return (nextDate.getUTCFullYear() % multiple == 0) ? true : false; }
	}
	else if(parseInt(unit) != "NaN")
	{
		incFunction = function(nextDate)
		{
			var incDate = new Date();
			incDate.setTime( nextDate.getTime() + incrementDirection*(parseInt(unit)*1000));
			return incDate;
		}
		multMatchFunction = function(nextDate,multiple){ return true; }
	}

	//var increment =  multiple*(incDate.getTime()-nextDate.getTime());

	var iter = 0;	
	while(	(nextDate.getTime() <= currentDate.getTime() && incrementDirection > 0) ||
		(nextDate.getTime() >= currentDate.getTime() && incrementDirection < 0)
		)
	{
		if(multiple < 1)
		{
			var increment =  multiple*((incFunction(nextDate)).getTime()-nextDate.getTime());
			nextDate.setTime(nextDate.getTime() + Math.floor(increment))
		}
		else
		{
			var maxInc = Math.floor(multiple); //if multiple > 0, should always be an integer, but be sure
			var incrementIndex;
			nextDate = incFunction(nextDate);
			for(incrementIndex=0;incrementIndex < maxInc && (!multMatchFunction(nextDate,multiple)); incrementIndex++)
			{
				nextDate = incFunction(nextDate);
			}
		}
		
		iter++;
		if(iter > (50+multiple))
		{
			alert("BAD BAD BAD!!!!!\nunit = " + unit + "\ncurrentTime = " + currentTime + ", increment = " + increment)
			return nextDate;
		}
	}
	nextDate.setUTCMinutes( nextDate.getUTCMinutes()-tzMinutes)
	nextTime = Math.floor(nextDate.getTime()/1000);
	return nextTime;
}
