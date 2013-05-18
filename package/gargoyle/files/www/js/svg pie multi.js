/*
	This program is copyright 2008,2009,2013 Eric Bishop and is distributed under the terms of the GNU GPL 
	version 2.0. 
	See http://gargoyle-router.com/faq.html#qfoss for more information
*/

var svgNs="http://www.w3.org/2000/svg";
var svgDoc;
var selectedId = null;

var topCoor=1;
var leftCoor=1;
var bottomCoor=1600;
var rightCoor=1600;

var initialized = false;
var visibleIndividuals = [];
var otherIndividuals = [];
var unselectedColors = [];
var selectedColors = [];
var orderedNames = [];
var numVisible = 0;
var numPies = 0;

function init(evt)
{
	svgDoc = evt.target.ownerDocument;
}

function addPercentsToLabel(labelBase, pieValues, pieTotals)
{
	var pcts = [];
	var pieIndex;
	for(pieIndex = 0; pieIndex < pieValues.length; pieIndex++)
	{
		pct = Math.floor((parseFloat(pieValues[pieIndex])*1000)/pieTotals[pieIndex])/10;
		pctStr = "" + pct + "%";
		pcts.push(pctStr);
	}
	var labelStr = labelBase + " ( " + pcts.join(" / ") + " )";
	return labelStr;
}

