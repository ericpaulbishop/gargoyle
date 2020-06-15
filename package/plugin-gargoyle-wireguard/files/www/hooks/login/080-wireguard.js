var wgStr = new Object(); //part of i18n


function setWireguardData()
{
	if( wireguardClientEnabled == "1" || wireguardClientEnabled == "true" )
	{
		document.getElementById("wireguard_fields").style.display="block";
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
		setChildText("wireguard_status", wgStatusStr, txtCol, true, null, document)
	}
	else
	{
		document.getElementById("wireguard_fields").style.display="none";
	}
}



addLoadFunction(setWireguardData);

