#!/usr/bin/haserl
<%in templates/client_server_template %>
<script>

<%
	# This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	# Get value of UCI path $1.$2.$3 or section type with *empty or no* $3.
	uci_get()
	{
		[ "$3" ] && option=".$3" || option=""
		echo "$(uci -q get "$1.$2$option")"
	}
	# Set value of uciOriginal path $1.$2.$3 to $4 or to UCI value with *no* $4.
	uci_set()
	{
		[ $# = 4 ] && value=$4 || value=$(uci_get "$1" "$2" "$3")
		# Escape backslash and double quote for double quoted JS strings.
		value=$(echo "$value" | sed 's/\([\"]\)/\\\1/g')
		echo "uciOriginal.set('$1', '$2', '$3', "\"$value\"");"
	}

	# Only load wireless network data revealing keys when the configured
	# conditions are met, as whether this page is accessed via LAN or WAN.
	embedHome=$(uci_get qr_code_gargoyle home_wifi embed_via_$accessArea)
	embedGuest=$(uci_get qr_code_gargoyle guest_wifi embed_via_$accessArea)
	# Using sed since UCI CLI seems to lack a way to get all section names of a section type.
	# Only section names allow to distinguish between home and guest wireless networks.
	# Additionally, ignore anonymous dummy sections on first boot with /ap_.*/.
	aps=$(uci show wireless | sed -n 's/^wireless\.\(ap_.*\)=wifi-iface$/\1/p')
	for ap in $aps; do
		# Whether home or guest wireless network.
		echo "$ap" | grep -q _gn_ && gn=1 || gn=0
		# Whether home or guest wireless network is configured to be embedded via LAN or WAN.
		if [ $gn = 0 -a "$embedHome" = 1 -o $gn = 1 -a "$embedGuest" = 1 ]; then
			uci_set wireless "$ap"
			uci_set wireless "$ap" ssid
			uci_set wireless "$ap" hidden
			uci_set wireless "$ap" encryption
			uci_set wireless "$ap" key
		fi
	done

	# Only load webcam data revealing login credentials when the configured
	# conditions are met, as whether this page is accessed via LAN or WAN.
	embedSnapshot=$(uci_get qr_code_gargoyle webcam_snapshot embed_via_$accessArea)
	embedStream=$(uci_get qr_code_gargoyle webcam_stream embed_via_$accessArea)
	if [ "$embedSnapshot" = 1 -o "$embedStream" = 1 ]; then
		enabled=$(uci_get mjpg-streamer core enabled)
		remotePort=$(uci_get firewall webcam_wan_access remote_port)
		# Only load webcam data when enabled and when accessed via LAN,
		# or when accessed via WAN and remote access is enabled.
		if [ "$enabled" = 1 -a \( $accessArea = lan -o "$remotePort" \) ]; then
			uci_set mjpg-streamer core username
			uci_set mjpg-streamer core password
			uci_set mjpg-streamer core port
			uci_set mjpg-streamer core enabled "$enabled"
			# Only expose remote port when required.
			[ $accessArea = wan ] && uci_set firewall webcam_wan_access remote_port "$remotePort"
		fi
	fi
%>
</script>
<div id="qr_code_fields" style="display:none" class="row">
	<div class="col-lg-6">
<%in templates/qr_code_viewer_template %>
	</div>
</div>
