#!/usr/bin/haserl
<%
	# This program is copyright © 2022 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "diagnostics" -j "table.js diagnostics.js" -z "diagnostics.js" -i gargoyle firewall network
%>

<script>
<!--
<%
	echo "var ramoopsFiles = [];"
	if [ -d /sys/fs/pstore/ ]; then
		ls -1 /sys/fs/pstore/ 2>/dev/null | awk '{print "ramoopsFiles.push(\""$1"\");"}'
	fi
%>
//-->
</script>

<h1 class="page-header"><%~ diagnostics.mDiag %></h1>
<div class="row">
	<div id="device_section" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DevSection %></h3>
			</div>
			<div class="panel-body">
				<div id="ramoops_container" class="col-xs-12">
					<span class="row" style="text-decoration:underline"><%~ crashdump %>:</span>
					<div id="ramoops_table_container" class="table-responsive"></div>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ NetSection %></h3>
			</div>
			<div class="panel-body">
				<div id="ping_container" class="col-xs-12 col-md-4">
					<div class="row form-group">
						<label class="col-xs-5" for="ping_target" id="ping_target_label"><%~ PingTgt %>:</label>
						<span class="col-xs-7">
							<input type="text" id="ping_target" class="form-control" value="8.8.8.8" />
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-7 col-xs-offset-5">
							<input type="radio" id="ping_ipv4" name="ping_family" value="4" checked />
							<label for="ping_ipv4">IPv4</label>
						</span>
						<span class="col-xs-7 col-xs-offset-5">
							<input type="radio" id="ping_ipv6" name="ping_family" value="6" />
							<label for="ping_ipv6">IPv6</label>
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-7 col-xs-offset-5">
							<button id="ping_button" class="btn btn-primary" onclick="doPing()"><%~ Ping %></button>
						</span>
					</div>
				</div>

				<div id="traceroute_container" class="col-xs-12 col-md-4">
					<div class="row form-group">
						<label class="col-xs-5" for="tracert_target" id="tracert_target_label"><%~ TraceTgt %>:</label>
						<span class="col-xs-7">
							<input type="text" id="tracert_target" class="form-control" value="8.8.8.8" />
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-7 col-xs-offset-5">
							<input type="radio" id="tracert_ipv4" name="tracert_family" value="4" checked />
							<label for="tracert_ipv4">IPv4</label>
						</span>
						<span class="col-xs-7 col-xs-offset-5">
							<input type="radio" id="tracert_ipv6" name="tracert_family" value="6" />
							<label for="tracert_ipv6">IPv6</label>
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-7 col-xs-offset-5">
							<button id="tracert_button" class="btn btn-primary" onclick="doTracert()"><%~ Trace %></button>
						</span>
					</div>
				</div>

				<div id="nslookup_container" class="col-xs-12 col-md-4">
					<div class="row form-group">
						<label class="col-xs-5" for="nslookup_target" id="nslookup_target_label"><%~ NSLookupTgt %>:</label>
						<span class="col-xs-7">
							<input type="text" id="nslookup_target" class="form-control" value="google.com" />
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-7 col-xs-offset-5">
							<button id="nslookup_button" class="btn btn-primary" onclick="doNSLookup()"><%~ NSLookup %></button>
						</span>
					</div>
				</div>

				<div class="col-xs-12">
					<div class="row form-group">
						<span class="col-xs-12"><textarea class="form-control" rows=30 id='output' style="width:100%;" readonly></textarea></span>
					</div>
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
	gargoyle_header_footer -f -s "system" -p "diagnostics"
%>
