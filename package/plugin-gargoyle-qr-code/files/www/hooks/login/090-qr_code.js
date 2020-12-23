/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

addLoadFunction(() =>
{
	// Whether shared login hook resources of plugin are loaded.
	const pkg = uciOriginal.get("gargoyle", "qr_code", "pkg");
	if(!pkg)
	{
		return;
	}

	// QR code viewer selection.
	const viewer = byId("qr_code");
	// Add event listener of QR code viewer.
	viewer.addEventListener("change", () =>
	{
		setQrCodeTitle();
		setQrCodeFrame();
		const print = (application, id) => uciOriginal.get(pkg, application, id) == "1";
		const guide = (application, id) => uciOriginal.getLines(pkg, application, id + "_line");
		setQrCodePrint(print, guide);
		scrollUntilInView(byId("qr_code_viewer"));
	});

	// Add home/guest wireless network groups.
	const home = addWifiQrCodeApplication(viewer, QrCode.HomeWifi, "home_wifi");
	const guest = addWifiQrCodeApplication(viewer, QrCode.GuestWifi, "guest_wifi");
	// Add home/guest wireless network options to their corresponding groups.
	const wireless = "wireless";
	const wirelessSections = uciOriginal.getAllSectionsOfType(wireless, "wifi-iface");
	for(const section of wirelessSections)
	{
		const ssid = uciOriginal.get(wireless, section, "ssid");
		const hidden = uciOriginal.get(wireless, section, "hidden") == "1";
		const encryption = uciOriginal.get(wireless, section, "encryption");
		const key = uciOriginal.get(wireless, section, "key");
		addWifiQrCode(section.includes("_gn_") ? guest : home, section, ssid, hidden, encryption, key);
	}

	// Add local/remote/indirect router access group and client URL options.
	if(uciOriginal.get(pkg, "web_access", "embed_via_" + accessArea) == "1")
	{
		const text = directView ? (remoteView ? QrCode.RemoteWebAccess : QrCode.LocalWebAccess) : QrCode.IndirectWebAccess;
		const web = addUrlQrCodeApplication(viewer, text, "web_access", remoteView);
		addUrlQrCode(web, clientProt, clientHost, clientPort);
	}

	// Add local/remote/indirect webcam snapshot/stream groups and client URL options.
	const mjpgStreamer = "mjpg-streamer", core = "core";
	if(uciOriginal.get(mjpgStreamer, core, "enabled") == "1")
	{
		// Load local port.
		const localWebcamPort = uciOriginal.get(mjpgStreamer, core, "port");
		// Offer remote access even when disabled by using local port since
		// remote port either equals local port or is empty when disabled.
		const remoteWebcamPort = localWebcamPort;
		// Load login credentials.
		const user = uciOriginal.get(mjpgStreamer, core, "username");
		const pass = uciOriginal.get(mjpgStreamer, core, "password");

		const actions = ["snapshot", "stream"];
		const texts = [
			directView ? (remoteView ? QrCode.RemoteWebcamSnapshot : QrCode.LocalWebcamSnapshot) : QrCode.IndirectWebcamSnapshot,
			directView ? (remoteView ? QrCode.RemoteWebcamStream : QrCode.LocalWebcamStream) : QrCode.IndirectWebcamStream,
		];
		for(const nth in actions)
		{
			const application = "webcam_" + actions[nth];
			if(uciOriginal.get(pkg, application, "embed_via_" + accessArea) == "1")
			{
				// Add local/remote/indirect snapshot/stream group.
				const webcam = addUrlQrCodeApplication(viewer, texts[nth], application, remoteView);
				// Add client URL option.
				const prot = directView ? "http" : clientProt;
				const port = directView ? (remoteView ? remoteWebcamPort : localWebcamPort) : clientPort;
				addUrlQrCode(webcam, prot, clientHost, port, user, pass, "/", "?action=" + actions[nth]);
			}
		}
	}

	// Load QR code width.
	setQrCodeWidth(+uciOriginal.get(pkg, "viewer", "width"));

	// Disable QR code viewer selection with no options.
	viewer.disabled = !hasOptions(viewer);
	// Hide QR code viewer panel with no options.
	byId("qr_code_fields").style.display = viewer.disabled ? "none" : "";
});
