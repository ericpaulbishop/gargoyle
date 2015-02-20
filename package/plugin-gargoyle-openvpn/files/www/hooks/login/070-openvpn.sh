#!/usr/bin/haserl

<script>
<%
	openvpn_enabled=$( uci get openvpn.custom_config.enable 2>/dev/null )
	openvpn_client_enabled=$(uci get openvpn_gargoyle.@client[0].enabled 2>/dev/null)
	
	echo "var openvpnEnabled = \"$openvpn_enabled\";"
	echo "var openvpnClientEnabled = \"$openvpn_client_enabled\";"
	echo "var tunIp=\""$(ifconfig tun0 2>/dev/null | awk ' { if ( $0 ~ /inet addr:/) { gsub(/^.*:/, "", $2) ; print $2 } }')"\";"
	echo "var openvpnProc=\""$(ps | grep openvpn | grep -v grep | grep -v haserl | awk ' { printf $1 }')"\";"

	# don't want to explicitly add -z to header, since this is an optional hook
	# however we can explicitly load translation variables from file here for javascript
	# since in shell script we can load from file name instead of internal variable
%>
var ovpnTransRunC='<%~ openvpn.RunC %>';
var ovpnTransRunNC='<%~ openvpn.RunNC %>';
var ovpnTransRunNot='<%~ openvpn.RunNot %>';
</script>
<fieldset id="openvpn_fields" style="display:none">
	<legend class="sectionheader"><%~ openvpn.OClt %></legend>
	<div>
		<span class="leftcolumn" id='openvpn_status_label'><%~ openvpn.OSts %>:</span>
		<span class="rightcolumn" id='openvpn_status' style="font-weight:bold;"></span>
	</div>

</fieldset>

