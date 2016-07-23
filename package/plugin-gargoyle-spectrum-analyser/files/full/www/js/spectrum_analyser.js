/*
 * This program is copyright © 2015 Michael Gray and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var spectrum=new Object(); //part of i18n

var interfaces;
var freq_low;
var freq_high;
var detected;
var band;
var plotdata = [];

// Xband = [[channel #],[centre freq],[low freq],[high freq]]
var gband = [
				[1,2,3,4,5,6,7,8,9,10,11,12,13,14],
				[2.412,2.417,2.422,2.427,2.432,2.437,2.442,2.447,2.452,2.457,2.462,2.467,2.472,2.484],
				[2.401,2.406,2.411,2.416,2.421,2.426,2.431,2.436,2.441,2.446,2.451,2.456,2.461,2.473],
				[2.423,2.428,2.433,2.438,2.443,2.448,2.453,2.458,2.463,2.468,2.473,2.478,2.483,2.495]
			]
var aband = [
				[36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165],
				[5.180,5.200,5.220,5.240,5.260,5.280,5.300,5.320,5.500,5.520,5.540,5.560,5.580,5.600,5.620,5.640,5.660,5.680,5.700,5.745,5.765,5.785,5.805,5.825],
				[5.170,5.190,5.210,5.230,5.250,5.270,5.290,5.310,5.490,5.510,5.530,5.550,5.570,5.590,5.610,5.630,5.650,5.670,5.690,5.735,5.755,5.775,5.795,5.815],
				[5.190,5.210,5.230,5.250,5.270,5.290,5.310,5.330,5.510,5.530,5.550,5.570,5.590,5.610,5.630,5.650,5.670,5.690,5.710,5.755,5.775,5.795,5.815,5.835]
			]

function initialiseAll()
{
	var ivalues = [];
	var inames = [];
	//First, we should adjust the drop down list for the interfaces. We should identify which ones are 2.4 and 5ghz as well.
	interfaces = parseInterfaces(wifiLines);
	if(interfaces != -1)
	{
		for(var x = 0; x < interfaces.length; x++)
		{
			ivalues.push(interfaces[x][0]);
			inames.push(interfaces[x][1]);
		}
		setAllowableSelections('interface', ivalues, inames);	//populate the dropdown list

		initialisePlots();

		getWifiData();
	}
	//otherwise do nothing
}


function initialisePlots()
{
	//get the selected band, and then set the limits of the graph appropriately
	band = interfaces[document.getElementById("interface").selectedIndex][1];
	if(band == "2.4GHz")
	{
		freq_low = 2.4;		//technically 2.401, but 2.400 graphs better
		freq_high = 2.5;		//technically 2.495, but 2.500 graphs better
	}
	else
	{
		freq_low = 5.165;	//technically 5.170, but 5.165 graphs better
		freq_high = 5.840;	//technically 5.835, but 5.840 graphs better
	}
}

function changeBand()
{
	initialisePlots();
	getWifiData();
}

function parseInterfaces(lines)
{
	//if we have no interfaces detected, exit
	if(lines.length == 0) { return -1; }

	//otherwise, populate the data and assign correct frequency band
	var interfaceData = [];
	lineIndex = 0;

	//lines is of the format:	wlanX ##; wlanY ##; etc etc.
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];
		var wlan = nextLine.split(" ");

		var guest  = wlan[0].indexOf("-");
		if(guest != -1)
		{
			continue;
		}

		var interfaceid = wlan[0];
		if(wlan[1] > 14)
		{
			var interfaceband = "5GHz";
		}
		else
		{
			var interfaceband = "2.4GHz";
		}

		interfaceData.push( [ interfaceid, interfaceband ] );
	}
	return interfaceData;
}


function getWifiData()
{
	var Commands = [];
	var parsedWifiData = [];
	var selectedband = interfaces[document.getElementById("interface").selectedIndex][0];

	//scan command, substituting the correct WLAN
	Commands.push("iw " + selectedband + " scan 2>&1 | awk -F'\\\n' '{print \"\\\"\"$0\"\\\"\" }'");

	var param = getParameterDefinition("commands", Commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));	
	var stateChangeFunction = function(req)
	{
		if (req.readyState == 4)
		{
			var shell_output = req.responseText.replace(/Success/, "");		//raw output from the shell
			shell_output = shell_output.replace(/\"/g,"");					//remove any doubleq quotes
			parsedWifiData = parseWifiData(shell_output);
			if(parsedWifiData == -1)
			{
				alert(spectrum.Noscan);
				d3.select("svg").remove();
				plotdata = [];
				window.removeEventListener("resize", plotall);//plotall(parsedWifiData);
			}
			else
			{
				generateGraphData(parsedWifiData);
			}
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function parseWifiData(rawScanOutput)
{
	if((rawScanOutput != null) && (rawScanOutput.indexOf("\n") != 0) && (rawScanOutput.indexOf("\r") != 0))
	{
		var parsed = [ [],[],[],[],[] ];
		var cells = rawScanOutput.split(/BSS [A-Fa-f0-9]{2}[:]/g);
		cells.shift(); //get rid of anything before first AP data
	
		var getCellValues=function(id, cellLines)
		{
			var vals=[];
			var lineIndex;
			for(lineIndex=0; lineIndex < cellLines.length; lineIndex++)
			{
				var line = cellLines[lineIndex];
				var idIndex = line.indexOf(id);
				var cIndex  = line.indexOf(":");
				var eqIndex = line.indexOf("=");
				var splitIndex = cIndex;
				if(splitIndex < 0 || (eqIndex >= 0 && eqIndex < splitIndex))
				{
					splitIndex = eqIndex;
				}
				if(idIndex >= 0 && splitIndex > idIndex)
				{
					var val=line.substr(splitIndex+1);
					val = val.replace(/^[^\"]*\"/g, "");
					val = val.replace(/\".*$/g, "");
					val = val.replace(/^[ ]/g,"");
					val = val.replace(/ dBm/g,"");
					val = val.replace(/channel /g,"");
					vals.push(val);
				}
			}
			return vals;
		}

		while(cells.length > 0)
		{
			var cellData  = cells.shift();
			var cellLines = cellData.split(/[\r\n]+/);

			var ssid    = getCellValues("SSID", cellLines).shift();
			var prichannel = getCellValues("DS Parameter set", cellLines).shift();
			var secchannel = getCellValues("secondary channel offset", cellLines).shift();
			var sigStr = getCellValues("signal", cellLines).shift();
			var vhtwidth = getCellValues("* channel width", cellLines).shift();

			//if we don't get a primary channel then the network isn't following the standard. attempt to retrieve it from the HT operation data. If we can't find this section then toss it out.
			if (! prichannel)
			{
				var prichannel = getCellValues("* primary channel", cellLines).shift();
			}
			if (! secchannel)
			{
				secchannel = "no secondary";	//if we don't get a result for the secondary channel, set it to this so we don't error
			}

			if(ssid != null && prichannel != null && secchannel != null && sigStr != null ) 
			{
				parsed[0].push(ssid);
				parsed[1].push(prichannel);
				parsed[2].push(secchannel);
				parsed[3].push(sigStr);
				parsed[4].push(vhtwidth);
				//parsed[4].push( prichannel > 30 ? "5GHz" : "2.4GHz")	we don't need this anymore
			}
		}
		//check for duplicate data and append _# if necessary
		for(x = 0; x < parsed[0].length; x++)
		{
			append = 2;
			for(y = (x+1); y < parsed[0].length; y++)
			{
				if((parsed[0][x] == parsed[0][y])/* && (parsed[4][x] == parsed[4][y])*/)	//second part only necessary if we end up scanning both spectrum at once
				{
					parsed[0][y] = parsed[0][y]+"_"+append;
					append += 1;
				}
			}
			if(append > 2)
			{
				parsed[0][x] = parsed[0][x]+"_1";
			}
		}
		return parsed;
	}
	else
	{
		return(-1);
	}
}

