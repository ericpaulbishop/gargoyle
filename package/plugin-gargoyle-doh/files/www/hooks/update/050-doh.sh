#!/usr/bin/haserl

<script>
<%
	httpsdnsproxy_enabled=$(uci -q get https-dns-proxy.config.enabled)
	echo "var dohenabled = \"$httpsdnsproxy_enabled\";"
%>
</script>
<div class="row form-group" id="dohupservwarning" style="display:none">
	<div class="col-lg-12">
		<div class="alert alert-danger" role="alert">
			<span><%~ doh.DoHNoDNS %></span>
		</div>
	</div>
</div>
