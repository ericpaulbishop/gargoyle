/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

// Part of i18n.
let QrCode = {};

// Set title of QR code viewer panel.
function setQrCodeTitle()
{
	let qrCode = getOption(byId("qr_code"));
	byId("qr_code_title").innerHTML = qrCode && qrCode.value ? qrCode.parentNode.label : QrCode.QrCodeViewer;
}

// Set alert info below QR code selection.
function setQrCodeAlert(info = "")
{
	byId("qr_code_alert").innerHTML = info ? info : "";
	byId("qr_code_alert_container").style.display = info ? "block" : "none";
}

// Set QR code width from 20 to 80 % and synchronize range and number inputs.
function setQrCodeWidth(percent, current)
{
	percent = percent < 20 ? 20 : percent > 80 ? 80 : percent;
	["qr_code_width", "qr_code_width_range"]
		.filter(id => id != current).forEach(id => byId(id).value = percent);
	byId("qr_code_frame").style.width = percent + "%";
}

// Draw SVG of selected QR code option holding pre-calculated QR code data.
function setQrCodeFrame()
{
	// QR code frame is an anchor enclosing an SVG.
	let frame = byId("qr_code_frame");
	// Reset frame.
	if(frame.href)
	{
		window.URL.revokeObjectURL(frame.href);
		frame.removeAttribute("href");
		frame.removeAttribute("title");
		frame.removeAttribute("download");
		frame.innerHTML = "";
	}
	// Selected option or null.
	let qrCode = getOption(byId("qr_code"));
	// Skip hidden placeholder option (empty value).
	if(qrCode && qrCode.value)
	{
		// Generate QR code in a version range from 2 to 40, skipping version 1 which lacks the
		// bottom right alignment pattern. Boost ECC level (true) within same version starting
		// from LOW. Automatically choose the mask pattern (-1).
		let segs = qrcodegen.QrSegment.makeSegments(qrCode.value);
		let qr = qrcodegen.QrCode.encodeSegments(segs, qrcodegen.QrCode.Ecc.LOW, 2, 40, -1, true);
		// Generate an SVG with a quiet zone of 4 modules, parse it, and get its document element.
		let doc = new DOMParser().parseFromString(qr.toSvgString(4), "image/svg+xml");
		let svg = doc.documentElement;
		// Disallow distortion when resizing.
		svg.setAttribute("preserveAspectRatio", "xMidYMid slice");
		// Emphasize contrast between clean edges over rendering speed and geometric precision.
		svg.setAttribute("shape-rendering", "crispEdges");
		// Clone SVG element into its anchor.
		frame.appendChild(svg.cloneNode(true));

		// Scale SVG of a quite small default size by an integral multiple of 6. An integral
		// multiple helps renderers to keep the edges crispy without inserting white padding
		// lines between the modules.
		let viewBox = svg.getAttribute("viewBox").split(" ");
		svg.setAttribute("width", viewBox[2] * 6);
		svg.setAttribute("height", viewBox[3] * 6);
		// Serialize SVG document into a blob and offer it to be downloaded as a named SVG file
		// when the anchor is clicked.
		let xml = new XMLSerializer().serializeToString(doc);
		frame.href = window.URL.createObjectURL(new Blob([xml], { type: "image/svg+xml" }));
		let web = qrCode.parentNode.getAttribute("data-application").startsWith("web");
		frame.title = qrCode.parentNode.label + (web ? "" : " - " + qrCode.text) + ".svg";
		frame.download = frame.title;
	}
	// Update viewer visibility.
	byId("qr_code_container").style.display = qrCode && qrCode.value ? "block" : "none";
}

