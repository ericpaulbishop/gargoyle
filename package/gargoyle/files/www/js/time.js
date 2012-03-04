/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var previousTimezoneDefinition = "PST8PDT,M3.2.0/2,M11.1.0/2";


function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\nChanges could not be applied.";
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		
		sectionDeleteCommands = [];
		ntpServerSections = uciOriginal.getAllSectionsOfType("ntpclient", "ntpserver");
		for(sectionIndex=0; sectionIndex < ntpServerSections.length; sectionIndex++)
		{
			sectionDeleteCommands.push("uci del ntpclient." + ntpServerSections[sectionIndex]);
			uciOriginal.removeSection("ntpclient", ntpServerSections[sectionIndex]);
		}
		sectionDeleteCommands.push("uci commit");

		uci = uciOriginal.clone();	
		for(sectionIndex=0; sectionIndex < 3; sectionIndex++)
		{
			serverName = document.getElementById("server" + (sectionIndex+1)).value;
			if(serverName != "")
			{
				sectionName = "cfg" + (sectionIndex+1);
				uci.set("ntpclient", sectionName, "", "ntpserver");
				uci.set("ntpclient", sectionName, "port", "123");
				uci.set("ntpclient", sectionName, "hostname", document.getElementById("server" + (sectionIndex+1)).value);
			}
		}
	
		//update timezone and update frequency
		var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
		var systemOptions = uciOriginal.getAllOptionsInSection("system", systemSections[0]);
		var ntpClientSections = uciOriginal.getAllSectionsOfType("ntpclient", "ntpclient");
		uci.set("ntpclient", ntpClientSections[0], "interval", getSelectedValue("update_frequency"));

		//update date format
		var systemDateFormat = getSelectedValue("date_format"); 
		uci.set("gargoyle", "global", "dateformat", getSelectedValue("date_format"));

		//update command to output date
		var formatStrings=[];
		formatStrings["iso"]       = "\"+%Y/%m/%d %H:%M %Z\"";
		formatStrings["iso8601"]   = "\"+%Y-%m-%d %H:%M %Z\"";
		formatStrings["australia"] = "\"+%d/%m/%y %H:%M %Z\"";
		formatStrings["usa"]       = "\"+%m/%d/%y %H:%M %Z\"";
		formatStrings["russia"]    = "\"+%d.%m.%y %H:%M %Z\"";
		var outputDateCommand = "";
		if(getSelectedValue("timezone").match(/UTC/))
		{
			outputDateCommand ="date " + formatStrings[systemDateFormat] + " | sed 's/UTC/UTC-" + getSelectedValue("timezone").replace(/UTC/, "") + "/g' | sed 's/\\-\\-/+/g'";
		}
		else
		{
			outputDateCommand ="date " + formatStrings[systemDateFormat];
		}
		//copy old system section to new one with specific name
		var systemCommands = [];
		uci.set("system", systemSections[0], "timezone", getSelectedValue("timezone"));
		if(systemSections[0] != "system")
		{
			systemCommands.push("uci del system." + systemSections[0] );
			systemCommands.push("uci commit");
			systemCommands.push("uci set system.system=system");
			systemCommands.push("uci commit");
		}	
		uci.set("system", "system", "", "system");
		var sysIndex=0;
		for(sysIndex=0; sysIndex < systemOptions.length; sysIndex++)
		{
			uci.set("system", "system", systemOptions[sysIndex], uci.get("system", systemSections[0], systemOptions[sysIndex]));
		}
		if(systemSections[0] != "system")
		{
			uciOriginal.removeSection("system", systemSections[0]);
			uci.removeSection("system", systemSections[0]);
		}
		var setTimezoneCommand = "uci show system | grep timezone | sed 's/^.*=//g' >/etc/TZ\n";
		



		commands = sectionDeleteCommands.join("\n") + "\n" + systemCommands.join("\n") + "\n" + uci.getScriptCommands(uciOriginal) + "\n" + setTimezoneCommand + "\n" + "ACTION=ifup /etc/hotplug.d/iface/20-ntpclient\n/usr/bin/set_kernel_timezone\n" +  outputDateCommand;
		//document.getElementById("output").value = commands;	

		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var responseText=req.responseText.split(/[\r\n]+/);
				if(responseText != null)
				{
					while(responseText.length > 0 && (!responseText[ responseText.length-1].match(/:/)))
					{
						responseText.pop();
					}
					if(responseText.length > 0)
					{
						currentTime=responseText[ responseText.length-1];
					}
				}
				
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);

				//alert(req.responseText);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{

	return "";
}

