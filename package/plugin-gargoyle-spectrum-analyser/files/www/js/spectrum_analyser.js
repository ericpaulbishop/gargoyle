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

var bands = {
	'2g': {
		'channels': {
			1: 2.412, 2: 2.417, 3: 2.422, 4: 2.427, 5: 2.432, 6: 2.437, 7: 2.442, 8: 2.447, 9: 2.452, 10: 2.457, 11: 2.462, 12: 2.467, 13: 2.472, 14:2.484,
		},
		'chanwidth': 0.022,
	},
	'5g': {
		'channels': {
			36: 5.18, 40: 5.2, 44: 5.22, 48: 5.24, 52: 5.26, 56: 5.28, 60: 5.3, 64: 5.32, 100: 5.5, 104: 5.52, 108: 5.54, 112: 5.56, 116: 5.58, 120: 5.6, 124: 5.62, 128: 5.64, 132: 5.66, 136: 5.68, 140: 5.7, 144: 5.72, 149: 5.745, 153: 5.765, 157: 5.785, 161: 5.805, 165: 5.825,
		},
		'chanwidth': 0.020,
	},
	'6g': {
		'channels': {
			1: 5.955, 5: 5.975, 9: 5.995, 13: 6.015, 17: 6.035, 21: 6.055, 25: 6.075, 29: 6.095, 33: 6.115, 37: 6.135, 41: 6.155, 45: 6.175, 49: 6.195, 53: 6.215, 57: 6.235, 61: 6.255, 65: 6.275, 69: 6.295, 73: 6.315, 77: 6.335, 81: 6.355, 85: 6.375, 89: 6.395, 93: 6.415, 97: 6.435, 101: 6.455, 105: 6.475, 109: 6.495, 113: 6.515, 117: 6.535, 121: 6.555, 125: 6.575, 129: 6.595, 133: 6.615, 137: 6.635, 141: 6.655, 145: 6.675, 149: 6.695, 153: 6.715, 157: 6.735, 161: 6.755, 165: 6.775, 169: 6.795, 173: 6.815, 177: 6.835, 181: 6.855, 185: 6.875, 189: 6.895, 193: 6.915, 197: 6.935, 201: 6.955, 205: 6.975, 209: 6.995, 213: 7.015, 217: 7.035, 221: 7.055, 225: 7.075, 229: 7.095, 233: 7.115,
		},
		'chanwidth': 0.020,
	},
};

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
	else
	{
		document.getElementById("interface").disabled = true;	//do nothing and disable interface
	}
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
	else if(band == "5GHz")
	{
		freq_low = 5.165;	//technically 5.170, but 5.165 graphs better
		freq_high = 5.840;	//technically 5.835, but 5.840 graphs better
	}
	else if(band == "6GHz")
	{
		freq_low = 5.940;	//technically 5.945, but 5.940 graphs better
		freq_high = 7.130;	//technically 7.125, but 7.130 graphs better
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

	//lines is of the format:	radioX ##; radioY ##; etc etc.
	for(lineIndex=0; lineIndex < lines.length; lineIndex++)
	{
		var nextLine = lines[lineIndex];
		var wlan = nextLine.split(" ");

		var guest  = wlan[0].indexOf("-ap1");
		if(guest != -1)
		{
			continue;
		}

		var interfaceid = wlan[0];
		if(wlan[1] == '6g')
		{
			var interfaceband = "6GHz";
		}
		else if(wlan[1] == '5g')
		{
			var interfaceband = "5GHz";
		}
		else if(wlan[1] == '2g')
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
	Commands.push("iwinfo " + selectedband + " scan 2>&1 | awk -F'\\\n' '{print \"\\\"\"$0\"\\\"\" }'");

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
				document.getElementById("table-container-row").style.display="none";
				plotdata = [];
				window.removeEventListener("resize", plotall);//plotall(parsedWifiData);
			}
			else
			{
				generateGraphData(parsedWifiData);
				generateTableData(parsedWifiData);
			}
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function parseWifiData(rawScanOutput)
{
	if((rawScanOutput != null) && (rawScanOutput.indexOf("\n") != 0) && (rawScanOutput.indexOf("\r") != 0) && (rawScanOutput.match(/command failed: Resource busy/g) == null))
	{
		var parsed = [ [],[],[],[],[],[],[],[],[],[],[],[] ];
		var cells = rawScanOutput.split(/Cell [0-9]+ - /g);
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
				if(id == "Address")
				{
					var pat = /([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})/g;
					if (pat.test(line) == true)
					{
						splitIndex = line.lastIndexOf(":");
						var val=line.substr(0, splitIndex+3);
						//val = val.replace(/:/g,"");	removes colons from mac address, might be useful later
						val = val.replace(/ /g,"");
						val = val.replace(/BSS/g,"");
						vals.push(val);
					}
				}
			}
			return vals;
		}

		var splitDoubleSpace = function(cellLines)
		{
			var splitIDs = [
				['Mode','Frequency','Band','Channel'],
				['Signal','Quality']
			];

			for(var x = 0; x < cellLines.length; x++)
			{
				var cellLine = cellLines[x];
				var lineMatch = splitIDs.some(function(splitID) {
					return splitID.every(function(split) {
						return cellLine.indexOf(split) > -1;
					});
				});
				if(lineMatch)
				{
					var newCellLines = cellLine.trim().split('  ');
					newCellLines.forEach(function(newCellLine,idx) {
						if(idx == 0)
						{
							cellLines.splice(x,1,newCellLine);
						}
						else
						{
							cellLines.splice(x+idx,0,newCellLine);
						}
					});
				}
			}

			return cellLines;
		}

		while(cells.length > 0)
		{
			var cellData  = cells.shift();
			var cellLines = cellData.split(/[\r\n]+/);
			cellLines = splitDoubleSpace(cellLines);
			var htmode = 0;
			var vhtmode = 0;
			var hemode = 0;

			var bssid = getCellValues("Address", cellLines).shift();
			var ssid = getCellValues("ESSID", cellLines).shift();
			var sigStr = getCellValues("Signal", cellLines).shift();
			var encryption = getCellValues("Encryption", cellLines).shift();
			if(getCellValues(" HT Operation", cellLines).length > 0)
			{
				//we are in High Throughput
				var prichannel = getCellValues("Primary Channel", cellLines).shift();
				var secchannel = getCellValues("Secondary Channel Offset", cellLines).shift();
				htmode = 1;
			}
			if(prichannel == undefined || prichannel == 0)
			{
				//if we are not in HT, we can interpret the freq to get the primary channel
				var prichannel = getCellValues("Channel", cellLines).shift();
			}
			if(getCellValues("VHT Operation", cellLines).length > 0)
			{
				var vhtwidth = getCellValues("Channel Width", cellLines).shift();
				var vhtseg1 = getCellValues("Center Frequency 1", cellLines).shift();
				var vhtseg2 = getCellValues("Center Frequency 2", cellLines).shift();
				vhtmode = 1;
			}
			if(getCellValues("HE Operation", cellLines).length > 0 || band == "6GHz")
			{
				// TODO
				hemode = 1;
			}

			if (! secchannel)
			{
				secchannel = "no secondary";	//if we don't get a result for the secondary channel, set it to this so we don't error
			}
			//if no encryption, set to "None"
			if (! encryption)
			{
				encryption = "none";
			}

			if(ssid != null && prichannel != null && secchannel != null && sigStr != null && bssid != null ) 
			{
				parsed[0].push(ssid);
				parsed[1].push(prichannel);
				parsed[2].push(secchannel);
				parsed[3].push(sigStr);
				parsed[4].push(vhtwidth);
				parsed[5].push(vhtseg1);
				parsed[6].push(vhtseg2);
				parsed[7].push(bssid);
				parsed[8].push(htmode);
				parsed[9].push(vhtmode);
				parsed[10].push(hemode);
				parsed[11].push(encryption);
			}
		}
		//check for duplicate bssid data and append _# if necessary
		//now that we are working with bssid's, i don't expect many duplicates, but we should be careful anyway
		for(x = 0; x < parsed[7].length; x++)
		{
			append = 2;
			for(y = (x+1); y < parsed[7].length; y++)
			{
				if((parsed[7][x] == parsed[7][y])/* && (parsed[4][x] == parsed[4][y])*/)	//second part only necessary if we end up scanning both spectrum at once
				{
					parsed[7][y] = parsed[7][y]+"_"+append;
					append += 1;
				}
			}
			if(append > 2)
			{
				parsed[7][x] = parsed[7][x]+"_1";
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
	plotdata = [];
	var freqoffset = 0.001;	//for making the graphs look pretty
	var htoffset = 4;	//for 802.11n channels
	var vhtoffset = 12;	//for 802.11ac channels, double this for vht160
	var x, y = 0;
	for(x = 0; x < detected[0].length; x++)
	{
		var SSID = detected[0][x];
		var channel = detected[1][x];
		var secondary = detected[2][x];
		var level = detected[3][x];
		var vhtwidth = detected[4][x];
		var vhtseg1 = detected[5][x];
		var vhtseg2 = (detected[6][x] == 0 ? null:detected[6][x]);
		var BSSID = detected[7][x].replace(/:/g,"");
		
		if(band == "2.4GHz")
		{
			if(secondary == "no secondary")					//20mhz
			{
				plotdata[x*4] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',channel,'low')};
				plotdata[x*4+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',channel,'low')+freqoffset)};
				plotdata[x*4+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',channel,'high')-freqoffset)};
				plotdata[x*4+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',channel,'high')};
			}
			else if(secondary == "above")						//40mhz
			{
				plotdata[x*4] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',channel,'low')};
				plotdata[x*4+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',channel,'low')+freqoffset)};
				plotdata[x*4+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',(channel-(-htoffset)),'high')-freqoffset)};
				plotdata[x*4+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',(channel-(-htoffset)),'high')};
			}
			else if(secondary == "below")						//40mhz
			{
				plotdata[x*4] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',(channel-htoffset),'low')};
				plotdata[x*4+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',(channel-htoffset),'low')+freqoffset)};
				plotdata[x*4+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData('2g',channel,'high')-freqoffset)};
				plotdata[x*4+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData('2g',channel,'high')};
			}
		}
		else if(band == "5GHz" || band == "6GHz")
		{
			var bandcode = band == "5GHz" ? "5g" : "6g";
			y = plotdata.length;	//set the loop variable to the next available point in the array
			if(secondary == "no secondary")					//20mhz
			{
				plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,channel,'low')};
				plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,channel,'low')+freqoffset)};
				plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,channel,'high')-freqoffset)};
				plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,channel,'high')};
			}
			else if(secondary == "above")
			{
				if(! vhtwidth)								//40mhz
				{
					plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,channel,'low')};
					plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,channel,'low')+freqoffset)};
					plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(channel-(-htoffset)),'high')-freqoffset)};
					plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(channel-(-htoffset)),'high')};
				}
				else
				{
					if(! vhtseg2)							//80mhz
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg1-(0.5*vhtoffset),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg1-(0.5*vhtoffset),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg1-(-0.5*vhtoffset)),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg1-(-0.5*vhtoffset)),'high')};
					}
					else if(Math.abs(vhtseg1 - vhtseg2) == 8)	//160mhz
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg2-(2+vhtoffset),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg2-(2+vhtoffset),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg2-(-2-vhtoffset),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg2-(-2-vhtoffset),'high')};
					}
					else									//80+80mhz
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg1-(0.5*vhtoffset)),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg1-(0.5*vhtoffset)),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,((vhtseg1-(0.5*vhtoffset))-(-vhtoffset)),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,((vhtseg1-(0.5*vhtoffset))-(-vhtoffset)),'high')};
						plotdata[y+4] = {"bssid":BSSID, "ssid":SSID, "level":undefined, "freq":getfreqData(bandcode,vhtseg1,'high')};		//undefined point anywhere (yes, even on the moon) to disjoint the two parts of the graph
						plotdata[y+5] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg2-(0.5*vhtoffset)),'low')};
						plotdata[y+6] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg2-(0.5*vhtoffset)),'low')+freqoffset)};
						plotdata[y+7] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,((vhtseg2-(0.5*vhtoffset))-(-vhtoffset)),'high')-freqoffset)};
						plotdata[y+8] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,((vhtseg2-(0.5*vhtoffset))-(-vhtoffset)),'high')};
					}
				}
			}
			else if(secondary == "below")	//need to handle vht here as well once i do some testing
			{
				if(! vhtwidth)								//40mhz
				{
					plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(channel-htoffset),'low')};
					plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(channel-htoffset),'low')+freqoffset)};
					plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,channel,'high')-freqoffset)};
					plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,channel,'high')};
				}
				else
				{
					if(! vhtseg2)							//80mhz
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg1-(0.5*vhtoffset),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg1-(0.5*vhtoffset),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg1-(-0.5*vhtoffset)),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg1-(-0.5*vhtoffset)),'high')};
					}
					else if(Math.abs(vhtseg1 - vhtseg2) == 8)	//160mhz
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg2-(2+vhtoffset),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg2-(2+vhtoffset),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,vhtseg2-(-2-vhtoffset),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,vhtseg2-(-2-vhtoffset),'high')};
					}
					else									//80+80mhz. i believe this is the same above or below
					{
						plotdata[y] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg1-(0.5*vhtoffset)),'low')};
						plotdata[y+1] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg1-(0.5*vhtoffset)),'low')+freqoffset)};
						plotdata[y+2] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,((vhtseg1-(0.5*vhtoffset))-(-vhtoffset)),'high')-freqoffset)};
						plotdata[y+3] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,((vhtseg1-(0.5*vhtoffset))-(-vhtoffset)),'high')};
						plotdata[y+4] = {"bssid":BSSID, "ssid":SSID, "level":undefined, "freq":getfreqData(bandcode,vhtseg1,'high')};		//undefined point anywhere (yes, even on the moon) to disjoint the two parts of the graph
						plotdata[y+5] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,(vhtseg2-(0.5*vhtoffset)),'low')};
						plotdata[y+6] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,(vhtseg2-(0.5*vhtoffset)),'low')+freqoffset)};
						plotdata[y+7] = {"bssid":BSSID, "ssid":SSID, "level":level, "freq":(getfreqData(bandcode,((vhtseg2-(0.5*vhtoffset))-(-vhtoffset)),'high')-freqoffset)};
						plotdata[y+8] = {"bssid":BSSID, "ssid":SSID, "level":"-100", "freq":getfreqData(bandcode,((vhtseg2-(0.5*vhtoffset))-(-vhtoffset)),'high')};
					}
				}
			}
		}
		else
		{
			//something is wroooooooooooooooooong
			plotdata = -1;
		}
	}
	
	plotall();
	window.addEventListener('resize', plotall);
}

