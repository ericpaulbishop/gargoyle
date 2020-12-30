/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

let pkg = "qr_code_gargoyle";

let uci = {};

function updateEditor()
{
	// QR code editor selection.
	let editor = byId("application");
	// Print/Embed checkbox IDs.
	let ids = ["print_subject", "print_secrets", "print_comment", "embed_via_lan", "embed_via_wan"];
	// Comment text area.
	let comment = byId("comment");

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
	else if(!application.includes("wifi"))
	{
		subject = QrCode.Name;
		secrets = QrCode.KeyPair;
	}
	byId("print_subject_label").innerHTML = subject;
	byId("print_secrets_label").innerHTML = secrets;

	// Update editor visibility.
	byId("print_secrets_container").style.display = secrets ? "block" : "none";
	["print_container", "embed_container", "qr_code_help"]
		.forEach(id => byId(id).style.display = application ? "block": "none");
	["wifi", "wireguard", "web_access", "webcam"]
		.forEach(id => byId(id + "_help").style.display = application.includes(id) ? "block": "none");

	// QR code viewer selection.
	let viewer = byId("qr_code");
	// Reset QR code viewer selection.
	viewer.innerHTML = "";
	addPlaceholderOption(viewer, QrCode.SelectQrCode);

	// Add home/guest wireless network groups.
	if(application.endsWith("wifi"))
	{
		// Wether to add guest wireless network group and options.
		let guest = application.includes("guest");
		// Add home/guest wireless network group.
		let wifi = addWifiQrCodeApplication(viewer, guest ? QrCode.GuestWifi : QrCode.HomeWifi, application);
		// Add home/guest wireless network options to their corresponding group.
		let wireless = "wireless";
		let wirelessSections = uciOriginal.getAllSectionsOfType(wireless, "wifi-iface");
		for(let section of wirelessSections)
		{
			let ssid = uciOriginal.get(wireless, section, "ssid");
			let hidden = uciOriginal.get(wireless, section, "hidden") == "1";
			let encryption = uciOriginal.get(wireless, section, "encryption");
			let key = uciOriginal.get(wireless, section, "key");
			addWifiQrCode(wifi, section, ssid, hidden, encryption, key, guest);
		}
	}

	// Add allowed WireGuard client group and options.
	if(application == "wireguard")
	{
		// Add allowed WireGuard client group.
		let client = addWireGuardQrCodeApplication(viewer, QrCode.WireGuardClient, application);
		// Add allowed WireGuard client group options.
		let wireGuard = "wireguard_gargoyle";
		let wireGuardSections = uciOriginal.getAllSectionsOfType(wireGuard, "allowed_client");
		for(let id of wireGuardSections)
		{
			let name = uciOriginal.get(wireGuard, id, "name");
			let iface = {
				address: uciOriginal.get(wireGuard, id, "ip") + "/32",
				publicKey: uciOriginal.get(wireGuard, id, "public_key"),
				privateKey: uciOriginal.get(wireGuard, id, "private_key"),
			};
			let peer = {
				allowedIPs: uci.get(wireGuard, "server", "all_client_traffic") == "false"
					? ipToStr(parseIp(currentLanIp) & parseIp(currentLanMask)) + "/" + parseCidr(currentLanMask) : "0.0.0.0/0",
				endpoint: uci.get(wireGuard, id, "remote") + ":" + uci.get(wireGuard, "server", "port"),
				publicKey: uciOriginal.get(wireGuard, "server", "public_key"),
			};
			let enabled = uciOriginal.get(wireGuard, id, "enabled") == "1";
			addWireGuardQrCode(client, name, iface, peer, enabled);
		}
	}

	// Filter and map global IPv6 addresses as hosts for IP URLs.
	let globalIp6 = ip => ip6_scope(ip)[0] == "Global";
	let ip6Host = ip => "[" + ip + "]";
	let currentLanIps = [currentLanIp].concat(currentLanIp6.filter(globalIp6).map(ip6Host));
	let currentWanIps = [currentWanIp].concat(currentWanIp6.filter(globalIp6).map(ip6Host));

	// Load host for host URLs.
	let systemSections = uciOriginal.getAllSectionsOfType("system", "system");
	let dnsmasqSections = uciOriginal.getAllSectionsOfType("dhcp", "dnsmasq");
	let hostname = uciOriginal.get("system", systemSections[0], "hostname");
	let domain = uciOriginal.get("dhcp", dnsmasqSections[0], "domain");
	let host = hostname + "." + domain;

	// Add local/remote/indirect router access groups and IP/host/client URL options.
	if(application == "web_access")
	{
		// Load local/remote http/https ports.
		let localHttpPort = getHttpPort(uciOriginal);
		let localHttpsPort = getHttpsPort(uciOriginal);
		let remoteHttpPort = "";
		let remoteHttpsPort = "";
		let remoteAcceptSections = uciOriginal.getAllSectionsOfType("firewall", "remote_accept")
		for(let section of remoteAcceptSections)
		{
			let localPort = uciOriginal.get("firewall", section, "local_port");
			let remotePort = uciOriginal.get("firewall", section, "remote_port");
			remotePort = remotePort ? remotePort : localPort;
			let proto = uciOriginal.get("firewall", section, "proto").toLowerCase();
			let zone = uciOriginal.get("firewall", section, "zone").toLowerCase();
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
		let local = addUrlQrCodeApplication(viewer, QrCode.LocalWebAccess, application, false);
		// Add local IP URLs.
		for(let ip of currentLanIps)
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
		let remote = addUrlQrCodeApplication(viewer, QrCode.RemoteWebAccess, application, true);
		// Add remote IP URLs.
		for(let ip of currentWanIps)
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
		let indirect = addUrlQrCodeApplication(viewer, QrCode.IndirectWebAccess, application, remoteView);
		// Add indirect client URL.
		if(!directView)
		{
			addUrlQrCode(indirect, clientProt, clientHost, clientPort);
		}
	}

	// Add local/remote/indirect webcam snapshot/stream groups and IP/host/client URL options.
	if(application.startsWith("webcam"))
	{
		let mjpgStreamer = "mjpg-streamer", core = "core";
		// Wether to add snapshot group and options.
		let snapshot = application.endsWith("snapshot");
		// Load local port.
		let localWebcamPort = uciOriginal.get(mjpgStreamer, core, "port");
		// Offer remote access even when disabled by using local port since
		// remote port either equals local port or is empty when disabled.
		let remoteWebcamPort = localWebcamPort;
		// Load login credentials.
		let user = uciOriginal.get(mjpgStreamer, core, "username");
		let pass = uciOriginal.get(mjpgStreamer, core, "password");

		// URL parameters.
		let path = "/";
		let parm = "?action=" + (snapshot ? "snapshot" : "stream");

		// Add local URL group.
		let local = addUrlQrCodeApplication(viewer, snapshot ? QrCode.LocalWebcamSnapshot : QrCode.LocalWebcamStream, application, false);
		// Add local IP URL.
		for(let ip of currentLanIps)
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
		let remote = addUrlQrCodeApplication(viewer, snapshot ? QrCode.RemoteWebcamSnapshot : QrCode.RemoteWebcamStream, application, true);
		// Add remote IP URL.
		for(let ip of currentWanIps)
		{
			addUrlQrCode(remote, "http", ip, localWebcamPort, user, pass, path, parm);
		}
		// Add local client URL.
		if(directView && remoteView)
		{
			addUrlQrCode(remote, clientProt, clientHost, remoteWebcamPort, user, pass, path, parm);
		}

		// Add indirect URL group.
		let indirect = addUrlQrCodeApplication(viewer, snapshot ? QrCode.IndirectWebcamSnapshot : QrCode.IndirectWebcamStream, application, remoteView);
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
	let option = getOption(byId("qr_code"));
	let optgroup = option.parentNode;
	let application = optgroup.getAttribute("data-application");
	if(application)
	{
		if(application == "wireguard")
		{
			if(uci.get("wireguard_gargoyle", "server", "enabled") == "1")
			{
				// Set alert info if WireGuard server is enabled but the selected WireGuard client is disabled.
				if(option.getAttribute("data-enabled") == "false")
				{
					setQrCodeAlert(QrCode.WireGuardClientDisabled);
				}
			}
			else
			{
				// Set alert info if WireGuard server is disabled.
				setQrCodeAlert(QrCode.WireGuardServerDisabled);
			}
		}
		else if(application.startsWith("webcam"))
		{
			if(uci.get("mjpg-streamer", "core", "enabled") == "1")
			{
				// Set alert info if webcam is enabled and a remote URL is selected but remote access is disabled.
				let remote = optgroup.getAttribute("data-remote") == "true";
				let remotePort = uci.get("firewall", "webcam_wan_access", "remote_port");
				setQrCodeAlert(remote && !remotePort ? QrCode.RemoteWebcamDisabled : "");
			}
			else
			{
				// Set alert info if webcam is disabled.
				setQrCodeAlert(QrCode.WebcamDisabled);
			}
		}
	}
	setQrCodeFrame();
	updatePrint();
	updateGuide();
	scrollUntilInView(byId("qr_code_viewer"));
}

function updatePrint() {
	let print = (application, id) => byId(id).checked;
	let guide = (application, id) => byId(id).value;
	setQrCodePrint(print, guide);
}

function updateGuide() {
	// Comment print checkbox value and text area.
	let print = byId("print_comment").checked;
	let comment = byId("comment");
	// Update comment text area visibility.
	byId("comment_container").style.display = print && byId("application").value ? "block" : "none";
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
	let editor = byId("application");
	editor.innerHTML = "";
	editor.removeAttribute("data-application");
	addPlaceholderOption(editor, QrCode.SelectApplication);

	// Add wireless network group and options.
	let wifi = addOptGroup(editor, QrCode.Wifi);
	addOption(wifi, QrCode.HomeWifi, "home_wifi");
	addOption(wifi, QrCode.GuestWifi, "guest_wifi");
	// Add virtual private network group and options.
	let vpn = addOptGroup(editor, QrCode.Vpn);
	addOption(vpn, QrCode.WireGuard, "wireguard");
	// Hide virtual private network group when WireGuard plugin is not installed.
	vpn.hidden = !uci.get("gargoyle", "connection", "wireguard");
	// Add router access group and options.
	let access = addOptGroup(editor, QrCode.RouterAccess);
	addOption(access, QrCode.WebAccess, "web_access");
	// Add webcam group and options.
	let webcam = addOptGroup(editor, QrCode.Webcam);
	addOption(webcam, QrCode.WebcamSnapshot, "webcam_snapshot");
	addOption(webcam, QrCode.WebcamStream, "webcam_stream");
	// Hide webcam group when webcam plugin is not installed.
	webcam.hidden = !uci.get("gargoyle", "system", "webcam");
	editor.disabled = !hasOptions(editor);

	// Update editor visibility.
	updateEditor();

	// Load QR code width with 50 as default.
	let width = +uci.get(pkg, "viewer", "width");
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
	let global = "gargoyle", plugin = "qr_code";
	uci.removeSection(global, plugin);
	let embed = (embed, application) => embed
		|| uci.get(pkg, application, "embed_via_lan") == "1"
		|| uci.get(pkg, application, "embed_via_wan") == "1";
	if(["home_wifi", "guest_wifi", "wireguard", "web_access", "webcam_snapshot", "webcam_stream"].reduce(embed, false))
	{
		uci.set(global, plugin, "", "login_hook");
		uci.set(global, plugin, "css", "qr_code.css");
		uci.set(global, plugin, "js", "drawdown.js qrcodegen.js optgroup.js qr_code_common.js");
		uci.set(global, plugin, "i18n", "qr_code.js");
		uci.set(global, plugin, "pkg", pkg);
	}

	// Save QR code width.
	let viewer = "viewer";
	uci.set(pkg, viewer, "", viewer);
	uci.set(pkg, viewer, "width", byId("qr_code_width").value);

	// Save changes.
	let commands = uci.getScriptCommands(uciOriginal) + "\n";
	let hash = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
	let param = getParameterDefinition("commands", commands)  + "&" + getParameterDefinition("hash", hash);
	let stateChangeFunction = req =>
	{
		if(req.readyState == 4)
		{
			uciOriginal = uci.clone();
			setControlsEnabled(true);
		}
	};
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
