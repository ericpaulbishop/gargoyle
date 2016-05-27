/*
 * This program is copyright © 2008,2009-2013 Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var logS=new Object(); //part of i18n

function doLogin()
{
	var password = document.getElementById("password").value;
	if(password.length == 0)
	{
		alert(logS.passErr);
	}
	else
	{
		setControlsEnabled(false, true, logS.Lging);
		
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

function checkKey(e)
{
	var keycode = 0;

	if ( window.event )
		keycode = window.event.keyCode;
	else if ( e )
		keycode = e.which;
	if ( keycode == 13 )
		doLogin();
}

function setStatusAndQuotas()
{
	setChildText("current_time_date", cnv_LocaleTime(currentTime));

	if(sessionExpired)
	{
		setChildText("login_status", logS.SExp, "red");
	}
	else if(passInvalid)
	{
		setChildText("login_status", logS.InvP, "red");
	}
	else if(loggedOut)
	{
		setChildText("login_status", logS.LOut, "black");
	}
	else
	{
		setChildText("login_status", "", "black");
	}

	var globalQuotas = [];
	var localQuotas = [];
	var otherQuotas = [];
	var idIndex;
	for(idIndex=0;idIndex < quotaIdList.length; idIndex++)
	{
		var id=quotaIdList[idIndex];
		var testIps = quotaIpLists[id];
		if(testIps.length > 0)
		{
			if(testIps[0] == "ALL")
			{
				globalQuotas.push(id);
			}
			else if(testIps[0] == "ALL_OTHERS_COMBINED")
			{
				otherQuotas.push(id)
			}
			else if(testAddrOverlap(connectedIp, testIps.join(",")))
			{
				localQuotas.push(id);
				localIpName = connectedIp;
			}
		}
	}
	localQuotas = localQuotas.length == 0 ? otherQuotas : localQuotas;

	var normalFontParams = ["black", false, "11px"];
	var usedFontParams = ["red", true, "12px"];

	var qTypes = [ localQuotas, globalQuotas ];
	var qFieldsets = [ "local_quotas", "global_quotas" ];
	var typeIndex;
	for(typeIndex=0; typeIndex < qTypes.length; typeIndex++)
	{
		var qIds = qTypes[typeIndex];
		var qFieldset = qFieldsets[typeIndex];
		var qIp = typeIndex == 0 ? connectedIp : "ALL";

		clearFieldset(qFieldset);
		if(qIds.length > 0)
		{
			var quotaIndex;
			for(quotaIndex=0; quotaIndex < qIds.length; quotaIndex++)
			{
				var quotaNumber = qIds.length > 1 ? quotaIndex+1 : -1;
				var div = createQuotaDiv(qIds[quotaIndex], qIp, quotaNumber, normalFontParams, usedFontParams);
				document.getElementById(qFieldset).appendChild(div);
			}
		}
		document.getElementById(qFieldset).style.display = qIds.length > 0 ? "block" :"none";
	}
}

function clearFieldset(fieldsetId)
{
	var fieldsetEl = document.getElementById(fieldsetId);
	if(fieldsetEl != null)
	{
		var sectionHeader = null;
		while(fieldsetEl.firstChild != null)
		{
			var rmEl = fieldsetEl.firstChild;
			sectionHeader = rmEl.className == "sectionheader" ? rmEl : sectionHeader;
			fieldsetEl.removeChild(rmEl);
		}
		if(sectionHeader != null)
		{
			fieldsetEl.appendChild(sectionHeader);
		}
	}
}

function createQuotaDiv(quotaId, fieldsetIp, quotaNumber, normalFontParams, usedFontParams)
{
	var ip = "ALL_OTHERS_COMBINED";
	var testIps = quotaIpLists[quotaId];
	var ipIndex=0;
	for(ipIndex=0; ipIndex < testIps.length; ipIndex++)
	{
		ip = testAddrOverlap(fieldsetIp, testIps[ipIndex]) ? testIps[ipIndex] : ip;
	}


	var usd = quotaUsed[quotaId][ip];
	var pct = quotaPercents[quotaId][ip];
	var lim = quotaLimits[quotaId][ip];

	
	var parentDiv = document.createElement("div");
	if(quotaNumber > 0)
	{
		var nameSpan = document.createElement("span");
		nameSpan.style.fontSize = usedFontParams[2];
		nameSpan.style.display = "block";
		nameSpan.style.fontStyle = "italic";
		nameSpan.style.textDecoration = "underline";
		nameSpan.appendChild( document.createTextNode("Quota" + quotaNumber + ":"));
		parentDiv.appendChild(nameSpan);

		var timeLines = timeParamsToLines(quotaTimes[quotaId]);
		var timeSpan = document.createElement("span");
		var timeActiveSpan = document.createElement("span");
		var timeParamSpan = document.createElement("span");
		
		timeActiveSpan.appendChild(document.createTextNode("Active " + (quotaTimes[quotaId][3] == "only" ? "Only:" : "All Times Except:")));
		timeActiveSpan.style.fontSize = normalFontParams[2];
		timeActiveSpan.style.marginLeft="25px";
		timeActiveSpan.style.display="block";
		timeActiveSpan.style.width="150px";
		timeActiveSpan.style.cssFloat="left";
		timeActiveSpan.style.styleFloat="left";
		
		timeParamSpan.appendChild(document.createTextNode(timeLines.shift()));
		timeParamSpan.style.fontSize = normalFontParams[2];
		timeParamSpan.style.marginLeft = "25px";
		timeParamSpan.style.display="inline";

		timeSpan.appendChild(timeActiveSpan);
		timeSpan.appendChild(timeParamSpan);
		parentDiv.appendChild(timeSpan);

		while(timeLines.length > 0)
		{
			var timeParamSpan = document.createElement("span");
			timeParamSpan.appendChild(document.createTextNode(timeLines.shift()));
			timeParamSpan.style.fontSize = normalFontParams[2];
			timeParamSpan.style.marginLeft = "200px"; //25+150+25 = 175
			timeParamSpan.style.display="block";
			timeActiveSpan.style.clear="left";
			timeActiveSpan.style.clear="left";
			parentDiv.appendChild(timeParamSpan);
		}




	}

	var names = logS.Qnam;
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
			used = usd[typeIndex] > lim[typeIndex] ? limit.replace(/ .*$/,"") : used;
			var span = document.createElement("span");
			var par = document.createElement("p");
			if(quotaNumber > 0)
			{
				par.appendChild( document.createTextNode(perc + "% "+logS.of+" " + name + " "+logS.fQuo + quotaNumber + " "+logS.husd+" (" + used + "/" + limit + ")"));
			}
			else
			{
				par.appendChild( document.createTextNode(perc + "% "+logS.of+" " + name + " "+logS.qusd+" (" + used + "/" + limit + ")"));
			}
			var fontParams = (pct[typeIndex] == 100) ? usedFontParams : normalFontParams;
			par.style.color = fontParams[0];
			par.style.fontWeight = fontParams[1] ? "bold" : "normal";
			par.style.fontSize = fontParams[2];
			span.appendChild(par);
			span.style.display="block";
			span.style.clear="left";
			if(quotaNumber > 0)
			{
				span.style.marginLeft = quotaNumber > 0 ? "25px" : "0px";
			}
			parentDiv.appendChild(span);
		}
	}
	return parentDiv;
}

function timeParamsToLines(timeParameters)
{
	var hours = timeParameters[0];
       	var days = timeParameters[1];
	var weekly = timeParameters[2];
	var active = timeParameters[3];
	
	
	var textList = [];
	if(active == "always")
	{
		textList.unshift(UI.Always);
	}
	else
	{
		if(weekly != "")
		{
			textList = weekly.match(",") ? weekly.split(/[\t ]*,[\t ]*/) : [ weekly ];
		}
		else
		{
			if(hours != ""){ textList = hours.match(",") ? hours.split(/[\t ]*,[\t ]*/) : [ hours ]; }
			if(days  != ""){ textList.unshift(days); }
		}
	}
	return textList;
}