function setPieChartData(individualNames, pieNames, individualLabels, values, rankIndex, maxIndividualNames, recomputeColorsAndVisible, adjustLabelFunction)
{
	if(svgDoc == null)
	{
		return [];
	}
	rankIndex = parseInt(rankIndex);
	maxIndividualNames = parseInt(maxIndividualNames);	

	if(!initialized){ recomputeColorsAndVisible = true; }
	
	var numIndividuals = individualNames.length;
	numPies = values.length;
	
	
	//sort individualNames based on values[rankIndex]
	var nameIndices = [];
	var ni = 0;
	for(ni=0; ni < individualNames.length; ni++){ nameIndices.push(ni); }
	var sortIndices= function(a,b) { return values[rankIndex][b] - values[rankIndex][a]; }
	nameIndices.sort(sortIndices);

	if(recomputeColorsAndVisible)
	{
		visibleIndividuals = [];
		otherIndividuals = [];
		unselectedColors = [];
		selectedColors = [];
		numVisible = 0;
		selectedId = null;

		var numColors = individualNames.length <= maxIndividualNames ? maxIndividualNames : maxIndividualNames+1;
		var color = [255,0,0];

		var colorNum=0;
		for(colorNum=0; colorNum < numColors; colorNum++)
		{
			var nextColor = incrementColor(color, colorNum*Math.ceil(360/numColors) );
			var nextSelectedColor = getSelectedRgb(nextColor);
			unselectedColors.push(getColorStr(nextColor));
			selectedColors.push(getColorStr(nextSelectedColor));

			if(colorNum <  maxIndividualNames || numColors <= maxIndividualNames)
			{
				if(nameIndices[colorNum] != null)
				{
					visibleIndividuals[ individualNames[nameIndices[colorNum]] ] = [getColorStr(nextColor), getColorStr(nextSelectedColor)];
					numVisible++;

				}
			}
			else
			{
				var nameIndex;
				for(nameIndex=colorNum; nameIndex < nameIndices.length; nameIndex++)
				{
					otherIndividuals[ individualNames[nameIndices[nameIndex]] ] =  [getColorStr(nextColor), getColorStr(nextSelectedColor)];
				}
			}
		}
		initialized = true;
	}

	orderedNames = [];
	var orderedLabels  = [];
	var orderedValues  = [];
	var otherValueSums = [];
	var selectedIndex = -1;
	var pieIndex;
	for(pieIndex=0;pieIndex < values.length; pieIndex++)
	{
		orderedValues.push([]);
	}

	// we already have a list of sorted name indices
	// so.. go through the list in order, 
	// if it's an "other" name, add values to other sum(s)
	// otherwise it add it to ordered names, and add necessary
	// values to orderedValues
	while(nameIndices.length > 0)
	{
		var nextNameIndex = nameIndices.shift();
		var nextName = individualNames[nextNameIndex];
		var nextLabel = individualLabels[nextNameIndex];
		if(otherIndividuals[nextName] != null || (numVisible == maxIndividualNames && visibleIndividuals[nextName] == null) )
		{
			var valueIndex=0;
			for(valueIndex=0; valueIndex < numPies; valueIndex++)
			{
				otherValueSums[valueIndex] = (otherValueSums[valueIndex] == null ? 0 : otherValueSums[valueIndex]) + parseFloat(values[valueIndex][nextNameIndex]) ;
			}
		}
		else if(visibleIndividuals[nextName] != null  )
		{
			orderedNames.push(nextName);
			orderedLabels.push(nextLabel);
			for(valueIndex=0; valueIndex < numPies; valueIndex++)
			{
				(orderedValues[valueIndex]).push( parseFloat(values[valueIndex][nextNameIndex]) );
			}
			if(visibleIndividuals[nextName] == null )
			{
				var nextColorIndex = visibleIndividuals.length;
				visibleIndividuals[nextName] = [ unselectedColors[nextColorIndex], selectedColors[nextColorIndex] ];
				numVisible++;
			}
		}
	}
	if(otherValueSums.length > 0)
	{
		for(pieIndex=0; pieIndex < values.length; pieIndex++)
		{
			(orderedValues[pieIndex]).push( otherValueSums[pieIndex] );
		}
		if(individualLabels.length > individualNames.length)
		{
			orderedLabels.push( individualLabels[individualLabels.length-1] );
		}
		else
		{
			orderedLabels.push( "Others");
		}
	}

	var totalHeight = (bottomCoor-topCoor)+1;
	var radius = Math.ceil(((rightCoor-leftCoor)+1)/4); //diameter should never be bigger than 1/2 width
	if(numPies > 1)
	{
		vertRadius = Math.ceil( (((bottomCoor-topCoor)+1)/(2*numPies))*.85   );
		radius = radius < vertRadius ? radius : vertRadius;
	}
	var buffer =  Math.floor((totalHeight-(radius*2*numPies))/(2*numPies));
	var titleHeight = Math.ceil(buffer*.80);
	var titleOffset = Math.ceil(buffer*.30);	
	var regularStrokeWidth=Math.ceil(.0015*totalHeight);



	//construct pies
	var idValues = [];
	var pieTotals = [];
	var idIndex;
	for(idIndex=0; idIndex < orderedLabels.length; idIndex++){ idValues.push([]); }


	//construct colorIndex=>orderedIndex hash, since we want to build
	//pie in order of colors, not of decreasing percentage like labels
	var colorIndexToOrderedIndex = [];
	var colorIndex;
	for(colorIndex=0; colorIndex < unselectedColors.length; colorIndex++)
	{
		var matchingId = unselectedColors.length+1;
		var matchingIndex;
		for(matchingIndex = 0; matchingIndex < unselectedColors.length; matchingIndex++)
		{
			var name = orderedNames[matchingIndex] != null ? orderedNames[matchingIndex] : "others";
			var nameUnselectedColor = name == "others" ? unselectedColors[unselectedColors.length-1] : visibleIndividuals[name][0];
			matchingId = (nameUnselectedColor == unselectedColors[colorIndex]) ? matchingIndex : matchingId;
		}
		colorIndexToOrderedIndex.push(matchingId);
	}

	var centerPoints = [];
	var pieChartPaths = [];
	var pieIndex;
	for(pieIndex=0;pieIndex < numPies; pieIndex++)
	{
		var centerX = rightCoor-(radius+buffer);
		var centerY = (radius+buffer) + (pieIndex*2*(radius+buffer));

		var titleText = svgDoc.createElementNS(svgNs, "text");
		titleText.setAttribute("x", centerX-radius);
		titleText.setAttribute("y", centerY-radius-titleOffset );
		titleText.setAttribute("font-size", titleHeight + "px");
		titleText.setAttribute("font-family", "serif");
		titleText.setAttribute("font-weight", "bold");
		titleText.appendChild( svgDoc.createTextNode( pieNames[pieIndex] ));
		titleText.setAttribute("id", "pie_title_" + pieIndex);
		pieChartPaths.push(titleText);


		var radiansPlotted = 0;
		var previousX = centerX + (radius*Math.cos(radiansPlotted));
		var previousY = centerY + (-1*radius*Math.sin(radiansPlotted));
		var data = orderedValues[pieIndex];
		var dataSum = 0;
		for(dataIndex=0; dataIndex < data.length; dataIndex++) { dataSum = parseFloat(data[dataIndex]) + dataSum; }
		if(dataSum == 0)
		{
			for(dataIndex=0; dataIndex < data.length; dataIndex++) { data[dataIndex] = 1; dataSum++; }
		}
		
		pieTotals.push(dataSum);
		for(colorIndex=0; colorIndex < colorIndexToOrderedIndex.length; colorIndex++)
		{
			var dataIndex = colorIndexToOrderedIndex[colorIndex];
			var idValueList= idValues[dataIndex];
			if(idValueList != null)
			{
				idValueList.push( data[dataIndex] );
			
				var sliceName = orderedNames[dataIndex] != null ? orderedNames[dataIndex] : "others" ;
				var unselectedColor = sliceName == "others" ? unselectedColors[unselectedColors.length-1] : visibleIndividuals[sliceName][0];
				var selectedColor = sliceName == "others" ? selectedColors[selectedColors.length-1] : visibleIndividuals[sliceName][1];
		
				if(parseFloat(data[dataIndex]) != 0)
				{
					var angle = radiansPlotted + ((2 * Math.PI * parseFloat(data[dataIndex]))/dataSum);
					var x= centerX + (radius*Math.cos(angle));
					var y= centerY + (-1*radius*Math.sin(angle));
					var xDiff = x - previousX;
					var yDiff = y - previousY;


					var largeArc = angle-radiansPlotted > Math.PI ? 1 : 0;
					var defaultPathData = "M " + centerX + "," + centerY + " L " + previousX + "," + previousY + " a" + radius + "," + radius + " 0 " + largeArc + ",0 " + xDiff + "," + yDiff + " z";


					var newPath = svgDoc.createElementNS(svgNs, "path");
					newPath.setAttribute("d", defaultPathData );
					newPath.setAttribute("stroke-linejoin", "round");
					
					
					//special case where we have one slice occupying the whole pie
					if( data[dataIndex] == dataSum  )
					{
						newPath = svgDoc.createElementNS(svgNs, "circle");
						newPath.setAttribute("r", radius);
						newPath.setAttribute("cx", centerX);
						newPath.setAttribute("cy", centerY);
					}
			
					newPath.setAttribute("fill", unselectedColor);
					newPath.setAttribute("stroke", "black");
					newPath.setAttribute("stroke-width", regularStrokeWidth);
					newPath.setAttribute("id", "slice_" + pieIndex + "_" + dataIndex);
					if(selectedId == dataIndex)
					{
						newPath.setAttribute("stroke-width", 2*regularStrokeWidth);
						newPath.setAttribute("fill", selectedColor);
					}
			
					newPath.onmouseover=piePieceSelected;
					newPath.onmouseout=piePieceDeselected;
					pieChartPaths.push(newPath);
					previousX = x;
					previousY = y;
					radiansPlotted = angle;	
				}
			}
		}
	}

	//construct labels;
	var labelX = .005*totalHeight;
	var nextLabelY=buffer-titleOffset;
	var labelYIncrement=Math.floor(.07*totalHeight);
	var labelBoxWidth=Math.floor(.03*totalHeight);
	var labelIndex;
	var pieChartLabels = [];
	for(labelIndex = 0; labelIndex < orderedLabels.length; labelIndex++)
	{
		var labelName = orderedNames[labelIndex] != null ? orderedNames[labelIndex] : "others" ;
		
		adjustLabelFunction = adjustLabelFunction == null ? addPercentsToLabel : adjustLabelFunction;
		var baseLabel = orderedLabels[labelIndex];
		if(baseLabel == null)
		{
			baseLabel = (labelIndex > 0 && labelIndex == orderedLabels.length-1) ? "Others" : "Unknown";	
		}
		var labelStr = adjustLabelFunction(baseLabel, idValues[labelIndex], pieTotals);
		
		var unselectedColor = unselectedColors[unselectedColors.length-1];
		var selectedColor = selectedColors[selectedColors.length-1];
		if(labelName != "others")
		{
			unselectedColor = visibleIndividuals[labelName][0];
			selectedColor = visibleIndividuals[labelName][1];
		}

		var labelRect = svgDoc.createElementNS(svgNs, "rect");
		labelRect.setAttribute("x", labelX);
		labelRect.setAttribute("y", nextLabelY);
		labelRect.setAttribute("width", labelBoxWidth);
		labelRect.setAttribute("height", labelBoxWidth);
		labelRect.setAttribute("stroke", "black");
		labelRect.setAttribute("stroke-width", regularStrokeWidth);
		labelRect.setAttribute("id", "color_" + labelIndex);
		labelRect.setAttribute("fill",  unselectedColor);

		var labelText = svgDoc.createElementNS(svgNs, "text");
		labelText.setAttribute("x", labelX+(labelBoxWidth*1.25));
		labelText.setAttribute("y", nextLabelY+(labelBoxWidth*.85) );
		labelText.setAttribute("font-size", Math.floor(labelBoxWidth*.70) + "px");
		labelText.setAttribute("font-family", "serif");
		labelText.appendChild( svgDoc.createTextNode( labelStr ));
		labelText.setAttribute("id", "label_" + labelIndex);
		labelText.setAttribute("font-weight", "normal");

		if(selectedId == labelIndex)
		{
			labelRect.setAttribute("stroke-width", 2*regularStrokeWidth);
			labelText.setAttribute("font-weight", "bold");
			labelRect.setAttribute("fill", selectedColor);
		}

		var labelGroup = svgDoc.createElementNS(svgNs, "g");
		labelGroup.setAttribute("id", "group_" + labelIndex);
		labelGroup.appendChild(labelRect);
		labelGroup.appendChild(labelText);
		labelGroup.onmouseover=piePieceSelected;
		labelGroup.onmouseout=piePieceDeselected;
		pieChartLabels.push(labelGroup);
		nextLabelY = nextLabelY + labelYIncrement;
	}

	//finally, add labels & pie slices to svgDoc
	var pieContainer = svgDoc.getElementById("pie_container");
	while(pieContainer.firstChild != null)
	{
		pieContainer.removeChild(pieContainer.firstChild);
	}	
	while(pieChartLabels.length > 0)
	{
		pieContainer.appendChild(pieChartLabels.shift());
	}
	while(pieChartPaths.length > 0)
	{
		pieContainer.appendChild(pieChartPaths.shift());
	}

	//for convenience, return pie totals
	return pieTotals;
}

