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
					setStatus();
				}
				else
				{
					var cookieLines=req.responseText.split(/[\n\r]+/);
					var cIndex=0;
					for(cIndex=0; cIndex < cookieLines.length; cIndex++)
					{
						var cookie = cookieLines[cIndex].replace(/^.*:/, "").replace(/;.*$/, "");
						if(cookie.match(/=/))
						{
							//alert(cookie);
							document.cookie=cookie;	
						}
					}
					currentProtocol = window.location.href.match(/^https:/) ? "https" : "http";
					window.location.href = currentProtocol + "://" + window.location.host + "/overview.sh";
				}
				setControlsEnabled(true);
					
			}
		}
		runAjax("POST", "/utility/get_password_cookie.sh", param, stateChangeFunction);
	}
}

function setStatus()
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
	
}
