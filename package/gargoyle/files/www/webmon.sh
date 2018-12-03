#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "webmon" -j "webmon.js table.js" -z "webmon.js" -n webmon_gargoyle gargoyle
%>

<script>
<!--
<%
	webmon_enabled=$(ls /etc/rc.d/*webmon_gargoyle* 2>/dev/null)
	if [ -n "$webmon_enabled" ] ; then
		echo "var webmonEnabled=true;"
	else
		echo "var webmonEnabled=false;"
	fi

%>
//-->
</script>

<h1 class="page-header"><%~ webmon.mWMon %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ webmon.PrSect %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="webmon_enabled" onclick="setWebmonEnabled()" />
						<label id="webmon_enabled_label" for="webmon_enabled"><%~ EMon %></label>
					</span>
				</div>

				<div>
					<div class="row form-group">
						<label class="col-xs-5" for="num_domains" id="num_domains_label"><%~ NumSt %>:</label>
						<span class="col-xs-7"><input type="text" class="form-control" id="num_domains" onkeyup="proofreadNumericRange(this,1,9999)" size="6" maxlength="4" /></span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" for="num_searches" id="num_searches_label"><%~ NumSr %>:</label>
						<span class="col-xs-7"><input type="text" class="form-control" id="num_searches" onkeyup="proofreadNumericRange(this,1,9999)" size="6" maxlength="4" /></span>
					</div>

					<div class="row form-group">
						<span class="col-xs-12">
							<select id="include_exclude" class="form-control" onchange="setIncludeExclude()">
								<option value="all"><%~ MnAH %></option>
								<option value="include"><%~ MnOnly %></option>
								<option value="exclude"><%~ MnExcl %></option>
							</select>
						</span>
					</div>

					<div id="add_ip_container">
						<div class="row form-group">
							<span class="col-xs-12">
								<input type="text" id="add_ip" onkeyup="proofreadMultipleIps(this)" size="30" />
								<button class="btn btn-default btn-add" id="add_ip_button" onclick="addAddressesToTable(document, 'add_ip', 'ip_table_container', 'ip_table', false, 3, 1, 250)"><%~ Add %></button>
							</span>
							<em class="col-xs-12"><%~ SpcIP %></em>
						</div>
						<div id="ip_table_container" class="table-responsive"></div>
					</div>
				</div>

				<div class="internal_divider"></div>

				<div id="bottom_button_container">
					<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
					<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
					<button id="clear_history" class="btn btn-danger btn-lg" onclick="clearHistory()"><%~ Clear %></button>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ RctSt %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<select id="domain_host_display" class="row form-group" onchange="updateMonitorTable()">
							<option value="hostname"><%~ DspHn %></option>
							<option value="ip"><%~ DspHIP %></option>
						</select>
					</span>
				</div>

				<div id="webmon_domain_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ RctSr %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<select id="search_host_display" class="form-control" onchange="updateMonitorTable()">
							<option value="hostname"><%~ DspHn %></option>
							<option value="ip"><%~ DspHIP %></option>
						</select>
					</span>
				</div>

				<div id="webmon_search_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div id="download_web_usage_data" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DlWD %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ cmsep %>:</span>
					<code class="col-xs-12"><%~ dForm %></code>
				</div>

				<div>
					<button id="download_domain_button" class="btn btn-info btn-lg" onclick="window.location='webmon_domains.csv';"><%~ VSit %></button>
					<button id="download_search_button" class="btn btn-info btn-lg" onclick="window.location='webmon_searches.csv';"><%~ SRqst %></button>
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
	gargoyle_header_footer -f -s "status" -p "webmon"
%>
