#!/usr/bin/haserl
<%
	# This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "ping_watchdog" -c "internal.css" -j "ping_watchdog.js" -z "ping.js"
%>

<h1 class="page-header"><%~ ping.Pdog %></h1>
<div id="log" class="row">

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-body">
				<div class="form-group">
					<input type='checkbox' id='ping_watchdog_enable' onchange="unlockFields();"/>
					<label for='ping_watchdog_enable' id='ping_watchdog_enable_label'><%~ EnbP %></label>
				</div>

				<div class="form-group">
					<label for='address_to_ping' id='address_to_ping_label'><%~ PgIP %>:</label>
					<div class="form-group">
						<input type='text' id='address_to_ping' class="form-control" size='20' onkeyup='proofreadIp(this)'/>
						<p class="help-block"><%~ IPAd %></p>
					</div>
				</div>

				<div class="form-group">
					<label for='ping_interval' id='ping_interval_label'><%~ Intv %>:</label>
					<div class="form-group">
						<input type='text' id='ping_interval' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,59)'/>
						<p class="help-block">1 - 59 <%~ minutes %></p>
					</div>
				</div>

				<div class="form-group">
					<label for='startup_delay' id='startup_delay_label'><%~ StDly %>:</label>
					<div class="form-group">
						<input type='text' id='startup_delay' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,999)'/>
						<p class="help-block">1 - 999 <%~ seconds %></p>
					</div>
				</div>

				<div class="form-group">
					<label for='failure_count' id='failure_count_label'><%~ FlCnt %>:</label>
					<div class="form-group">
						<input type='text' id='failure_count' class="form-control" size='20' onkeyup='proofreadNumericRange(this,1,10)'/>
						<p class="help-block">1 - 10</p>
					</div>
				</div>

				<div class="form-group">
					<label for='failure_action' id='failure_action_label' class="form-control"><%~ Actn %>:</label>
					<select id='failure_action' class="form-control" onchange='showScript(this.value);'>
						<option value='wan'><%~ WRcon %></option>
						<option value='reboot'><%~ Rbot %></option>
						<option value='custom'><%~ Rscp %></option>
					</select>
					<div id='custom_script' class="form-group" style='display: none;'>
						<label for='script' id='script_label'><%~ Scpt %>:</label>
						<input type='text' class="form-control" id='script' size='30'/>
					</div>
				</div>
			</div>

		</div>
	</div>

</div>
<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-danger" onclick="resetData()"><%~ Reset %></button>
</div>
<span id="update_container"><%~ WaitSettings %></span>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "ping_watchdog"
%>
