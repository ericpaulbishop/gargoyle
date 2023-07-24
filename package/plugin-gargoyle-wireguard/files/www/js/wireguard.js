var wgStr = new Object();

function resetData()
{
	serverEnabled = uciOriginal.get("wireguard_gargoyle","server","enabled");
	clientEnabled = uciOriginal.get("wireguard_gargoyle","client","enabled");
	serverEnabled = serverEnabled == "true" || serverEnabled == "1" ? true : false;
	clientEnabled = clientEnabled == "true" || clientEnabled == "1" ? true : false;

	var mode = "disabled";
	mode = serverEnabled ? "server" : mode;
	mode = clientEnabled ? "client" : mode;
	setSelectedValue("wireguard_config",mode);

	//Server
	getServerVarWithDefault = function(variable, defaultDef) {
		var def = uciOriginal.get("wireguard_gargoyle", "server", variable)
		def = def == "" ? defaultDef : def
		return def
	}

	document.getElementById("wireguard_server_ip").value = getServerVarWithDefault("ip","10.64.0.1");
	document.getElementById("wireguard_server_mask").value = getServerVarWithDefault("submask","255.255.255.0");
	document.getElementById("wireguard_server_port").value = getServerVarWithDefault("port","51820");
	setSelectedValue("wireguard_server_client_to_client",getServerVarWithDefault("c2c","true"));
	setSelectedValue("wireguard_server_subnet_access",getServerVarWithDefault("lan_access","true"));
	setSelectedValue("wireguard_server_redirect_gateway",getServerVarWithDefault("all_client_traffic","true"));
	document.getElementById("wireguard_server_privkey").value = getServerVarWithDefault("private_key","");
	document.getElementById("wireguard_server_pubkey").value = getServerVarWithDefault("public_key","");
	if(wgStatus != "Interface wg0 not found")
	{
		var wgStatusJSON = JSON.parse(wgStatus);
		var wgStatusStr = "";
		var txtCol = "#880000";
		if(wgStatusJSON["up"])
		{
			wgStatusStr = "Online, ";
			txtCol = "#008800";
		}
		else
		{
			wgStatusStr = "Offline, ";
		}
		if(wgStatusJSON["ipv4-address"] !== undefined)
		{
			wgStatusStr = wgStatusStr + "IP: " + wgStatusJSON["ipv4-address"][0]["address"];
		}
		else
		{
			wgStatusStr = wgStatusStr + "IP: -";
		}
		setChildText("wireguard_config_status", wgStatusStr, txtCol, true, null, document)
	}

	var acTableData = []
	var allowedClients = uciOriginal.getAllSectionsOfType("wireguard_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var rowData = []
		var id          = allowedClients[aci]
		var name        = uciOriginal.get("wireguard_gargoyle", id, "name")
		var ip          = uciOriginal.get("wireguard_gargoyle", id, "ip")
		var subnetIp   = uciOriginal.get("wireguard_gargoyle", id, "subnet_ip")
		var subnetMask = uciOriginal.get("wireguard_gargoyle", id, "subnet_mask")
		var enabled     = uciOriginal.get("wireguard_gargoyle", id, "enabled")
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""
		var pubkey     = uciOriginal.get("wireguard_gargoyle", id, "public_key")
		var haveprivkey     = uciOriginal.get("wireguard_gargoyle", id, "private_key") == "" ? false : true;

		var ipElementContainer = document.createElement("span")
		var naContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id

		rowData.push(name + "\n ")
		rowData.push(ipElementContainer)
		rowData.push(pubkey)
		
		var controls = createAllowedClientControls(haveprivkey)
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}

		enabled = enabled != "false" && enabled != "0" ? true : false;
		rowData[3].checked = enabled
		
		acTableData.push(rowData)
	}

	var acTable = createTable([ wgStr.ClntN, wgStr.IntIP, wgStr.wgPubKey, UI.Enabled, wgStr.ClntCfg, ""], acTableData, "wireguard_allowed_client_table", true, false, removeAcCallback)
	var tableContainer = document.getElementById("wireguard_allowed_client_table_container");
	while(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(acTable);

	//Client
	getClientVarWithDefault = function(variable, defaultDef) {
		var def = uciOriginal.get("wireguard_gargoyle", "client", variable)
		def = def == "" ? defaultDef : def
		return def
	}
	document.getElementById("wireguard_client_ip").value = getClientVarWithDefault("ip","10.64.0.2");
	document.getElementById("wireguard_client_server_pubkey").value = getClientVarWithDefault("server_public_key","");
	document.getElementById("wireguard_client_server_host").value = getClientVarWithDefault("server_host","");
	document.getElementById("wireguard_client_server_port").value = getClientVarWithDefault("server_port","51820");
	document.getElementById("wireguard_client_privkey").value = getClientVarWithDefault("private_key","");
	document.getElementById("wireguard_client_pubkey").value = getClientVarWithDefault("public_key","");
	document.getElementById("wireguard_client_allowed_ips").value = getClientVarWithDefault("allowed_ips","0.0.0.0/0");
	setSelectedValue("wireguard_client_allow_nonwg_traffic",getClientVarWithDefault("allow_nonwg_traffic","true"));
	if(uciOriginal.get("wireguard_gargoyle","client","enabled") == "1")
	{
		document.getElementById("wireguard_client_config_manual").checked = true;
	}

	setWireguardVisibility()
}

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
		
		var wgConfig = getSelectedValue("wireguard_config")

		configureFirewall = function(enabled,isServer,client2client,wgPort)
		{
			if(enabled)
			{
				uci.set("firewall", "wg_zone", "",        "zone")
				uci.set("firewall", "wg_zone", "name",    "wg")
				uci.set("firewall", "wg_zone", "device", "wg0")
				uci.set("firewall", "wg_zone", "input",   "ACCEPT")
				uci.set("firewall", "wg_zone", "output",  "ACCEPT")
				if(client2client)
				{
					uci.set("firewall", "wg_zone", "forward", "ACCEPT")
				}
				else
				{
					uci.set("firewall", "wg_zone", "forward", "REJECT")
				}
				uci.set("firewall", "wg_zone", "mtu_fix", "1")
				uci.set("firewall", "wg_zone", "masq",    "1")

				uci.set("firewall", "wg_lan_forwarding", "",     "forwarding")
				uci.set("firewall", "wg_lan_forwarding", "src",  "lan")
				uci.set("firewall", "wg_lan_forwarding", "dest", "wg")

				uci.set("firewall", "lan_wg_forwarding", "",     "forwarding")
				uci.set("firewall", "lan_wg_forwarding", "src",  "wg")
				uci.set("firewall", "lan_wg_forwarding", "dest", "lan")	

				if(isServer)
				{
					uci.set("firewall", "ra_wireguard", "",            "remote_accept")
					uci.set("firewall", "ra_wireguard", "zone",        "wan")
					uci.set("firewall", "ra_wireguard", "local_port",  wgPort)
					uci.set("firewall", "ra_wireguard", "remote_port", wgPort)
					uci.set("firewall", "ra_wireguard", "proto",       "udp")

					uci.set("firewall", "wg_wan_forwarding", "",     "forwarding")
					uci.set("firewall", "wg_wan_forwarding", "src",  "wg")
					uci.set("firewall", "wg_wan_forwarding", "dest", "wan")

					if(getSelectedValue("wireguard_server_subnet_access") != "true" )
					{
						uci.removeSection("firewall", "lan_wg_forwarding")
					}
				}
				else
				{
					uci.removeSection("firewall", "wg_wan_forwarding")
				}
			}
			else
			{
				uci.removeSection("firewall", "wg_zone")
				uci.removeSection("firewall", "lan_wg_forwarding")
				uci.removeSection("firewall", "wg_lan_forwarding")
				uci.removeSection("firewall", "wg_wan_forwarding")
				uci.removeSection("firewall", "ra_wireguard")
			}
		}

		configureAC = function(clientId,pubkey,ipArr,endpointHost,endpointPort)
		{
			uci.set("network", clientId, "",        "wireguard_wg0")
			uci.set("network", clientId, "public_key", pubkey)
			uci.createListOption("network", clientId, "allowed_ips", true)
			uci.set("network", clientId, "allowed_ips", ipArr)
			uci.set("network", clientId, "route_allowed_ips", "1")
			if(endpointHost != null && endpointPort != null)
			{
				uci.set("network", clientId, "endpoint_host", endpointHost)
				uci.set("network", clientId, "endpoint_port", endpointPort)
			}
		}

		configureNetwork = function(enabled,wgPrivKey,wgIP,wgPort)
		{
			if(enabled)
			{
				uci.set("network", "wg0", "",        "interface")
				uci.set("network", "wg0", "proto",    "wireguard")
				uci.set("network", "wg0", "private_key", wgPrivKey)
				uci.set("network", "wg0", "listen_port",   wgPort)
				uci.createListOption("network", "wg0", "addresses", true)
				uci.set("network", "wg0", "addresses",   [wgIP])
			}
			else
			{
				uci.removeSection("network", "wg0")
			}
		}

		if(wgConfig == "disabled")
		{
			configureFirewall(false)
			configureNetwork(false)
			uci.removeAllSectionsOfType("network","wireguard_wg0");
			uci.remove("gargoyle", "status", "wireguard_connections")
			uci.set("wireguard_gargoyle", "server", "enabled", "0")
			uci.set("wireguard_gargoyle", "client", "enabled", "0")
		}
		if(wgConfig == "server")
		{
			var prefix   = "wireguard_server_"
			var wgPort  = document.getElementById(prefix + "port").value
			var client_to_client = document.getElementById(prefix + "client_to_client").value == "true" ? true : false;
			configureFirewall(true,true,client_to_client,wgPort)

			uci.set("gargoyle", "status", "wireguard_connections", "501")

			uci.set("wireguard_gargoyle", "server", "enabled", "1")
			uci.set("wireguard_gargoyle", "client", "enabled", "0")

			var privkey = document.getElementById(prefix + "privkey").value;
			uci.set("wireguard_gargoyle", "server", "private_key", privkey)
			uci.set("wireguard_gargoyle", "server", "public_key", document.getElementById(prefix + "pubkey").value)
			var ip = document.getElementById(prefix + "ip").value;
			uci.set("wireguard_gargoyle", "server", "ip", ip)
			var submask = document.getElementById(prefix + "mask").value;
			var subcidr = parseCidr(submask);
			uci.set("wireguard_gargoyle", "server", "submask", submask)
			uci.set("wireguard_gargoyle", "server", "port", wgPort)
			uci.set("wireguard_gargoyle", "server", "c2c", getSelectedValue(prefix + "client_to_client"))
			uci.set("wireguard_gargoyle", "server", "lan_access", getSelectedValue(prefix + "subnet_access"))
			uci.set("wireguard_gargoyle", "server", "all_client_traffic", getSelectedValue(prefix + "redirect_gateway"))

			configureNetwork(true,privkey,ip + "/" + subcidr,wgPort);

			uci.removeAllSectionsOfType("network","wireguard_wg0");
			wgACs = uci.getAllSectionsOfType("wireguard_gargoyle","allowed_client");
			var wgACIdx = 0;
			for(wgACIdx = 0; wgACIdx < wgACs.length; wgACIdx ++)
			{
				if(uci.get("wireguard_gargoyle",wgACs[wgACIdx],"enabled") == "1")
				{
					var clientId = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"id");
					var pubkey = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"public_key");
					var ip = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"ip");
					var ipArr = [];
					ipArr.push(ip + "/32");
					var subnetip = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"subnet_ip");
					var subnetmask = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"subnet_mask");
					if(subnetip != "" && subnetmask != "")
					{
						subnetmask = parseCidr(subnetmask);
						ipArr.push(subnetip + "/" + subnetmask);
					}
					configureAC(clientId,pubkey,ipArr,null,null);
				}
			}
		}
		if(wgConfig == "client")
		{
			var prefix   = "wireguard_client_"
			configureFirewall(true,false,true)
			var privkey = document.getElementById(prefix + "privkey").value;
			var ip = document.getElementById(prefix + "ip").value;
			configureNetwork(true,privkey,ip + "/32","51820")

			uci.remove("gargoyle", "status", "wireguard_connections")

			uci.set("wireguard_gargoyle", "server", "enabled", "0")
			uci.set("wireguard_gargoyle", "client", "enabled", "1")

			uci.set("wireguard_gargoyle", "client", "private_key", privkey)
			uci.set("wireguard_gargoyle", "client", "public_key", document.getElementById(prefix + "pubkey").value)
			uci.set("wireguard_gargoyle", "client", "ip", ip)
			var allowed_ips = document.getElementById(prefix + "allowed_ips").value;
			uci.set("wireguard_gargoyle", "client", "allowed_ips", allowed_ips)
			uci.set("wireguard_gargoyle", "client", "allow_nonwg_traffic", document.getElementById(prefix + "allow_nonwg_traffic").value)
			var endpoint_host = document.getElementById(prefix + "server_host").value;
			var endpoint_port = document.getElementById(prefix + "server_port").value;
			uci.set("wireguard_gargoyle", "client", "server_host", endpoint_host)
			uci.set("wireguard_gargoyle", "client", "server_port", endpoint_port)
			var server_pubkey = document.getElementById(prefix + "server_pubkey").value;
			uci.set("wireguard_gargoyle", "client", "server_public_key", server_pubkey)
			uci.removeAllSectionsOfType("network","wireguard_wg0");
			configureAC("wgserver",server_pubkey,allowed_ips.split(','),endpoint_host,endpoint_port);
		}


		var commands = uci.getScriptCommands(uciOriginal) + "\n" ;
		// If we are doing anything to network config that isn't just adding clients, we need to restart
		if(commands.match(/(network\.wg0\.|firewall\.)/) != null)
		{
			commands = commands + "\n/usr/lib/gargoyle/restart_network.sh ;\n"
			commands = commands + "\n/etc/wireguard.firewall update_enabled ;\n"
		}
		else
		{
			commands = commands + "\nifup wg0;\n"
		}

		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				setTimeout(function () {
					//Give wireguard 5 seconds to come up.
					//It is much quicker than this, but it helps the status flow
					//through to the user in a more expected way if we wait
					window.location=window.location;
				}, 5000);		
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function setWireguardVisibility()
{
	mode = getSelectedValue("wireguard_config");
	if(mode == "disabled")
	{
		document.getElementById("wireguard_server_fieldset").style.display = "none";
		document.getElementById("wireguard_allowed_client_fieldset").style.display = "none";
		document.getElementById("wireguard_client_fieldset").style.display = "none";
	}
	else
	{
		document.getElementById("wireguard_server_fieldset").style.display = mode == "server" ? "block" : "none";
		document.getElementById("wireguard_allowed_client_fieldset").style.display = mode == "server" ? "block" : "none";
		document.getElementById("wireguard_client_fieldset").style.display = mode == "client" ? "block" : "none";
	}
	document.getElementById("wireguard_config_status_container").style.display = mode == "disabled" ? "none" : "block";

	setClientVisibility()
}

