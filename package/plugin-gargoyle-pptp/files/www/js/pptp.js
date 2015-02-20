/*
 * This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

pptpS = new Object();

function resetData()
{
	var server = uciOriginal.get("network", "vpnpptp", "server");
	if (server == "")
	{
		setSelectedValue("pptp_config", "disabled");
	}
	else
	{
		setSelectedValue("pptp_config", "client");
		document.getElementById("pptp_server").value = server;
		document.getElementById("pptp_username").value = uciOriginal.get("network", "vpnpptp", "username");
		document.getElementById("pptp_password").value = uciOriginal.get("network", "vpnpptp", "password");

		document.getElementById("pptp_config_status_container").style.display = "block";
		if(pptpIp != "")
		{
			setChildText("pptp_config_status", pptpS.RunC+", IP: " + pptpIp, "#008800", true, null, document)
		}
		else
		{
			setChildText("pptp_config_status", pptpS.RunNot, "#880000", true, null, document)
		}
	}

	setpptpVisibility();
}

function setpptpVisibility()
{
	document.getElementById("pptp_client_fieldset").style.display = getSelectedValue("pptp_config") == "client" ? "block" : "none"
}

function saveChanges()
{
	var errorList = [];

	var pptpConfig = getSelectedValue("pptp_config")
	if (pptpConfig == "client")
	{
		var server = document.getElementById("pptp_server").value;
		var username = document.getElementById("pptp_username").value;
		var password = document.getElementById("pptp_password").value;

		if (server == "") { errorList.push(pptpS.SErr);}
		if (username == "") { errorList.push(pptpS.UErr);}
		if (password == "") { errorList.push(pptpS.PErr);}

		if(errorList.length > 0)
		{
			errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
			alert(errorString);
		}

		uci.set("network", "vpnpptp", "",             "interface")
		uci.set("network", "vpnpptp", "proto",        "pptp")
		uci.set("network", "vpnpptp", "server",        server)
		uci.set("network", "vpnpptp", "username",      username)
		uci.set("network", "vpnpptp", "password",      password)

		uci.set("firewall", "vpnpptp_zone", "",        "zone")
		uci.set("firewall", "vpnpptp_zone", "name",    "vpnpptp")
		uci.set("firewall", "vpnpptp_zone", "network", "vpnpptp")
		uci.set("firewall", "vpnpptp_zone", "input",   "ACCEPT")
		uci.set("firewall", "vpnpptp_zone", "output",  "ACCEPT")
		uci.set("firewall", "vpnpptp_zone", "forward", "ACCEPT")
		uci.set("firewall", "vpnpptp_zone", "mtu_fix", "1")
		uci.set("firewall", "vpnpptp_zone", "masq",    "1")

		uci.set("firewall", "vpnpptp_lan_forwarding", "",     "forwarding")
		uci.set("firewall", "vpnpptp_lan_forwarding", "src",  "lan")
		uci.set("firewall", "vpnpptp_lan_forwarding", "dest", "vpnpptp")

	}
	else
	{
		uci.removeSection("network",  "vpnpptp")
		uci.removeSection("firewall", "vpnpptp_zone")
		uci.removeSection("firewall", "vpnpptp_lan_forwarding")
	}
	execute([uci.getScriptCommands(uciOriginal) + "\n/usr/lib/gargoyle/restart_network.sh\n"]);
}

function pptpReconnect()
{
	execute(["ifup vpnpptp\nsleep 5\n"]);
}
