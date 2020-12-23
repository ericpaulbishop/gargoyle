/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

const pkg = "qr_code_gargoyle";

let uci = {};

function updateEditor()
{
	// QR code editor selection.
	const editor = byId("application");
	// Print/Embed checkbox IDs.
	const ids = ["print_subject", "print_secrets", "print_comment", "embed_via_lan", "embed_via_wan"];
	// Comment text area.
	const comment = byId("comment");

	// Deselected (previously selected) application to save.
	let application = editor.getAttribute("data-application");
	if(application)
	{
		// Each application has its own editor section.
		uci.set(pkg, application, "", "editor");
		// Save print/embed checkboxes.
		ids.forEach(id => uci.set(pkg, application, id, byId(id).checked ? "1" : "0"));
		// Save multi-line comment.
		uci.setLines(pkg, application, "comment_line", comment.value);
		// Save and return to keep viewer on save button click.
		if(application == editor.value)
		{
			return;
		}
	}

	// Selected application to load.
	application = editor.value;
	if(application)
	{
		// Load print/embed checkboxes with true, false, false, false, false as defaults.
		byId(ids[0]).checked = uci.get(pkg, application, ids[0]) != "0";
		ids.slice(1).forEach(id => byId(id).checked = uci.get(pkg, application, id) == "1");
		// Load multi-line comment.
		comment.value = uci.getLines(pkg, application, "comment_line");
	}
	// Remember to save this application on deselect.
	editor.setAttribute("data-application", application);
	// Set title of editor panel.
	byId("title").innerHTML = application ? getOptGroup(editor).label : QrCode.QrCodeEditor;

	// Set subject/secrets labels.
	let subject = QrCode.Ssid;
	let secrets = QrCode.Password;
	if(application.includes("web"))
	{
		subject = QrCode.Url;
		secrets = application.includes("cam") ? QrCode.LoginCredentials : "";
	}
	byId("print_subject_label").innerHTML = subject;
	byId("print_secrets_label").innerHTML = secrets;

	// Update editor visibility.
	byId("print_secrets_container").style.display = secrets ? "" : "none";
	["print_container", "embed_container", "qr_code_help"]
		.forEach(id => byId(id).style.display = application ? "": "none");
	["wifi", "web_access", "webcam"]
		.forEach(id => byId(id + "_help").style.display = application.includes(id) ? "": "none");

	// QR code viewer selection.
	const viewer = byId("qr_code");
	// Reset QR code viewer selection.
	viewer.innerHTML = "";
	addPlaceholderOption(viewer, QrCode.SelectQrCode);

	// Add home/guest wireless network groups.
	if(application.endsWith("wifi"))
	{
		// Wether to add guest wireless network group and options.
		const guest = application.includes("guest");
		// Add home/guest wireless network group.
		const wifi = addWifiQrCodeApplication(viewer, guest ? QrCode.GuestWifi : QrCode.HomeWifi, application);
		// Add home/guest wireless network options to their corresponding group.
		const wireless = "wireless";
		const wirelessSections = uciOriginal.getAllSectionsOfType(wireless, "wifi-iface");
		for(const section of wirelessSections)
		{
			const ssid = uciOriginal.get(wireless, section, "ssid");
			const hidden = uciOriginal.get(wireless, section, "hidden") == "1";
			const encryption = uciOriginal.get(wireless, section, "encryption");
			const key = uciOriginal.get(wireless, section, "key");
			addWifiQrCode(wifi, section, ssid, hidden, encryption, key, guest);
		}
	}

	// Filter and map global IPv6 addresses as hosts for IP URLs.
	const globalIp6 = ip => ip6_scope(ip)[0] == "Global";
	const ip6Host = ip => `[${ip}]`;
	const currentLanIps = [currentLanIp].concat(currentLanIp6.filter(globalIp6).map(ip6Host));
	const currentWanIps = [currentWanIp].concat(currentWanIp6.filter(globalIp6).map(ip6Host));

	// Load host for host URLs.
	const systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	const dnsmasqSections = uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq");
	const hostname = uciOriginal.get("system", systemSections[0], "hostname");
	const domain = uciOriginal.get("dhcp", dnsmasqSections[0], "domain");
	const host = hostname + "." + domain;

	// Add local/remote/indirect router access groups and IP/host/client URL options.
	if(application == "web_access")
	{
		// Load local/remote http/https ports.
		const localHttpPort = getHttpPort(uciOriginal);
		const localHttpsPort = getHttpsPort(uciOriginal);
		let remoteHttpPort = "";
		let remoteHttpsPort = "";
		const remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept")
		for(const section of remoteAcceptSections)
		{
			const localPort = uciOriginal.get("firewall", section, "local_port");
			let remotePort = uciOriginal.get("firewall", section, "remote_port");
			remotePort = remotePort ? remotePort : localPort;
			const proto = uciOriginal.get("firewall", section, "proto").toLowerCase();
			const zone = uciOriginal.get("firewall", section, "zone").toLowerCase();
			if((zone == "wan" || !zone) && (proto == "tcp" || !proto))
			{
				if(localPort == localHttpsPort)
				{
					remoteHttpsPort = remotePort;
				}
				else if(localPort == localHttpPort)
				{
					remoteHttpPort = remotePort;
				}
			}
		}

		// Add local URL group.
		const local = addUrlQrCodeApplication(viewer, QrCode.LocalWebAccess, application, false);
		// Add local IP URLs.
		for(const ip of currentLanIps)
		{
			addUrlQrCode(local, "http", ip, localHttpPort);
			addUrlQrCode(local, "https", ip, localHttpsPort);
		}
		// Add local host URLs.
		addUrlQrCode(local, "http", host, localHttpPort);
		addUrlQrCode(local, "https", host, localHttpsPort);
		// Add local client URL.
		if(directView && !remoteView)
		{
			addUrlQrCode(local, clientProt, clientHost, clientPort);
		}

		// Add remote URL group.
		const remote = addUrlQrCodeApplication(viewer, QrCode.RemoteWebAccess, application, true);
		// Add remote IP URLs.
		for(const ip of currentWanIps)
		{
			addUrlQrCode(remote, "http", ip, remoteHttpPort);
			addUrlQrCode(remote, "https", ip, remoteHttpsPort);
		}
		// Add remote client URL.
		if(directView && remoteView)
		{
			addUrlQrCode(remote, clientProt, clientHost, clientPort);
		}

		// Add indirect URL group.
		const indirect = addUrlQrCodeApplication(viewer, QrCode.IndirectWebAccess, application, remoteView);
		// Add indirect client URL.
		if(!directView)
		{
			addUrlQrCode(indirect, clientProt, clientHost, clientPort);
		}
	}

	// Add local/remote/indirect webcam snapshot/stream groups and IP/host/client URL options.
	if(application.startsWith("webcam"))
	{
		const mjpgStreamer = "mjpg-streamer", core = "core";
		// Wether to add snapshot group and options.
		const snapshot = application.endsWith("snapshot");
		// Load local port.
		const localWebcamPort = uciOriginal.get(mjpgStreamer, core, "port");
		// Offer remote access even when disabled by using local port since
		// remote port either equals local port or is empty when disabled.
		const remoteWebcamPort = localWebcamPort;
		// Load login credentials.
		const user = uciOriginal.get(mjpgStreamer, core, "username");
		const pass = uciOriginal.get(mjpgStreamer, core, "password");

		// URL parameters.
		const path = "/";
		const parm = "?action=" + (snapshot ? "snapshot" : "stream");

		// Add local URL group.
		const local = addUrlQrCodeApplication(viewer, snapshot ? QrCode.LocalWebcamSnapshot : QrCode.LocalWebcamStream, application, false);
		// Add local IP URL.
		for(const ip of currentLanIps)
		{
			addUrlQrCode(local, "http", ip, localWebcamPort, user, pass, path, parm);
		}
		// Add local host URL.
		addUrlQrCode(local, "http", host, localWebcamPort, user, pass, path, parm);
		// Add local client URL.
		if(directView && !remoteView)
		{
			addUrlQrCode(local, clientProt, clientHost, localWebcamPort, user, pass, path, parm);
		}

		// Add remote URL group.
		const remote = addUrlQrCodeApplication(viewer, snapshot ? QrCode.RemoteWebcamSnapshot : QrCode.RemoteWebcamStream, application, true);
		// Add remote IP URL.
		for(const ip of currentWanIps)
		{
			addUrlQrCode(remote, "http", ip, localWebcamPort, user, pass, path, parm);
		}
		// Add local client URL.
		if(directView && remoteView)
		{
			addUrlQrCode(remote, clientProt, clientHost, remoteWebcamPort, user, pass, path, parm);
		}

		// Add indirect URL group.
		const indirect = addUrlQrCodeApplication(viewer, snapshot ? QrCode.IndirectWebcamSnapshot : QrCode.IndirectWebcamStream, application, remoteView);
		// Add indirect client URL.
		if(!directView)
		{
			addUrlQrCode(indirect, clientProt, clientHost, clientPort, user, pass, path, parm);
		}
	}

	// Disable QR code viewer with no options.
	viewer.disabled = !hasOptions(viewer);
}