function togglePass(name)
{
	password_field = document.getElementById(name);
	if(password_field.type == 'password')
	{
		password_field.type = 'text';
	}
	else
	{
		password_field.type = 'password';
	}
}

function generateKeyPair(section)
{
	commands = "mkdir -p /tmp/wireguard\ncd /tmp/wireguard\nwg genkey | tee ./privatekey | wg pubkey > ./publickey\ncat /tmp/wireguard/*";

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var lines = req.responseText;
			lines = lines.split("\n");
			if(lines.length >= 2)
			{
				if(section == "server")
				{
					document.getElementById("wireguard_server_privkey").value = lines[0];
					document.getElementById("wireguard_server_pubkey").value = lines[1];
				}
				if(section == "allowed_client")
				{
					document.getElementById("wireguard_allowed_client_privkey").value = lines[0];
					document.getElementById("wireguard_allowed_client_pubkey").value = lines[1];
				}
				if(section == "client")
				{
					document.getElementById("wireguard_client_privkey").value = lines[0];
					document.getElementById("wireguard_client_pubkey").value = lines[1];
				}
			}
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function createAllowedClientControls(haveDownload)
{

	var enabledCheck = createInput("checkbox")
	enabledCheck.onclick = toggleAcEnabled;
	var downloadButton = haveDownload ? createButton(wgStr.Dload, "btn-download", downloadAc, false) : createButton(wgStr.Dload, "btn-download disabled", function(){ return; }, true ) ;

	var editButton = createButton(UI.Edit, "btn-edit", editWgClientModal, false)

	return [enabledCheck, downloadButton, editButton]
}

function createButton(text, cssClass, actionFunction, disabled)
{
	var button = createInput("button")
	button.textContent = text
	button.className = "btn btn-default " + cssClass
	button.onclick = actionFunction
	button.disabled = disabled
	return button;
}

function toggleAcEnabled()
{
	var toggleRow=this.parentNode.parentNode;
	var toggleId = toggleRow.childNodes[1].firstChild.id;

	uci.set("wireguard_gargoyle", toggleId, "enabled", (this.checked? "1" : "0"));
}

function downloadAc()
{
	var downloadRow=this.parentNode.parentNode;
	var downloadId = downloadRow.childNodes[1].firstChild.id;
	var wgServerIP = uci.get("wireguard_gargoyle","server","ip");
	var wgServerLanMask = uci.get("wireguard_gargoyle","server","submask");
	var allClientTraffic = uci.get("wireguard_gargoyle","server","all_client_traffic");
	// Generate config
	commands = [];
	commands.push("touch /tmp/wg.ac.tmp.conf");
	commands.push("rm /tmp/wg.ac.tmp.conf");
	commands.push("touch /tmp/wg.ac.tmp.conf");
	
	// Create Interface Section
	var intaddr = uci.get("wireguard_gargoyle",downloadId,"ip") + "/32";
	var intprivkey = uci.get("wireguard_gargoyle",downloadId,"private_key");
	commands.push("echo '[Interface]' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo 'Address = " + intaddr + "' >> /tmp/wg.ac.tmp.conf");
	if(allClientTraffic == "true")
	{
		commands.push("echo 'DNS = " + wgServerIP + "' >> /tmp/wg.ac.tmp.conf");
	}
	commands.push("echo 'PrivateKey = " + intprivkey + "' >> /tmp/wg.ac.tmp.conf");
	
	// Create Peer Section
	var prroutedips = ["0.0.0.0/0"];
	var prendpoint = uci.get("wireguard_gargoyle",downloadId,"remote") + ":" + uci.get("wireguard_gargoyle","server","port");
	var prpubkey = uci.get("wireguard_gargoyle","server","public_key");
	if(allClientTraffic == "false")
	{
		prroutedips = [ipToStr(parseIp(wgServerIP) & parseIp(wgServerLanMask)) + "/" + parseCidr(wgServerLanMask)];
		prroutedips.push(ipToStr(parseIp(currentLanIp) & parseIp(currentLanMask)) + "/" + parseCidr(currentLanMask));
		wgACs = uci.getAllSectionsOfType("wireguard_gargoyle","allowed_client");
		var wgACIdx = 0;
		for(wgACIdx = 0; wgACIdx < wgACs.length; wgACIdx ++)
		{
			if(uci.get("wireguard_gargoyle",wgACs[wgACIdx],"enabled") == "1" && uci.get("wireguard_gargoyle",wgACs[wgACIdx],"id") != downloadId)
			{
				var subnetip = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"subnet_ip");
				var subnetmask = uci.get("wireguard_gargoyle",wgACs[wgACIdx],"subnet_mask");
				if(subnetip != "" && subnetmask != "")
				{
					subnetmask = parseCidr(subnetmask);
					prroutedips.push(subnetip + "/" + subnetmask);
				}
			}
		}
	}
	commands.push("echo '' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo '[Peer]' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo 'AllowedIPs = " + prroutedips.join(",") + "' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo 'Endpoint = " + prendpoint + "' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo 'PersistentKeepalive = 25' >> /tmp/wg.ac.tmp.conf");
	commands.push("echo 'PublicKey = " + prpubkey + "' >> /tmp/wg.ac.tmp.conf");

	commands = commands.join("\n");

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			// Download
			setControlsEnabled(true);
			window.location="/utility/wireguard_download_credentials.sh?id=" + downloadId
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function removeAcCallback(table, row)
{
	var id = row.childNodes[1].firstChild.id;
	uci.removeSection("wireguard_gargoyle", id);
}

function addWgClientModal()
{
	modalButtons = [
		{"title" : UI.Add, "classes" : "btn btn-primary", "function" : addAc},
		"defaultDismiss"
	];

	modalElements = [];

	setAcDocumentFromUci(new UCIContainer(), "dummy", false, document.getElementById("wireguard_server_ip").value )
	modalPrepare('wireguard_allowed_client_modal', wgStr.ClntCfg, modalElements, modalButtons);
	openModalWindow('wireguard_allowed_client_modal');
}

function editWgClientModal()
{
	editRow=this.parentNode.parentNode;
	var editId = editRow.childNodes[1].firstChild.id;
	var serverInternalIp = document.getElementById("wireguard_server_ip").value;
	var serverInternalMask = document.getElementById("wireguard_server_mask").value;

	modalButtons = [
		{"title" : UI.CApplyChanges, "classes" : "btn btn-primary", "function" : function(){editAc(editRow,editId,serverInternalIp,serverInternalMask);}},
		"defaultDiscard"
	];

	modalElements = [];

	setAcDocumentFromUci(uci, editId, false, serverInternalIp);

	modalPrepare('wireguard_allowed_client_modal', wgStr.EditWCS, modalElements, modalButtons);
	openModalWindow('wireguard_allowed_client_modal');
}

function setAcDocumentFromUci(srcUci, id, dupeCn, serverInternalIp)
{
	var name = srcUci.get("wireguard_gargoyle", id, "name")
	
	if( srcUci.get("wireguard_gargoyle", id, "remote") == "" )
	{
		var allIdList = getDefinedAcIds(false)
		var allIdHash = getDefinedAcIds(true)
		var clientCount = allIdList.length +1
		name = wgStr.Clnt + clientCount
		id = "client" + clientCount
		while(allIdHash[id] == 1)
		{
			clientCount++
			name = wgStr.Clnt + clientCount
			id = "client" + clientCount
		}
		document.getElementById("wireguard_allowed_client_default_id").value = id
	}
	else
	{
		document.getElementById("wireguard_allowed_client_initial_id").value = id
	}

	document.getElementById("wireguard_allowed_client_name").value = name
	

	var ip = srcUci.get("wireguard_gargoyle", id, "ip")
	if(ip == "")
	{
		ip = getUnusedAcIp(serverInternalIp)

	}
	document.getElementById("wireguard_allowed_client_ip").value = ip
	
	setRemoteNames(srcUci.get("wireguard_gargoyle", id, "remote"))

	var subnetIp   = srcUci.get("wireguard_gargoyle", id, "subnet_ip")
	var subnetMask = srcUci.get("wireguard_gargoyle", id, "subnet_mask")

	setSelectedValue("wireguard_allowed_client_have_subnet", (subnetIp != "" && subnetMask != "" ? "true" : "false"), document)
	subnetIp   = subnetIp   == "" ? "192.168.2.1" : subnetIp;
	subnetMask = subnetMask == "" ? "255.255.255.0" : subnetMask;
	document.getElementById("wireguard_allowed_client_subnet_ip").value = subnetIp;
	document.getElementById("wireguard_allowed_client_subnet_mask").value = subnetMask;

	var pubkey = srcUci.get("wireguard_gargoyle", id, "public_key")
	var privkey = srcUci.get("wireguard_gargoyle", id, "private_key")
	setSelectedValue("wireguard_allowed_client_have_privkey", (privkey != "" ? "true" : "false"), document)
	document.getElementById("wireguard_allowed_client_pubkey").value = pubkey;
	document.getElementById("wireguard_allowed_client_privkey").value = privkey;

	setAllowedClientVisibility();
}

function getDefinedAcIps(retHash)
{
	var ips = []
	var allowedClients = getDefinedAcIds(false)
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var ip = uci.get("wireguard_gargoyle", allowedClients[aci], "ip")
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
	var allowedClients = uci.getAllSectionsOfType("wireguard_gargoyle", "allowed_client")
	var aci;
	for(aci=0; aci < allowedClients.length; aci++)
	{
		var id = allowedClients[aci]
		var enabled = uci.get("wireguard_gargoyle", id, "enabled")
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

function addAc()
{
	var errors = validateAc(document.getElementById("wireguard_server_ip").value , document.getElementById("wireguard_server_mask").value );
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+wgStr.AddCErr);
	}
	else
	{
		var name       = document.getElementById("wireguard_allowed_client_name").value
		var ip         = document.getElementById("wireguard_allowed_client_ip").value
		var subnetIp   = ""
		var subnetMask = ""
		if( getSelectedValue("wireguard_allowed_client_have_subnet", document) == "true")
		{
			subnetIp   = document.getElementById("wireguard_allowed_client_subnet_ip").value
			subnetMask = document.getElementById("wireguard_allowed_client_subnet_mask").value
		}
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""
		var pubkey       = document.getElementById("wireguard_allowed_client_pubkey").value
	
		var id = name.replace(/[\t\r\n ]+/g, "_").toLowerCase().replace(/[^a-z0-9_-]/g, "");
		var idCount = 1;
		var testId = id
		while(uci.get("wireguard_gargoyle", testId) != "")
		{
			testId = id + "_" + idCount
			idCount++
		}
		id = testId

		setAcUciFromDocument(id)
		uci.set("wireguard_gargoyle", id, "enabled", "1")

		var ipElementContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = id

		var acTable = document.getElementById("wireguard_allowed_client_table");

		var rowData = [ name, ipElementContainer, pubkey ]
		var controls = createAllowedClientControls(false)
		while(controls.length > 0)
		{
			rowData.push( controls.shift() )
		}
		rowData[3].checked = true
		addTableRow(acTable, rowData, true, false, removeAcCallback);
	
		setAcDocumentFromUci(new UCIContainer(), "dummy", false, document.getElementById("wireguard_server_ip").value )
		closeModalWindow('wireguard_allowed_client_modal');
	}
}

function setRemoteNames(selectedRemote)
{
	var selectId = "wireguard_allowed_client_remote";
	selectedRemote = selectedRemote == null ? "" : selectedRemote;

	var names = []
	var values = []
	
	var definedDdns = uciOriginal.getAllSectionsOfType("ddns_gargoyle", "service")
	var ddi
	var selectedFound = false
	for(ddi=0; ddi < definedDdns.length; ddi++)
	{
		var enabled = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "enabled")
		var domain  = uciOriginal.get("ddns_gargoyle", definedDdns[ddi], "domain").replace("@",".")
		if( (enabled != "0" && enabled != "false") && domain != "")
		{
			names.push(wgStr.DDNS+": " + domain)
			values.push(domain)
			selectedFound = selectedRemote == domain ? true : selectedFound
		}
	}
	selectedFound = (selectedRemote == currentWanIp) || selectedFound
	if(currentWanIp)
	{
		names.push("WAN IP: " + currentWanIp)
		values.push(currentWanIp)
	}
	names.push(wgStr.OthIPD)
	values.push("custom")
	
	setAllowableSelections(selectId, values, names, document)
	var chosen = selectedRemote == "" ? values[0] : selectedRemote
	chosen = (!selectedFound) && selectedRemote != "" ? "custom" : selectedRemote
	setSelectedValue(selectId, chosen, document)
	if(chosen == "custom")
	{
		document.getElementById("wireguard_allowed_client_remote_custom").value = selectedRemote
	}
}

