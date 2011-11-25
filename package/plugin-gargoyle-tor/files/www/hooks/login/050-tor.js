function setTorData()
{
	if(torEnabled == "2")
	{
		document.getElementById("tor_fields").style.display="block";

		torIsActive = torIsActive == "" ? false: true;
		setChildText("tor_status", (torIsActive ? "Enabled" : "Disabled"), (torIsActive ? "#27c650" : "#949494"), torIsActive)
		document.getElementById("set_tor_button").value = torIsActive ? "Disable Tor For Your IP" : "Enable Tor For Your IP";
	}
	else
	{
		document.getElementById("tor_fields").style.display="none";
	}
}

function updateTorStatus()
{
	setControlsEnabled(false, true, "Please Wait...")

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{

			if(req.responseText.match(/^bad_ip/))
			{
				alert("ERROR: Your IP was not assigned by the DHCP server and is not configured as a known static IP\n\nTor configuration prohibited");
			}
			else if(req.responseText.match(/^tor_per_ip_disabled/))
			{
				//should never get here
				alert("ERROR: Tor Per-IP matching disabled\n\nTor configuration prohibited")
			}
			else if(req.responseText.match(/^success_disabled/))
			{
				alert("Tor Successfully Disabled for your IP")
				torIsActive = false;
			}
			else if(req.responseText.match(/^success_enabled/))
			{
				alert("Tor Successfully Enabled for your IP")
				torIsActive = true;
			}
			setTorData()
			setControlsEnabled(true)
		}
	}
	runAjax("POST", "/torip.sh", "", stateChangeFunction);
}

addLoadFunction(setTorData);