// Set description list items and guide comprising Markdown comment below QR code frame.
function setQrCodePrint(print, guide)
{
	// Description list items.
	let items = byId('qr_code_items');
	// Reset or append description list items.
	let setQrCodeItems = (title = "", value = "") =>
	{
		items.innerHTML = title ? items.innerHTML + "<dt>" + title + "</dt><dd>" + value + "</dd>" : "";
	};
	// Reset description list items.
	setQrCodeItems();
	// Reset guide comprising Markdown comment.
	setQrCodeGuide();
	// Selected option or null.
	let qrCode = getOption(byId("qr_code"));
	// Skip hidden placeholder option (empty value).
	if(qrCode && qrCode.value)
	{
		// QR code application:
		// * home_wifi, guest_wifi,
		// * wireguard,
		// * web_access,
		// * webcam_snapshot, webcam_stream.
		let application = qrCode.parentNode.getAttribute("data-application");
		let wifi = application.endsWith("wifi");
		let wireGuard = application == "wireguard";
		// Append subject if checked.
		if(print(application, "print_subject"))
		{
			setQrCodeItems(wifi ? QrCode.Ssid : wireGuard ? QrCode.Name : QrCode.Url, qrCode.text);
		}
		// Append secrets if checked.
		if(print(application, "print_secrets"))
		{
			if(wifi)
			{
				// Append Wi-Fi key.
				setQrCodeItems(QrCode.Password, qrCode.getAttribute("data-key"));
			}
			else
			{
				if(wireGuard)
				{
					// Append allowed WireGuard client key pair.
					setQrCodeItems(QrCode.PublicKey, qrCode.getAttribute("data-public-key"));
					setQrCodeItems(QrCode.PrivateKey, qrCode.getAttribute("data-private-key"));
				}
				else
				{
					// Append URL login credentials.
					let user = qrCode.getAttribute("data-username");
					let pass = qrCode.getAttribute("data-password");
					if(user && pass)
					{
						setQrCodeItems(QrCode.Username, user);
						setQrCodeItems(QrCode.Password, pass);
					}
				}
			}
		}
		// Render comment if checked.
		if(print(application, "print_comment"))
		{
			setQrCodeGuide(guide(application, "comment"));
		}
	}
}

// Render Markdown comment below description list.
function setQrCodeGuide(text = "")
{
	let guide = byId("qr_code_guide");
	guide.innerHTML = markdown(escapeHTML(text));
	guide.style.display = text ? "inline-block" : "none";
}

// Add option group and set QR code application.
function addWifiQrCodeApplication(select, label, application)
{
	let optgroup = addOptGroup(select, label);
	optgroup.setAttribute("data-application", application);
	return optgroup;
}

// Add option holding QR code data of Wi-Fi configuration if SSID is unique within option group.
function addWifiQrCode(optgroup, section, ssid, hidden, encryption, key, guest)
{
	// Filter out SSIDs:
	// * Skip on home/guest wireless network application mismatch.
	// * Skip dummy SSIDs on first boot by requiring them to start with "ap_".
	// * Skip duplicates when both bands (2.4 and 5 GHz) share the same SSID.
	// * Skip RADIUS encryption types, though its encoding is defined, due to lack
	//   of user credentials as Gargoyle cannot (yet) be a RADIUS server on its own.
	let applicable = guest == undefined || guest == section.includes("_gn_");
	if(applicable && section.startsWith("ap_") && !getOptionByText(optgroup, ssid) && !encryption.includes("wpa"))
	{
		// Encode Wi-Fi configuration.
		let data = encWifiQrCode(ssid, hidden, encryption, key);
		// Add option per SSID with SSID as text and QR code data as value.
		let option = addOption(optgroup, ssid, data);
		// Add key as data attribute to append to description list.
		option.setAttribute("data-key", key);
	}
}

