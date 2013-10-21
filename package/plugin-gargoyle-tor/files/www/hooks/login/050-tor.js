var torLS=new Object(); //part of i18n

function setTorData()
{
	if(torEnabled == "1" && torClientMode == "2")
	{
		document.getElementById("tor_fields").style.display="block";

		torIsActive = torIsActive == "" ? false: true;
		setChildText("tor_status", (torIsActive ? UI.Enabled : UI.Disabled), (torIsActive ? "#27c650" : "#949494"), torIsActive)
		document.getElementById("set_tor_button").value = torIsActive ? torLS.tDisa : torLS.tEnab;
	}
	else
	{
		document.getElementById("tor_fields").style.display="none";
	}
}

function updateTorStatus()
{
	setControlsEnabled(false, true, UI.WaitSettings)

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{

			if(req.responseText.match(/^bad_ip/))
			{
				alert(torLS.IPErr);
			}
			else if(req.responseText.match(/^tor_per_ip_disabled/))
			{
				//should never get here
				alert(torLS.EqErr)
			}
			else if(req.responseText.match(/^success_disabled/))
			{
				alert(torLS.EnabMsg)
				torIsActive = false;
			}
			else if(req.responseText.match(/^success_enabled/))
			{
				alert(torLS.DisbMsg)
				torIsActive = true;
			}
			setTorData()
			setControlsEnabled(true)
		}
	}
	runAjax("POST", "/torip.sh", "", stateChangeFunction);
}

addLoadFunction(setTorData);