function generateGraphData(detected)
{
	//var plotdata = [];
	var freqoffset = 0.001;	//for making the graphs look pretty
	var htoffset = 4;	//for 802.11n channels
	var vhtoffset = 12;	//for 802.11ac channels
	if(band == "2.4GHz")
	{
		for(x = 0; x < detected[0].length; x++)
		{
			var SSID = detected[0][x];
			var channel = detected[1][x];
			var secondary = detected[2][x];
			var level = detected[3][x];
			//var band = detected[4][x];
			if(secondary == "no secondary")
			{
				plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,2)};
				plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,2)+freqoffset)};
				plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,3)-freqoffset)};
				plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,3)};
			}
			if(secondary == "above")
			{
				plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,2)};
				plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,2)+freqoffset)};
				plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData((channel-(-htoffset)),3)-freqoffset)};
				plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData((channel-(-htoffset)),3)};
			}
			if(secondary == "below")
			{
				plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData((channel-htoffset),2)};
				plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData((channel-htoffset),2)+freqoffset)};
				plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,3)-freqoffset)};
				plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,3)};
			}
		}
	}
	else if(band == "5GHz")
	{
		for(x = 0; x < detected[0].length; x++)
		{
			var SSID = detected[0][x];
			var channel = detected[1][x];
			var secondary = detected[2][x];
			var level = detected[3][x];
			var vhtwidth = detected[4][x];
			//var band = detected[4][x];
			if(secondary == "no secondary")
			{
				plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,2)};
				plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,2)+freqoffset)};
				plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,3)-freqoffset)};
				plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,3)};
			}
			if(secondary == "above")
			{
				if(! vhtwidth)
				{
					plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,2)};
					plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,2)+freqoffset)};
					plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData((channel-(-htoffset)),3)-freqoffset)};
					plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData((channel-(-htoffset)),3)};
				}
				else		//when 160MHz channels come along, we should look for vhtwidth = 2, and then grab the 2 lots of "centre frequency"s data. for now this is fine.
				{
					plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,2)};
					plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,2)+freqoffset)};
					plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData((channel-(-vhtoffset)),3)-freqoffset)};
					plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData((channel-(-vhtoffset)),3)};
				}
			}
			if(secondary == "below")
			{
				plotdata[x*4] = {"ssid":SSID, "level":"-100", "freq":getfreqData((channel-htoffset),2)};
				plotdata[x*4+1] = {"ssid":SSID, "level":level, "freq":(getfreqData((channel-htoffset),2)+freqoffset)};
				plotdata[x*4+2] = {"ssid":SSID, "level":level, "freq":(getfreqData(channel,3)-freqoffset)};
				plotdata[x*4+3] = {"ssid":SSID, "level":"-100", "freq":getfreqData(channel,3)};
			}
		}
	}
	else
	{
		//something is wroooooooooooooooooong
		plotdata = -1;
	}
	
	plotall();
	window.addEventListener('resize', plotall);
}

