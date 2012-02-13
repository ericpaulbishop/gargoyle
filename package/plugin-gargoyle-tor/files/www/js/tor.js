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
		var torEnabledType = getSelectedValue("tor_enabled")
		uci.set('tor', 'global', 'enabled', torEnabledType)
		if(torEnabledType != "0")
		{
			uci.set('tor', 'global', 'hidden_service_subnet',    document.getElementById("tor_hidden_subnet").value)
			uci.set('tor', 'global', 'hidden_service_mask_bits', maskToBits(document.getElementById("tor_hidden_mask").value))
			if(torEnabledType != "3")
			{
				uci.set('tor', 'global', 'block_unsupported_proto',  getSelectedValue("tor_other_proto"))
			}
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
	var inputIds    = ["tor_hidden_subnet", "tor_hidden_mask"]
	var functions   = [ validateIP, validateNetMask]
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
	var torEnabled = uciOriginal.get("tor", "global", "enabled")
	torEnabled = (torEnabled != "1" && torEnabled != "2" && torEnabled != "3") ? "0" : torEnabled
	var blockOtherProtos = uciOriginal.get("tor", "global", "block_unsupported_proto") == "1" ? "1" : "0"
	setSelectedValue("tor_enabled", torEnabled)
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

	setTorVisibility()
}

function setTorVisibility()
{
	var enabledType =  getSelectedValue("tor_enabled")
	setVisibility( [ "tor_hidden_subnet_container", "tor_hidden_mask_container" ], enabledType == "0" ? [0,0] : [1,1] )
	setVisibility( [ "tor_other_proto_container" ], (enabledType == "0" || enabledType == "3") ? [0] : [1] )

	var modeDescriptions = []
	modeDescriptions["0"] = ""
	modeDescriptions["1"] = "All traffic will be anonymized"
	modeDescriptions["2"] = "Users can choose whether traffic will be anonymized"
	modeDescriptions["3"] = "Tor hidden services can be accessed, but no other traffic is anonymized"

	setChildText("mode_description", modeDescriptions[enabledType])

}

