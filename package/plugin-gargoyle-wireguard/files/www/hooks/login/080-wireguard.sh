#!/usr/bin/haserl

<script>
<%
	wireguard_client_enabled=$( uci get wireguard_gargoyle.client.enabled 2>/dev/null )

	echo "var wireguardClientEnabled = \"$wireguard_client_enabled\";"
	echo "var wgStatus='"$(ifstatus wg0)"';"
%>
</script>
<div id="wireguard_fields" style="display:none" class="row">
	<div class="col-lg-6">
		<div class="panel panel-info">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ wireguard.WGClt %></h3>
			</div>
			<div class="panel-body">
				<div class="row">
					<span class="col-xs-5" id='wireguard_status_label'><%~ wireguard.wgSts %>:</span>
					<span class="col-xs-7" id='wireguard_status' style="font-weight:bold;"></span>
				</div>
			</div>
		</div>
	</div>
</div>