function setAllowedClientVisibility()
{
	var selectedVis = document.getElementById("wireguard_allowed_client_remote_container").style.display == "none" ? "none" : "block"
	document.getElementById("wireguard_allowed_client_remote_custom_container").style.display  = getSelectedValue("wireguard_allowed_client_remote", document) == "custom" ? selectedVis : "none";

	var selectedVis = document.getElementById("wireguard_allowed_client_have_subnet_container").style.display == "none" ? "none" : "block"
	document.getElementById("wireguard_allowed_client_subnet_ip_container").style.display = getSelectedValue("wireguard_allowed_client_have_subnet", document) == "true" ? selectedVis : "none";
	document.getElementById("wireguard_allowed_client_subnet_mask_container").style.display = getSelectedValue("wireguard_allowed_client_have_subnet", document) == "true" ? selectedVis : "none";

	var selectedVis = document.getElementById("wireguard_allowed_client_have_privkey_container").style.display == "none" ? "none" : "block"
	document.getElementById("wireguard_allowed_client_privkey_container").style.display = getSelectedValue("wireguard_allowed_client_have_privkey", document) == "true" ? selectedVis : "none";
	document.getElementById("generate_allowed_client_keys_button").disabled = getSelectedValue("wireguard_allowed_client_have_privkey", document) == "true" ? false : true;
	if(getSelectedValue("wireguard_allowed_client_have_privkey", document) == "true")
	{
		document.getElementById("wireguard_allowed_client_pubkey").setAttribute("readonly", true);
	}
	else
	{
		document.getElementById("wireguard_allowed_client_pubkey").removeAttribute("readonly");
	}
}

