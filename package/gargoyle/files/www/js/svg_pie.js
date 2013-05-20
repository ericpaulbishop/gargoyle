/*
	This program is copyright 2008,2009,2013 Eric Bishop and is distributed under the terms of the GNU GPL 
	version 2.0. 
	See http://gargoyle-router.com/faq.html#qfoss for more information
*/

var svgNs="http://www.w3.org/2000/svg";
var svgDoc;
var selectedId = null;
var unselectedColors = [];
var selectedColors = [];

function init(evt)
{
	svgDoc = evt.target.ownerDocument;

	/*
	var percentages = [ 25, 10, 8, 55, 2];
	var labels = ["red", "blue", "green", "orange", "purple"];
	setPieChartData(percentages, labels);
	*/
}

function setPieChartData(data, labels)
{
	if(svgDoc == null)
	{
		return;
	}

	//first construct labels;
	pieChartLabels = [];
	labelX = 1;
	nextLabelY=25;
	labelYIncrement=30;
	labelBoxWidth=15;
	colorIncrement = Math.ceil(360/data.length);
	color = [255,0,0];
	nextColorIncrement = 0;
	for(labelIndex = 0; labelIndex < labels.length; labelIndex++)
	{
		var unselectedColor = getColorStr(incrementColor(color, nextColorIncrement));
		var selectedColor = getColorStr(getSelectedRgb(incrementColor(color, nextColorIncrement)));
		unselectedColors[labelIndex] = unselectedColor;
		selectedColors[labelIndex] = selectedColor;

		labelRect = svgDoc.createElementNS(svgNs, "rect");
		labelRect.setAttribute("x", labelX);
		labelRect.setAttribute("y", nextLabelY);
		labelRect.setAttribute("width", labelBoxWidth);
		labelRect.setAttribute("height", labelBoxWidth);
		labelRect.setAttribute("stroke", "black");
		labelRect.setAttribute("stroke-width", "1");
		labelRect.setAttribute("id", "color_" + labelIndex);
		labelRect.setAttribute("fill",  unselectedColor);

		labelText = svgDoc.createElementNS(svgNs, "text");
		labelText.setAttribute("x", labelX+25);
		labelText.setAttribute("y", nextLabelY+labelBoxWidth);
		labelText.setAttribute("font-size", Math.floor(labelBoxWidth*.66) + "px");
		labelText.setAttribute("font-family", "serif");
		labelText.appendChild( svgDoc.createTextNode( labels[labelIndex] ));
		labelText.setAttribute("id", "label_" + labelIndex);

		if(selectedId == labelIndex)
		{
			labelRect.setAttribute("stroke-width", 3);
			labelText.setAttribute("font-weight", "bolder");
			labelRect.setAttribute("fill", selectedColor);
		}

		labelGroup = svgDoc.createElementNS(svgNs, "g");
		labelGroup.setAttribute("id", "group_" + labelIndex);
		labelGroup.appendChild(labelRect);
		labelGroup.appendChild(labelText);
		labelGroup.onmouseover=piePieceSelected;
		labelGroup.onmouseout=piePieceDeselected;
		pieChartLabels.push(labelGroup);

		nextColorIncrement = nextColorIncrement + colorIncrement;
		nextLabelY = nextLabelY + labelYIncrement;
	}

	//now construct pie slices
	centerX=325;
	centerY=150;
	radius=125;
	dataSum = 0;
	for(dataIndex=0; dataIndex < data.length; dataIndex++)
	{
		dataSum = dataSum + data[dataIndex];
	}
	colorIncrement = Math.ceil(360/data.length);
	color = [255,0,0];
	nextColorIncrement = 0;

	pieChartPaths = [];
	radiansPlotted = 0;
	previousX = centerX + (radius*Math.cos(radiansPlotted));
	previousY = centerY + (-1*radius*Math.sin(radiansPlotted));
	for(dataIndex=0; dataIndex < data.length; dataIndex++)
	{
		angle = radiansPlotted + ((2 * Math.PI * data[dataIndex])/dataSum);
		x= centerX + (radius*Math.cos(angle));
		y= centerY + (-1*radius*Math.sin(angle));
		xDiff = x - previousX;
		yDiff = y - previousY;
		largeArc = angle-radiansPlotted > Math.PI ? 1 : 0;
		defaultPathData = "M " + centerX + "," + centerY + " L " + previousX + "," + previousY + " a" + radius + "," + radius + " 0 " + largeArc + ",0 " + xDiff + "," + yDiff + " z";

		/*
		// This code was originally for moving a piece out from the pie when it was selected
		// when I actually implemented it the effect seemed a bit over the top, so i decided against using it
		// I'm leaving this code here in case I change my mind
		selectedAdjustment = 25;
		selectedAdjustmentAngle = (angle + radiansPlotted)/2;
		selX = selectedAdjustment*Math.cos(selectedAdjustmentAngle);
		selY = -1*selectedAdjustment*Math.sin(selectedAdjustmentAngle);
		selectedPathData = "M " + (centerX+selX) + "," + (centerY+selY) + " L " + (previousX+selX) + "," + (previousY+selY) + " a" + radius + "," + radius + " 0 " + largeArc + ",0 " + xDiff + "," + yDiff + " z";
		selectedPaths[""+dataIndex] = selectedPathData;
		defaultPaths[ ""+dataIndex ] = defaultPathData;
		*/

		var newPath;
		if(data[dataIndex] == dataSum)
		{
			newPath = svgDoc.createElementNS(svgNs, "circle");
			newPath.setAttribute("r", radius);
			newPath.setAttribute("cx", centerX);
			newPath.setAttribute("cy", centerY);
		}
		else
		{
			newPath = svgDoc.createElementNS(svgNs, "path");
			newPath.setAttribute("d", defaultPathData );
			newPath.setAttribute("stroke-linejoin", "round");
		}
		newPath.setAttribute("fill", unselectedColors[dataIndex]);
		newPath.setAttribute("stroke", "black");
		newPath.setAttribute("stroke-width", "1");
		newPath.setAttribute("id", "slice_" + dataIndex);
		if(selectedId == dataIndex)
		{
			newPath.setAttribute("stroke-width", "3");
			newPath.setAttribute("fill", selectedColors[dataIndex]);
		}
		newPath.onmouseover=piePieceSelected;
		newPath.onmouseout=piePieceDeselected;
		pieChartPaths.push(newPath);
		
		previousX = x;
		previousY = y;
		radiansPlotted = angle;	
		nextColorIncrement = nextColorIncrement + colorIncrement;
	}

	//finally, add labels & pie slices to svgDoc
	pieContainer = svgDoc.getElementById("pie_container");
	labelContainer=svgDoc.getElementById("label_container");
	while(labelContainer.firstChild != null)
	{
		labelContainer.removeChild(firstChild);
	}
	while(pieContainer.firstChild != null)
	{
		pieContainer.removeChild(pieContainer.firstChild);
	}
	for(pathIndex=0; pathIndex < pieChartPaths.length; pathIndex++)
	{
		if(pathIndex < pieChartLabels.length)
		{
			pieContainer.appendChild(pieChartLabels[pathIndex]);
		}
		pieContainer.appendChild(pieChartPaths[pathIndex]);
	}

}

