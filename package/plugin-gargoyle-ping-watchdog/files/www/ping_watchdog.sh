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

<form>
	<fieldset id="log">
		<legend class="sectionheader"><%~ ping.Pdog %></legend>
		<div class="nocolumn">
			<input type='checkbox' id='ping_watchdog_enable' onchange="unlockFields();" />
			<label for='ping_watchdog_enable' id='ping_watchdog_enable_label' ><%~ EnbP %></label>
		</div>
		<div>
			<label class='leftcolumn' for='address_to_ping' id='address_to_ping_label' ><%~ PgIP %>:</label>
			<div class='rightcolumn'>
				<input type='text' id='address_to_ping' size='20' onkeyup='proofreadIp(this)' />
				<em><%~ IPAd %></em>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='ping_interval' id='ping_interval_label' ><%~ Intv %>:</label>
			<div class='rightcolumn'>
				<input type='text' id='ping_interval' size='20' onkeyup='proofreadNumericRange(this,1,59)' />
				<em>1 - 59 <%~ minutes %></em>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='startup_delay' id='startup_delay_label' ><%~ StDly %>:</label>
			<div class='rightcolumn'>
				<input type='text' id='startup_delay' size='20' onkeyup='proofreadNumericRange(this,1,999)' />
				<em>1 - 999 <%~ seconds %></em>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='failure_count' id='failure_count_label' ><%~ FlCnt %>:</label>
			<div class='rightcolumn'>
				<input type='text' id='failure_count' size='20' onkeyup='proofreadNumericRange(this,1,10)' />
				<em>1 - 10</em>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='failure_action' id='failure_action_label' ><%~ Actn %>:</label>
			<select class='rightcolumn' id='failure_action' onchange='showScript(this.value);' >
				<option value='wan'><%~ WRcon %></option>
				<option value='reboot'><%~ Rbot %></option>
				<option value='custom'><%~ Rscp %></option>
			</select>
			<div id='custom_script' style='display: none;'>
				<label class='leftcolumn' for='script' id='script_label' ><%~ Scpt %>:</label>
				<input class='rightcolumn' type='text' id='script' size='30' />
			</div>
		</div>

	</fieldset>
	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>
	<span id="update_container" ><%~ WaitSettings %></span>
</form>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "ping_watchdog"
%>
