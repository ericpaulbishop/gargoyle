#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -i -s "connection" -p "dhcp" -j "gs_sortable.js table.js dhcp.js" -z "dhcp.js" network wireless dhcp firewall
	subnet=$(ifconfig br-lan | awk 'BEGIN {FS=":"}; $0 ~ /inet.addr/ {print $2}' | awk 'BEGIN {FS="."}; {print $1"\."$2"\."$3"\."}')
%>

<script>
<!--
<%
	echo "var dhcpEnabled = true;"
	echo "var subnet=\"$subnet\";"
	echo "var dhcpSection = getDhcpSection(uciOriginal);"

	echo "var hostData = new Array();"
	if [ -e /etc/hosts ] ; then
		awk ' $0 ~ /^[\t ]*[0-9]/ {print "hostData.push([\""$1"\",\""$2"\"]);"};' /etc/hosts
	fi

	echo "var host6Data = new Array();"
	if [ -e /tmp/hosts/dhcp.* ] ; then
		awk ' $0 ~ /^[\t ]*[0-9a-fA-F]/ {print "host6Data.push([\""$1"\",\""$2"\"]);"};' /tmp/hosts/dhcp.*
	fi

	echo "var leaseData = new Array();";
	if [ -e /tmp/dhcp.leases ] ; then
		awk ' $0 ~ /[a-z,A-Z,0-9]+/ {print "leaseData.push([\""$2"\",\""$3"\",\""$4"\"]);"};' /tmp/dhcp.leases
	fi

	echo "var ip6leaseData = new Array();";
	if [ -e /tmp/hosts/odhcpd ] ; then
		while read lease; do
			wcnt="$(echo $lease | wc -w)"
			idx=9
			while [ "$idx" -le "$wcnt" ]
			do
				echo "$lease" | awk -v idx="$idx" ' $0 ~ /#.*[a-z,A-Z,0-9]+/ {print "ip6leaseData.push([\""$3"\",\""$idx"\",\""$5"\"]);"};'
				idx=$(($idx+1))
			done
		done </tmp/hosts/odhcpd
	fi

	echo "var ip6neighData = new Array();";
	ip -6 neigh | grep -v "FAILED" | grep -v "^fe80:" | awk '{print "ip6neighData[\""$1"\"] = \""$5"\";"};'
%>

var ipHostHash = new Array();
var ipMacHash = new Array();
var ipDUIDHash = new Array();
for (hostIndex in hostData)
{
	host=hostData[hostIndex];
	ipHostHash[ host[0] ] = host[1];
}
for (host6Index in host6Data)
{
	host=host6Data[host6Index];
	ipHostHash[ host[0] ] = host[1];
}
for (leaseIndex in leaseData)
{
	host = leaseData[leaseIndex];
	ipHostHash[host[1]] = host[2];
	ipMacHash[host[1]] = host[0];
}
for (ip6leaseIndex in ip6leaseData)
{
	host = ip6leaseData[ip6leaseIndex];
	ipHostHash[ip6_splitmask(host[1]).address] = host[2];
	ipMacHash[ip6_splitmask(host[1]).address] = ip6neighData[ip6_splitmask(host[1]).address];
	ipDUIDHash[ip6_splitmask(host[1]).address] = host[0];
}
ipHostHash["127.0.0.1"] = "localhost";
ipHostHash["::1"] = "localhost6";

//-->
</script>