// Encode Wi-Fi configuration according to:
//   https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config-android-ios-11
// With the exception that we do *not* quote hexadecimal strings according to:
//   https://github.com/evgeni/qifi/issues/4#issuecomment-365815774
function encWifiQrCode(ssid, hidden, encryption, key)
{
	let field = (name, data) => data ? name + ":" + data.replace(/([";,:\\])/g, "\\$1") + ";" : "";
	let data = "WIFI:";
	data += field("S", ssid);
	data += field("H", hidden ? "true" : "");
	data += field("T", encryption == "none" ? "" : (encryption == "wep" ? "WEP" : "WPA"));
	data += field("P", key);
	data += ";";
	return data;
}

// Add option group and set QR code application.
function addWireGuardQrCodeApplication(select, label, application)
{
	let optgroup = addOptGroup(select, label);
	optgroup.setAttribute("data-application", application);
	return optgroup;
}

// Add option holding QR code data of allowed WireGuard client with keys managed by Gargoyle.
function addWireGuardQrCode(optgroup, name, iface, peer, enabled)
{
	if(iface.privateKey && !peer.endpoint.startsWith(":"))
	{
		// Encode allowed WireGuard client.
		let data = encWireGuardQrCode(iface, peer);
		// Add option per client name with client name as text and QR code data as value.
		let option = addOption(optgroup, name, data);
		// Add client key pair as data attributes to append to description list.
		option.setAttribute("data-public-key", iface.publicKey);
		option.setAttribute("data-private-key", iface.privateKey);
		// Add data attribute of whether client is enabled to set alert info otherwise.
		option.setAttribute("data-enabled", enabled);
	}
}

// Encode WireGuard client configuration.
function encWireGuardQrCode(iface, peer)
{
	return [
		"[Interface]",
		"Address = " + iface.address,
		"PrivateKey = " + iface.privateKey,
		"",
		"[Peer]",
		"AllowedIPs = " + peer.allowedIPs,
		"Endpoint = " + peer.endpoint,
		"PersistentKeepalive = 25",
		"PublicKey = " + peer.publicKey,
	].join("\n");
}

// Add option group and set QR code application and whether URL is remote.
function addUrlQrCodeApplication(select, label, application, remote)
{
	let optgroup = addOptGroup(select, label);
	optgroup.setAttribute("data-application", application);
	optgroup.setAttribute("data-remote", remote);
	return optgroup;
}

// Add option holding QR code data of URL if URL is unique within option group.
function addUrlQrCode(optgroup, prot, host, port, user = "", pass = "", path = "", parm = "")
{
	// Skip URL if LAN/WAN or port is not available.
	if(host && port)
	{
		port = (prot == "http" && port != "80") || (prot == "https" && port != "443") ? ":" + port : "";
		let auth = user && pass ? encodeURIComponent(user) + ":" + encodeURIComponent(pass) + "@" : "";
		let url = prot + "://" + auth + host + port + path + parm;
		// Add option per URL with URL as text and QR code data as value.
		if(!getOptionByText(optgroup, url))
		{
			let option = addOption(optgroup, url, encUrlQrCode(url));
			// Add login credentials as data attributes to append to description list.
			option.setAttribute("data-username", user);
			option.setAttribute("data-password", pass);
			return option;
		}
	}
	return null;
}

// Encode URL where QR code data is just the plain URL.
function encUrlQrCode(url)
{
	return url;
}

// Add event listeners of QR code viewer.
addLoadFunction(function()
{
	// Event listeners to print, download, and hide QR code.
	byId("qr_code_print_button").addEventListener("click", () => window.print());
	byId("qr_code_download_button").addEventListener("click", () => byId("qr_code_frame").click());
	byId("qr_code_hide_button").addEventListener("click", () =>
	{
		byId("qr_code").selectedIndex = 0;
		setQrCodeTitle();
		setQrCodeAlert();
		setQrCodeFrame();
		setQrCodePrint();
	});
	// Event listeners to scale QR code using regular functions (this == event.currentTarget).
	byId("qr_code_frame").addEventListener("wheel", function(event)
	{
		event.preventDefault();
		// Change QR code width on vertical wheel events in steps of 10 %.
		setQrCodeWidth(+this.style.width.slice(0, -1) - Math.sign(event.deltaY) * 10, this.id);
	});
	byId("qr_code_width").addEventListener("input", function()
	{
		proofreadNumericRange(this, 20, 80);
		setQrCodeWidth(this.value, this.id);
	});
	byId("qr_code_width_range").addEventListener("input", function()
	{
		setQrCodeWidth(this.value, this.id);
	});
});
