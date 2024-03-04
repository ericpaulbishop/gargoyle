#!/usr/bin/haserl
<%
	# This program is copyright © 2024 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "doh" -j "table.js doh.js" -z "doh.js" gargoyle https-dns-proxy

%>

<script>
<!--
<%
	echo "providerData = new Array();"
	echo "providerData.push('[');"
	for file in /usr/lib/https-dns-proxy/* ; do
		awk '{print "providerData.push('"\'"'"$0"'"\'"');" ;}' $file
		echo "providerData.push(',');"
	done
	echo "providerData[providerData.length-1] = ']';"
	echo "providerData = JSON.parse(providerData.join('\n'));"
	echo "providerData.sort(function(a,b) {return (a.title < b.title) ? -1 : ((a.title > b.title) ? 1 : 0)});"

	$(curl --version | grep -q 'nghttp2') && echo "curlhttp2=true;" || echo "curlhttp2=false;"
	hdprunning=$(ubus call service list "{'name':'https-dns-proxy'}" | jsonfilter -q -e "@['https-dns-proxy'].instances[*].running" | uniq)
	[ "$hdprunning" = "true" ] && echo "serviceRunning=true;" || echo "serviceRunning=false;"
	echo "serviceRunningPorts='"$(ubus call service list "{'name':'https-dns-proxy'}" | jsonfilter -q -e "@['https-dns-proxy'].instances[*].data.mdns.*.port")"';"
%>
//-->
</script>

<style>
	/* Hide the bootstrap dns column */
	table#doh_configured_instances_table th:nth-child(5),table#doh_configured_instances_table td:nth-child(5)
	{
   		display: none;
	}
</style>

<h1 class="page-header"><%~ doh.mDoH %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ doh.DoHSect %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" id="hdp_status_label" for="hdp_status"><%~ doh.Status %>:</label>
					<span class="col-xs-7" id="hdp_status"></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="doh_enable"/>
						<label id="doh_enable_label" for="doh_enable"><%~ doh.Enable %> DNS over HTTPS (DoH) Proxy</label>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="icloud_canary_domain_label" for="icloud_canary_domain"><%~ doh.BlockiCloud %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="icloud_canary_domain">
							<option value="1"><%~ doh.Block %></option>
							<option value="0"><%~ doh.Allow %></option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="mozilla_canary_domain_label" for="mozilla_canary_domain"><%~ doh.BlockMozilla %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="mozilla_canary_domain">
							<option value="1"><%~ doh.Block %></option>
							<option value="0"><%~ doh.Allow %></option>
						</select>
					</span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ doh.DoHConfInst %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<div class="row col-xs-12">
						<span class="col-xs-5">
							<div class="row col-xs-12">
								<label id="select_provider_label" for="select_provider"><%~ doh.Prov %>:</label>
							</div>

							<div class="row col-xs-12">
								<select class="form-control" id="select_provider" onchange="setProviderOptionSelect()"></select>
							</div>
						</span>

						<span class="col-xs-5">
							<div class="row col-xs-12">
								<label id="provider_option_label" for="provider_option"></label>
							</div>

							<div class="row col-xs-12">
								<select class="form-control" id="select_provider_option"></select>
								<input type="text" class="form-control" id="text_provider_option" value="" style="display:none">
							</div>
						</span>

						<div class="col-xs-2">
							<div class="row col-xs-12">
								<label id=doh_resolver_add_label" for="doh_resolver_add"></label>
							</div>

							<div class="row col-xs-12">
								<button id='doh_resolver_add' class='btn btn-default btn-add' onclick='addDoHResolver()'><%~ Add %></button>
							</div>
						</div>
					</div>

					<div class='col-xs-12'>
						<div id="doh_configured_instances_table_container" class="table table-responsive"></div>
					</div>
				</div>
			</div>
		</div>
	</div>
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
	gargoyle_header_footer -f -s "connection" -p "doh"
%>