function updateViewer() {
	setQrCodeTitle();
	setQrCodeAlert();
	// Set QR code alert info.
	const optgroup = getOption(byId("qr_code")).parentNode;
	const application = optgroup.getAttribute("data-application");
	if(application && application.startsWith("webcam"))
	{
		if(uci.get("mjpg-streamer", "core", "enabled") == "1")
		{
			// Set alert info if webcam is enabled and a remote URL is selected but remote access is disabled.
			const remote = optgroup.getAttribute("data-remote") == "true";
			const remotePort = uci.get("firewall", "webcam_wan_access", "remote_port");
			setQrCodeAlert(remote && !remotePort ? QrCode.RemoteWebcamDisabled : "");
		}
		else
		{
			// Set alert info if webcam is disabled.
			setQrCodeAlert(QrCode.WebcamDisabled);
		}
	}
	setQrCodeFrame();
	updatePrint();
	updateGuide();
	scrollUntilInView(byId("qr_code_viewer"));
}

function updatePrint() {
	const print = (application, id) => byId(id).checked;
	const guide = (application, id) => byId(id).value;
	setQrCodePrint(print, guide);
}

function updateGuide() {
	// Comment print checkbox value and text area.
	const print = byId("print_comment").checked;
	const comment = byId("comment");
	// Update comment text area visibility.
	byId("comment_container").style.display = print && byId("application").value ? "" : "none";
	// Update comment text area height after updating its visibility.
	autoHeight(comment);
	// Render Markdown comment if checked and viewed.
	setQrCodeGuide(print && byId("qr_code").value ? comment.value : "");
}

