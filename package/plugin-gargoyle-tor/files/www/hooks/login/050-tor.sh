#!/usr/bin/haserl

<script>
<%
	tor_enabled=$(uci get tor.global.enabled 2>/dev/null)
	tor_client_mode=$(uci get tor.client.client_mode 2>/dev/null)
	tor_is_active=$(ipset --test tor_active_ips "$REMOTE_ADDR" 2>&1 | grep -v NOT)

	echo "var torEnabled = \"$tor_enabled\";"
	echo "var torClientMode = \"$tor_client_mode\";"
	echo "var torIsActive = \"$tor_is_active\";"
%>
</script>
<fieldset id="tor_fields" style="display:none">
	<legend class="sectionheader">Tor</legend>
	<div>
		<span class="leftcolumn" id='tor_status_label'><%~ 050-tor.tIP %>:</span>
		<span class="rightcolumn" id='tor_status' style="font-weight:bold;"></span>
	</div>
	<div>
		<span class="leftcolumn"><input id="set_tor_button" class="default_button" type="button" value="<%~ tEnab %>" onclick="updateTorStatus()" /></span>
	</div>
</fieldset>