function validateAc(internalServerIp, internalServerMask)
{
	var validateHaveText = function(txt) {  return txt.length > 0 ? 0 : 1 }

	var prefix = "wireguard_allowed_client_"
	var inputIds = [ prefix + "name", prefix + "pubkey", prefix + "privkey", prefix + "ip", prefix + "remote_custom", prefix + "subnet_ip", prefix + "subnet_mask" ]
	var labelIds = [ prefix + "name_label", prefix + "pubkey_label", prefix + "privkey", prefix + "ip_label", prefix + "remote_label",  prefix + "have_subnet_label", prefix + "have_subnet_label" ]
	var functions = [ validateHaveText, validateHaveText, validateHaveText, validateIP, validateHaveText, validateIP, validateNetMask  ];
	var validReturnCodes = [0,0,0,0,0,0,0]
	var visibilityIds = [  prefix + "name_container", prefix + "pubkey_container", prefix + "privkey_container", prefix + "ip_container", prefix + "remote_custom_container", prefix + "subnet_ip_container", prefix + "subnet_mask_container" ]

	var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, visibilityIds, document );
	if(errors.length == 0 && document.getElementById(prefix + "ip_container").style.display != "none")
	{
		var testIp  = parseIp(document.getElementById(prefix + "ip").value)
		var wgIp   = parseIp(internalServerIp)
		var wgMask = parseMask(internalServerMask)
		if( ( testIp & wgMask ) != ( wgIp & wgMask ) )
		{
			errors.push(wgStr.ClntIntIP+" " + document.getElementById(prefix + "ip").value + " "+wgStr.OSubErr)
		}
	}
	if(errors.length == 0)
	{
		var name = document.getElementById(prefix + "name").value;
		var id = name.replace(/[\t\r\n ]+/g, "_").toLowerCase().replace(/[^a-z0-9_-]/g, "");
		if(id == "lan" || id == "wan" || id == "tun0" || id == "wg0" || id == "wgserver")
		{
			errors.push(wgStr.ClntNotAllow);
		}
	}
	if(errors.length == 0 && document.getElementById(prefix + "subnet_ip_container").style.display != "none")
	{
		var subnetIpEl   = document.getElementById(prefix + "subnet_ip")
		var subnetMaskEl = document.getElementById(prefix + "subnet_mask")
		subnetIpEl.value = applyMask(subnetIpEl.value, subnetMaskEl.value)
	}

	return errors;
}

