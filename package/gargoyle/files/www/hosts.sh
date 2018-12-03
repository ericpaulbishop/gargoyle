#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "hosts" -j "gs_sortable.js  common.js hosts.js table.js" -z "hosts.js basic.js" -i -n dhcp wireless
%>

<script>
<!--
var dhcpLeaseLines;
var wifiLines;
var wirelessDriver;
var conntrackLines;
var arpLines;
var currentTime;
var wlanLines;
<%
	sh /usr/lib/gargoyle/define_host_vars.sh
	wan_ip=$(uci -p /tmp/state get network.wan.ipaddr 2>/dev/null)
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr 2>/dev/null)
	echo "currentWanIp=\"$wan_ip\";"
	echo "currentLanIp=\"$lan_ip\";"
%>
//-->
</script>
<h1 class="page-header"><%~ hosts.mHosts %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ hosts.RefreshR %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<select id="refresh_rate" class="form-control">
							<option value="2000">2 <%~ seconds %></option>
							<option value="10000">10 <%~ seconds %></option>
							<option value="30000">30 <%~ seconds %></option>
							<option value="60000">60 <%~ seconds %></option>
							<option value="never"><%~ never %></option>
						</select>
					</span>
					<br/>
					<span class="col-xs-12"><p><em><%~ RInfo %></em></p></span>
				</div>
			</div>
		</div>
	</div>
</div>


<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ CurrLeases %></h3>
			</div>

			<div class="panel-body">
				<div id="dhcp_data" class="form-group form-inline">
					<div id="lease_table_container" class="table-responsive"></div>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ConWifiHosts %></h3>
			</div>

			<div class="panel-body">
				<div id="wifi_data" class="form-group form-inline">
					<div id="wifi_table_container" class="table-responsive"></div>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ConClientWifi %></h3>
			</div>

			<div class="panel-body">
				<div id="client_wifi_data" class="form-group form-inline">
					<div id="client_wifi_table_container" class="table-responsive"></div>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ActiveHosts %></h3>
			</div>

			<div class="panel-body">
				<div id="active_data" class="form-group form-inline">
					<div id="active_table_container" class="table-responsive"></div>
				</div>
			</div>
		</div>
	</div>
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "hosts"
%>