function piePieceSelected()
{
	id = this.id.match(/_(.*)$/)[1];
	selectedId = id;
	svgDoc.getElementById("color_" + id).setAttribute("stroke-width", 3);
	svgDoc.getElementById("color_" + id).setAttribute("fill", selectedColors[id]);
	svgDoc.getElementById("label_" + id).setAttribute("font-weight", "bolder");
	
	slice = svgDoc.getElementById("slice_" + id);
	slice.setAttribute("stroke-width", "3");
	slice.setAttribute("fill", selectedColors[id]);
	
	pieContainer = svgDoc.getElementById("pie_container");
	pieContainer.removeChild(slice);
	pieContainer.appendChild(slice);
}
function piePieceDeselected()
{
	id = this.id.match(/_(.*)$/)[1];
	if(id == selectedId) { selectedId = null; }
	svgDoc.getElementById("color_" + id).setAttribute("stroke-width", "1");
	svgDoc.getElementById("color_" + id).setAttribute("fill", unselectedColors[id]);
	svgDoc.getElementById("label_" + id).setAttribute("font-weight", "normal");
	
	slice = svgDoc.getElementById("slice_" + id);
	slice.setAttribute("stroke-width", "1");
	slice.setAttribute("fill", unselectedColors[id]);
	
	pieContainer = svgDoc.getElementById("pie_container");
	pieContainer.removeChild(slice);
	pieContainer.appendChild(slice);

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