function parseMask(mask)
{
	if(mask.match(/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/))
	{
		return parseIp(mask)
	}
	else
	{
		return -1<<(32-mask)
	}
}

function applyMask(ip, mask)
{
	return ipToStr( parseIp(ip) & parseMask(mask) )
}

function setAcUciFromDocument(id)
{
	var name = document.getElementById("wireguard_allowed_client_name").value;
	
	var ipContainer = document.getElementById("wireguard_allowed_client_ip_container")
	var ip = document.getElementById("wireguard_allowed_client_ip").value
	ip = ipContainer.style.display == "none" ? "" : ip
	
	var remote = getSelectedValue("wireguard_allowed_client_remote", document)
	remote = remote == "custom" ? document.getElementById("wireguard_allowed_client_remote_custom").value : remote
	
	var haveSubnet = getSelectedValue("wireguard_allowed_client_have_subnet", document) == "true" ? true : false
	haveSubnet     = ipContainer.style.display == "none" ? false : haveSubnet
	var subnetIp   = document.getElementById("wireguard_allowed_client_subnet_ip").value
	var subnetMask = document.getElementById("wireguard_allowed_client_subnet_mask").value

	var havePrivkey = getSelectedValue("wireguard_allowed_client_have_privkey", document) == "true" ? true : false
	var privkey = document.getElementById("wireguard_allowed_client_privkey").value
	var pubkey = document.getElementById("wireguard_allowed_client_pubkey").value

	var pkg = "wireguard_gargoyle"
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
	else
	{
		uci.remove(pkg, id, "subnet_ip")
		uci.remove(pkg, id, "subnet_mask")
	}
	uci.set(pkg, id, "public_key", pubkey)
	if(havePrivkey)
	{
		uci.set(pkg, id, "private_key",   privkey)
	}
	else
	{
		uci.remove(pkg, id, "private_key")
	}
}

