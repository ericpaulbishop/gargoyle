/*
 * This program is copyright © 2008,2009-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var rbS = new Object(); //part of i18n

var toggleReload = false;

function reboot()
{
	setControlsEnabled(false, true, rbS.SysR);
	
	var commands = "\nsh /usr/lib/gargoyle/reboot.sh\n";
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

	//test for router coming back up
	currentProtocol = location.href.match(/^https:/) ? "https" : "http";
	testLocation = currentProtocol + "://" + window.location.host + "/utility/reboot_test.sh";
	testReboot = function()
	{
		toggleReload = true;
		setTimeout( "testReboot()", 5*1000);  //try again after 5 seconds
		document.getElementById("reboot_test").src = testLocation; 
	}
	setTimeout( "testReboot()", 25*1000);  //start testing after 15 seconds
	setTimeout( "reloadPage()", 240*1000); //after 4 minutes, try to reload anyway

}

function reloadPage()
{
	if(toggleReload)
	{
		//IE calls onload even when page isn't loaded -- it just times out and calls it anyway
		//We can test if it's loaded for real by looking at the (IE only) readyState property
		//For Browsers NOT designed by dysfunctional cretins whose mothers were a pack of sewer-dwelling, shit-eating rodents,
		//well, for THOSE browsers, readyState (and therefore reloadState) should be null 
		var reloadState = document.getElementById("reboot_test").readyState;
		if( typeof(reloadState) == "undefined" || reloadState == null || reloadState == "complete")
		{
			toggleReload = false;
			document.getElementById("reboot_test").src = "";
			window.location.href = window.location.href;
		}
	}
}


function saveChanges()
{
	setControlsEnabled(false, true)
	if(getSelectedValue("sched_reboot") != "none")
	{
		var rebootInterval = getSelectedValue("reboot_interval");
		var day = getSelectedValue("reboot_day");
		
		var hour = getSelectedValue("reboot_hour");
		var weekday = rebootInterval == "week" ? day : "*";
		var monthday = rebootInterval == "month" ? day : "*";

		cronLine = "0 " + hour + " " + monthday + " * " + weekday + " sh /usr/lib/gargoyle/reboot.sh"
	}
	else
	{
		cronLine = "";
	}

	var commands = [];
	commands.push("mkdir -p /etc/crontabs");
	commands.push("touch /etc/crontabs/root");
	commands.push("cat /etc/crontabs/root | grep -v \"usr\\/lib\\/gargoyle\\/reboot.sh\" > /tmp/tmp.cron");
	if(cronLine != "")
	{
		commands.push("echo \'" + cronLine + "\' >>/tmp/tmp.cron");
	}
	commands.push("mv /tmp/tmp.cron /etc/crontabs/root");
	commands.push("/etc/init.d/cron restart");

	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4){ setControlsEnabled(true); }
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);

}


function resetData()
{
	var cronParts = cronLine.split(/[\t ]+/);
	if(cronParts.length > 4)
	{
		var hour     = parseInt(cronParts[1]) + "";
		var weekday  = parseInt(cronParts[4]) + "";
		var monthday = parseInt(cronParts[2]) + "";
		var day = "";
		
		hour     = hour == "NaN" ? "0" : hour;
		weekday  = weekday  == "NaN" ? "*" : weekday;
		monthday = monthday == "NaN" ? "*" : monthday;
		day = weekday  != "*" ? weekday  : day;
		day = monthday != "*" ? monthday : day;
		
		var rebootInterval = weekday == "*" && monthday == "*" ? "hour" : "";
		rebootInterval = weekday != "*" && monthday == "*" ? "week" : rebootInterval;
		rebootInterval = weekday == "*" && monthday != "*" ? "month" : rebootInterval;

		setSelectedValue("sched_reboot", "scheduled");
		setSelectedValue("reboot_interval", rebootInterval);
		setVisibility();
		setSelectedValue("reboot_hour", hour);
		setSelectedValue("reboot_day", day);
	}
	else
	{
		setSelectedValue("sched_reboot", "none");
		setVisibility();
	}
	
}
function setVisibility()
{

	var rebootInterval = getSelectedValue("reboot_interval");
	if(rebootInterval == "month")
	{
		var vals = [];	
		var names = [];	
		var day=1;
		for(day=1; day <= 28; day++)
		{
			var dayStr = "" + day;
			var lastDigit = dayStr.substr( dayStr.length-1, 1);
			var suffix=rbS.Digs
			if( day % 100  != 11 && lastDigit == "1")
			{
				suffix=rbS.LD1s
			}
			if( day % 100 != 12 && lastDigit == "2")
			{
				suffix=rbS.LD2s
			}
			if( day %100 != 13 && lastDigit == "3")
			{
				suffix=rbS.LD3s
			}
			names.push(dayStr + suffix);
			vals.push( (day-1) + "" );
		}
		setAllowableSelections("reboot_day", vals, names);
	}
	else if(rebootInterval == "week")
	{
		var names = rbS.DaysWArr;
		var vals = [];
		var dayIndex;
		for(dayIndex=0; dayIndex < 7; dayIndex++)
		{
			vals.push( dayIndex + "")
		}
		setAllowableSelections("reboot_day", vals, names);
	}
	setInvisibleIfIdMatches("reboot_interval", ["day"], "reboot_day_container");
	setInvisibleIfIdMatches("sched_reboot", ["none"], "schedule_reboot_container");
}

