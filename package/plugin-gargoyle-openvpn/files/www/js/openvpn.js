/*
 * This program is copyright © 2008-2014 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var ovpnS=new Object(); //part of i18n

var newRouterIp=""

function saveChanges()
{
	var errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		
		var openvpnConfig = getSelectedValue("openvpn_config")
		uci.set("openvpn", "custom_config", "", "openvpn")

		configureFirewall = function(enabled,isServer,vpnPort,vpnProto)
		{
			if(enabled)
			{
				uci.set("network", "vpn", "",             "interface")
				uci.set("network", "vpn", "ifname",       "tun0")
				uci.set("network", "vpn", "proto",        "none")
				uci.set("network", "vpn", "defaultroute", "0")
				uci.set("network", "vpn", "peerdns",      "0")

				uci.set("firewall", "vpn_zone", "",        "zone")
				uci.set("firewall", "vpn_zone", "name",    "vpn")
				uci.set("firewall", "vpn_zone", "network", "vpn")
				uci.set("firewall", "vpn_zone", "input",   "ACCEPT")
				uci.set("firewall", "vpn_zone", "output",  "ACCEPT")
				uci.set("firewall", "vpn_zone", "forward", "ACCEPT")
				uci.set("firewall", "vpn_zone", "mtu_fix", "1")
				uci.set("firewall", "vpn_zone", "masq",    "1")

				uci.set("firewall", "vpn_lan_forwarding", "",     "forwarding")
				uci.set("firewall", "vpn_lan_forwarding", "src",  "lan")
				uci.set("firewall", "vpn_lan_forwarding", "dest", "vpn")


				if(isServer)
				{
					uci.set("firewall", "ra_openvpn", "",            "remote_accept")
					uci.set("firewall", "ra_openvpn", "zone",        "wan")
					uci.set("firewall", "ra_openvpn", "local_port",  vpnPort)
					uci.set("firewall", "ra_openvpn", "remote_port", vpnPort)
					uci.set("firewall", "ra_openvpn", "proto",       vpnProto)
					

					uci.set("firewall", "vpn_wan_forwarding", "",     "forwarding")
					uci.set("firewall", "vpn_wan_forwarding", "src",  "vpn")
					uci.set("firewall", "vpn_wan_forwarding", "dest", "wan")
				}
				else
				{
					uci.removeSection("firewall", "vpn_wan_forwarding")
				}
			}
			else
			{
				uci.removeSection("network",  "vpn")
				uci.removeSection("firewall", "vpn_zone")
				uci.removeSection("firewall", "vpn_lan_forwarding")
				uci.removeSection("firewall", "vpn_wan_forwarding")
				uci.removeSection("firewall", "ra_openvpn")

			}
		}


		if(openvpnConfig == "disabled")
		{
			configureFirewall(false,false)
			uci.remove("gargoyle", "status", "openvpn_connections")
			uci.set("openvpn_gargoyle", "server", "enabled", "false")
			uci.set("openvpn_gargoyle", "client", "enabled", "false")
			uci.set("openvpn", "custom_config", "enable", "0")
		}
		if(openvpnConfig == "server")
		{
			if(!haveDh)
			{
				var doSave = confirm(ovpnS.CryptoWaitMsg+"\n")
				if(!doSave)
				{
					setControlsEnabled(true);
					return;
				}
			}

			var prefix   = "openvpn_server_"
			var vpnPort  = document.getElementById(prefix + "port").value
			var vpnProto = getSelectedValue(prefix + "protocol")
			configureFirewall(true,true,vpnPort,vpnProto)


			uci.set("gargoyle", "status", "openvpn_connections", "500")
			uci.set("openvpn", "custom_config", "enable", "1")
			uci.set("openvpn", "custom_config", "config", "/etc/openvpn/server.conf")

			uci.set("openvpn_gargoyle", "server", "enabled", "true")
			uci.set("openvpn_gargoyle", "client", "enabled", "false")

			uci.set("openvpn_gargoyle", "server", "internal_ip", document.getElementById(prefix + "ip").value)
			uci.set("openvpn_gargoyle", "server", "internal_mask", document.getElementById(prefix + "mask").value)
			uci.set("openvpn_gargoyle", "server", "port", vpnPort)
			uci.set("openvpn_gargoyle", "server", "proto", vpnProto)
			uci.set("openvpn_gargoyle", "server", "client_to_client", getSelectedValue(prefix + "client_to_client"))
			uci.set("openvpn_gargoyle", "server", "subnet_access", getSelectedValue(prefix + "subnet_access"))
			uci.set("openvpn_gargoyle", "server", "duplicate_cn", getSelectedValue(prefix + "duplicate_cn"))
			uci.set("openvpn_gargoyle", "server", "redirect_gateway", getSelectedValue(prefix + "redirect_gateway"))
			if( getSelectedValue(prefix + "subnet_access") == "true")
			{
				uci.set("openvpn_gargoyle", "server", "subnet_ip",   adjustSubnetIp(currentLanIp, currentLanMask) )
				uci.set("openvpn_gargoyle", "server", "subnet_mask", currentLanMask )
			}
			else
			{
				uci.remove("openvpn_gargoyle", "server", "subnet_ip")
				uci.remove("openvpn_gargoyle", "server", "subnet_mask")
			}
			if( getSelectedValue(prefix + "duplicate_cn") == "true" )
			{
				var vpnIp   = document.getElementById(prefix + "ip").value
				var vpnMask = document.getElementById(prefix + "mask").value
				var numericVpnIp = getNumericIp(vpnIp)
				var minIp = getNumericIp(vpnIp) & getNumericMask(vpnMask)
				var maxIp = ( ~getNumericMask(vpnMask) ) | minIp
				var pool="";
				if(numericVpnIp - minIp < maxIp-numericVpnIp)
				{
					pool = "" + numericIpToStr(numericVpnIp+1) + " " + numericIpToStr(maxIp-1) + " " + vpnMask
				}
				else
				{
					pool = "" + numericIpToStr(minIp+1) + " " + numericIpToStr(numericVpnIp-1) + " " + vpnMask
				}
				uci.set("openvpn_gargoyle", "server", "pool", pool)
			}
			else
			{
				uci.remove("openvpn_gargoyle", "server", "pool")
			}
			

			
			var cipher = getSelectedValue(prefix + "cipher")
			if(cipher.match(/:/))
			{
				var cipherParts = cipher.split(/:/)
				cipher = cipherParts[0]
				var keysize = cipherParts[1]
				uci.set("openvpn_gargoyle", "server", "keysize", keysize)
			}
			else
			{
				uci.remove("openvpn_gargoyle", "server", "keysize")
			}
			uci.set("openvpn_gargoyle", "server", "cipher", cipher);
		}
		if(openvpnConfig == "client")
		{
			configureFirewall(true,false)
			uci.remove("gargoyle", "status", "openvpn_connections")

			if (document.getElementById("openvpn_client_manual_controls").style.display != "none")
			{
				clientManualCheckCaCertKey() 
				blockNonOpenVpn = getSelectedValue("openvpn_client_block_nonovpn");
				uci.set("openvpn_gargoyle", "client", "block_non_openvpn", blockNonOpenVpn == "block" ? true : false)
			}

		}


		var commands = uci.getScriptCommands(uciOriginal) + "\n" ;
	       	if(openvpnConfig == "server")
		{
			commands = commands + "\n. /usr/lib/gargoyle/openvpn.sh ; regenerate_server_and_allowed_clients_from_uci ;\n"
		}
		
		// If we need to restart firewall, do that before restarting openvpn
		if(commands.match(/uci.*firewall\.vpn_zone\./) || commands.match(/uci.*firewall\.vpn_lan_forwarding\./) || commands.match(/uci.*firewall\.vpn_wan_forwarding\./) || commands.match(/uci.*firewall\.ra_openvpn\./))
		{
			commands = commands + "\n/usr/lib/gargoyle/restart_network.sh ;\n"
		}
		else
		{
			commands = commands + "\n/etc/openvpn.firewall update_enabled ;\n"
		}

		// if anything in server section or client section has changed, restart openvpn
		// otherwise we're just adding client certs to a server and restart shouldn't be needed
		if(openvpnConfig == "disabled")
		{
			commands = "/etc/init.d/openvpn stop ; " + commands
		}
		else if(commands.match(/uci.*openvpn_gargoyle\.server\./) || openvpnConfig == "client" )
		{
			commands = commands + "/etc/init.d/openvpn restart ; sleep 3 ; "
		}

		if(openvpnConfig != "client")
		{
			var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		
			var stateChangeFunction = function(req)
			{
				if(req.readyState == 4)
				{
					window.location=window.location
				}
			}
			runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
		}
		else
		{
			document.getElementById("openvpn_client_commands").value = commands;
			document.getElementById("openvpn_client_hash").value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
			document.getElementById("openvpn_client_form").submit();
		}
	}
}

function clientNetMismatchQuery(expected, current, newIp)
{
	var continueFun = function(result)
	{
		if(result == UI.Cancel)
		{
			window.location=window.location
		}
		else
		{
			if(result == (ovpnS.Switch+" "+newIp))
			{
				document.getElementById("net_mismatch_action").value = "change"
				newRouterIp = newIp
			}
			if(result == ovpnS.KeepC)
			{
				document.getElementById("net_mismatch_action").value = "keep"
			}
			document.getElementById("openvpn_client_form").submit();
		}
	}
	query(ovpnS.SubMis, ovpnS.ExpSubN+" " + expected + ovpnS.ActSubN+" " + current + ".  "+ovpnS.WantQ, 
		[ ovpnS.Switch+" "+newIp, ovpnS.KeepC, UI.Cancel], continueFun );

}


function clientSaved(result)
{
	//Success value here does not need to be and should not be translated
	//it is an internal value only used for determining return status, never displayed
	if(result != "Success")
	{
		alert(UI.Err+": " + result)
		if(result == ovpnS.uc_conn_Err)
		{
			window.location=window.location
		}
	}
	else
	{
		uci = uciOriginal.clone()
		newLocation = window.location
		if(newRouterIp != "")
		{
			newLocation = newLocation.replace(currentLanIp, newRouterIp)
		}
		window.location=newLocation
	}
	document.getElementById("net_mismatch_action").value = "query"

	setControlsEnabled(true)
}

function proofreadAll()
{
	errors = [];
	if(getSelectedValue("openvpn_config") == "server")
	{
		var prefix = "openvpn_server_"
		var inputIds = [ prefix + "ip", prefix + "mask", prefix + "port" ]
		var labelIds = [ prefix + "ip_label", prefix + "mask_label", prefix + "port_label" ]
		var functions = [ validateIP, validateNetMask, validatePort  ];
		var validReturnCodes = [0,0,0]

		var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, inputIds, document );
	
		if(errors.length == 0)
		{
			var serverPort  = document.getElementById(prefix + "port").value
			var serverProto = getSelectedValue(prefix + "protocol", document)
			var oldServerEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled").toLowerCase()
			var oldServerProto   = uciOriginal.get("openvpn_gargoyle", "server", "proto")
			var oldServerPort    = uciOriginal.get("openvpn_gargoyle", "server", "port")
			var oldServerPortDef = [];
			if(oldServerEnabled == "true" || oldServerEnabled == 1 )
			{
				oldServerPortDef[oldServerProto] = [];
				oldServerPortDef[oldServerProto][oldServerPort] = 1
			}

			var serverPortConflict = checkForPortConflict(serverPort, serverProto, oldServerPortDef)
			if(serverPortConflict != "")
			{
				errors.push(ovpnS.SrvPrtErr+" " + serverPortConflict)
			}
		}
	}
	if(getSelectedValue("openvpn_config") == "client")
	{
		if(document.getElementById("openvpn_client_manual_controls").style.display != "none" )
		{
			var clientRemote = document.getElementById("openvpn_client_remote").value
			var clientPort   = document.getElementById("openvpn_client_port").value
			var clientConf   = document.getElementById("openvpn_client_conf_text").value
			if(clientRemote == "")
			{
				errors.push(ovpnS.SrvAddErr)
			}
			if(clientPort < 1 || clientPort > 65535)
			{
				errors.push(ovpnS.OPrtErr)
			}
			if(clientConf.match(/^[\t ]*dev[\t ]+tap.*$/i))
			{
				errors.push(ovpnS.GTAPErr);
			}
		}
	}
	return errors;
}

function clientManualCheckCaCertKey()
{
	var toAdd = "";
	var elem = document.getElementById("openvpn_client_conf_text");
	var clientConf = elem.value;
	if ("" !== document.getElementById("openvpn_client_ca_text").value && !clientConf.match(/^[\t ]*ca[\t ].*$/im))
	{
		toAdd += "\nca ca.crt";
	}
	if ("" !== document.getElementById("openvpn_client_cert_text").value && !clientConf.match(/^[\t ]*cert[\t ].*$/im))
	{
		toAdd += "\ncert cert.crt";
	}
	if ("" !== document.getElementById("openvpn_client_key_text").value && !clientConf.match(/^[\t ]*key[\t ].*$/im))
	{
		toAdd += "\nkey key.key";
	}
	if ("" !== toAdd)
	{
		elem.value += toAdd;
	}
}



function resetData()
{
	var serverEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled") 
	var clientEnabled = uciOriginal.get("openvpn_gargoyle", "client", "enabled")
	serverEnabled = serverEnabled == "true" || serverEnabled == "1" ? true : false;
	clientEnabled = clientEnabled == "true" || clientEnabled == "1" ? true : false;
	
	var openvpnMode = "disabled"
	openvpnMode = serverEnabled ? "server" : openvpnMode
	openvpnMode = clientEnabled ? "client" : openvpnMode
	setSelectedValue("openvpn_config", openvpnMode)

	document.getElementById("openvpn_config_status_container").style.display= openvpnMode == "disabled" ? "none"  : "block"
	
	if(openvpnMode != "disabled")
	{
		if( tunIp != "" && openvpnProc != "" && (remotePing != "" || openvpnMode == "server") )
		{
			setChildText("openvpn_config_status", ovpnS.RunC+", IP: " + tunIp, "#008800", true, null, document)
		}
		else if(openvpnProc != "")
		{
			setChildText("openvpn_config_status", ovpnS.RunNC, "#880000", true, null, document)
		}
		else
		{
			setChildText("openvpn_config_status", ovpnS.RunNot, "#880000", true, null, document)
		}
	}
	
	getServerVarWithDefault = function(variable, defaultDef) {
		var def = uciOriginal.get("openvpn_gargoyle", "server", variable)
		def = def == "" ? defaultDef : def
		return def
	}

	document.getElementById("openvpn_server_ip").value = getServerVarWithDefault("internal_ip", "10.8.0.1")
	document.getElementById("openvpn_server_mask").value = getServerVarWithDefault("internal_mask", "255.255.255.0")
	document.getElementById("openvpn_server_port").value = getServerVarWithDefault("port", "1194")

	
	var serverCipher  = uciOriginal.get("openvpn_gargoyle", "server", "cipher")
	var serverKeysize = uciOriginal.get("openvpn_gargoyle", "server", "keysize")
	if(serverCipher == "")
	{
		serverCipher = "BF-CBC"
		serverKeysize = "128"
	}
	serverCipher = serverKeysize == "" ? serverCipher : serverCipher + ":" + serverKeysize

	setSelectedValue("openvpn_server_protocol", getServerVarWithDefault("proto", "udp"))
	setSelectedValue("openvpn_server_cipher", serverCipher)
	setSelectedValue("openvpn_server_client_to_client", getServerVarWithDefault("client_to_client", "false"))
	setSelectedValue("openvpn_server_subnet_access", getServerVarWithDefault("subnet_access", "false"))
	setSelectedValue("openvpn_server_duplicate_cn", getServerVarWithDefault("duplicate_cn", "false"))
	setSelectedValue("openvpn_server_redirect_gateway", getServerVarWithDefault("redirect_gateway", "true"))

	var acTableData = []
	var allowedClients = uciOriginal.getAllSectionsOfType("openvpn_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var rowData = []
		var id          = allowedClients[aci]
		var name        = uciOriginal.get("openvpn_gargoyle", id, "name")
		var ip          = uciOriginal.get("openvpn_gargoyle", id, "ip")
		var subnetIp   = uciOriginal.get("openvpn_gargoyle", id, "subnet_ip")
		var subnetMask = uciOriginal.get("openvpn_gargoyle", id, "subnet_mask")
		var enabled     = uciOriginal.get("openvpn_gargoyle", id, "enabled")
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""

		var ipElementContainer = document.createElement("span")
		var naContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		naContainer.appendChild( document.createTextNode("---") )
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(naContainer)
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id
		


		rowData.push(name + "\n ")
		rowData.push(ipElementContainer)
		
		var controls = createAllowedClientControls(true)
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}

		enabled = enabled != "false" && enabled != "0" ? true : false;
		rowData[2].checked = enabled
		
		acTableData.push(rowData)
	}

	var acTable = createTable([ ovpnS.ClntN, ovpnS.IntIP, UI.Enabled, ovpnS.CfgCredF, ""], acTableData, "openvpn_allowed_client_table", true, false, removeAcCallback)
	var tableContainer = document.getElementById("openvpn_allowed_client_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(acTable);


	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"

	setAcDocumentFromUci(document, new UCIContainer(), "dummy", dupeCn, document.getElementById("openvpn_server_ip").value )


	//client
	var upCheckEl  = document.getElementById("openvpn_client_config_upload");
	var manCheckEl = document.getElementById("openvpn_client_config_manual");
	if(curClientConf.length >0 && curClientCa.length >0 && curClientCert.length >0 && curClientKey.length >0)
	{
		manCheckEl.checked = true;
		upCheckEl.checked  = false;
		
		document.getElementById("openvpn_client_conf_text").value    = curClientConf.join("\n");
		document.getElementById("openvpn_client_ca_text").value      = curClientCa.join("\n");
		document.getElementById("openvpn_client_cert_text").value    = curClientCert.join("\n");
		document.getElementById("openvpn_client_key_text").value     = curClientKey.join("\n");
		document.getElementById("openvpn_client_ta_key_text").value  = curClientTaKey.join("\n");
		var textTaCheck = document.getElementById("openvpn_client_use_ta_key_text");	
		textTaCheck.checked = curClientTaKey.length > 0 ? true : false;

		blockNonOpenVpn = uciOriginal.get("openvpn_gargoyle", "client", "block_non_openvpn")
		setSelectedValue("openvpn_client_block_nonovpn", (blockNonOpenVpn == "true" || blockNonOpenVpn == "1") ? "block" : "allowed")

		updateClientControlsFromConfigText()
	}
	else
	{
		upCheckEl.checked  = true;
		manCheckEl.checked = false;
	}

	setOpenvpnVisibility()
}

function updateClientControlsFromConfigText()
{
	var configLines = document.getElementById("openvpn_client_conf_text").value.split(/[\r\n]+/);
	var remote       = null;
	var port         = null;
	var proto        = null;
	var cipher       = null;
	var keysize      = null;
	var taDirection = null;

	var portFrom = "undefined";

	while(configLines.length >0)
	{
		var line = configLines.shift();
		var lineParts = line.replace(/^[\t ]+/, "").split(/[\t ]+/);
		
		if(lineParts[0].toLowerCase() == "remote")
		{
			remote = lineParts[1] != null ? lineParts[1] : remote;
			port   = lineParts[2] != null ? lineParts[2] : port;
			portFrom = (lineParts[2] != null) ? "remote" : portFrom;
		}
		else if (lineParts[0].toLowerCase() == "rport" && portFrom != "remote")
		{
			port   = lineParts[1] != null ? lineParts[2] : port;
			portFrom = (lineParts[1] != null) ? "rport" : portFrom;
		}
		else if (lineParts[0].toLowerCase() == "port" && portFrom != "remote" && portFrom != "rport")
		{
			port   = lineParts[1] != null ? lineParts[2] : port;
			portFrom = (lineParts[1] != null) ? "port" : portFrom;
		}
		else if(lineParts[0].toLowerCase() == "proto")
		{
			if(lineParts[1] != null)
			{
				proto = lineParts[1] == "udp" ? "udp" : "tcp"
			}
		}
		else if(lineParts[0].toLowerCase() == "cipher")
		{
			cipher = lineParts[1] != null ? lineParts[1] : cipher;
		}
		else if(lineParts[0].toLowerCase() == "keysize")
		{
			keysize = lineParts[1] != null ? lineParts[1] : keysize;
		}
		else if(lineParts[0].toLowerCase() == "tls-auth")
		{
			taDirection = lineParts[2] != null ? lineParts[2] : "";
		}

	}
	if(remote != null)
	{
		document.getElementById("openvpn_client_remote").value = remote;
	}
	if(port != null)
	{
		document.getElementById("openvpn_client_port").value = port;
	}
	if(proto != null)
	{
		setSelectedValue("openvpn_client_protocol", proto)
	}
	if(cipher != null)
	{
		if(cipher == "BF-CBC" && (keysize == "128" || keysize == "256" || keysize == null))
		{
			keysize = keysize == null ? "128" : keysize
			setSelectedValue("openvpn_client_cipher", cipher + ":" + keysize)
		}
		else if(cipher == "AES-128-CBC" || cipher == "AES-256-CBC")
		{
			setSelectedValue("openvpn_client_cipher", cipher)
		}
		else
		{
			setSelectedValue("openvpn_client_cipher", "other")
			document.getElementById("openvpn_client_cipher_other").value = cipher
			document.getElementById("openvpn_client_key_other").value = keysize == null ? "" : keysize
		}
	}
	if(taDirection != null)
	{
		taDirection = taDirection == "1" ? "1" : "omitted"
		setSelectedValue("openvpn_client_ta_direction", taDirection )
		var taCheck = document.getElementById('openvpn_client_use_ta_key_text')
		taCheck.checked = true
		enableAssociatedField(taCheck, "openvpn_client_ta_key_text", "")
		enableAssociatedField(taCheck, "openvpn_client_ta_direction", "1")
	}
	proofreadPort(document.getElementById("openvpn_client_port"))
}

function updateClientConfigTextFromControls()
{
	var remote      = document.getElementById("openvpn_client_remote").value;
	var port        = document.getElementById("openvpn_client_port").value;
	var proto       = getSelectedValue("openvpn_client_protocol");
	var cipher      = getSelectedValue("openvpn_client_cipher");
	var taDirection = getSelectedValue("openvpn_client_ta_direction") == "1" ? " 1" : ""
	
	var cipherParts = cipher.split(/:/);
	cipher = cipherParts[0];
	var keysize = cipherParts[1] == null ? "" : cipherParts[1];
	if(cipher == "other")
	{
		cipher  = document.getElementById("openvpn_client_cipher_other").value;
	       	keysize = document.getElementById("openvpn_client_cipher_other").value;
	}

	var configLines = document.getElementById("openvpn_client_conf_text").value.split(/[\r\n]+/);
	var newLines = [];
	var foundVars = [];
	var defaultCipher = cipher == "Blowfish-CBC" && keysize == "128" ? true : false;
	while(configLines.length >0)
	{
		var line = configLines.shift();
		var lineParts = line.replace(/^[\t ]+/, "").split(/[\t ]+/);
		if(lineParts[0].toLowerCase() == "remote")
		{
			line = "remote " + remote + " " + port
			foundVars["remote"] = 1
		}
		else if(lineParts[0].toLowerCase() == "proto")
		{
			line = "proto " + (proto == "tcp" ? "tcp-client" : "udp" )
			foundVars["proto"] = 1
		}
		else if(lineParts[0].toLowerCase() == "cipher")
		{
			line = "cipher " + cipher
			foundVars["cipher"] = 1
		}
		else if(lineParts[0].toLowerCase() == "keysize")
		{
			line = keysize == "" ? "" : "keysize " + keysize
			foundVars["keysize"] = 1
		}
		else if(lineParts[0].toLowerCase() == "rport" || lineParts[0].toLowerCase() == "port" )
		{
			//specify port in remote line instead of with these directives, so get rid of these lines
			line = ""
		}
		else if(lineParts[0].toLowerCase() == "tls-auth")
		{
			if( document.getElementById("openvpn_client_use_ta_key_text").checked )
			{
				lineParts[1] = lineParts[1] == null ? "ta.key" : lineParts[1]
				line = lineParts[0] + " " + lineParts[1] + taDirection
				foundVars["tls-auth"] = 1
			}
			else
			{
				line = ""
			}
		}
		newLines.push(line)
	}

	if(foundVars["keysize"] == null && keysize != "" && (!(defaultCipher && foundVars["cipher"] == null)) )
	{
		newLines.unshift("keysize " + keysize )
	}
	if(foundVars["cipher"] == null && (!defaultCipher) )
	{
		newLines.unshift("cipher " + cipher);
	}
	if(foundVars["proto"] == null)
	{
		newLines.unshift("proto " + (proto == "tcp" ? "tcp-client" : "udp" ))
	}
	if(foundVars["remote"] == null)
	{
		newLines.unshift("remote " + remote + " " + port)
	}
	if(foundVars["tls-auth"] == null && document.getElementById("openvpn_client_use_ta_key_text").checked)
	{
		newLines.unshift("tls-auth ta.key" + taDirection)
	}

	document.getElementById("openvpn_client_conf_text").value = newLines.join("\n");

}


function createAllowedClientControls(haveDownload)
{

	var enabledCheck = createInput("checkbox")
	enabledCheck.onclick = toggleAcEnabled;
	var downloadButton = haveDownload ? createButton(ovpnS.Dload, "default_button", downloadAc, false) : createButton(ovpnS.Dload, "default_button_disabled", function(){ return; }, true ) ;
	var editButton     = createButton(UI.Edit,     "default_button", editAc, false)

	return [enabledCheck, downloadButton, editButton]
}

function createButton(text, cssClass, actionFunction, disabled)
{
	var button = createInput("button")
	button.value = text
	button.className=cssClass
	button.onclick = actionFunction
	button.disabled = disabled
	return button;
}


function updateDupeCn()
{
	var serverInternalIp = document.getElementById("openvpn_server_ip").value
	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"
	

	var allowedTable = document.getElementById("openvpn_allowed_client_table");
	var setNewIp = false;
	if(allowedTable != null)
	{
		var rows = allowedTable.rows;
		var ri;
		for(ri =1; ri < rows.length ; ri++)
		{
			var ipElementContainer = rows[ri].childNodes[1].firstChild;
			var id = ipElementContainer.id;
		
			if(!dupeCn && uci.get("openvpn_gargoyle", id, "ip") == "")
			{
				var serverInternalIp = document.getElementById("openvpn_server_ip").value
				var ip = getUnusedAcIp(serverInternalIp)
				uci.set("openvpn_gargoyle", id, "ip", ip)
				
				var ipContainer = ipElementContainer.childNodes[1]	
				setSingleChild(ipContainer, document.createTextNode(ip))
				ipContainer.appendChild( document.createElement("br") )
				ipContainer.appendChild( document.createTextNode("") )

				setNewIp = true
			}
		}
	}
	if(setNewIp)
	{
		var definedIps = getDefinedAcIps(true);
		definedIps[serverInternalIp] = 1
		if( definedIps[ document.getElementById('openvpn_allowed_client_ip').value ] != null )
		{
			var ip = getUnusedAcIp(serverInternalIp)
			document.getElementById('openvpn_allowed_client_ip').value = ip
		}
	}

	setOpenvpnVisibility()
}


function setOpenvpnVisibility()
{

	var originalEnabled = false
	var originalServerEnabled = uciOriginal.get("openvpn_gargoyle", "server", "enabled") 
	var originalClientEnabled = uciOriginal.get("openvpn_gargoyle", "client", "enabled")
	if(originalServerEnabled == "true" || originalServerEnabled == "1" || originalClientEnabled == "true" || originalClientEnabled == "1")
	{
		originalEnabled = true
	}

	openvpnMode = getSelectedValue("openvpn_config");
	
	
	document.getElementById("openvpn_clear_keys_container").style.display = (openvpnMode == "disabled" && (!originalEnabled)) ? "block" : "none"



	document.getElementById("openvpn_server_fieldset").style.display         = openvpnMode == "server" ? "block" : "none"
	document.getElementById("openvpn_allowed_client_fieldset").style.display = openvpnMode == "server" ? "block" : "none"
	document.getElementById("openvpn_client_fieldset").style.display         = openvpnMode == "client" ? "block" : "none"
	
	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"


	var allowedTable = document.getElementById("openvpn_allowed_client_table");
	if(allowedTable != null)
	{
		var rows = allowedTable.rows;
		var ri;
		for(ri =1; ri < rows.length ; ri++)
		{
			var ipElementContainer = rows[ri].childNodes[1].firstChild;
			var ipChildIndex;
			for(ipChildIndex=0; ipChildIndex < ipElementContainer.childNodes.length ; ipChildIndex++)
			{
				ipElementContainer.childNodes[ipChildIndex].style.display = (ipChildIndex == 0 && dupeCn) || (ipChildIndex > 0 && (!dupeCn)) ? "inline" : "none"
			}

		}
	}




	initializeAllowedClientVisibility(document, dupeCn);
	setClientVisibility(document)
}

function initializeAllowedClientVisibility(controlDocument, dupeCn)
{
	controlDocument.getElementById("openvpn_allowed_client_ip_container").style.display          = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_have_subnet_container").style.display = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip_container").style.display   = dupeCn ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask_container").style.display = dupeCn ? "none" : "block"
	setAllowedClientVisibility(controlDocument)

}

function setAllowedClientVisibility( controlDocument )
{
	var selectedVis = document.getElementById("openvpn_allowed_client_remote_container").style.display == "none" ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_remote_custom_container").style.display  = getSelectedValue("openvpn_allowed_client_remote", controlDocument) == "custom" ? selectedVis : "none";
	

	var selectedVis = document.getElementById("openvpn_allowed_client_have_subnet_container").style.display == "none" ? "none" : "block"
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip_container").style.display   = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? selectedVis : "none";
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask_container").style.display = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? selectedVis : "none";
}

function setClientVisibility(controlDocument)
{
	var upCheckEl  = controlDocument.getElementById("openvpn_client_config_upload");
	var manCheckEl = controlDocument.getElementById("openvpn_client_config_manual");
	if( (!upCheckEl.checked) && (!manCheckEl.checked) )
	{
		upCheckEl.checked = true;
	}
	var boolToInt = function(b) { return b ? 1 : 0 ; }
	
	var fileTaCheck = document.getElementById("openvpn_client_use_ta_key_file");
	var textTaCheck = document.getElementById("openvpn_client_use_ta_key_text");
	enableAssociatedField(fileTaCheck, "openvpn_client_ta_key_file", "")
	enableAssociatedField(textTaCheck, "openvpn_client_ta_key_text", "")
	enableAssociatedField(textTaCheck, "openvpn_client_ta_direction", "1")

	var single = getSelectedValue("openvpn_client_file_type", controlDocument) == "zip" ? 1 : 0;
	var multi  = single == 1 ? 0 : 1;
	controlDocument.getElementById("openvpn_client_cipher_other_container").style.display = getSelectedValue("openvpn_client_cipher", controlDocument) == "other" ? "block" : "none"
	setVisibility( ["openvpn_client_zip_file_container", "openvpn_client_conf_file_container", "openvpn_client_ca_file_container", "openvpn_client_cert_file_container", "openvpn_client_key_file_container", "openvpn_client_ta_key_file_container"], [single, multi, multi, multi, multi, multi], ["block", "block", "block", "block", "block", "block"], controlDocument)
	setVisibility( ["openvpn_client_file_controls", "openvpn_client_manual_controls"], [ boolToInt(upCheckEl.checked), boolToInt(manCheckEl.checked) ], ["block","block"], controlDocument)
	
}

function setRemoteNames( controlDocument, selectedRemote)
{
	var selectId = "openvpn_allowed_client_remote";
	selectedRemote = selectedRemote == null ? "" : selectedRemote;

	var names = []
	var values = []
	
	var definedDdns = uciOriginal.getAllSectionsOfType("ddns_gargoyle", "service")
	var ddi
	var selectedFound = false
	for(ddi=0; ddi < definedDdns.length; ddi++)
	{
		var enabled = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "enabled")
		var domain  = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "domain")
		if( (enabled != "0" && enabled != "false") && domain != "")
		{
			names.push(ovpnS.DDNS+": " + domain)
			values.push(domain)
			selectedFound = selectedRemote == domain ? true : selectedFound
		}
	}
	selectedFound = (selectedRemote == currentWanIp) || selectedFound
	names.push(ovpnS.WANIP+": " + currentWanIp, ovpnS.OthIPD)
	values.push(currentWanIp, "custom")
	
	setAllowableSelections(selectId, values, names, controlDocument)
	var chosen = selectedRemote == "" ? values[0] : selectedRemote
	chosen = (!selectedFound) && selectedRemote != "" ? "custom" : selectedRemote
	setSelectedValue(selectId, chosen, controlDocument)
	if(chosen == "custom")
	{
		controlDocument.getElementById("openvpn_allowed_client_remote_custom").value = selectedRemote
	}
}

function getUnusedAcIp(serverInternalIp)
{
	var ipParts = serverInternalIp.split(/\./)
	var fourthIpPart = ipParts.pop()
	var thirdIpPart  = ipParts.pop()
	var secondIpPart = ipParts.pop()
	var firstIpPart  = ipParts.pop()

	fourthIpPart = parseInt(fourthIpPart);
	thirdIpPart  = parseInt(thirdIpPart);
	secondIpPart = parseInt(secondIpPart);
	fourthIpPart++;

	
	var candidateDefaultIp = firstIpPart + "." + secondIpPart + "." + thirdIpPart + "." + fourthIpPart

	var definedIps = getDefinedAcIps(true);
	definedIps[serverInternalIp] = 1
	while( (fourthIpPart < 255 || thirdIpPart < 255 || secondIpPart < 255) && definedIps[candidateDefaultIp] == 1)
	{
		fourthIpPart++
		if(fourthIpPart == 255)
		{
			fourthIpPart = 1
			thirdIpPart++
		}
		if(thirdIpPart == 255)
		{
			thirdIpPart = 0
			secondIpPart++
		}
		if(secondIpPart != 255)
		{	
			candidateDefaultIp = firstIpPart + "." + secondIpPart + "." + thirdIpPart + "." + fourthIpPart
		}
	}
	return candidateDefaultIp
}

function setAcDocumentFromUci(controlDocument, srcUci, id, dupeCn, serverInternalIp)
{
	var name = srcUci.get("openvpn_gargoyle", id, "name")
	
	if( srcUci.get("openvpn_gargoyle", id, "remote") == "" )
	{
		var allIdList = getDefinedAcIds(false)
		var allIdHash = getDefinedAcIds(true)
		var clientCount = allIdList.length +1
		name = ovpnS.Clnt + clientCount
		id = "client" + clientCount
		while(allIdHash[id] == 1)
		{
			clientCount++
			name = ovpnS.Clnt + clientCount
			id = "client" + clientCount
		}
		controlDocument.getElementById("openvpn_allowed_client_default_id").value = id
	}
	else
	{
		controlDocument.getElementById("openvpn_allowed_client_initial_id").value = id
	}

	controlDocument.getElementById("openvpn_allowed_client_name").value = name
	

	var ip = srcUci.get("openvpn_gargoyle", id, "ip")
	if(ip == "")
	{
		ip = getUnusedAcIp(serverInternalIp)

	}
	controlDocument.getElementById("openvpn_allowed_client_ip").value = ip
	
	setRemoteNames(controlDocument, srcUci.get("openvpn_gargoyle", id, "remote"))

	var subnetIp   = srcUci.get("openvpn_gargoyle", id, "subnet_ip")
	var subnetMask = srcUci.get("openvpn_gargoyle", id, "subnet_mask")

	setSelectedValue("openvpn_allowed_client_have_subnet", (subnetIp != "" && subnetMask != "" ? "true" : "false"), controlDocument)
	subnetIp   = subnetIp   == "" ? "192.168.2.1" : subnetIp;
	subnetMask = subnetMask == "" ? "255.255.255.0" : subnetMask;
	controlDocument.getElementById("openvpn_allowed_client_subnet_ip").value = subnetIp
	controlDocument.getElementById("openvpn_allowed_client_subnet_mask").value = subnetMask



	initializeAllowedClientVisibility(controlDocument, dupeCn)
}

function getDefinedAcIps(retHash)
{
	var ips = []
	var allowedClients = getDefinedAcIds(false)
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var ip = uci.get("openvpn_gargoyle", allowedClients[aci], "ip")
		if(ip != "")
		{
			if(retHash)
			{
				ips[ip] = 1;
			}
			else
			{
				ips.push(ip)
			}
		}
	}
	return ips;
}

function getDefinedAcIds(retHash)
{
	var ids = []
	var allowedClients = uci.getAllSectionsOfType("openvpn_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var id = allowedClients[aci]
		var enabled = uci.get("openvpn_gargoyle", id, "enabled")
		if(enabled != "0" && enabled != "false")
		{
			if(retHash)
			{
				ids[id] = 1;
			}
			else
			{
				ids.push(id)
			}
		}
	}
	return ids;
}




function setAcUciFromDocument(controlDocument, id)
{
	var name = controlDocument.getElementById("openvpn_allowed_client_name").value;
	
	var ipContainer = controlDocument.getElementById("openvpn_allowed_client_ip_container")
	var ip = controlDocument.getElementById("openvpn_allowed_client_ip").value
	ip = ipContainer.style.display == "none" ? "" : ip
	
	var remote = getSelectedValue("openvpn_allowed_client_remote", controlDocument)
	remote = remote == "custom" ? controlDocument.getElementById("openvpn_allowed_client_remote_custom").value : remote
	
	var haveSubnet = getSelectedValue("openvpn_allowed_client_have_subnet", controlDocument) == "true" ? true : false
	haveSubnet     = ipContainer.style.display == "none" ? false : haveSubnet
	var subnetIp   = controlDocument.getElementById("openvpn_allowed_client_subnet_ip").value
	var subnetMask = controlDocument.getElementById("openvpn_allowed_client_subnet_mask").value

	var pkg = "openvpn_gargoyle"
	uci.set(pkg, id, "", "allowed_client")
	uci.set(pkg, id, "id", id)
	uci.set(pkg, id, "name", name)
	if(ip != "")
	{
		uci.set(pkg, id, "ip", ip)
	}
	else
	{
		uci.remove(pkg, id, "ip")
	}
	uci.set(pkg, id, "remote", remote)
	if(haveSubnet)
	{
		uci.set(pkg, id, "subnet_ip",   subnetIp)
		uci.set(pkg, id, "subnet_mask", subnetMask)
	}
}

function getNumericIp(ip)
{
	var i = ip.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/)
	if(i)
	{
        	return (+i[1]<<24) + (+i[2]<<16) + (+i[3]<<8) + (+i[4])
	}
	return null
}

function getNumericMask(mask)
{
	if(mask.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/))
	{
		return getNumericIp(mask)
	}
	else
	{
		return -1<<(32-mask)
	}
}
function numericIpToStr(numIp)
{
	return ( (numIp>>>24) +'.' + (numIp>>16 & 255) + '.' + (numIp>>8 & 255) + '.' + (numIp & 255) );
}
function adjustSubnetIp(ip, mask)
{
	return numericIpToStr( getNumericIp(ip) & getNumericMask(mask) )
}



function validateAc(controlDocument, internalServerIp, internalServerMask)
{
	controlDocument = controlDocument == null ? document : controlDocument;
	
	var validateHaveText = function(txt) {  return txt.length > 0 ? 0 : 1 }

	var prefix = "openvpn_allowed_client_"
	var inputIds = [ prefix + "name", prefix + "ip", prefix + "remote_custom", prefix + "subnet_ip", prefix + "subnet_mask" ]
	var labelIds = [ prefix + "name_label", prefix + "ip_label", prefix + "remote_label",  prefix + "have_subnet_label", prefix + "have_subnet_label" ]
	var functions = [ validateHaveText, validateIP, validateHaveText, validateIP, validateNetMask  ];
	var validReturnCodes = [0,0,0,0,0]
	var visibilityIds = [  prefix + "name_container", prefix + "ip_container", prefix + "remote_custom_container", prefix + "subnet_ip_container", prefix + "subnet_mask_container" ]

	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, controlDocument );
	if(errors.length == 0 && controlDocument.getElementById(prefix + "ip_container").style.display != "none")
	{
		var testIp  = getNumericIp(controlDocument.getElementById(prefix + "ip").value)
		var vpnIp   = getNumericIp(internalServerIp)
		var vpnMask = getNumericMask(internalServerMask)
		if( ( testIp & vpnMask ) != ( vpnIp & vpnMask ) )
		{
			errors.push(ovpnS.ClntIntIP+" " + controlDocument.getElementById(prefix + "ip").value + " "+ovpnS.OSubErr)
		}
	}
	if(errors.length == 0 && controlDocument.getElementById(prefix + "subnet_ip_container").style.display != "none")
	{
		var subnetIpEl   = controlDocument.getElementById(prefix + "subnet_ip")
		var subnetMaskEl = controlDocument.getElementById(prefix + "subnet_mask")
		subnetIpEl.value = adjustSubnetIp(subnetIpEl.value, subnetMaskEl.value)
	}


	return errors;

}


function toggleAcEnabled()
{
	var toggleRow=this.parentNode.parentNode;
	var toggleId = editRow.childNodes[1].firstChild.id;

	uci.set("openvpn_gargoyle", id, "enabled", (this.checked? "true" : "false"));
}


function removeAcCallback(table, row)
{
	var id = row.childNodes[1].firstChild.id;
	uci.removeSection("openvpn_gargoyle", id);
}

function addAc()
{
	var errors = validateAc(document, document.getElementById("openvpn_server_ip").value , document.getElementById("openvpn_server_mask").value );
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+ovpnS.AddCErr);
	}
	else
	{
		var name       = document.getElementById("openvpn_allowed_client_name").value
		var ip         = document.getElementById("openvpn_allowed_client_ip").value
		var subnetIp   = ""
		var subnetMask = ""
		if( getSelectedValue("openvpn_allowed_client_have_subnet", document) == "true")
		{
			subnetIp   = document.getElementById("openvpn_allowed_client_subnet_ip").value
			subnetMask = document.getElementById("openvpn_allowed_client_subnet_mask").value
		}
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""
	
		var id = name.replace(/[\t\r\n ]+/g, "_").toLowerCase().replace(/[^a-z0-9_-]/g, "");
		var idCount = 1;
		var testId = id
		while(uci.get("openvpn_gargoyle", testId) != "")
		{
			testId = id + "_" + idCount
			idCount++
		}
		id = testId

		setAcUciFromDocument(document, id)
		uci.set("openvpn_gargoyle", id, "enabled", "true")


		var ipElementContainer = document.createElement("span")
		var naContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		naContainer.appendChild( document.createTextNode("---") )
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(naContainer)
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id

		var acTable = document.getElementById("openvpn_allowed_client_table");


		var rowData = [ name, ipElementContainer ]
		var controls = createAllowedClientControls(false)
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}
		rowData[2].checked = true
		addTableRow(acTable, rowData, true, false, removeAcCallback);
	
		var dupeCn = getSelectedValue("openvpn_server_duplicate_cn")
		dupeCn= dupeCn == "true" || dupeCn == "1"
		setAcDocumentFromUci(document, new UCIContainer(), "dummy", dupeCn, document.getElementById("openvpn_server_ip").value )
		
		setOpenvpnVisibility()
	}

}


function downloadAc()
{
	var downloadRow=this.parentNode.parentNode;
	var downloadId = downloadRow.childNodes[1].firstChild.id;
	window.location="/utility/openvpn_download_credentials.sh?id=" + downloadId
}



function editAc()
{
	if( typeof(editAcWindow) != "undefined" )
	{
		//opera keeps object around after
		//window is closed, so we need to deal
		//with error condition
		try
		{
			editAcWindow.close();
		}
		catch(e){}
	}

	
	try
	{
		xCoor = window.screenX + 225;
		yCoor = window.screenY+ 225;
	}
	catch(e)
	{
		xCoor = window.left + 225;
		yCoor = window.top + 225;
	}


	editAcWindow = window.open("openvpn_allowed_client_edit.sh", "edit", "width=560,height=600,left=" + xCoor + ",top=" + yCoor );
	
	var saveButton = createInput("button", editAcWindow.document);
	var closeButton = createInput("button", editAcWindow.document);
	saveButton.value = UI.CApplyChanges;
	saveButton.className = "default_button";
	closeButton.value = UI.CDiscardChanges;
	closeButton.className = "default_button";

	var editRow=this.parentNode.parentNode;
	var editId = editRow.childNodes[1].firstChild.id;
	
	var dupeCn = getSelectedValue("openvpn_server_duplicate_cn");
	dupeCn= dupeCn == "true" || dupeCn == "1"
	var serverInternalIp   = document.getElementById("openvpn_server_ip").value 
	var serverInternalMask = document.getElementById("openvpn_server_mask").value 


	var runOnEditorLoaded = function () 
	{
		var updateDone=false;
		if(editAcWindow.document != null)
		{
			if(editAcWindow.document.getElementById("bottom_button_container") != null)
			{
				editAcWindow.document.getElementById("bottom_button_container").appendChild(saveButton);
				editAcWindow.document.getElementById("bottom_button_container").appendChild(closeButton);
				
				setAcDocumentFromUci(editAcWindow.document, uci, editId, dupeCn, serverInternalIp)


				closeButton.onclick = function()
				{
					editAcWindow.close();
				}
				saveButton.onclick = function()
				{
					var errors = validateAc(editAcWindow.document, serverInternalIp, serverInternalMask);
					if(errors.length > 0)
					{
						alert(errors.join("\n") + "\n"+ovpnS.UpCErr);
					}
					else
					{
						var name       = editAcWindow.document.getElementById("openvpn_allowed_client_name").value
						var ip         = editAcWindow.document.getElementById("openvpn_allowed_client_ip").value
						var subnetIp   = ""
						var subnetMask = ""
						if( getSelectedValue("openvpn_allowed_client_have_subnet", editAcWindow.document) == "true")
						{
							subnetIp   = editAcWindow.document.getElementById("openvpn_allowed_client_subnet_ip").value
							subnetMask = editAcWindow.document.getElementById("openvpn_allowed_client_subnet_mask").value
						}
						var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""


						setAcUciFromDocument(editAcWindow.document, editId)
					
						while( editRow.childNodes[0].firstChild != null)
						{
							editRow.childNodes[0].removeChild( editRow.childNodes[0].firstChild )
						}
						editRow.childNodes[0].appendChild(document.createTextNode(name))


						var ipElementContainer = document.createElement("span")
						var naContainer = document.createElement("span")
						var ipContainer = document.createElement("span")
						naContainer.appendChild( document.createTextNode("---") )
						ipContainer.appendChild( document.createTextNode(ip) )
						ipContainer.appendChild( document.createElement("br") )
						ipContainer.appendChild( document.createTextNode(subnet) )
						ipElementContainer.appendChild(naContainer)
						ipElementContainer.appendChild(ipContainer)
						ipElementContainer.id = editId

						while( editRow.childNodes[1].firstChild != null)
						{
							editRow.childNodes[1].removeChild( editRow.childNodes[1].firstChild )
						}
						
						editRow.childNodes[1].appendChild( ipElementContainer )
						setOpenvpnVisibility()

						editAcWindow.close();
					}
				}
				editAcWindow.moveTo(xCoor,yCoor);
				editAcWindow.focus();
				updateDone = true;
				
			}
		}
		if(!updateDone)
		{
			setTimeout(runOnEditorLoaded, 250);
		}
	}
	runOnEditorLoaded();
}
function clearOpenvpnKeys()
{
	var confirmed = confirm(ovpnS.OClrC)
	if(confirmed)
	{
		setControlsEnabled(false, true);

		var commands = "rm -rf /etc/openvpn/* ; ln -s /var/openvpn/current_status /etc/openvpn/current_status ;"
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				window.location=window.location
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}