function editAc(editRow,editId,serverInternalIp,serverInternalMask)
{
	var errors = validateAc(serverInternalIp, serverInternalMask);
	if(errors.length > 0)
	{
		alert(errors.join("\n") + "\n"+wgStr.UpCErr);
	}
	else
	{
		var name       = document.getElementById("wireguard_allowed_client_name").value
		var ip         = document.getElementById("wireguard_allowed_client_ip").value
		var subnetIp   = ""
		var subnetMask = ""
		if(getSelectedValue("wireguard_allowed_client_have_subnet", document) == "true")
		{
			subnetIp   = document.getElementById("wireguard_allowed_client_subnet_ip").value
			subnetMask = document.getElementById("wireguard_allowed_client_subnet_mask").value
		}
		var subnet = subnetIp != "" && subnetMask != "" ? subnetIp + "/" + subnetMask : ""
		var pubkey   = document.getElementById("wireguard_allowed_client_pubkey").value

		setAcUciFromDocument(editId)
					
		while( editRow.childNodes[0].firstChild != null)
		{
			editRow.childNodes[0].removeChild( editRow.childNodes[0].firstChild )
		}
		editRow.childNodes[0].appendChild(document.createTextNode(name))

		var ipElementContainer = document.createElement("span")
		var ipContainer = document.createElement("span")
		ipContainer.appendChild( document.createTextNode(ip) )
		ipContainer.appendChild( document.createElement("br") )
		ipContainer.appendChild( document.createTextNode(subnet) )
		ipElementContainer.appendChild(ipContainer)
		ipElementContainer.id = editId

		while( editRow.childNodes[1].firstChild != null)
		{
			editRow.childNodes[1].removeChild( editRow.childNodes[1].firstChild )
		}						
		editRow.childNodes[1].appendChild( ipElementContainer )

		while( editRow.childNodes[2].firstChild != null)
		{
			editRow.childNodes[2].removeChild( editRow.childNodes[2].firstChild )
		}
		editRow.childNodes[2].appendChild(document.createTextNode(pubkey))
		closeModalWindow('wireguard_allowed_client_modal');
	}
}

