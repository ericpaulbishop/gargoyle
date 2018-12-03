#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "dhcp" -j "gs_sortable.js table.js dhcp.js" -z "dhcp.js" network wireless dhcp firewall
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

	echo "";
	echo "var etherData = new Array();";
	if [ -e /etc/ethers ] ; then
		awk ' $0 ~ /^[\t ]*[0-9abcdefABCDEF]/ {print "etherData.push([\""$1"\",\""$2"\"]);"};' /etc/ethers
	fi

	echo "";
	echo "var leaseData = new Array();";
	if [ -e /tmp/dhcp.leases ] ; then
		awk ' $0 ~ /[a-z,A-Z,0-9]+/ {print "leaseData.push([\""$2"\",\""$3"\",\""$4"\"]);"};' /tmp/dhcp.leases
	fi

%>

var ipHostHash = new Array();
for (hostIndex in hostData)
{
	host=hostData[hostIndex];
	ipHostHash[ host[0] ] = host[1];
}

var staticIpTableData = new Array();
for (etherIndex in etherData)
{
	ether=etherData[etherIndex];
	mac=ether[0].toUpperCase();
	ip=ether[1];
	host= ipHostHash[ip] == null ? '-' : ipHostHash[ip];
	staticIpTableData.push([host, mac, ip]);
}

//-->
</script>


<h1 class="page-header">DHCP</h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
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
						<input type="text" class="form-control" id="dhcp_start" onkeyup="proofreadNumeric(this)" size="5" maxlength="3" />
					</span>
				</div>

				<div id="dhcp_end_container" class="row form-group">
					<label class="col-xs-5" for="dhcp_end" id="dhcp_end_label"><%~ End %>:</label>
					<span class="col-xs-7">
						<% echo -n "$subnet" %>
						<input type="text" class="form-control" id="dhcp_end" onkeyup="proofreadNumeric(this)" size="5" maxlength="3" />
					</span>
				</div>

				<div id="dhcp_lease_container" class="row form-group">
					<label class="col-xs-5" for="dhcp_lease" id="dhcp_lease_label"><%~ LsTm %>:</label>
					<span class="col-xs-7">
						<input type="text" class="form-control" onkeyup="proofreadNumeric(this)" id="dhcp_lease" size="5" maxlength="4" />
						<em>(<%~ hours %>)</em>
					</span>
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
					<div class="col-xs-12 table-responsive" id="staticip_add_container">
						<%in templates/static_ip_template %>
					</div>
					<div>
						<span class="col-xs-12">
							<select id="static_from_connected" class="form-control" onchange="staticFromConnected()">
								<option value="none"><%~ SelH %></option>
							</select>
						</span>
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


<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "dhcp"
%>