<h1 class="page-header">DHCP</h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title">IPv4</h3>
			</div>

			<div class="panel-body">
				<div class="row form-group" id="dhcp_enabled_container">
					<span class="col-xs-12">
						<input type="checkbox" id="dhcp_enabled" onclick="setEnabled(this.checked)" />
						<label id="dhcp_enabled_label" for="dhcp_enabled"><%~ dhcp.SrvE %></label>
					</span>
				</div>

				<div id="dhcp_range_container" class="row form-group">
					<label class="col-xs-12" style="text-decoration:underline" for="dhcp_start"><%~ Drng %>:</label>
				</div>

				<div id="dhcp_start_container" class="row form-group">
					<label class="col-xs-5" for="dhcp_start" id="dhcp_start_label"><%~ Strt %>:</label>
					<span class="col-xs-7">
						<% echo -n "$subnet" %>
						<input type="text" class="form-control" id="dhcp_start" oninput="proofreadNumeric(this)" size="5" maxlength="3" />
					</span>
				</div>

				<div id="dhcp_end_container" class="row form-group">
					<label class="col-xs-5" for="dhcp_end" id="dhcp_end_label"><%~ End %>:</label>
					<span class="col-xs-7">
						<% echo -n "$subnet" %>
						<input type="text" class="form-control" id="dhcp_end" oninput="proofreadNumeric(this)" size="5" maxlength="3" />
					</span>
				</div>

				<div id="dhcp_lease_container" class="row form-group">
					<label class="col-xs-5" for="dhcp_lease" id="dhcp_lease_label"><%~ LsTm %>:</label>
					<span class="col-xs-7">
						<input type="text" class="form-control" oninput="proofreadNumeric(this)" id="dhcp_lease" size="5" maxlength="4" />
						<em>(<%~ hours %>)</em>
					</span>
				</div>

			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title">IPv6</h3>
			</div>

			<div class="panel-body">
				<div id="dhcpv6_container" class="row form-group">
					<label class="col-xs-5" for="dhcpv6" id="dhcpv6_label">DHCPv6:</label>
					<span class="col-xs-7">
						<select class="form-control" id="dhcpv6">
							<option value="server"><%~ Enabled %></option>
							<option value="relay" disabled><%~ Relayed %></option>
							<option value="disabled"><%~ Disabled %></option>
						</select>
					</span>
				</div>

				<div id="ra_container" class="row form-group">
					<label class="col-xs-5" for="ra" id="ra_label"><%~ RtrAdv %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="ra">
							<option value="server"><%~ Enabled %></option>
							<option value="relay" disabled><%~ Relayed %></option>
							<option value="disabled"><%~ Disabled %></option>
						</select>
					</span>
				</div>

				<div id="ra_slaac_container" class="row form-group">
					<label class="col-xs-5" for="ra_slaac" id="ra_slaac_label">SLAAC:</label>
					<span class="col-xs-7">
						<select class="form-control" id="ra_slaac">
							<option value="1"><%~ Enabled %></option>
							<option value="0"><%~ Disabled %></option>
						</select>
					</span>
				</div>

				<div id="ip6prefix_container" class="row form-group">
					<label class="col-xs-5" for="ip6prefix" id="ip6prefix_label"><%~ IP6Prefs %>:</label>
					<span class="col-xs-7" id="ip6prefix"></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SIPs %></h3>
			</div>
			<div class="panel-body">
				<div id="block_mismatches_container" class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="block_mismatches" />
						<label id="block_mismatch_label" for="block_mismatches"><%~ BlckM %></label>
					</span>
				</div>

				<div id="staticip_add_heading_container" class="row form-group">
					<label class="col-xs-12" id="staticip_add_heading_label" style="text-decoration:underline"><%~ AdSIP %>:</label>
				</div>

				<div class="row form-group">
					<div>
						<span class="col-xs-12">
							<select id="static_from_connected" class="form-control">
								<option value="none"><%~ SelH %></option>
							</select>
							<em>(<%~ opt %>)</em>
						</span>
						<span class="col-xs-12"><button id="add_button" class="btn btn-default btn-add" onclick="addStaticModal()"><%~ Add %></button></span>
					</div>
				</div>

				<div id="staticip_table_heading_container" class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ AsSIP %>:</span>
				</div>

				<div class="row form-group">
					<div id="staticip_table_container" class="table-responsive col-xs-12"></div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="firefox3_bug_correct" style="display:none">
	<input type="text" value="firefox3_bug" />
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="static_ip_modal" aria-hidden="true" aria-labelledby="static_ip_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="static_ip_modal_title" class="panel-title"><%~ dhcp.AdSIP %></h3>
			</div>
			<div class="modal-body">
				<%in templates/static_ip_template %>
			</div>
			<div class="modal-footer" id="static_ip_modal_button_container">
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
	gargoyle_header_footer -f -s "connection" -p "dhcp"
%>