function resetData()
{
	// Add button event listeners.
	byId("reset_button").addEventListener("click", resetData);
	byId("save_button").addEventListener("click", saveChanges);
	// Add editor event listeners.
	byId("application").addEventListener("change", updateEditor);
	byId("application").addEventListener("change", updateViewer);
	byId("print_subject").addEventListener("click", updatePrint);
	byId("print_secrets").addEventListener("click", updatePrint);
	byId("print_comment").addEventListener("click", updateGuide);
	byId("comment").addEventListener("input", updateGuide);
	// Add viewer event listeners.
	byId("qr_code").addEventListener("change", updateViewer);

	// Update help visibility.
	initializeDescriptionVisibility(uciOriginal, "qr_code_help");
	uciOriginal.removeSection("gargoyle", "help");

	// Reset UCI.
	uci = uciOriginal.clone();

	// Reset editor selection.
	const editor = byId("application");
	editor.innerHTML = "";
	editor.removeAttribute("data-application");
	addPlaceholderOption(editor, QrCode.SelectApplication);

	// Add wireless network group and options.
	const wifi = addOptGroup(editor, QrCode.Wifi);
	addOption(wifi, QrCode.HomeWifi, "home_wifi");
	addOption(wifi, QrCode.GuestWifi, "guest_wifi");
	// Add router access group and options.
	const access = addOptGroup(editor, QrCode.RouterAccess);
	addOption(access, QrCode.WebAccess, "web_access");
	// Add webcam group and options.
	const webcam = addOptGroup(editor, QrCode.Webcam);
	addOption(webcam, QrCode.WebcamSnapshot, "webcam_snapshot");
	addOption(webcam, QrCode.WebcamStream, "webcam_stream");
	// Hide webcam group when webcam plugin is not installed, that is with no mandatory local port.
	webcam.hidden = uci.get("mjpg-streamer", "core", "port") == "";
	editor.disabled = !hasOptions(editor);

	// Update editor visibility.
	updateEditor();

	// Load QR code width with 50 as default.
	const width = +uci.get(pkg, "viewer", "width");
	setQrCodeWidth(width ? width : 50);
	// Update viewer visibility.
	updateViewer();
}

function saveChanges()
{
	// Disable controls until changes are saved.
	setControlsEnabled(false, true);

	// Save current editor application.
	updateEditor();

	// Save shared login hook resources.
	const global = "gargoyle", plugin = "qr_code";
	uci.removeSection(global, plugin);
	const embed = (embed, application) => embed
		|| uci.get(pkg, application, "embed_via_lan") == "1"
		|| uci.get(pkg, application, "embed_via_wan") == "1";
	if(["home_wifi", "guest_wifi", "web_access", "webcam_snapshot", "webcam_stream"].reduce(embed, false))
	{
		uci.set(global, plugin, "", "login_hook");
		uci.set(global, plugin, "css", "qr_code.css");
		uci.set(global, plugin, "js", "drawdown.js qrcodegen.js qr_code_common.js");
		uci.set(global, plugin, "i18n", "qr_code.js");
		uci.set(global, plugin, "pkg", pkg);
	}

	// Save QR code width.
	const viewer = "viewer";
	uci.set(pkg, viewer, "", viewer);
	uci.set(pkg, viewer, "width", byId("qr_code_width").value);

	// Save changes.
	const commands = uci.getScriptCommands(uciOriginal) + "\n";
	const hash = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
	const param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", hash);
	const stateChangeFunction = req =>
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			setControlsEnabled(true);
		}
	};
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