function resetData()
{
	setChildText("current_time", currentTime);
	
	timezoneList = timezoneData[0];
	timezoneDefinitions = timezoneData[2];


	removeAllOptionsFromSelectElement(document.getElementById("timezone"));
	for(tzIndex = 0; tzIndex < timezoneList.length; tzIndex++)
	{
		timezone = timezoneList[tzIndex];
		addOptionToSelectElement("timezone", timezone, timezoneDefinitions[timezone]);
	}

	var systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	var currentTimezone = uciOriginal.get("system", systemSections[0], "timezone");
	currentTimezone = currentTimezone == "UTC" ? "UTC0" : currentTimezone;
	setSelectedValue("timezone", previousTimezoneDefinition); //set default value
	setSelectedValue("timezone", currentTimezone); //set value from config
	previousTimezoneDefinition = currentTimezone;


	var ntpClientSections = uciOriginal.getAllSectionsOfType("ntpclient", "ntpclient");
	var updateFrequency = uciOriginal.get("ntpclient", ntpClientSections[0], "interval");
	var systemDateFormat = uciOriginal.get("gargoyle",  "global", "dateformat");
	setSelectedValue("update_frequency", "43200"); //set default value
	setSelectedValue("update_frequency", updateFrequency); //set value loaded from config
	setSelectedValue("date_format", "usa"); //set default value for date
	setSelectedValue("date_format", systemDateFormat); //set value loaded value from config


	var ntpServerSections = uciOriginal.getAllSectionsOfType("ntpclient", "ntpserver");
	var tzServers = [];
	for(sectionIndex=0; sectionIndex < ntpServerSections.length; sectionIndex++)
	{
		server = uciOriginal.get("ntpclient", ntpServerSections[sectionIndex], "hostname");
		if(server != "" && sectionIndex < 3)
		{
			tzServers.push(server );
			document.getElementById("server" + (1+sectionIndex)).value = server;
		}
	}

	currentRegion = "custom";
	if(tzServers.length >= 3)
	{
		testRegion = "";
		validRegion = true;
		for(serverIndex=0; serverIndex < 3 && validRegion; serverIndex++)
		{
			validRegion = false;
			nextRegion = tzServers[serverIndex].match(/[0-9]+\.([^\.]+)\.pool\.ntp\.org/);
			if(nextRegion)
			{
				if(nextRegion[1] == testRegion || testRegion == "")
				{
					testRegion = nextRegion[1];
					validRegion = true;
				}
			}
		}
		currentRegion = validRegion ? testRegion : currentRegion;
	}
	else
	{
		for(serverIndex=tzServers.length; serverIndex < 3; serverIndex++)
		{
			document.getElementById("server" + (1+sectionIndex)).value = (2-serverIndex) + ".pool.ntp.org"
		}
	}
	
	setSelectedValue("region", "custom"); // set default value
	setSelectedValue("region", currentRegion); //set value from config

	updateServerList();

}

function timezoneChanged()
{
	timezoneRegions = timezoneData[1];
	definitionTimezones = timezoneData[3];

	newTimezoneDefinition = getSelectedValue("timezone");
	if( getSelectedValue("region") == timezoneRegions[ definitionTimezones[previousTimezoneDefinition] ])
	{
		setSelectedValue("region", timezoneRegions[ definitionTimezones[newTimezoneDefinition] ]);
		updateServerList();
	}

	previousTimezoneDefinition = newTimezoneDefinition;
}

function updateServerList()
{
	region=getSelectedValue("region");
	for(serverIndex=1; serverIndex <= 3; serverIndex++)
	{
		if(region == "custom")
		{
			setElementEnabled( document.getElementById("server" + serverIndex), true, document.getElementById("server" + serverIndex).value);
		}
		else
		{
			setElementEnabled( document.getElementById("server" + serverIndex), false, (3-serverIndex) + "." + region + ".pool.ntp.org");	
		}
	}	
}

