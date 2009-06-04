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
						var cookie = cookieLines[cIndex].replace(/^.*:/, "").replace(/\";.*$/, "");
						if(cookie.match(/=/))
						{
							//alert(cookie);
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
	
	if(ipq.length != 0)
	{
		var parsedIpQuota = parseQuotaData(ipq);
		if(parsedIpQuota[0] != null) { setChildText("up_your_quota", parsedIpQuota[0]); }
		if(parsedIpQuota[1] != null) { setChildText("down_your_quota", parsedIpQuota[1]); }
		if(parsedIpQuota[2] != null) { setChildText("combined_your_quota", parsedIpQuota[2]); }
		document.getElementById("up_your_quota_container").style.display = parsedIpQuota[0] == null ? "none" : "block";
		document.getElementById("down_your_quota_container").style.display = parsedIpQuota[1] == null ? "none" : "block";
		document.getElementById("combined_your_quota_container").style.display = parsedIpQuota[2] == null ? "none" : "block";
		document.getElementById("your_quota").style.display="block";
	}
	if(allq.length != 0)
	{
		var parsedAllQuota = parseQuotaData(allq);
		if(parsedAllQuota[0] != null) { setChildText("up_all_quota", parsedAllQuota[0]); }
		if(parsedAllQuota[1] != null) { setChildText("down_all_quota", parsedAllQuota[1]); }
		if(parsedAllQuota[2] != null) { setChildText("combined_all_quota", parsedAllQuota[2]); }
		document.getElementById("up_all_quota_container").style.display = parsedAllQuota[0] == null ? "none" : "block";
		document.getElementById("down_all_quota_container").style.display = parsedAllQuota[1] == null ? "none" : "block";
		document.getElementById("combined_all_quota_container").style.display = parsedAllQuota[2] == null ? "none" : "block";
		document.getElementById("network_quota").style.display="block";
	}

	

}

function parseQuotaData(quotaData)
{
	var upLimit = -1;
	var downLimit = -1;
	var combinedLimit = -1;
	var upUsed = 0;
	var downUsed = 0;
	var combinedUsed = 0;


	var qIndex=0;
	for(qIndex=0; qIndex < quotaData.length; qIndex++)
	{
		if(quotaData[qIndex].match(/egress_limit=/))
		{
			upLimit = parseInt(quotaData[qIndex].replace(/^.*egress_limit=/, "").replace(/[;\t ]+/, ""));
		}
		if(quotaData[qIndex].match(/ingress_limit=/))
		{
			downLimit = parseInt(quotaData[qIndex].replace(/^.*ingress_limit=/, "").replace(/[;\t ]+/, ""));
		}
		if(quotaData[qIndex].match(/combined_limit=/))
		{
			combinedLimit = parseInt(quotaData[qIndex].replace(/^.*combined_limit=/, "").replace(/[;\t ]+/, ""));
		}
		if(quotaData[qIndex].match(/egress_used=/))
		{
			upUsed = parseInt(quotaData[qIndex].replace(/^.*egress_used=/, "").replace(/[;\t ]+/, ""));
		}
		if(quotaData[qIndex].match(/ingress_used=/))
		{
			downUsed = parseInt(quotaData[qIndex].replace(/^.*ingress_used=/, "").replace(/[;\t ]+/, ""));
		}
		if(quotaData[qIndex].match(/combined_used=/))
		{
			combinedUsed = parseInt(quotaData[qIndex].replace(/^.*combined_used=/, "").replace(/[;\t ]+/, ""));
		}
	}
	
	var getLineForQuota = function(limit, used, name)
	{
		var line = null;
		if(limit > 0)
		{
			var percent = truncateDecimal( (used*100)/limit );
			percent = percent > 100 ? 100 : percent;
			var parsedLimit = parseBytes(limit);
			var unit = parsedLimit.replace(/^.* /, "");
			var parsedUsed = parseBytes(used, unit).replace(/ .*$/, "");
			line = percent + "% of " + name + " quota has been used (" + parsedUsed + "/" + parsedLimit + ")";
		}
		return line;
	}
	
	var quotaReportLines = [ getLineForQuota(upLimit, upUsed, "upload"),  getLineForQuota(downLimit, downUsed, "download"), getLineForQuota(combinedLimit, combinedUsed, "combined upload/download") ];

	return quotaReportLines;
}
