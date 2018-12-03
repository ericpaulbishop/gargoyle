#!/usr/bin/haserl
<%
	# This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "ping_watchdog" -j "ping_watchdog.js" -z "ping.js"
%>

<h1 class="page-header"><%~ ping.Pdog %></h1>
<div id="log" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Pdog %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' id='ping_watchdog_enable' onchange="unlockFields();"/>
						<label for='ping_watchdog_enable' id='ping_watchdog_enable_label'><%~ EnbP %></label>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for='address_to_ping' id='address_to_ping_label'><%~ PgIP %>:</label>
					<span class="col-xs-7">
						<input type='text' id='address_to_ping' class="form-control" size='20' onkeyup='proofreadIp(this)'/>
						<em class="help-block"><%~ IPAd %></em>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for='ping_interval' id='ping_interval_label'><%~ Intv %>:</label>
					<span class="col-xs-7">
						<input type='text' id='ping_interval' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,59)'/>
						<em class="help-block">1 - 59 <%~ minutes %></em>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for='startup_delay' id='startup_delay_label'><%~ StDly %>:</label>
					<span class="col-xs-7">
						<input type='text' id='startup_delay' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,999)'/>
						<em class="help-block">1 - 999 <%~ seconds %></em>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for='failure_count' id='failure_count_label'><%~ FlCnt %>:</label>
					<span class="col-xs-7">
						<input type='text' id='failure_count' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,10)'/>
						<em class="help-block">1 - 10</em>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for='failure_action' id='failure_action_label'><%~ Actn %>:</label>
					<span class="col-xs-7">
						<select id='failure_action' class="form-control" onchange='showScript(this.value);'>
							<option value='wan'><%~ WRcon %></option>
							<option value='reboot'><%~ Rbot %></option>
							<option value='custom'><%~ Rscp %></option>
						</select>
					</span>
					<span class="row form-group">
						<div id='custom_script' class="indent" style='display: none;'>
							<label class="col-xs-5" for='script' id='script_label'><%~ Scpt %>:</label>
							<span class="col-xs-7"><input type='text' class="form-control" id='script' size='30'/></span>
						</div>
					</span>
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
	gargoyle_header_footer -f -s "connection" -p "ping_watchdog"
%>
