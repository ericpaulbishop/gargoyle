#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "dhcp" -c "internal.css" -j "table.js dhcp.js" -z "dhcp.js" network wireless dhcp firewall
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

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-body">

		<div id='dhcp_enabled_container'>
			<div class='form-group form-inline'>
				<input type='checkbox' id='dhcp_enabled' onclick="setEnabled(this.checked)" />
				<label id='dhcp_enabled_label' for='dhcp_enabled'><%~ dhcp.SrvE %></label>
			</div>
		</div>

		<div id='dhcp_range_container' class='form-group form-inline'>
			<label for='dhcp_start'><%~ Drng %>:</label>
		</div>

		<div id='dhcp_start_container' class='form-group form-inline'>
			<label for='dhcp_start' id='dhcp_start_label'><%~ Strt %>:</label>
			<span class='rightcolumn'><% echo -n "$subnet" %></span>
			<input type='text' class='form-control' id='dhcp_start' onkeyup='proofreadNumeric(this)' size='5' maxlength='3' />
		</div>

		<div id='dhcp_end_container' class='form-group form-inline'>
			<label for='dhcp_end' id='dhcp_end_label'><%~ End %>:</label>
			<span class='rightcolumn'><% echo -n "$subnet" %></span>
			<input type='text' class='form-control' id='dhcp_end' onkeyup='proofreadNumeric(this)' size='5' maxlength='3' />
		</div>

		<div id='dhcp_lease_container' class='form-group form-inline'>
			<label for='dhcp_lease' id='dhcp_lease_label'><%~ LsTm %>:</label>
			<input type='text' class='form-control' onkeyup='proofreadNumeric(this)' id='dhcp_lease' size='5' maxlength='4' />
			<em>(<%~ hours %>)</em>
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

		<div id='block_mismatches_container' class='form-group form-inline'>
			<div>
				<input type='checkbox' id='block_mismatches' />
				<label id='block_mismatch_label' for='block_mismatches'><%~ BlckM %></label>
			</div>
		</div>

		<div id='staticip_add_heading_container' class='form-group form-inline'>
			<label class='nocolumn' id='staticip_add_heading_label' style='text-decoration:underline'><%~ AdSIP %>:</label>
		</div>

		<div class='form-group form-inline'>
			<div id='staticip_add_container'>
				<%in templates/static_ip_template %>
			</div>
			<div>
				<select id="static_from_connected" class="form-control" onchange="staticFromConnected()">
					<option value="none"><%~ SelH %></option>
				</select>
			</div>
		</div>

		<div id='staticip_table_heading_container' class='form-group form-inline'>
			<span class='nocolumn'><%~ AsSIP %>:</span>
		</div>

		<div class='form-group form-inline'>
			<div id='staticip_table_container' class="bottom_gap"></div>
		</div>

	</div>
</div>
</div>
</div>

	<div id="firefox3_bug_correct" style="display:none">
		<input type='text' value='firefox3_bug' />
	</div>

	<div id="bottom_button_container">
	  <button id="save_button" class="btn btn-primary" onclick='saveChanges()'><%~ SaveChanges %></button>
	  <button id="reset_button" class="btn btn-danger" onclick='resetData()'><%~ Reset %></button>
	</div>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "dhcp"
%>
