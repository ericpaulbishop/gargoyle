#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "quotause" -c "internal.css" -j "table.js quota_usage.js" -z "quotas.js" -n gargoyle firewall
%>

<script>
<!--
<%
	print_quotas
%>

//-->
</script>

<h1 class="page-header"><%~ quotas.USect %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-body">
				<div class="form-group form-inline">
					<select id="host_display" class="form-control">
						<option value="hostname"><%~ DspHn %></option>
						<option value="ip"><%~ DspHIP %></option>
					</select>
					<select id="data_display" class="form-control">
						<option value="pcts"><%~ DspPct %></option>
						<option value="usds"><%~ DspUsd %></option>
						<option value="lims"><%~ DspLim %></option>
					</select>
				</div>
				<div id="quota_table_container"></div>
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
	gargoyle_header_footer -f -s "status" -p "quotause"
%>
