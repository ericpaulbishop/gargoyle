/*
 * This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var accessStr=new Object(); //part of i18n

function saveChanges()
{
	var errorList = proofreadAll();
	if(errorList.length > 0)
	{
		alert(errorList.join("\n") + "\n\n" + UI.ErrChanges);
	}
	else
	{
		setControlsEnabled(false, true);

		var uci = uciOriginal.clone();

		var oldLocalHttpPort  = getHttpPort(uciOriginal);
		var oldLocalHttpsPort = getHttpsPort(uciOriginal);

		//remove all old firewall remote_accept sections that redirected to ssh or http server
		var dropbearSections = uciOriginal.getAllSections("dropbear");
		var oldLocalSshPort = uciOriginal.get("dropbear", dropbearSections[0], "Port");

		var remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept");
		while(remoteAcceptSections.length > 0)
		{
			var lastSection = remoteAcceptSections.pop();
			var localPort = uciOriginal.get("firewall", lastSection, "local_port");
			if(localPort == oldLocalSshPort || localPort == oldLocalHttpsPort || localPort == oldLocalHttpPort)
			{
				uci.removeSection("firewall", lastSection);
			}
		}



		// add updated remote accepts
		var restartFirewall = false;
		var addAccept = function(local,remote,family)
		{
			var id = "ra_" + local + "_" + remote + "_" + family;
			uci.set("firewall", id, "", "remote_accept");
			uci.set("firewall", id, "local_port", local);
			uci.set("firewall", id, "remote_port", remote);
			uci.set("firewall", id, "proto", "tcp");
			uci.set("firewall", id, "zone", "wan");
			uci.set("firewall", id, "family", family);
		};
		var localHttpPort = document.getElementById("local_http_port").value;
		var remoteHttpPort = document.getElementById("remote_http_port").value;
		if(document.getElementById("remote_http_port_container").style.display != "none")
		{
			addAccept(localHttpPort, remoteHttpPort, localHttpPort == remoteHttpPort ? "any" : "ipv4");
		}
		else
		{
			remoteHttpPort = "";
		}
		if(oldRemoteHttpPort != remoteHttpPort) { restartFirewall = true; }
		var localHttpsPort = document.getElementById("local_https_port").value;
		var remoteHttpsPort = document.getElementById("remote_https_port").value;
		if(document.getElementById("remote_https_port_container").style.display != "none")
		{
			addAccept(localHttpsPort, remoteHttpsPort, localHttpsPort == remoteHttpsPort ? "any" : "ipv4");
		}
		else
		{
			remoteHttpsPort = "";
		}
		if(oldRemoteHttpsPort != remoteHttpsPort) { restartFirewall = true; }
		var localSshPort = document.getElementById("local_ssh_port").value;
		var remoteSshPort = document.getElementById("remote_ssh_port").value;
		if(!document.getElementById("remote_ssh_port").disabled)
		{
			addAccept(localSshPort, remoteSshPort, localSshPort == remoteSshPort ? "any" : "ipv4");
		}
		else
		{
			remoteSshPort = "";
		}
		if(oldRemoteSshPort != remoteSshPort) { restartFirewall = true; }

		//recreate dropbear config section if anonymous-- anonymous uci section can cause problems saving
		var remoteAttempts =  document.getElementById("remote_ssh_attempts").disabled ? "" : getSelectedValue("remote_ssh_attempts");
		var oldSshPwdEnabled = uciOriginal.get("dropbear", dropbearSections[0], "PasswordAuth");
		var sshPwdEnabled = document.getElementById("pwd_auth_enabled").checked ? "on" : "off";
		for(s = 0; s < dropbearSections.length; s++)
		{
			if(dropbearSections[s] != "global")
			{
				uci.removeSection("dropbear", dropbearSections[s]);
			}
		}
		//update dropbear uci configuration
		uci.set("dropbear", "global", "", "dropbear");
		uci.set("dropbear", "global", "Port", localSshPort);
		if(remoteAttempts != "") { uci.set("dropbear", "global", "max_remote_attempts",  remoteAttempts ); }
		uci.set("dropbear", "global", "PasswordAuth", sshPwdEnabled);

		//only restart dropbear if we need to
		var restartDropbear = oldLocalSshPort != localSshPort || oldSshPwdEnabled != sshPwdEnabled;

		var authorizedKeys = new Array();
		for (var key in authorizedKeyMap)
		{
			if (authorizedKeyMap.hasOwnProperty(key))
			{
				authorizedKeys.push(authorizedKeyMap[key]);
			}
		}
		var sshKeysCommands = ["rm /etc/dropbear/authorized_keys"];
		sshKeysCommands.push("echo '" + authorizedKeys.join("\n") + "' >> /etc/dropbear/authorized_keys");

		//set RFC 1918 filter enabled/disabled
		var disableRfc1918Filter = document.getElementById("disable_rfc1918_filter").checked;
		if(getSelectedValue("remote_web_protocol") != "disabled")
		{
			uci.set("uhttpd", "main", "rfc1918_filter", disableRfc1918Filter ? "0" : "1");
		}
		else
		{
			uci.set("uhttpd", "main", "rfc1918_filter", "1");
		}

		//set web password enabled/disabled
		if(document.getElementById("disable_web_password").checked)
		{
			uci.set("gargoyle", "global", "require_web_password", "0");
		}
		else
		{
			uci.set("gargoyle", "global", "require_web_password", "1");
		}

		//set web session timout
		uci.set("gargoyle", "global", "session_timeout", getSelectedValue("session_length"));


		//set local web protocols
		localWebProtocol = getSelectedValue("local_web_protocol");
		if(localWebProtocol == "https" || localWebProtocol == "redirect" || localWebProtocol == "both")
		{
			uci.set("uhttpd", "main", "listen_https", [ "0.0.0.0:" + document.getElementById("local_https_port").value, "[::]:" + document.getElementById("local_https_port").value ] );
		}
		else
		{
			uci.set("uhttpd", "main", "listen_https", [ ] );
		}
		if(localWebProtocol == "http" || localWebProtocol == "redirect" || localWebProtocol == "both")
		{
			uci.set("uhttpd", "main", "listen_http",  [ "0.0.0.0:" + document.getElementById("local_http_port").value, "[::]:" + document.getElementById("local_http_port").value ] );
		}
		else
		{
			uci.set("uhttpd", "main", "listen_http",  [ ] );
		}
		uci.set("uhttpd", "main", "redirect_https", localWebProtocol == "redirect" ? "1" : "0");

		var restartUhttpd = oldLocalWebProtocol != localWebProtocol || oldDisableRfc1918Filter != disableRfc1918Filter ||
			oldLocalHttpPort != localHttpPort || oldLocalHttpsPort != localHttpsPort;

		// replace the default https private-key and certificate if https is required
		remoteWebProtocol = getSelectedValue("remote_web_protocol");
		var httpsCommands = new Array();
		if(opensslInstalled && (localWebProtocol == "https" || localWebProtocol == "redirect" || localWebProtocol == "both" || remoteWebProtocol == "https"))
		{
			is_default_key = uhttpd_key_md5.localeCompare("023ef078ceccc023ca7c2b521a6682fe") == 0;
			is_default_crt = uhttpd_crt_md5.localeCompare("932613eda838a7d1df15f659abadb094") == 0;
			if(is_default_key || is_default_crt)
			{ // generate and install a new private-key and self-signed certificate
				httpsCommands.push("openssl genpkey -algorithm RSA -out /etc/uhttpd.key -pkeyopt rsa_keygen_bits:2048");
				httpsCommands.push("openssl req -new -key /etc/uhttpd.key -out /etc/uhttpd.csr -subj '/O=gargoyle-router.com/CN=Gargoyle Router Management Utility'");
				httpsCommands.push("openssl x509 -req -days 3650 -in /etc/uhttpd.csr -signkey /etc/uhttpd.key -out /etc/uhttpd.crt");
				httpsCommands.push("chmod 640 uhttpd.key");
				httpsCommands.push("rm /etc/uhttpd.csr");
				httpsCommands.push("chmod 640 uhttpd.crt");
			}
		}


		//password update
		passwordCommands = "";
		newPassword = document.getElementById("password1").value;
		if(newPassword != "")
		{
			var escapedPassword = newPassword.replace(/'/, "'\"'\"'");
			passwordCommands = "(echo \'" + escapedPassword + "' ; sleep 1 ; echo \'" + escapedPassword + "\') | passwd root \n";
			restartDropbear = true;
		}

		// Treat client's view as default target.
		var targetProt = clientProt;
		var targetHost = clientHost;
		var targetPort = clientPort;
		// Only try to be smart when client sees what server sees,
		// otherwise there's a reverse proxy between the two.
		if(directView)
		{
			// Either local or remote protocol.
			var accessProt = document.getElementById(accessArea + "_web_protocol").value;
			// When there is either HTTP or HTTPS, use the available one.
			if(accessProt != "redirect" && accessProt != "both")
			{
				targetProt = accessProt;
			}
			// Avoid CORS error the next time we save changes.
			var anticipateRedirect = accessProt == "redirect" && clientProt == "http";
			// Port of either local or remote target protocol.
			targetPort = document.getElementById(accessArea + "_" + targetProt + "_port").value;
			// Redirect on demand.
			needRedirect = clientProt != targetProt || clientPort != targetPort || anticipateRedirect;
		}
		else
		{
			// Redirect or in this case refresh still happens on exceptional timeout.
			needRedirect = false;
		}
		// Path with leading slash.
		var targetPath = uciOriginal.get("gargoyle", "global", "bin_root").replace(/^./, "") + "/" + uciOriginal.get("gargoyle", "scripts", "system_access");
		// Hide default ports for browsers which don't.
		var targetHref = targetProt + "://" + targetHost + (targetPort == 80 || targetPort == 443 ? "" : ":" + targetPort) + targetPath;
		// No magic here.
		doRedirect = function() { window.location.href = targetHref; };

		var commands = passwordCommands + "\n";
		commands += httpsCommands.join("\n") + "\n";
		commands += uci.getScriptCommands(uciOriginal) + "\n";
		commands += sshKeysCommands.join("\n") + "\n";
		commands += restartFirewall ? "sh /usr/lib/gargoyle/restart_firewall.sh\n" : "";
		commands += restartDropbear ? "/etc/init.d/dropbear restart\n" : "";
		commands += restartUhttpd ? "killall uhttpd\n/etc/init.d/uhttpd restart\n" : "";
		//document.getElementById("output").value = commands;

		// Exceptional timeout.
		run_commands = setTimeout(doRedirect, 120000);
		var param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
		var stateChangeFunction = function(req)
		{
			if(req.readyState == 4)
			{
				clearTimeout(run_commands);
				uciOriginal = uci.clone();
				resetData();
				// Restarting uhttpd takes roughly 333 ms when idle.
				setTimeout(function() { needRedirect ? doRedirect() : setControlsEnabled(true); }, restartUhttpd ? 1000: 0);
			}
		}
		runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	}
}

function proofreadAll()
{
	validatePort = function(text){  return validateNumericRange(text,1,65535); };
	labelIds = [];
	functions = [];
	returnCodes = [];
	visibilityIds = [];

	for(idIndex = 0; idIndex < controlIds.length; idIndex++)
	{
		id=controlIds[idIndex];
		labelIds.push(id + "_label");
		functions.push(validatePort);
		returnCodes.push(0);
		visibilityIds.push(   id.match(/ssh/) ? id : id + "_container" ); //ssh control just gets disabled, while others have containers that become invisible
	}
	errors = proofreadFields(controlIds, labelIds, functions, returnCodes, visibilityIds);



	localIds = ['local_https_port', 'local_http_port', 'local_ssh_port'];
	remoteIds = ['remote_https_port', 'remote_http_port', 'remote_ssh_port'];
	for(localIndex=0; localIndex < localIds.length; localIndex++)
	{
		localValue = document.getElementById(localIds[localIndex]).value;
		for(remoteIndex=0; remoteIndex < remoteIds.length; remoteIndex++)
		{
			remoteValue = document.getElementById(remoteIds[remoteIndex]).value;
			//alert ("local = " + localValue + ", remote = " + remoteValue +  ", localIndex = " + localIndex + ", remoteIndex = " + remoteIndex);
			if(remoteIndex != localIndex && remoteValue == localValue && remoteValue != "" && document.getElementById(remoteIds[remoteIndex]).disabled == false)
			{
				localName = document.getElementById( localIds[localIndex] + "_label").firstChild.data.replace(/:/, "");
				remoteName = 	document.getElementById( remoteIds[remoteIndex] + "_label").firstChild.data.replace(/:/, "");
				errors.push(remoteName + " cannot be " + localName);
			}
		}
		for(localIndex2=0; localIndex2 < localIds.length; localIndex2++)
		{
			localValue2 = document.getElementById(localIds[localIndex2]).value;
			//alert ("local = " + localValue + ", localValue2 = " + localValue2 +  ", localIndex = " + localIndex + ", localIndex2 = " + localIndex2);
			if(localIndex2 != localIndex && localValue2 == localValue && localValue2 !="" && document.getElementById(localIds[localIndex2]).disabled == false)
			{
				localName = document.getElementById( localIds[localIndex] + "_label").firstChild.data.replace(/:/, "");
				localName2 = 	document.getElementById( localIds[localIndex2] + "_label").firstChild.data.replace(/:/, "");
				errors.push(localName + " cannot be " + localName2);
			}
		}
	}


	var ak = new Array();
	for (var key in authorizedKeyMap)
	{
		if (authorizedKeyMap.hasOwnProperty(key))
		{
			ak.push(authorizedKeyMap[key]);
		}
	}
	if(ak.length == 0 && (!document.getElementById("pwd_auth_enabled").checked))
	{
		errors.push(accessStr.CnntDsblPwd);
	}


	pass1 = document.getElementById("password1").value;
	pass2 = document.getElementById("password2").value;
	if( (pass1 != "" || pass2 != "") && pass1 != pass2)
	{
		errors.push(accessStr.PasswordsDiffer);
	}
	return errors;
}


function resetData()
{
	// Global variables are used in saveChanges().

	// What client (browser) sees.
	clientProt = window.location.protocol.slice(0, -1);
	clientHost = window.location.hostname;
	clientPort = window.location.port == "" ? (clientProt == "http" ? 80 : 443) : window.location.port;
	// What server (uhttpd) sees.
	serverProt = HTTPS == "on" ? "https" : "http";
	serverHost = HTTP_HOST == "" ? SERVER_ADDR : HTTP_HOST;
	serverPort = SERVER_PORT;

	// Whether client sees what server sees.
	directView = clientProt == serverProt && clientHost == serverHost && clientPort == serverPort;

	// Whether web interface is accessed locally or remotely.
	accessArea = SERVER_ADDR == currentWanIp ? "remote" : "local";

	controlIds = ['local_https_port', 'local_http_port', 'local_ssh_port', 'remote_https_port', 'remote_http_port', 'remote_ssh_port'];
	resetProofreadFields(controlIds);

	document.getElementById("disable_web_password").checked = uciOriginal.get("gargoyle", "global", "require_web_password") == "0" ? true : false;
	setSelectedValue("session_length", uciOriginal.get("gargoyle", "global", "session_timeout"));

	localHttpsPort = getHttpsPort();
	localHttpPort = getHttpPort();

	var oldRedirectHttps = uciOriginal.get("uhttpd", "main", "redirect_https") == "1" ? true : false;

	oldDisableRfc1918Filter = uciOriginal.get("uhttpd", "main", "rfc1918_filter") == "0" ? true : false;
	document.getElementById("disable_rfc1918_filter").checked = oldDisableRfc1918Filter;

	oldLocalWebProtocol = "";
	oldLocalWebProtocol = oldLocalWebProtocol + ( localHttpsPort != "" ? "https" : "" );
	oldLocalWebProtocol = oldLocalWebProtocol + ( localHttpPort != "" ? "http" : "" );
	oldLocalWebProtocol = oldLocalWebProtocol == "httpshttp" ? (oldRedirectHttps ? "redirect" : "both") : oldLocalWebProtocol;

	setSelectedValue("local_web_protocol", oldLocalWebProtocol);

	if(oldLocalWebProtocol == "https" || oldLocalWebProtocol == "redirect" || oldLocalWebProtocol == "both")
	{
		document.getElementById("local_https_port").value = localHttpsPort;
	}
	if(oldLocalWebProtocol == "http" || oldLocalWebProtocol == "redirect" || oldLocalWebProtocol == "both")
	{
		document.getElementById("local_http_port").value  = localHttpPort;
	}




	var dropbearSections = uciOriginal.getAllSections("dropbear");
	var localSshPort = uciOriginal.get("dropbear", dropbearSections[0], "Port");
	var connectionAttempts=uciOriginal.get("dropbear", dropbearSections[0], "max_remote_attempts");
	connectionAttempts = connectionAttempts == "" ? 10 : connectionAttempts;
	setSelectedValue("remote_ssh_attempts", connectionAttempts);
	document.getElementById("local_ssh_port").value = localSshPort;




	oldRemoteHttpsPort = "";
	oldRemoteHttpPort = "";
	oldRemoteSshPort = "";
	var remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept")
	var acceptIndex=0;
	for(acceptIndex=0; acceptIndex < remoteAcceptSections.length; acceptIndex++)
	{
		var section = remoteAcceptSections[acceptIndex];
		var localPort = uciOriginal.get("firewall", section, "local_port");
		var remotePort = uciOriginal.get("firewall", section, "remote_port");
		var proto = uciOriginal.get("firewall", section, "proto").toLowerCase();
		var zone = uciOriginal.get("firewall", section, "zone").toLowerCase();
		if((zone == "wan" || zone == "") && (proto == "tcp" || proto == ""))
		{
			remotePort = remotePort == "" ? localPort : remotePort;
			if(localPort == localHttpsPort)
			{
				oldRemoteHttpsPort = remotePort;
			}
			else if(localPort == localHttpPort)
			{
				oldRemoteHttpPort = remotePort;
			}
			else if(localPort == localSshPort)
			{
				oldRemoteSshPort = remotePort;
			}
		}
	}

	resetAuthorizedKeysTable();
	sshPwdEnabled = uciOriginal.get("dropbear", dropbearSections[0], "PasswordAuth") == "on" ? true : false;
	document.getElementById("pwd_auth_enabled").checked = sshPwdEnabled;
	document.getElementById("public_key_file").value = "";
	document.getElementById('public_key_file').addEventListener('change', readKeyFile, false);

	allOptionValueHash=getRemoteOptionValueHash();
	setSelectElementOptions("remote_web_protocol", ["http", "both", "redirect", "https", "disabled"], allOptionValueHash);
	if(!isBridge(uciOriginal))
	{
		if(oldRemoteHttpsPort != "" && oldRemoteHttpPort != "")
		{
			setSelectedValue("remote_web_protocol", oldRedirectHttps ? "redirect" : "both");
		}
		else if(oldRemoteHttpsPort != "")
		{
			setSelectedValue("remote_web_protocol", "https");
		}
		else if(oldRemoteHttpPort != "")
		{
			setSelectedValue("remote_web_protocol", "http");
		}
		else
		{
			setSelectedValue("remote_web_protocol", "disabled");
		}
		document.getElementById("remote_https_port").value = oldRemoteHttpsPort;
		document.getElementById("remote_http_port").value  = oldRemoteHttpPort;
		document.getElementById("remote_ssh_port").value   = oldRemoteSshPort;

		if(oldRemoteSshPort != "")
		{
			document.getElementById("remote_ssh_enabled").checked = true;
		}
		else
		{
			document.getElementById("remote_ssh_enabled").checked = false;
		}
	}
	else
	{
		setSelectedValue("remote_web_protocol", "disabled");
		document.getElementById("remote_ssh_enabled").checked = false;
		var hideIds = ["remote_web_divider", "remote_web_protocol_container", "remote_web_ports_container", "remote_ssh_enabled_container", "remote_ssh_port_container" ];
		var hi;
		for(hi=0; hi < hideIds.length; hi++)
		{
			document.getElementById(hideIds[hi]).style.display="none";
		}
	}

	//clear public ssh key fields

	document.getElementById('public_key_file').value = '';
	document.getElementById('public_key_name').value = '';
	document.getElementById('file_contents').value = '';



	//clear password fields
	document.getElementById("password1").value = "";
	document.getElementById("password2").value = "";




	//initialize help settings
	initializeDescriptionVisibility(uciOriginal, "ssh_help");
	uciOriginal.removeSection("gargoyle", "help");

	//enable/disable proper fields
	updateVisibility();

}


function updateVisibility()
{
	var localWebProtocol = getSelectedValue("local_web_protocol");
	var localHttpsElement = document.getElementById("local_https_port");
	var localHttpElement = document.getElementById("local_http_port");
	var allOptionValueHash = getRemoteOptionValueHash();
	// Preselects the next more secure remote web protocol when the old
	// selection is no longer available with the new local web protocol.
	// Available web protocols are offered in ascending order regarding
	// security as in ["http", "both", "redirect", "https", "disabled"].
	if(localWebProtocol == "both")
	{
		localHttpsElement.value = localHttpsElement.value == "" ? 443 : localHttpsElement.value;
		localHttpElement.value = localHttpElement.value == "" ? 80 : localHttpElement.value;
		var oldSelection = getSelectedValue("remote_web_protocol");
		setSelectElementOptions("remote_web_protocol", ["http", "both", /* "redirect", */ "https", "disabled"], allOptionValueHash);
		if(oldSelection == "redirect")
		{
			setSelectedValue("remote_web_protocol", "https");
		}
	}
	else if(localWebProtocol == "redirect")
	{
		localHttpsElement.value = localHttpsElement.value == "" ? 443 : localHttpsElement.value;
		localHttpElement.value = localHttpElement.value == "" ? 80 : localHttpElement.value;
		var oldSelection = getSelectedValue("remote_web_protocol");
		setSelectElementOptions("remote_web_protocol", [/* "http", "both", */ "redirect", "https", "disabled"], allOptionValueHash);
		if(oldSelection == "http" || oldSelection == "both")
		{
			setSelectedValue("remote_web_protocol", "redirect");
		}
	}
	else if(localWebProtocol == "https")
	{
		localHttpsElement.value = localHttpsElement.value == "" ? 443 : localHttpsElement.value;
		localHttpElement.value = "";
		var oldSelection = getSelectedValue("remote_web_protocol");
		setSelectElementOptions("remote_web_protocol", [/* "http", "both", "redirect", */ "https", "disabled"], allOptionValueHash);
		if(oldSelection != "disabled")
		{
			setSelectedValue("remote_web_protocol", "https");
		}
	}
	else
	{
		localHttpsElement.value = "";
		localHttpElement.value = localHttpElement.value == "" ? 80 : localHttpElement.value;
		var oldSelection = getSelectedValue("remote_web_protocol");
		setSelectElementOptions("remote_web_protocol", ["http", /* "both", "redirect", "https" */ "disabled"], allOptionValueHash);
		if(oldSelection != "http")
		{
			setSelectedValue("remote_web_protocol", "disabled");
		}
	}

	var remoteWebProtocol = getSelectedValue("remote_web_protocol");
	var remoteHttpsElement = document.getElementById("remote_https_port");
	var remoteHttpElement = document.getElementById("remote_http_port");
	setElementReadOnly(remoteHttpsElement, false);
	if(remoteWebProtocol == "both")
	{
		remoteHttpsElement.value = remoteHttpsElement.value == "" ? localHttpsElement.value : remoteHttpsElement.value;
		remoteHttpElement.value = remoteHttpElement.value == "" ? localHttpElement.value : remoteHttpElement.value;
	}
	else if(remoteWebProtocol == "redirect")
	{
		setElementReadOnly(remoteHttpsElement, true);
		remoteHttpsElement.value = localHttpsElement.value;
		remoteHttpElement.value = remoteHttpElement.value == "" ? localHttpElement.value : remoteHttpElement.value;
	}
	else if(remoteWebProtocol == "https")
	{
		if(localWebProtocol == "redirect")
		{
			setElementReadOnly(remoteHttpsElement, true);
			remoteHttpsElement.value = localHttpsElement.value;
		}
		else
		{
			remoteHttpsElement.value = remoteHttpsElement.value == "" ? localHttpsElement.value : remoteHttpsElement.value;
		}
		remoteHttpElement.value = "";
	}
	else if(remoteWebProtocol == "http")
	{
		remoteHttpsElement.value = "";
		remoteHttpElement.value = remoteHttpElement.value == "" ? localHttpElement.value : remoteHttpElement.value;
	}
	else
	{
		remoteHttpsElement.value = "";
		remoteHttpElement.value = "";
	}
	document.getElementById("noip6redi").style.display = remoteWebProtocol != "disabled" ? "block" : "none";

	// RFC 1918 filter checkbox.
	var disableRfc1918Filter = document.getElementById("disable_rfc1918_filter").checked;
	// Either local or remote web protocol.
	var accessWebProtocol = getSelectedValue(accessArea + "_web_protocol");
	// Warn when about to lock out by disabling remote web access.
	var protocolLockout = accessArea == "remote" && remoteWebProtocol == "disabled";
	// Warn when about to lock out by enabling RFC 1918 filter again.
	var rfc1918FilterLockout = accessArea == "remote" && remoteWebProtocol != "disabled" && !disableRfc1918Filter && ipInPrivate(REMOTE_ADDR) && !ipInPrivate(SERVER_ADDR);
	// Warn when about to lock out on unfit protocol change if uhttpd is used as a reverse proxy's backend.
	var reverseProxyLockout = !directView && accessWebProtocol != "disabled" && accessWebProtocol != "both" && accessWebProtocol.replace("redirect", "https") != serverProt;
	// Reset warning container for either local or remote web protocol.
	var accessWebProtocolLockoutContainer = document.getElementById(accessArea + "_web_protocol_lockout_container");
	accessWebProtocolLockoutContainer.style.display = "none";
	accessWebProtocolLockoutContainer.innerHTML = "";
	// Reset warning container for RFC 1918 filter checkbox.
	var rfc1918FilterLockoutContainer = document.getElementById("rfc1918_filter_lockout_container");
	rfc1918FilterLockoutContainer.style.display = "none";
	rfc1918FilterLockoutContainer.innerHTML = "";
	var lockoutTemplate =
	'<span class="alert alert-danger col-xs-12" role="alert" id="lockout">' +
		'<p>' +  accessStr.LockoutWarning + '</p>' +
		'<span id="reverse_proxy_smooth" style="display: none">' +
			'<p>' + accessStr.ReverseProxySmooth + ':</p>' +
			'<ol>' +
				'<li>' + accessStr.ReverseProxySelect + ' <i>HTTP & HTTPS</i> ' + accessStr.ReverseProxySaveCh + '.</li>' +
				'<li>' + accessStr.ReverseProxySwitch + ' <i><span id="reverse_proxy_switch"></span></i>.</li>' +
				'<li>' + accessStr.ReverseProxySelect + ' <i><span id="reverse_proxy_select"></span></i> ' + accessStr.ReverseProxySaveCh + '.</li>' +
			'</ol>' +
		'</span>' +
		'<p>' + accessStr.AccessChain + ':</p>' +
		'<p><span id="access_chain"></span></p>' +
	'</span>';
	if(protocolLockout || reverseProxyLockout || rfc1918FilterLockout)
	{
		var lockoutContainer = rfc1918FilterLockout ? rfc1918FilterLockoutContainer : accessWebProtocolLockoutContainer;
		lockoutContainer.innerHTML = lockoutTemplate;

		// Help avoid lockout with reverse proxy.
		document.getElementById("reverse_proxy_smooth").style.display = reverseProxyLockout ? "block" : "none";
		document.getElementById("reverse_proxy_switch").innerText = allOptionValueHash[serverProt == "http" ? "https" : "http"];
		document.getElementById("reverse_proxy_select").innerText = allOptionValueHash[accessWebProtocol];

		// Web access trace of domains and/or IP addresses from client, possibly over reverse proxy, to server.
		var lastOccurrence = function(value, index, self) { return value && self.lastIndexOf(value) === index; };
		var webAccessChain = [window.location.hostname, REMOTE_ADDR, HTTP_HOST, SERVER_ADDR].filter(lastOccurrence);
		document.getElementById("access_chain").innerText = webAccessChain.join(" ➔ ");

		lockoutContainer.style.display = "block";
	}

	var ids= ["local_https_port", "local_http_port", "remote_https_port", "remote_http_port"];
	var visIds=[];
	var vis = [];
	for(idIndex = 0; idIndex < ids.length; idIndex++)
	{
		visIds.push(ids[idIndex] + "_container");
		vis.push( document.getElementById(ids[idIndex]).value != "" ? true : false );
	}
	visIds.push("disable_rfc1918_filter_container");
	vis.push(remoteWebProtocol != "disabled");
	setVisibility(visIds, vis);

	var dropbearSections = uciOriginal.getAllSections("dropbear");
	var defaultConnectionAttempts=uciOriginal.get("dropbear", dropbearSections[0], "max_remote_attempts");
	defaultConnectionAttempts = defaultConnectionAttempts == "" ? 10 : defaultConnectionAttempts;

	enableAssociatedField( document.getElementById("remote_ssh_enabled"), "remote_ssh_port", document.getElementById("local_ssh_port").value);
	enableAssociatedField( document.getElementById("remote_ssh_enabled"), "remote_ssh_attempts", defaultConnectionAttempts);
}

