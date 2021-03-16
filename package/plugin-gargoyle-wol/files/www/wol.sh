#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "wol" -j "wol.js table.js gs_sortable.js" -z "wol.js" -n -i dhcp wireless gargoyle
%>

<script>
<!--
var dhcpLeaseLines;
var wifiLines;
var wirelessDriver;
var conntrackLines;
var arpLines;
<%
	. /lib/functions/network.sh
	sh /usr/lib/gargoyle/define_host_vars.sh
	echo "var etherData = new Array();";
	if [ -e /etc/ethers ] ; then
		awk ' $0 ~ /^[\t ]*[0-9abcdefABCDEF]/ {print "etherData.push([\""$1"\",\""$2"\"]);"};' /etc/ethers
	fi
	network_get_ipaddr wan_ip wan
	network_get_ipaddr lan_ip lan
	echo "currentWanIp=\"$wan_ip\";"
	echo "currentLanIp=\"$lan_ip\";"
	echo "var bcastIp=\"$(ifconfig | awk '/^br-lan/{s=$1;getline;print $3}' | cut -b7-)\";"
%>
//-->
</script>
<h1 class="page-header"><%~ wol.WLSect %></h1>
<div id="wol_data" class="row">

	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ WLSect %></h3>
			</div>
			<div class="panel-body">
				<div id="wol_table_container" class="table-responsive"></div>

				<div id="wol_help">
					<a id="wol_help_ref" href="javascript:setDescriptionVisibility('wol_help')"><%~ Hide %></a>
					<span id="wol_help_txt">
						<p><%~ WLHelp %></p>
					</span>
				</div>
			</div>

		</div>
	</div>

</div>

<script>
<!--
	initWolTable();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "wol"
%>
