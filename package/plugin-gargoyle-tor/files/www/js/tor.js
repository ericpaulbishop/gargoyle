/*
 * This program is copyright © 2008-2012 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */


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

			uci.set('tor', 'relay', 'max_bw_burst_kb', "" + (parseInt(document.getElementById('tor_relay_max_bw').value)*2) )

		}
		var commands = uci.getScriptCommands(uciOriginal) + "\n" + "/etc/init.d/tor restart" + "\n";
		var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				uciOriginal = uci.clone();
				resetData();
				setControlsEnabled(true);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
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
	if( document.getElementById("tor_obfsproxy_port").value ==  document.getElementById("tor_relay_port").value && getSelectedValue("tor_relay_mode") != "0")
	{
		errors.push("Bridge Server Obfsproxy Port Cannot Be The Same As Bridge/Relay Port");
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

	setTorVisibility()
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
	modeDescriptions["1"] = "All traffic will be anonymized"
	modeDescriptions["2"] = "Users can choose whether traffic will be anonymized"
	modeDescriptions["3"] = "Tor hidden services can be accessed, but no other traffic is anonymized"

	setChildText("mode_description", modeDescriptions[clientMode])


	//relay visibility
	var relayMode = getSelectedValue("tor_relay_mode")
	var op        = relayMode == "3" ? 1 : 0
	var r         = relayMode == "2" ? 1 : 0
	setVisibility( ["tor_relay_port_container", "tor_obfsproxy_port_container", "tor_relay_max_bw_container", "tor_relay_nickname_container", "tor_relay_contact_container"], (relayMode == "1" || relayMode == "2" || relayMode == "3" ) ? [1,op,1,r,r] : [0,0,0,0,0])
	
	if(op==1)
	{
		var opel = document.getElementById("tor_obfsproxy_port")
		if(opel.value == "0")
		{
			opel.value = "9091"
		}
	}

}

