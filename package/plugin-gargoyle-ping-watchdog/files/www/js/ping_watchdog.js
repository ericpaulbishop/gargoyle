/*
 * This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var pingS=new Object(); //part of i18n

function saveChanges()
{
	var ping_watchdog_enable = document.getElementById("ping_watchdog_enable").checked;
	var address_to_ping = document.getElementById("address_to_ping").value;
	var ping_interval = document.getElementById("ping_interval").value;
	var startup_delay = document.getElementById("startup_delay").value;
	var failure_count = document.getElementById("failure_count").value;
	var failure_action = getSelectedValue("failure_action");

	var test59 = function(text){ return validateNumericRange(text,1,59); }
	var test999 = function(text){ return validateNumericRange(text,1,999); }
	var test10 = function(text){ return validateNumericRange(text,1,10); }

	var addIds=["address_to_ping","ping_interval","startup_delay","failure_count"];
	var labelIds=["address_to_ping_label","ping_interval_label","startup_delay_label","failure_count_label"];
	var functions = [validateIP, test59, test999, test10];
	var errors = proofreadFields(addIds, labelIds, functions, [0,0,0,0], addIds, document);

	if(failure_action == "custom")
	{
		failure_action = document.getElementById("script").value;
		if(failure_action == "")
		{
			errors.push(pingS.ScptErr);
		}
	}

	if (errors.length > 0)
	{
		errorString = errors.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
		return;
	}

	setControlsEnabled(false, true)
	var commands = [];
	commands.push("mkdir -p /etc/crontabs");
	commands.push("touch /etc/crontabs/root");
	commands.push("cat /etc/crontabs/root | grep -v \"/usr/lib/gargoyle/ping_watchdog.sh\" > /tmp/tmp.cron");
	if (ping_watchdog_enable)
	{
		commands.push("echo \"*/" + ping_interval + " * * * * /usr/lib/gargoyle/ping_watchdog.sh " + startup_delay + " " + failure_count + " " + address_to_ping + " " + failure_action.replace(/"/g,"\\\"")  +" \" >>/tmp/tmp.cron");
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
	var commands = [];
	commands.push("cat /etc/crontabs/root | grep \"/usr/lib/gargoyle/ping_watchdog.sh\"");

	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			data=req.responseText.split(/[\r\n+]/);
			document.getElementById("ping_watchdog_enable").checked = false;
			document.getElementById("address_to_ping").value = "8.8.8.8";
			document.getElementById("ping_interval").value = "3";
			document.getElementById("startup_delay").value = "240";
			document.getElementById("failure_count").value = "3";
			document.getElementById("failure_action").value = "wan";
			for(var i=0;i<data.length;i++)
			if(data[i].match(/ping_watchdog.sh/))
			{
				//0   1 2 3 4 5                                  6   7 8       9 
				//*/3 * * * * /usr/lib/gargoyle/ping_watchdog.sh 240 3 8.8.8.8 reboot
				ping_data = data[i].split(/ /);
				if (ping_data[5] == "/usr/lib/gargoyle/ping_watchdog.sh")
				{
					document.getElementById("ping_watchdog_enable").checked = true;
					document.getElementById("ping_interval").value = ping_data[0].replace("*/","");
					document.getElementById("startup_delay").value = ping_data[6];
					document.getElementById("failure_count").value = ping_data[7];
					document.getElementById("address_to_ping").value = ping_data[8]
					if(ping_data[9] == "wan" || ping_data[9] == "reboot")
					{
						document.getElementById("failure_action").value = ping_data[9];
					}
					else
					{
						document.getElementById("failure_action").value = "custom";
						var script = "";
						for(i=9;i<ping_data.length;i++) {script = script + ping_data[i] + " ";}
						document.getElementById("script").value = script;
					}
					showScript(ping_data[9]);
				}
			}
			unlockFields();
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function unlockFields()
{
	ids=["address_to_ping", "ping_interval","startup_delay","failure_count","failure_action","script"];
	var ping_watchdog_enable = document.getElementById("ping_watchdog_enable").checked;
	for (idx=0;idx<ids.length;idx++)
	{
		setElementEnabled(document.getElementById(ids[idx]), ping_watchdog_enable, document.getElementById(ids[idx]).value);
	}
}

function showScript(action)
{
	var show=action == "wan" || action == "reboot";
	setVisibility(["custom_script"],[!show]);
}