function generateTableData(detected)
{
	var x = 0;
	var htoffset = 4;	//for 802.11n channels
	var vhtoffset = 12;	//for 802.11ac channels, double this for vht160
	var columnNames = [ spectrum.SSID, spectrum.BSSID, spectrum.Channel, spectrum.Width, spectrum.Mode, spectrum.Encryption, spectrum.Signal];
	var TableData = new Array();
	
	for(x = 0; x < detected[0].length; x++)
	{
		var SSID = detected[0][x];
		var channel = detected[1][x];
		var secondary = detected[2][x];
		var level = detected[3][x];
		var vhtwidth = detected[4][x];
		var vhtseg1 = detected[5][x];
		var vhtseg2 = (detected[6][x] == 0 ? null:detected[6][x]);
		var BSSID = detected[7][x].toUpperCase();
		var htmode = detected[8][x];
		var vhtmode = detected[9][x];
		var hemode = detected[10][x];
		var encryption = detected[11][x];
		
		if(band == "2.4GHz")
		{
			if(secondary == "no secondary")					//20mhz
			{
				TableData.push([ SSID, BSSID, channel, "20MHz", ((htmode)?((hemode)?"g,n,ax":"b,g,n"):"b,g"), encryption, level+"dBm" ]);
			}
			else if(secondary == "above")						//40mhz
			{
				TableData.push([ SSID, BSSID, channel+"-"+(channel-(-htoffset)), "40MHz", (hemode)?"g,n,ax":"b,g,n", encryption, level+"dBm" ]);
			}
			else if(secondary == "below")						//40mhz
			{
				TableData.push([ SSID, BSSID, channel+"-"+(channel-htoffset), "40MHz", (hemode)?"g,n,ax":"b,g,n", encryption, level+"dBm" ]);
			}
		}
		else if(band == "5GHz" || band == "6GHz")
		{
			if(secondary == "no secondary")					//20mhz
			{
				var modestr = band == "6GHz" ? "ax" : ((htmode)?((vhtmode)?((hemode)?"n,ac,ax":"a,n,ac"):"a,n"):"a");
				TableData.push([ SSID, BSSID, channel, "20MHz", modestr, encryption, level+"dBm" ]);
			}
			else if(secondary == "above")
			{
				if(! vhtwidth)								//40mhz
				{
					var modestr = band == "6GHz" ? "ax" : ((vhtmode)?((hemode)?"n,ac,ax":"a,n,ac"):"a,n");
					TableData.push([ SSID, BSSID, channel+"-"+(channel-(-htoffset)), "40MHz", modestr, encryption, level+"dBm" ]);
				}
				else
				{
					var modestr = band == "6GHz" ? "ax" : ((hemode)?"n,ac,ax":"n,ac");
					if(! vhtseg2)							//80mhz
					{
						TableData.push([ SSID, BSSID, (vhtseg1-(0.5*vhtoffset))+"-"+(vhtseg1-(-0.5*vhtoffset)), "80MHz", modestr, encryption, level+"dBm" ]);
					}
					else if(Math.abs(vhtseg1 - vhtseg2) == 8)	//160mhz
					{
						TableData.push([ SSID, BSSID, (vhtseg2-(2+vhtoffset))+"-"+(vhtseg2-(-2-vhtoffset)), "160MHz", modestr, encryption, level+"dBm" ]);
					}
					else									//80+80mhz
					{
						TableData.push([ SSID, BSSID, ((vhtseg1-(0.5*vhtoffset))+"-"+(vhtseg1-(-0.5*vhtoffset)))+","+((vhtseg2-(0.5*vhtoffset))+"-"+(vhtseg2-(-0.5*vhtoffset))), "80+80MHz", modestr, encryption, level+"dBm" ]);
					}
				}
			}
			else if(secondary == "below")	//need to handle vht here as well once i do some testing
			{
				if(! vhtwidth)								//40mhz
				{
					TableData.push([ SSID, BSSID, channel+"-"+(channel-htoffset), "40MHz", ((vhtmode)?((hemode)?"n,ac,ax":"a,n,ac"):"a,n"), encryption, level+"dBm" ]);
				}
				else
				{
					var modestr = band == "6GHz" ? "ax" : ((hemode)?"n,ac,ax":"n,ac");
					if(! vhtseg2)							//80mhz
					{
						TableData.push([ SSID, BSSID, (vhtseg1-(0.5*vhtoffset))+"-"+(vhtseg1-(-0.5*vhtoffset)), "80MHz", modestr, encryption, level+"dBm" ]);
					}
					else if(Math.abs(vhtseg1 - vhtseg2) == 8)	//160mhz
					{
						TableData.push([ SSID, BSSID, (vhtseg2-(2+vhtoffset))+"-"+(vhtseg2-(-2-vhtoffset)), "160MHz", modestr, encryption, level+"dBm" ]);
					}
					else									//80+80mhz. i believe this is the same above or below
					{
						TableData.push([ SSID, BSSID, ((vhtseg1-(0.5*vhtoffset))+"-"+(vhtseg1-(-0.5*vhtoffset)))+","+((vhtseg2-(0.5*vhtoffset))+"-"+(vhtseg2-(-0.5*vhtoffset))), "80+80MHz", modestr, encryption, level+"dBm" ]);
					}
				}
			}
		}
		else
		{
			//something is wroooooooooooooooooong
		}
	}

	var Table = createTable(columnNames, TableData, "spectrum_table", false, false);
	var tableContainer = document.getElementById('spectrum_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(Table);
	document.getElementById("table-container-row").style.display="block";
}

function getfreqData(bandcode,channel,info)
{
	//info = 1, return centrefreq	info = 2, return lowfreq	info = 3, return highfreq
	var banddata = bands[bandcode];
	var centrefreq = banddata.channels[channel];
	var chanwidth = banddata.chanwidth;
	if(info == 'centre')
	{
		return centrefreq;
	}
	else if(info == 'low')
	{
		return centrefreq - chanwidth / 2;
	}
	else if(info == 'high')
	{
		return centrefreq + chanwidth / 2;
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
		.key(function(d) { return d.bssid; })
		.entries(plotdata);				//break the data up into "keys" based on BSSID which _should_ be unique (once i stuff around with them)

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
		.tickValues(function(d) { if(band == "2.4GHz"){return Object.values(bands['2g'].channels);}else if(band == "5GHz"){return Object.values(bands['5g'].channels);}else if(band == "6GHz"){return Object.values(bands['6g'].channels);} })
		.tickFormat(function(d) { if(band == "2.4GHz"){return Object.keys(bands['2g'].channels)[Object.values(bands['2g'].channels).indexOf(d)];}else if(band == "5GHz"){return Object.keys(bands['5g'].channels)[Object.values(bands['5g'].channels).indexOf(d)];}else if(band == "6GHz"){return Object.keys(bands['6g'].channels)[Object.values(bands['6g'].channels).indexOf(d)];} }),
  
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
		.defined(function(d) { return d.level != null && d.level != undefined})		//if the data is undefined or null, don't graph it.
		.x(function(d) { return xScale(d.freq); })
		.y(function(d) { return yScale(d.level); });				//create a line generation function
		
	dataGroup.forEach(function(d, i) {
		//randcolour = "hsl(" + Math.random() * 360 + ",100%,50%)";	//doesn't guarantee different colours but it is an easy implementation
		randcolour = "hsl(" + 60 * i + ",100%,50%)";	//guarantees at least 6 different colours. then we repeat. looks better.
		legendmarkersize = 10;
		
		spect.append('svg:path')
			.attr('class', 'gline')
			.attr('id', 'tag'+d.key.replace(/\s+/g, ''))	//unique id for each line based on the bssid
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
			.text(d.values[0].ssid);	//write the text under the coloured rectangles. looks pretty. going to be a problem if someone uses an SSID of length 32 though.
	});
	
}