function piePieceSelected()
{
	id = this.id.match(/_([^_]*)$/)[1];
	var totalHeight = (bottomCoor-topCoor)+1;
	var regularStroke = Math.ceil(.0015*totalHeight);
	var selectedStroke = 2*regularStroke;
	
	if(id == selectedId) { return; }
	selectedId = id;

	var sliceName = orderedNames[id] != null ? orderedNames[id] : "others" ;
	var unselectedColor = sliceName == "others" ? unselectedColors[unselectedColors.length-1] : visibleIndividuals[sliceName][0];
	var selectedColor = sliceName == "others" ? selectedColors[selectedColors.length-1] : visibleIndividuals[sliceName][1];

	svgDoc.getElementById("color_" + id).setAttribute("stroke-width", selectedStroke);
	svgDoc.getElementById("color_" + id).setAttribute("fill", selectedColor);
	var lab  = svgDoc.getElementById("label_" + id);

	var oldText = lab.firstChild.data;
	lab.setAttribute("font-weight", "bold");
	lab.removeChild (lab.firstChild);
	lab.appendChild ( svgDoc.createTextNode(oldText) );

	var pieContainer = svgDoc.getElementById("pie_container");

	var pieIndex;
	for(pieIndex=0; pieIndex< numPies; pieIndex++)
	{
		var slice = svgDoc.getElementById("slice_" + pieIndex + "_" + id);
		if(slice != null)
		{
			slice.setAttribute("stroke-width", selectedStroke);
			slice.setAttribute("fill", selectedColor);
	
			pieContainer.removeChild(slice);
			pieContainer.appendChild(slice);
		}
	}
}
function piePieceDeselected()
{
	id = this.id.match(/_([^_]*)$/)[1];
	var totalHeight = (bottomCoor-topCoor)+1;
	var regularStroke = Math.ceil(.0015*totalHeight);
	var selectedStroke = 2*regularStroke;

	if(id == selectedId) { selectedId = null; }
	var sliceName = orderedNames[id] != null ? orderedNames[id] : "others" ;
	var unselectedColor = sliceName == "others" ? unselectedColors[unselectedColors.length-1] : visibleIndividuals[sliceName][0];
	var selectedColor = sliceName == "others" ? selectedColors[selectedColors.length-1] : visibleIndividuals[sliceName][1];

	svgDoc.getElementById("color_" + id).setAttribute("stroke-width", regularStroke);
	svgDoc.getElementById("color_" + id).setAttribute("fill", unselectedColor);
	
	var lab  = svgDoc.getElementById("label_" + id);
	var oldText = lab.firstChild.data;
	lab.setAttribute("font-weight", "normal");
	lab.removeChild (lab.firstChild);
	lab.appendChild ( svgDoc.createTextNode(oldText) );

	
	var pieContainer = svgDoc.getElementById("pie_container");

	var pieIndex;
	for(pieIndex=0; pieIndex< numPies; pieIndex++)
	{
		var slice = svgDoc.getElementById("slice_" + pieIndex + "_" + id);
		if(slice != null)
		{
			slice.setAttribute("stroke-width", regularStroke);
			slice.setAttribute("fill", unselectedColor);
	
			pieContainer.removeChild(slice);
			pieContainer.appendChild(slice);
		}
	}
}