function setSelectElementOptions(selectId, enabledOptionValues, possibleOptionValueToTextHash)
{
	var originalSelection = getSelectedValue(selectId);
	removeAllOptionsFromSelectElement(document.getElementById(selectId));
	for(addIndex=0; addIndex < enabledOptionValues.length; addIndex++)
	{
		var optionValue = enabledOptionValues[addIndex];
		addOptionToSelectElement(selectId, possibleOptionValueToTextHash[optionValue], optionValue);
	}
	setSelectedValue(selectId, originalSelection);

}



function getRemoteOptionValueHash()
{
	var allOptionValueHash = new Array();
	allOptionValueHash["http"]     = "HTTP";
	allOptionValueHash["both"]     = "HTTP & HTTPS";
	allOptionValueHash["redirect"] = "HTTP ➔ HTTPS";
	allOptionValueHash["https"]    = "HTTPS";
	allOptionValueHash["disabled"] = UI.disabled;
	return allOptionValueHash;
}



function resetAuthorizedKeysTable()
{
	var keysTableData = new Array();
	for (var keyName in authorizedKeyMap)
	{
		if (authorizedKeyMap.hasOwnProperty(keyName))
		{
			var keyLine = authorizedKeyMap[keyName]
			var splitKey = keyLine.split(/[\t ]+/);
			var keyAbbrev = ""
			var keyName = ""
			if(splitKey.length > 1)
			{
				var keyData = splitKey[1]
				keyAbbrev = keyData.substr(0,5) + "... " + keyData.substr( keyData.length-6 ,5)
			}
			if(splitKey.length > 2)
			{
				keyName = splitKey[2]
			}
			keysTableData.push([keyAbbrev,keyName])
		}
	}

	var keysTable = createTable(['',''], keysTableData, "authorized_keys_table", true, false, removeKey);
	var tableContainer = document.getElementById('authorized_keys_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(keysTable);
}



function removeKey(table, row)
{
	var key = row.childNodes[1].firstChild.data;
	delete authorizedKeyMap[key];
	resetAuthorizedKeysTable();
}


function addKey()
{
	var file_contents = document.getElementById('file_contents').value;
	var splitKey = file_contents.split(/[\t ]+/);
	var keyName = document.getElementById('public_key_name').value
	if(keyName.length == 0 && splitKey.length > 2)
	{
		keyName = splitKey[2];
	}
	keyName = keyName.replace(/^[\r\n\t ]+/g, "");
	keyName = keyName.replace(/[\r\n\t ]+$/g, "");
	keyName = keyName.replace(/[\r\n\t ]+/g, "_");
	if(splitKey.length < 2 || keyName.length == 0)
	{
		alert(accessStr.SSHInvalidKey);
	}
	else
	{
		var key = splitKey[0] + " " + splitKey[1] + " " + keyName
		authorizedKeyMap[keyName]=key;
		resetAuthorizedKeysTable()
		document.getElementById('public_key_file').value = '';
		document.getElementById('public_key_name').value = '';
		document.getElementById('file_contents').value = '';

	}

}


function readKeyFile(e)
{
	var file = e.target.files[0];
	if (!file)
	{
		return;
	}
	var reader = new FileReader();
	reader.onload = function(e){ document.getElementById('file_contents').value = e.target.result; };
	reader.readAsText(file);
}
