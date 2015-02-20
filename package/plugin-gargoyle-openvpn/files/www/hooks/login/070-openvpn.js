var ovpnS = new Object(); //part of i18n


function setOpenVpnData()
{
	if( (openvpnEnabled == "1" || openvpnEnabled == "true") &&  (openvpnClientEnabled == "1" || openvpnClientEnabled == "true") )
	{
		document.getElementById("openvpn_fields").style.display="block";
		if( tunIp != "" && openvpnProc != "" )
		{
			setChildText("openvpn_status", ovpnTransRunC+", IP: " + tunIp, "#008800", true, null, document)
		}
		else if(openvpnProc != "")
		{
			setChildText("openvpn_status", ovpnTransRunNC, "#880000", true, null, document)
		}
		else
		{
			setChildText("openvpn_status", ovpnTransRunNot, "#880000", true, null, document)
		}
	}
	else
	{
		document.getElementById("openvpn_fields").style.display="none";
	}
}



addLoadFunction(setOpenVpnData);

