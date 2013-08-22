/*
 * This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var torS=new Object(); //part of i18n

// save timeout variables
var maxSaveWait=90
var saveCount=0


// initialize conversion constants
var mask2bits = []
mask2bits[255] = 8
mask2bits[254] = 7
mask2bits[252] = 6
mask2bits[248] = 5
mask2bits[240] = 4
mask2bits[224] = 3
mask2bits[192] = 2
mask2bits[128] = 1
mask2bits[0]   = 0

var bits2mask = []
var m;
for( m in mask2bits )
{
	bits2mask[ mask2bits[m] ] = m
}
// end conversion constant initialization

var driveToPath = [];


function saveChanges()
{
	errorList = proofreadAll();
	if(errorList.length > 0)
	{
		errorString = errorList.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
	}
	else
	{
		setControlsEnabled(false, true);
		var uci = uciOriginal.clone();
		var torClientMode = getSelectedValue("tor_client_mode")
		var torClientConnect = getSelectedValue("tor_client_connect")
		var torRelayMode  = getSelectedValue("tor_relay_mode")
		
		torRelayMode = torRelayMode == "3" ? "1" : torRelayMode
		uci.set('tor', 'global', 'enabled', (torClientMode=="0" && torRelayMode == "0" ? "0" : "1") )
		uci.set('tor', 'client', 'client_mode', torClientMode)
		uci.set('tor', 'relay',  'relay_mode',  torRelayMode)
		if(torClientMode != "0")
		{
			uci.set('tor', 'client', 'hidden_service_subnet',    document.getElementById("tor_hidden_subnet").value)
			uci.set('tor', 'client', 'hidden_service_mask_bits', maskToBits(document.getElementById("tor_hidden_mask").value))
			if(torClientMode != "3")
			{
				uci.set('tor', 'client', 'block_unsupported_proto',  getSelectedValue("tor_other_proto"))
			}
			if(torClientConnect != "relay")
			{
				uci.set('tor', 'client', 'use_bridge_ip', document.getElementById("tor_client_bridge_ip").value)
				uci.set('tor', 'client', 'use_bridge_port', document.getElementById("tor_client_bridge_port").value)
				uci.set('tor', 'client', 'use_bridge_obfsproxy', torClientConnect == "obfsproxy" ? "1" : "0")
			}
			else
			{
				uci.remove('tor', 'client', 'use_bridge_ip');
				uci.remove('tor', 'client', 'use_bridge_port');
				uci.remove('tor', 'client', 'use_bridge_obfsproxy');
			}

		}
		if(torRelayMode != "0")
		{
			var rvars = [
				["relay_port",     "tor_relay_port"], 
				["obfsproxy_port", "tor_obfsproxy_port"],
				["max_bw_rate_kb", "tor_relay_max_bw"],
				["relay_contact",  "tor_relay_contact"],
				["relay_nickname", "tor_relay_nickname"],

			];
			var rvIndex=0
			for(rvIndex=0; rvIndex < rvars.length; rvIndex++)
			{
				var setEl  = document.getElementById(  rvars[rvIndex][1] )
				var setVal = document.getElementById(  rvars[rvIndex][1] ).value
				setVal.replace(/[\r\n]+/, "") //make sure contact data contains no newlines
				uci.set('tor', 'relay', rvars[rvIndex][0], setVal )
			}
			if( !getSelectedText("tor_relay_mode").match(/proxy/) )
			{
				uci.set('tor', 'relay', 'obfsproxy_port', "0")
			}

			var publish = torRelayMode == "2" || getSelectedValue("tor_relay_publish") == "1" ? "1" : "0" ;
			uci.set('tor', 'relay', 'publish', publish);


			//uci.set('tor', 'relay', 'max_bw_burst_kb', "" + (parseInt(document.getElementById('tor_relay_max_bw').value)*2) )

		}
		var dataDrive = getSelectedValue("tor_dir_drive_select")
		if(dataDrive != "ramdisk" && dataDrive != "" && dataDrive != null)
		{
			var dataDrivePath = dataDrive == "root" ? "/usr/lib/tor" : driveToPath[dataDrive]
			var dataDriveDir  = dataDrive == "root" ? "/usr/lib/tor" : document.getElementById("tor_dir_text").value
			dataDriveDir = (dataDriveDir[0] == '/' ? "" : "/" ) + dataDriveDir
			dataDirPath = dataDrive == "root" ? dataDriveDir : dataDrivePath + dataDriveDir

			uci.set("tor", "global", "data_dir", dataDirPath)
			uci.set("tor", "global", "data_drive", dataDrive)
			uci.set("tor", "global", "data_drive_dir", dataDriveDir)
		}
		else
		{
			uci.set("tor", "global", "data_dir", "/var/tor")
			uci.remove("tor", "global", "data_drive")
			uci.remove("tor", "global", "data_drive_dir")
		}


		var commands = uci.getScriptCommands(uciOriginal) + "\n" + "/etc/init.d/tor restart" + "\n";
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				var torClientMode = getSelectedValue("tor_client_mode")
				var torRelayMode  = getSelectedValue("tor_relay_mode")
				uciOriginal = uci.clone()
				resetData()
				if(torClientMode == "0" && torRelayMode == "0" )
				{
					setControlsEnabled(true)
				}
				else
				{
					saveCount=0
					savePartTwo()
				}
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function savePartTwo()
{
	var statusCommand = "printf  \"Authenticate \\\"\\\"\\nGETINFO status/bootstrap-phase\\nGETINFO status/reachability-succeeded/or\\n\" | nc 127.0.0.1 9051 | grep status"
	var param = getParameterDefinition("commands", statusCommand) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""))
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var done = false
			var lines = req.responseText.split(/[\r\n]+/)
			var i;
			for(i=0; i < lines.length; i++)
			{
				line = lines[i]
				done = line.match(/PROGRESS=100/) ? true : done
			}
			
			saveCount++
			if(done)
			{
				setControlsEnabled(true)
			}
			else if(saveCount == maxSaveWait)
			{
				alert(torS.ConProb)
				setControlsEnabled(true)
			}
			else
			{
				setTimeout(savePartTwo, 1000)
			}
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction)
}



function proofreadAll()
{
	var inputIds    = ["tor_client_bridge_ip", "tor_client_bridge_port", "tor_hidden_subnet", "tor_hidden_mask", 'tor_relay_port', 'tor_relay_max_bw' ]
	var functions   = [ validateIP, validatePort, validateIP, validateNetMask, validatePort, validateNumeric ]
	var labelIds    = []
	var returnCodes = []
	var visIds      = []
	var i
	for(i=0; i < inputIds.length; i++)
	{
		var id = inputIds[i]
		labelIds.push( id + "_label")
		returnCodes.push(0)
		visIds.push( id + "_container")
	}

	var errors = proofreadFields(inputIds, labelIds, functions, returnCodes, visIds);
	

	if(getSelectedValue("tor_relay_mode") != "0" )
	{
		var relayPort = document.getElementById("tor_relay_port").value ;
		var obfsPort  = document.getElementById("tor_obfsproxy_port_container").display != "none" ? document.getElementById("tor_obfsproxy_port").value : "";
	

		var dropbearSections = uciOriginal.getAllSections("dropbear"); 
		var sshPort = uciOriginal.get("dropbear", dropbearSections[0], "Port");

		var httpsPort = uciOriginal.get("httpd_gargoyle", "server", "https_port");
		var httpPort = uciOriginal.get("httpd_gargoyle", "server", "http_port");



		var remoteHttpsPort = "";
		var remoteHttpPort  = "";
		var remoteSshPort = "";
		var remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept")
		var acceptIndex=0;
		for(acceptIndex=0; acceptIndex < remoteAcceptSections.length; acceptIndex++)
		{
			var section = remoteAcceptSections[acceptIndex];
			var localPort = uciOriginal.get("firewall", section, "local_port");
			var remotePort = uciOriginal.get("firewall", section, "remote_port");
			var proto = uciOriginal.get("firewall", section, "proto").toLowerCase();
			var zone = uciOriginal.get("firewall", section, "zone").toLowerCase();
			if((zone == "wan" || zone == "") && (proto == "tcp" || proto == ""))
			{
				remotePort = remotePort == "" ? localPort : remotePort;
				if(localPort == httpsPort)
				{
					remoteHttpsPort = remotePort;
				}
				else if(localPort == httpPort)
				{
					remoteHttpPort = remotePort;
				}
				else if(localPort == sshPort)
				{
					remoteSshPort = remotePort;
				}
			}
		}

		var relayType = getSelectedValue("tor_relay_mode") == "1" ? "Relay" : "Bridge";
		if(relayPort == obfsPort)
		{
			errors.push( relayType + " "+torS.ObPeBErr);
		}
		if(relayPort == httpPort || relayPort == httpsPort || relayPort == remoteHttpPort || relayPort == remoteHttpsPort)
		{
			errors.push( relayType + " "+torS.RPeWSErr);
		}
		if(obfsPort == httpPort || obfsPort == httpsPort || obfsPort == remoteHttpPort || obfsPort == remoteHttpsPort)
		{
			errors.push( relayType + " "+torS.ObPeWSErr);
		}
		if(relayPort == sshPort || relayPort == remoteSshPort )
		{
			errors.push( relayType + " "+torS.RPeSSHErr);
		}
		if(obfsPort == sshPort || obfsPort == remoteSshPort )
		{
			errors.push( relayType + " "+torS.ObPeSSHErr);
		}
	}

	return errors;
}

function bitsToMask(bits)
{
	bits = parseInt(bits)
	var quads = []
	var quadi = 0
	for(quadi=0; quadi< 4; quadi++)
	{
		var next = bits >= 8 ? 8 : bits
		bits = bits - next
		quads.push( "" + bits2mask[ next ] )
	}
	return quads.join(".")
}

function maskToBits(mask)
{
	var maskQuads = mask.split(/\./)
	var bits = 0
	for(quadi=0; quadi< 4; quadi++)
	{
		bits = bits + mask2bits[ parseInt(maskQuads[quadi]) ]
	}
	return bits
}


function resetData()
{
	var en = uciOriginal.get("tor", "global", "enabled")
	var torEnabled    = uciOriginal.get("tor", "global", "enabled")
	var torClientMode = torEnabled == "0" ? "0" : uciOriginal.get("tor", "client", "client_mode")
	var torRelayMode  = torEnabled == "0" ? "0" : uciOriginal.get("tor", "relay", "relay_mode")
	var opPort = uciOriginal.get("tor", "relay", "obfsproxy_port")

	torClientMode = (torClientMode != "1" && torClientMode != "2" && torClientMode != "3") ? "0" : torClientMode
	torRelayMode  = (torRelayMode != "1" && torRelayMode != "2" ) ? "0" : torRelayMode
	torRelayMode  = torRelayMode == "1" && opPort != "0" && opPort != "" ? "3" : torRelayMode

	
	//client 
	var blockOtherProtos = uciOriginal.get("tor", "client", "block_unsupported_proto") == "1" ? "1" : "0"
	var bridgeIp = uciOriginal.get("tor", "client", "use_bridge_ip")
	var bridgePort = uciOriginal.get("tor", "client", "use_bridge_port")
	var bridgeIsOp = uciOriginal.get("tor", "client", "use_bridge_obfsproxy") == "1" ? true : false;
	var clientConnect = bridgeIp != "" && bridgePort != "" ? "bridge" : "relay"
	clientConnect = clientConnect == "bridge" && bridgeIsOp ? "obfsproxy" : clientConnect

	setSelectedValue("tor_client_mode", torClientMode)
	setSelectedValue("tor_client_connect", clientConnect)
	setSelectedValue("tor_other_proto", blockOtherProtos)

	var hiddenSubnet = uciOriginal.get("tor", "client", "hidden_service_subnet")
	var hiddenBits   = uciOriginal.get("tor", "client", "hidden_service_mask_bits")
	if(hiddenSubnet == "")
	{
		hiddenSubnet = "10.192.0.0"
		hiddenBits   = 12
	}
	var hiddenMask   = bitsToMask(hiddenBits)

	
	document.getElementById("tor_client_bridge_ip").value    = bridgeIp
	document.getElementById("tor_client_bridge_port").value  = bridgePort
	document.getElementById("tor_hidden_subnet").value       = hiddenSubnet
	document.getElementById("tor_hidden_mask").value         = hiddenMask


	//relay
	setSelectedValue("tor_relay_mode", torRelayMode)
	var rvars = [
			["relay_port",     "tor_relay_port"], 
			["obfsproxy_port", "tor_obfsproxy_port"],
			["max_bw_rate_kb", "tor_relay_max_bw"],
			["relay_nickname", "tor_relay_nickname"],
			["relay_contact",  "tor_relay_contact"]
			];
	var rvIndex=0
	for(rvIndex=0; rvIndex < rvars.length; rvIndex++)
	{

		var val = uciOriginal.get("tor", "relay", rvars[rvIndex][0])
		val = (val == "" && (rvars[rvIndex][0]).match(/obfsproxy/)) ? "0" : val
		document.getElementById(  rvars[rvIndex][1] ).value = val
	}
	setSelectedValue("tor_relay_publish", uciOriginal.get("tor", "relay", "publish"));



	//data dir
	var rootDriveDisplay = [];
	var rootDriveValues  = [];
	


	var pkgrNames = ["ram", "root"]
	var dispNames = [torS.RAMD, torS.RootD]
	var ni
	for(ni=0;ni<2; ni++)
	{
		var prn = pkgrNames[ni];
		var dn  = dispNames[ni];
		if(pkg_dests[ prn ] != null)
		{
			rootDriveDisplay.push(dn + ": " + parseBytes(pkg_dests[prn]["Bytes-Total"]) + " "+torS.Totl+", " + parseBytes(pkg_dests[prn]["Bytes-Free"]) + " "+torS.Free);
		}
		else
		{
			rootDriveDisplay.push(dn)
		}
	}
	rootDriveValues.push("ramdisk");
	rootDriveValues.push("root");


	var driveIndex;
	for(driveIndex=0;driveIndex < storageDrives.length; driveIndex++)
	{
		rootDriveDisplay.push( storageDrives[driveIndex][0] + ": " + parseBytes(storageDrives[driveIndex][4]) + " "+torS.Totl+", " + parseBytes(storageDrives[driveIndex][5]) + " "+torS.Free )
		rootDriveValues.push( storageDrives[driveIndex][0] )
		driveToPath[ storageDrives[driveIndex][0] ] = storageDrives[driveIndex][1];
	}
	setAllowableSelections("tor_dir_drive_select", rootDriveValues, rootDriveDisplay, document);
	var torDataDrive    = uciOriginal.get("tor", "global", "data_drive")
	var torDataDriveDir = uciOriginal.get("tor", "global", "data_drive_dir")
	torDataDriveDir = torDataDrive == "" || torDataDriveDir == "" ? "/tor" : torDataDriveDir
	torDataDrive = torDataDrive == "" ? "ramdisk" : torDataDrive;
	setSelectedValue( "tor_dir_drive_select", torDataDrive)
	document.getElementById("tor_dir_text").value = torDataDriveDir
	
	setTorVisibility();

}

function setTorVisibility()
{
	//client visibility
	var clientMode =  getSelectedValue("tor_client_mode")
	setVisibility( [ "tor_client_connect_container" ], clientMode == "0" ? [0] : [1] )
	setVisibility( [ "tor_client_bridge_ip_container", "tor_client_bridge_port_container"], (clientMode =="0" || getSelectedValue("tor_client_connect") == "relay") ? [0,0] : [1,1])
	setVisibility( [ "tor_other_proto_container" ], (clientMode == "0" || clientMode == "3") ? [0] : [1] )
	setVisibility( [ "tor_hidden_subnet_container", "tor_hidden_mask_container" ], clientMode == "0" ? [0,0] : [1,1] )

	var modeDescriptions = []
	modeDescriptions["0"] = ""
	modeDescriptions["1"] = torS.AnonTraf
	modeDescriptions["2"] = torS.AnonOpt
	modeDescriptions["3"] = torS.TorTraf

	setChildText("mode_description", modeDescriptions[clientMode])


	//relay visibility
	var relayMode = getSelectedValue("tor_relay_mode")
	var op        = relayMode == "3"                     ? 1 : 0
	var r         = relayMode == "2"                     ? 1 : 0
	var b         = relayMode == "1" || relayMode == "3" ? 1 : 0 
	setVisibility( ["tor_relay_port_container", "tor_obfsproxy_port_container", "tor_relay_max_bw_container",'tor_relay_publish_container', "tor_relay_nickname_container", "tor_relay_contact_container", "tor_relay_status_link_container"], (relayMode == "1" || relayMode == "2" || relayMode == "3" ) ? [1,op,1,b,r,r,r] : [0,0,0,0,0,0,0])
	
	if(op==1)
	{
		var opel = document.getElementById("tor_obfsproxy_port")
		if(opel.value == "0")
		{
			opel.value = "9091"
		}
	}

	var showDataDirControls = relayMode == "1" || relayMode == "2" || relayMode == "3" || clientMode != "0" ? 1 : 0
	setVisibility( [ "tor_data_dir_section" ], [ showDataDirControls ] );
	if(showDataDirControls == 1)
	{
		var torDirDrive =  getSelectedValue("tor_dir_drive_select");
		var vis = [0,0,1];
		vis = torDirDrive == "ramdisk" || torDirDrive == "" ? [1,0,0] : vis;
		vis = torDirDrive == "root"    ? [0,1,0] : vis;
		setVisibility( ["tor_dir_ramdisk_static", "tor_dir_root_static", "tor_dir_text"], vis )
	}



}

