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
		var torRelayMode  = getSelectedValue("tor_relay_mode")
		uci.set('tor', 'global', 'enabled', (torClientMode=="0" && tor_relay_mode == "0" ? "0" : "1") )
		uci.set('tor', 'client', 'client_mode', torClientMode)
		uci.set('tor', 'relay',  'relay_mode',  torRelayMode)
		if(torClientMode != "0")
		{
			uci.set('tor', 'global', 'hidden_service_subnet',    document.getElementById("tor_hidden_subnet").value)
			uci.set('tor', 'global', 'hidden_service_mask_bits', maskToBits(document.getElementById("tor_hidden_mask").value))
			if(torClientMode != "3")
			{
				uci.set('tor', 'client', 'block_unsupported_proto',  getSelectedValue("tor_other_proto"))
			}
		}
		if(torRelayMode != "0")
		{
			var rvars = [
				["relay_port",     "tor_relay_port"], 
				["max_bw_rate_kb", "tor_relay_max_bw"],
				["relay_nickname", "tor_relay_nickname"],
				["relay_contact",  "tor_relay_contact"]
			];
			var rvIndex=0
			for(rvIndex=0; rvIndex < rvars.length; rvIndex++)
			{
				var setVal =  document.getElementById(  rvars[rvIndex][1] ).value
				setVal.replace(/[\r\n]+/, "") //make sure contact data contains no newlines
				uci.set('tor', 'relay', rvars[rvIndex][0], setVal )
			}
			uci.set('tor', 'relay', 'max_bw_burst_kb', parseInt(document.getElementById('max_bw_rate_kb'))*2 )

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
	var inputIds    = ["tor_hidden_subnet", "tor_hidden_mask", 'tor_relay_port', 'tor_relay_max_bw' ]
	var functions   = [ validateIP, validateNetMask, validatePort, validateNumeric ]
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

	return proofreadFields(inputIds, labelIds, functions, returnCodes, visIds);
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
	torClientMode = (torClientMode != "1" && torClientMode != "2" && torClientMode != "3") ? "0" : torClientMode
	torRelayMode  = (torRelayMode != "1" && torRelayMode != "2") ? "0" : torRelayMode
	
	
	//client / global
	var blockOtherProtos = uciOriginal.get("tor", "client", "block_unsupported_proto") == "1" ? "1" : "0"
	setSelectedValue("tor_client_mode", torClientMode)
	setSelectedValue("tor_other_proto", blockOtherProtos)

	var hiddenSubnet = uciOriginal.get("tor", "global", "hidden_service_subnet")
	var hiddenBits   = uciOriginal.get("tor", "global", "hidden_service_mask_bits")
	if(hiddenSubnet == "")
	{
		hiddenSubnet = "10.192.0.0"
		hiddenBits   = 12
	}
	var hiddenMask   = bitsToMask(hiddenBits)

	document.getElementById("tor_hidden_subnet").value = hiddenSubnet
	document.getElementById("tor_hidden_mask").value = hiddenMask


	//relay
	setSelectedValue("tor_relay_mode", torRelayMode)
	var rvars = [
			["relay_port",     "tor_relay_port"], 
			["max_bw_rate_kb", "tor_relay_max_bw"],
			["relay_nickname", "tor_relay_nickname"],
			["relay_contact",  "tor_relay_contact"]
			];
	var rvIndex=0
	for(rvIndex=0; rvIndex < rvars.length; rvIndex++)
	{

		var val = uciOriginal.get("tor", "relay", rvars[rvIndex][0])
		document.getElementById(  rvars[rvIndex][1] ).value = val
	}


	setTorVisibility()
}

function setTorVisibility()
{
	//client visibility
	var clientMode =  getSelectedValue("tor_client_mode")
	setVisibility( [ "tor_hidden_subnet_container", "tor_hidden_mask_container" ], clientMode == "0" ? [0,0] : [1,1] )
	setVisibility( [ "tor_other_proto_container" ], (clientMode == "0" || clientMode == "3") ? [0] : [1] )

	var modeDescriptions = []
	modeDescriptions["0"] = ""
	modeDescriptions["1"] = "All traffic will be anonymized"
	modeDescriptions["2"] = "Users can choose whether traffic will be anonymized"
	modeDescriptions["3"] = "Tor hidden services can be accessed, but no other traffic is anonymized"

	setChildText("mode_description", modeDescriptions[clientMode])


	//relay visibility
	var relayMode = getSelectedValue("tor_relay_mode")
	setVisibility( ["tor_relay_port_container", "tor_relay_max_bw_container", "tor_relay_nickname_container", "tor_relay_contact_container"], (relayMode == "1" || relayMode == "2" ) ? [1,1,1,1] : [0,0,0,0])

}