function setClientVisibility()
{
	var upCheckEl  = document.getElementById("wireguard_client_config_upload");
	var manCheckEl = document.getElementById("wireguard_client_config_manual");

	if( (!upCheckEl.checked) && (!manCheckEl.checked) )
	{
		upCheckEl.checked = true;
	}

	if(upCheckEl.checked)
	{
		document.getElementById("wireguard_client_manual_config").style.display = "none";
		document.getElementById("wireguard_client_upload_config").style.display = "block";
	}
	else
	{
		document.getElementById("wireguard_client_manual_config").style.display = "block";
		document.getElementById("wireguard_client_upload_config").style.display = "none";
	}
}

function doUpload()
{
	if(document.getElementById('wireguard_client_config_file').value.length == 0)
	{
		alert(wgStr.SelErr);
	}
	else
	{
		document.getElementById('wireguard_client_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
		document.getElementById('wireguard_client_form').submit();
		setControlsEnabled(false, true, wgStr.Uping);
	}
}

function uploaded()
{
	document.getElementById("wireguard_client_config_file").value = "";
	setControlsEnabled(true, false);
	uploadFrame = document.getElementById("client_add_target");
	uploadFrameDoc = (uploadFrame.contentDocument) ? uploadFrame.contentDocument : uploadFrame.contentWindow.document;

	cfgcontents = uploadFrameDoc.getElementById("cfgcontents").innerText;
	parseCfg(cfgcontents.split("\n"));
}

function parseCfg(cfgdata)
{
	// Look for sections first
	interfaceStart = cfgdata.indexOf("[Interface]");
	interfaceStop = interfaceStart;
	peerStart = cfgdata.indexOf("[Peer]");
	peerStop = peerStart;

	if(interfaceStart > -1 && peerStart > -1)
	{
		// Find section ends
		var cfgdataidx = 0;		
		for(cfgdataidx = interfaceStart+1; cfgdataidx < cfgdata.length; cfgdataidx++)
		{
			if(cfgdata[cfgdataidx] == "" || cfgdata[cfgdataidx].match(/^\[/) != null)
			{
				interfaceStop = cfgdataidx;
				break;
			}
			if(cfgdataidx == cfgdata.length -1)
			{
				interfaceStop = cfgdataidx;
			}
		}
		for(cfgdataidx = peerStart+1; cfgdataidx < cfgdata.length; cfgdataidx++)
		{
			if(cfgdata[cfgdataidx] == "" || cfgdata[cfgdataidx].match(/^\[/) != null)
			{
				peerStop = cfgdataidx;
				break;
			}
			if(cfgdataidx == cfgdata.length -1)
			{
				peerStop = cfgdataidx;
			}
		}
		// Do interface
		var cfgdataidx = 0;		
		for(cfgdataidx = interfaceStart+1; cfgdataidx <= interfaceStop; cfgdataidx++)
		{
			var lineParts = cfgdata[cfgdataidx].split(" = ");
			if(lineParts[0] == "Address")
			{
				var subLineParts = lineParts[1].split("/");
				document.getElementById("wireguard_client_ip").value = subLineParts[0];
			}
			else if(lineParts[0] == "PrivateKey")
			{
				document.getElementById("wireguard_client_privkey").value = lineParts[1];
				setPubkeyFromPrivkey(lineParts[1],"wireguard_client_pubkey");
			}
		}
		// Do peer (server)
		for(cfgdataidx = peerStart+1; cfgdataidx <= peerStop; cfgdataidx++)
		{
			var lineParts = cfgdata[cfgdataidx].split(" = ");
			if(lineParts[0] == "AllowedIPs")
			{
				document.getElementById("wireguard_client_allowed_ips").value = lineParts[1];
			}
			else if(lineParts[0] == "Endpoint")
			{
				var subLineParts = lineParts[1].split(":");
				document.getElementById("wireguard_client_server_host").value = subLineParts[0];
				document.getElementById("wireguard_client_server_port").value = subLineParts[1];
			}
			else if(lineParts[0] == "PublicKey")
			{
				document.getElementById("wireguard_client_server_pubkey").value = lineParts[1];
			}
		}
		wireguard_client_config_manual.checked = true;
		wireguard_client_config_upload.checked = false;
		setClientVisibility();
	}
	else
	{
		alert(wgStr.BadCfg);
	}
}

function setPubkeyFromPrivkey(privkey, section)
{
	commands = "echo \"" + privkey + "\" | wg pubkey";

	var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var lines = req.responseText;
			lines = lines.split("\n");
			if(lines.length >= 1)
			{
				document.getElementById(section).value = lines[0];
			}
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function proofreadAll()
{
	var validateHaveText = function(txt) {  return txt.length > 0 ? 0 : 1 }
	errors = [];
	if(getSelectedValue("wireguard_config") == "server")
	{
		var prefix = "wireguard_server_"
		var inputIds = [ prefix + "privkey", prefix + "pubkey", prefix + "ip", prefix + "mask", prefix + "port" ]
		var labelIds = [ prefix + "privkey_label", prefix + "pubkey_label", prefix + "ip_label", prefix + "mask_label", prefix + "port_label" ]
		var functions = [ validateHaveText, validateHaveText, validateIP, validateNetMask, validatePort ];
		var validReturnCodes = [0,0,0,0,0]

		var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, inputIds, document );
	
		if(errors.length == 0)
		{
			// Additional checks
			// port clash? ip clash?
		}
	}
	if(getSelectedValue("wireguard_config") == "client")
	{
		if(document.getElementById("wireguard_client_manual_config").style.display != "none" )
		{
			var prefix = "wireguard_client_"
			var inputIds = [ prefix + "server_pubkey", prefix + "server_host", prefix + "server_port", prefix + "privkey", prefix + "pubkey", prefix + "ip" ]
			var labelIds = [ prefix + "server_pubkey_label", prefix + "server_host_label", prefix + "server_port_label", prefix + "privkey_label", prefix + "pubkey_label", prefix + "ip_label" ]
			var functions = [ validateHaveText, validateHaveText, validatePort, validateHaveText, validateHaveText, validateIP ];
			var validReturnCodes = [0,0,0,0,0,0]

			var errors = proofreadFields(inputIds, labelIds, functions, validReturnCodes, inputIds, document );
	
			if(errors.length == 0)
			{
				// Additional checks
				// port clash? ip clash?
			}
		}
		else
		{
			errors.push(wgStr.noClientCfg);
		}
	}
	return errors;
}