function getfreqData(channel,info)
{
	//info = 1, return centrefreq	info = 2, return lowfreq	info = 3, return highfreq
	if(channel > 14)
	{
		a = aband[0].indexOf(parseInt(channel));
		return aband[info][a];
	}
	else
	{
		a = gband[0].indexOf(parseInt(channel));
		return gband[info][a];
	}
}

function plotall()
{
	if(plotdata.length == 0)
	{
		return -1;
	}
	var MARGINS = {}, HEIGHT, WIDTH, LEGENDOFFSET;

	LEGENDOFFSET = 30;

	function updateDimensions(winWidth)
	{
		MARGINS.top = 20;
		MARGINS.right = 10;
		MARGINS.left = 45;
		MARGINS.bottom = 50;

		WIDTH  = winWidth - MARGINS.left - MARGINS.right;
		HEIGHT = winWidth*0.8 - MARGINS.top - MARGINS.bottom;
	}

	d3.select("svg").remove();

	var maxsig = -150;		//set the maximum signal level obsurdly low first
	for (x = 0; x < plotdata.length; x++)		//then test for values higher than it until we find the highest
	{
		if(plotdata[x].level > maxsig)
		{
			maxsig = plotdata[x].level - (-10);	//set the maximum 10 above what it actually is so the graph doesn't look stupid
		}
	}
	if(maxsig > 0)
	{
		maxsig = 0;			//if the max signal level got above 0, set it to 0
	}
	
	var dataGroup = d3.nest()
		.key(function(d) { return d.ssid; })
		.entries(plotdata);				//break the data up into "keys" based on SSID which _should_ be unique (once i stuff around with them)

	var chartcontainerwidth = document.getElementById("spectrum_plot").offsetWidth;
	updateDimensions(chartcontainerwidth);

	var spect = d3.select("#spectrum_plot").append("svg"), 
		lSpace = WIDTH/dataGroup.length;	//set aside some space for the legend
		xScale = d3.scale.linear().range([MARGINS.left, WIDTH - MARGINS.right]).domain([freq_low,freq_high]),	//xscale width and range
		yScale = d3.scale.linear().range([HEIGHT - MARGINS.bottom, MARGINS.top + LEGENDOFFSET]).domain([-100,maxsig]),			//yscale height and range
		xAxis = d3.svg.axis().scale(xScale),
		yAxis = d3.svg.axis().scale(yScale);

	spect
		.attr("width", WIDTH + MARGINS.right + MARGINS.left)
		.attr("height", HEIGHT + MARGINS.top + MARGINS.bottom);
		
	
	
	xAxis = d3.svg.axis("xaxis")
		.scale(xScale)
		.tickValues(function(d) { if(band == "2.4GHz"){return gband[1];}else{return aband[1];} })
		.tickFormat(function(d) { if(band == "2.4GHz"){a = gband[1].indexOf(d); return gband[0][a];}else{a = aband[1].indexOf(d); return aband[0][a];} }),
  
	yAxis = d3.svg.axis("yaxis")
		.scale(yScale)
		.orient("left");		//create the two axes
		
	if(band == "2.4GHz")
	{
		spect.append("svg:g")
		.attr("class","axis")
		.attr("transform", "translate(0," + (HEIGHT - LEGENDOFFSET - MARGINS.bottom) + ")")
		.call(xAxis);
	}
	else		//because channel numbers are bigger here, we need to rotate the labels or we run out of room.
	{
		spect.append("svg:g")
		.attr("class","axis")
		.attr("transform", "translate(0," + (HEIGHT - LEGENDOFFSET - MARGINS.bottom) + ")")
		.call(xAxis)
		.selectAll("text")	
            .style("text-anchor", "end")
            .attr("dx", "-.8em")
            .attr("dy", "-.3em")
			.attr("transform", "rotate(-90)");
	}
	spect.append("svg:g")
		.attr("class","axis")
		.attr("transform", "translate(" + (MARGINS.left) + "," + (-LEGENDOFFSET) + ")")
		.call(yAxis);		//draw both axes
		
	var lineGen = d3.svg.line()
		.x(function(d) { return xScale(d.freq); })
		.y(function(d) { return yScale(d.level); });				//create a line generation function
		
	dataGroup.forEach(function(d, i) {
		//randcolour = "hsl(" + Math.random() * 360 + ",100%,50%)";	//doesn't guarantee different colours but it is an easy implementation
		randcolour = "hsl(" + 60 * i + ",100%,50%)";	//guarantees at least 6 different colours. then we repeat. looks better.
		legendmarkersize = 10;
		
		spect.append('svg:path')
			.attr('class', 'gline')
			.attr('id', 'tag'+d.key.replace(/\s+/g, ''))	//unique id for each line based on the ssid
			.attr('d', lineGen(d.values))
			.attr('stroke', randcolour)
			.attr('stroke-width', 2)
			.attr('fill', 'none')
			.attr("transform", "translate(0," + (-LEGENDOFFSET) + ")");			//draw each line
		
		spect.append('rect')
			.attr('width', legendmarkersize)
			.attr('height', legendmarkersize)
			.attr("x", (lSpace / 2) + i * lSpace)		//evenly space little squares based on number of data points
			.attr("y", HEIGHT-legendmarkersize-LEGENDOFFSET)
			.style('fill', randcolour)				//fill them the same colour as the line
			.style('stroke', randcolour)
			.on("mouseover", function() {
				var existingcolour = d3.select("#tag"+d.key.replace(/\s+/g, ''))
					.style("stroke");
				d3.select("#tag"+d.key.replace(/\s+/g, ''))
					.style("fill", existingcolour);
			})
			.on("mouseout", function() {
                d3.select("#tag"+d.key.replace(/\s+/g, ''))
					.style("fill", "none")
					.style("opacity", 1);
            });		//on mouseover, grab the colour of the line we are representing, and fill in the area below it. when we mouseout, set it back to normal
			
		spect.append("text")
			.style("fill", "black")
			.style("text-anchor", "end")
			.attr("transform", "translate(" + ((lSpace / 2) + i * lSpace + (legendmarkersize / 2)) + "," + (HEIGHT-LEGENDOFFSET+legendmarkersize) + ") rotate(-65)")
			.text(d.key);	//write the text under the coloured rectangles. looks pretty. going to be a problem if someone uses an SSID of length 32 though.
	});
	
}
