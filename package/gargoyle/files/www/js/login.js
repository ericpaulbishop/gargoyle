/*
 * This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

function doLogin()
{
	var password = document.getElementById("password").value;
	if(password.length == 0)
	{
		alert("ERROR: You must enter a password");
	}
	else
	{
		setControlsEnabled(false, true, "Logging In");
		
		sessionExpired=false;
		passInvalid=false;
		loggedOut=false;

		var param = getParameterDefinition("password", password);
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				if(req.responseText.match(/^invalid/))
				{
					passInvalid = true;
					setStatusAndQuotas();
				}
				else
				{
					var cookieLines=req.responseText.split(/[\n\r]+/);
					var cIndex=0;
					for(cIndex=0; cIndex < cookieLines.length; cIndex++)
					{
						var cookie = cookieLines[cIndex].replace(/^.*ookie:/, "").replace(/\";.*$/, "");
						if(cookie.match(/=/))
						{
							document.cookie=cookie;	
						}
					}
					window.location.href = window.location.href;
				}
				setControlsEnabled(true);
					
			}
		}
		runAjax("POST", "/utility/get_password_cookie.sh", param, stateChangeFunction);
	}
}

function setStatusAndQuotas()
{
	if(sessionExpired)
	{
		setChildText("login_status", "Session Expired", "red");
	}
	else if(passInvalid)
	{
		setChildText("login_status", "Invalid Password", "red");
	}
	else if(loggedOut)
	{
		setChildText("login_status", "Logged Out", "black");
	}
	else
	{
		setChildText("login_status", "", "black");
	}

	ipQuotaName = "ALL_OTHERS_COMBINED";
	if( quotaUsed[ connectedIp ] != null)
	{
		ipQuotaName = connectedIp;
	}
	
	if( quotaUsed[ ipQuotaName ] != null )
	{
		var ipQuotaUsed = quotaUsed[ ipQuotaName ];
		var ipQuotaLimits = quotaLimits[ ipQuotaName ];
		var ipQuotaPercents = quotaPercents[ ipQuotaName ];
		var ipQuotaData = getQuotaLines(ipQuotaUsed, ipQuotaLimits, ipQuotaPercents);
		if(ipQuotaData[0] != null) { setChildText("up_your_quota", ipQuotaData[0]); }
		if(ipQuotaData[1] != null) { setChildText("down_your_quota", ipQuotaData[1]); }
		if(ipQuotaData[2] != null) { setChildText("combined_your_quota", ipQuotaData[2]); }
		document.getElementById("up_your_quota_container").style.display = ipQuotaData[0] == null ? "none" : "block";
		document.getElementById("down_your_quota_container").style.display = ipQuotaData[1] == null ? "none" : "block";
		document.getElementById("combined_your_quota_container").style.display = ipQuotaData[2] == null ? "none" : "block";
		document.getElementById("your_quota").style.display="block";
	}
	if( quotaUsed["ALL"] != null )
	{
		var allQuotaUsed     = quotaUsed[ "ALL" ];
		var allQuotaLimits   = quotaLimits["ALL"];
		var allQuotaPercents = quotaPercents["ALL"];
		var allQuotaData     = getQuotaLines(allQuotaUsed, allQuotaLimits, allQuotaPercents);
		if(allQuotaData[0] != null) { setChildText("up_all_quota", allQuotaData[0]); }
		if(allQuotaData[1] != null) { setChildText("down_all_quota", allQuotaData[1]); }
		if(allQuotaData[2] != null) { setChildText("combined_all_quota", allQuotaData[2]); }
		document.getElementById("up_all_quota_container").style.display = allQuotaData[0] == null ? "none" : "block";
		document.getElementById("down_all_quota_container").style.display = allQuotaData[1] == null ? "none" : "block";
		document.getElementById("combined_all_quota_container").style.display = allQuotaData[2] == null ? "none" : "block";
		document.getElementById("network_quota").style.display="block";
	}

	

}

function getQuotaLines(usd, lim, pct)
{
	names = ["upload", "download", "combined upload/download" ];
	var lines = [null, null, null];
	var typeIndex;
	for(typeIndex=0; typeIndex < 3; typeIndex++)
	{
		if(lim[typeIndex] >= 0)
		{
			var name  = names[typeIndex];
			var limit = parseBytes(lim[typeIndex]);
			var unit  = limit.replace(/^.* /, "");
			var used  = parseBytes(usd[typeIndex], unit).replace(/ .*$/, "");
			var perc   = truncateDecimal( pct[typeIndex] );
			used = used > limit ? limit.replace(/ .*$/,"") : used;
			lines[typeIndex] = perc + "% of " + name + " quota has been used (" + used + "/" + limit + ")";
		}
	}

	return lines;
}
