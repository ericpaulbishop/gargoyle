/*
 * This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var WebC=new Object();

var pkg = "mjpg-streamer";
var webcamWanAccess = "webcam_wan_access"

function resetData()
{
	for (idx in webcams)
	{
		addOptionToSelectElement('webcam_device', webcams[idx]['webcam'], idx);
	}

	if (document.getElementById('webcam_device').options.length == 0)
	{
		document.getElementById("nowebcam").style.display = "block";
		document.getElementById("webcam").style.display = "none";
		document.getElementById("webcam_preview").style.display = "none";
		document.getElementById("bottom_button_container").style.display = "none";
		return;
	}

	document.getElementById("nowebcam").style.display = "none";
	document.getElementById("webcam").style.display = "block";
	document.getElementById("bottom_button_container").style.display = "block";

	var enabled = uciOriginal.get(pkg, "core", "enabled");
	document.getElementById("webcam_enable").checked = enabled == 1;
	updateWebcamWanAccess();

	var device = uciOriginal.get(pkg, "core", "device");
	setSelectedValue("webcam_device", device);
	fillRes(getSelectedValue("webcam_device"));

	var res = uciOriginal.get(pkg, "core", "resolution");
	setSelectedValue("webcam_res", res);

	var yuv = uciOriginal.get(pkg, "core", "yuv");
	document.getElementById("webcam_yuv").checked = yuv == 1;

	var port = uciOriginal.get(pkg, "core", "port");
	document.getElementById('webcam_port').value = port;

	var wanWebcam = uciOriginal.get("firewall", webcamWanAccess, "local_port") == port;
	document.getElementById("webcam_wan_access").checked = wanWebcam;

	var fps = uciOriginal.get(pkg, "core", "fps");
	document.getElementById('webcam_fps').value = fps;

	var username = uciOriginal.get(pkg, "core", "username");
	document.getElementById('webcam_username').value = username;
	var password = uciOriginal.get(pkg, "core", "password");
	document.getElementById('webcam_password').value = password;

	showPreview(enabled, port, username, password);
}

function fillRes(device)
{
	document.getElementById('webcam_res').options.length = 0;
	for (idx in webcams[device]['res'])
	{
		addOptionToSelectElement('webcam_res', webcams[device]['res'][idx], webcams[device]['res'][idx] );
	}
}

function saveChanges()
{
	var Commands = [];
	var enabled = document.getElementById("webcam_enable").checked ? "1":"0";
	var wanAccess = document.getElementById("webcam_wan_access").checked;
	var device = getSelectedValue("webcam_device");
	var res = getSelectedValue("webcam_res");
	var yuv = document.getElementById("webcam_yuv").checked ? "1":"0";
	var port = document.getElementById('webcam_port').value;
	var fps = document.getElementById('webcam_fps').value;
	var username = document.getElementById('webcam_username').value;
	var password = document.getElementById('webcam_password').value;

	var errors =[];

	var test59 = function(text){ return validateNumericRange(text,1,59); }
	var addIds=["webcam_fps","webcam_port"];
	var labelIds=["webcam_fps_label","webcam_port_label"];
	var functions = [test59, validatePort];
	var errors = proofreadFields(addIds, labelIds, functions, [0,0,0,0], addIds, document);

	if (errors.length == 0 && uciOriginal.get("firewall", webcamWanAccess, "local_port") != ""+port )
	{
		var conflict = checkForPortConflict("" + port, "tcp");
		if(conflict != "")
		{
			errors.push(WebC.ErrorPortWebC + conflict);
		}
	}

	if (errors.length > 0)
	{
		errorString = errors.join("\n") + "\n\n"+UI.ErrChanges;
		alert(errorString);
		return;
	}

	var uci = uciOriginal.clone()

	uci.set(pkg, "core", "enabled", enabled);
	uci.set(pkg, "core", "device", device);
	uci.set(pkg, "core", "resolution", res);
	uci.set(pkg, "core", "port", port);
	uci.set(pkg, "core", "fps", fps);
	uci.set(pkg, "core", "yuv", yuv);
	uci.set(pkg, "core", "username", username);
	uci.set(pkg, "core", "password", password);
	uci.removeSection("firewall", webcamWanAccess);
	if(enabled==1)
	{
		Commands.push("/etc/init.d/mjpg-streamer enable")
		Commands.push("/etc/init.d/mjpg-streamer stop")
		Commands.push("sleep 2")
		Commands.push("/etc/init.d/mjpg-streamer start")
		if (wanAccess)
		{
			uci.set("firewall", webcamWanAccess, "",            "remote_accept")
			uci.set("firewall", webcamWanAccess, "proto",       "tcp")
			uci.set("firewall", webcamWanAccess, "zone",        "wan")
			uci.set("firewall", webcamWanAccess, "local_port",  ""+port)
			uci.set("firewall", webcamWanAccess, "remote_port", ""+port)
		}
	}
	else
	{
		Commands.push("/etc/init.d/mjpg-streamer disable")
		Commands.push("/etc/init.d/mjpg-streamer stop")
	}
	Commands.push('/etc/init.d/firewall restart');
	var commands = uci.getScriptCommands(uciOriginal) + "\n" + Commands.join("\n")

	setControlsEnabled(false, true, UI.WaitSettings);
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			showPreview(enabled, port, username, password);
			uciOriginal = uci.clone();
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function showPreview(enabled, port, username, password)
{
	// Webcam elements.
	var preview = document.getElementById("webcam_preview");
	var snapshot = document.getElementById("webcam_snapshot");
	var snapshot_url = document.getElementById("webcam_snapshot_url");
	var stream = document.getElementById("webcam_stream");
	var stream_url = document.getElementById("webcam_stream_url");
	var frame = document.getElementById("webcam_frame");
	var image = frame.firstElementChild;
	var alert = document.getElementById("webcam_alert");
	// Hide preview panel.
	preview.style.display = "none";
	// Hide alert info.
	alert.style.display = "none";
	// Force reload in case URL is unchanged but other parameters have changed.
	image.src = "";
	// Finish here if disabled.
	if(enabled == "0")
	{
		return;
	}

	// TODO Deprecated but widely supported. Consider token based authentication in the future.
	if(username != "" && password != "")
	{
		auth=username + ":" + password + "@";
	}
	else
	{
		auth="";
	}
	// What client (browser) sees.
	var clientProt = window.location.protocol.slice(0, -1);
	var clientHost = window.location.hostname;
	var clientPort = window.location.port ? window.location.port : clientProt == "http" ? 80 : 443;
	// What server (uhttpd) sees.
	var serverProt = HTTPS == "on" ? "https" : "http";
	var serverHost = HTTP_HOST ? HTTP_HOST : SERVER_ADDR;
	var serverPort = SERVER_PORT;
	// Whether client sees what server sees.
	var directView = clientProt == serverProt && clientHost == serverHost && clientPort == serverPort;
	// Use saved port except when there is a reverse proxy. In that case use clientPort and simply
	// assume that reverse proxy passes the queries `?action=snapshot` and `?action=stream` to the
	// saved port. There is no reason for a reverse proxy to use a separate port on its own and we
	// cannot know it anyway since the port we see here is its web port and not its webcam port.
	var targetPort = directView ? port : clientPort;
	// Hide default ports for browsers which don't (in case of a reverse proxy).
	targetPort = targetPort == "80" || targetPort == "443" ? "" : ":" + targetPort;
	// Final URL without action value.
	var targetHref = clientProt + '://' + auth + clientHost + targetPort + '/?action=';

	// Setup snapshot URL, the `a` and `input` tag.
	snapshot.href = targetHref + 'snapshot';
	snapshot_url.value = snapshot.href;
	// Setup stream URL, the `a` and `input` tag.
	stream.href = targetHref + 'stream';
	stream_url.value = stream.href;
	// Setup preview frame, the `a` and `img` tag.
	frame.href = stream.href;
	image.src = stream.href;
	// As soon as stream is loaded, show frame.
	image.onload = function()
	{
		frame.style.display = "";
	};
	// If it fails to load, show alert info and hint at opening it manually. It could be that the
	// browser blocked this *subresource* URL due to embedded login credentials which is deprecated.
	image.onerror = function()
	{
		frame.style.display = "none";
		alert.style.display = "";
	};
	// Show preview panel.
	preview.style.display = "block";
}

function copy(input)
{
	input.select();
	document.execCommand("copy");
}

function updateWebcamWanAccess()
{
	if(document.getElementById("webcam_enable").checked)
	{
		setElementEnabled(document.getElementById('webcam_wan_access'), true, false);
	}
	else
	{
		setElementEnabled(document.getElementById('webcam_wan_access'), false, false);
		document.getElementById("webcam_wan_access").checked = false;
	}
}