function getSelectedRgb(unselectedRgb)
{
	var selectedRgb = [];
	var index=0;
	for(index=0; index < 3; index++)
	{
		var c = unselectedRgb[index];
		c = Math.ceil(c*.45)
		selectedRgb[index] = c;
	}
	return selectedRgb;
}

// takes R,G,B, array & increment size (in degrees of the color wheel), returns rgb array
//
// Note: you can't iteratively call this function because, given different starting points
// the function may increment in different directions.  Best to pick a single starting point 
// and then iteratively increase the increment to call function with
function incrementColor(rgb, increment)
{
	function sortRgb(num1, num2){ if(rgb[num1] == rgb[num2]){ return num2 - num1;} else {return rgb[num1] - rgb[num2]; }}
	indices = [0,1,2];
	indices.sort(sortRgb);
	max= rgb[indices[2]];
	min= rgb[indices[0]];
	middle = rgb[indices[1]];
	difference = max-min;

	increment = increment % 360;
	fullTurnDistance = 6*difference;
	incrementDistance = increment*fullTurnDistance/360;
	
	newRgb= [ rgb[0], rgb[1], rgb[2] ];
	lowIndex = indices[0];
	middleIndex = indices[1];
	highIndex = indices[2];


	while(incrementDistance > 0)
	{
		if(incrementDistance < (newRgb[highIndex]-newRgb[middleIndex]))
		{
			newRgb[middleIndex] = middle + incrementDistance;
			incrementDistance = 0;
		}
		else
		{
			incrementDistance = incrementDistance - (newRgb[highIndex]-newRgb[middleIndex]);
			newRgb[middleIndex] = max;
			subtractIncrement = incrementDistance < difference ? incrementDistance : difference ;
			newRgb[highIndex] = newRgb[highIndex] - subtractIncrement;
			incrementDistance = incrementDistance - subtractIncrement;
			
			oldHighIndex = highIndex;
			highIndex = middleIndex;
			middleIndex = lowIndex;
			lowIndex = oldHighIndex;
		}
	}

	return newRgb;
}
function getColorStr(rgb)
{
	function toHex(num) { return num < 16 ? "0" + Math.floor(num).toString(16) : Math.floor(num).toString(16) }
	return "#" + toHex(rgb[0]) + toHex(rgb[1]) + toHex(rgb[2]);
}
