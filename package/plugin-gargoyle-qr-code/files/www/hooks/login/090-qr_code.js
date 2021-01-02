/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

addLoadFunction(function()
{
	// Whether shared login hook resources of plugin are loaded.
	if(!uciOriginal.get("gargoyle", "qr_code", "pkg"))
	{
		return;
	}
	let pkg = "qr_code_gargoyle";

	// QR code viewer selection.
	let viewer = byId("qr_code");
	// Add event listener of QR code viewer.
	viewer.addEventListener("change", () =>
	{
		setQrCodeTitle();
		setQrCodeFrame();
		let print = (application, id) => uciOriginal.get(pkg, application, id) == "1";
		let guide = (application, id) => uciOriginal.getLines(pkg, application, id + "_line");
		setQrCodePrint(print, guide);
		scrollUntilInView(byId("qr_code_viewer"));
	});

	// Add home/guest wireless network groups.
	let home = addWifiQrCodeApplication(viewer, QrCode.HomeWifi, "home_wifi");
	let guest = addWifiQrCodeApplication(viewer, QrCode.GuestWifi, "guest_wifi");
	// Add home/guest wireless network options to their corresponding groups.
	let wireless = "wireless";
	let wirelessSections = uciOriginal.getAllSectionsOfType(wireless, "wifi-iface");
	for(let section of wirelessSections)
	{
		let ssid = uciOriginal.get(wireless, section, "ssid");
		let hidden = uciOriginal.get(wireless, section, "hidden") == "1";
		let encryption = uciOriginal.get(wireless, section, "encryption");
		let key = uciOriginal.get(wireless, section, "key");
		addWifiQrCode(section.includes("_gn_") ? guest : home, section, ssid, hidden, encryption, key);
	}

	// Add allowed WireGuard client group and options.
	let wireGuard = "wireguard_gargoyle";
	if(uciOriginal.get(wireGuard, "server", "enabled") == "1")
	{
		// Add allowed WireGuard client group.
		let client = addWireGuardQrCodeApplication(viewer, QrCode.WireGuardClient, "wireguard");
		// Add allowed WireGuard client group options.
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
				allowedIPs: wgAllowedIPs,
				endpoint: uciOriginal.get(wireGuard, id, "remote") + ":" + uciOriginal.get(wireGuard, "server", "port"),
				publicKey: uciOriginal.get(wireGuard, "server", "public_key"),
			};
			let enabled = uciOriginal.get(wireGuard, id, "enabled") == "1";
			if(enabled)
			{
				addWireGuardQrCode(client, name, iface, peer, enabled);
			}
		}
	}

	// Add local/remote/indirect router access group and client URL options.
	if(uciOriginal.get(pkg, "web_access", "embed_via_" + accessArea) == "1")
	{
		let text = directView ? (remoteView ? QrCode.RemoteWebAccess : QrCode.LocalWebAccess) : QrCode.IndirectWebAccess;
		let web = addUrlQrCodeApplication(viewer, text, "web_access", remoteView);
		addUrlQrCode(web, clientProt, clientHost, clientPort);
	}

	// Add local/remote/indirect webcam snapshot/stream groups and client URL options.
	let mjpgStreamer = "mjpg-streamer", core = "core";
	if(uciOriginal.get(mjpgStreamer, core, "enabled") == "1")
	{
		// Load local port.
		let localWebcamPort = uciOriginal.get(mjpgStreamer, core, "port");
		// Offer remote access even when disabled by using local port since
		// remote port either equals local port or is empty when disabled.
		let remoteWebcamPort = localWebcamPort;
		// Load login credentials.
		let user = uciOriginal.get(mjpgStreamer, core, "username");
		let pass = uciOriginal.get(mjpgStreamer, core, "password");

		let actions = ["snapshot", "stream"];
		let texts = [
			directView ? (remoteView ? QrCode.RemoteWebcamSnapshot : QrCode.LocalWebcamSnapshot) : QrCode.IndirectWebcamSnapshot,
			directView ? (remoteView ? QrCode.RemoteWebcamStream : QrCode.LocalWebcamStream) : QrCode.IndirectWebcamStream,
		];
		for(let nth in actions)
		{
			let application = "webcam_" + actions[nth];
			if(uciOriginal.get(pkg, application, "embed_via_" + accessArea) == "1")
			{
				// Add local/remote/indirect snapshot/stream group.
				let webcam = addUrlQrCodeApplication(viewer, texts[nth], application, remoteView);
				// Add client URL option.
				let prot = directView ? "http" : clientProt;
				let port = directView ? (remoteView ? remoteWebcamPort : localWebcamPort) : clientPort;
				addUrlQrCode(webcam, prot, clientHost, port, user, pass, "/", "?action=" + actions[nth]);
			}
		}
	}

	// Load QR code width.
	setQrCodeWidth(+uciOriginal.get(pkg, "viewer", "width"));

	// Disable QR code viewer selection with no options.
	viewer.disabled = !hasOptions(viewer);
	// Hide QR code viewer panel with no options.
	byId("qr_code_fields").style.display = viewer.disabled ? "none" : "block";
});
