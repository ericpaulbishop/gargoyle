#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "routing" -j "table.js routing.js" -z "routing.js" network
	subnet=$(ifconfig br-lan | awk 'BEGIN {FS=":"}; $0 ~ /inet.addr/ {print $2}' | awk 'BEGIN {FS="."}; {print $1"\."$2"\."$3"\."}')
%>

<script>
<!--
<%
	lan_iface=$(uci -P /var/state get network.lan.ifname)
	wan_iface=$(uci -q -P /var/state get network.wan.ifname)
	echo "var lanIface=\"$lan_iface\";"
	echo "var wanIface=\"$wan_iface\";"
	echo "var routingData = new Array();"
	route | awk ' {print "routingData.push(\""$0"\");"};'
%>

//-->
</script>

<h1 class="page-header"><%~ routing.mRoutes %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ routing.ARSect %></h3>
			</div>

			<div class="panel-body">
				<div id="active_route_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SRSect %></h3>
			</div>

			<div class="panel-body">
				<div id="static_route_add_heading_container">
					<label id="staticroute_add_heading_label" style="text-decoration:underline"><%~ ASRte %>:</label>
				</div>

				<div class="row form-group">
					<div id="static_route_add_container" class="col-xs-12">
						<button id="add_button" class="btn btn-primary btn-add" onclick="addStaticModal()"><%~ Add %></button>
					</div>
				</div>

				<div id="static_route_table_heading_container">
					<label id="staticroute_current_heading_label" style="text-decoration:underline"><%~ CSRSect %>:</label>
				</div>

				<div class="row form-group">
					<div id="static_route_table_container" class="col-xs-12 bottom_gap table-responsive"></div>
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

<div class="modal fade" tabindex="-1" role="dialog" id="static_route_modal" aria-hidden="true" aria-labelledby="static_route_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="static_route_modal_title" class="panel-title"><%~ ASRte %></h3>
			</div>
			<div class="modal-body">
				<%in templates/static_route_template %>
			</div>
			<div class="modal-footer" id="static_route_modal_button_container">
			</div>
		</div>
	</div>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>


<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "routing"
%>
