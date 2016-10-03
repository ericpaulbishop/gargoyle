#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "dyndns" -c "internal.css" -j "table.js ddns.js" -z "ddns.js" gargoyle ddns_gargoyle

%>

<script>
<!--
<%
	echo "providerData = new Array();"
	awk '{gsub(/"/,"'\''"); print "providerData.push(\""$0"\");" ;}' /etc/ddns_providers.conf

	updates=$(ls /var/last_ddns_updates 2>/dev/null )
	echo "updateTimes = new Array();"
	for update in $updates ; do
		echo "updateTimes[\"$update\"] = " $(cat /var/last_ddns_updates/$update) ";"
	done

%>
//-->
</script>

<h1 class="page-header"><%~ ddns.mDDNS %></h1>
<div class="row">

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ddns.DYSect %></h3>
			</div>
			<div class="panel-body">

		<span id="add_ddns_label"><p><%~ AddDy %>:</p></span>

		<div>
			<div class="form-group form-inline">
				<label for="ddns_provider" id="ddns_provider_label"><%~ SvPro %>:</label>
				<select class="form-control" id="ddns_provider" onchange="setProvider()"></select>
			</div>
		</div>

		<div id="ddns_variable_container"></div>

		<div>

			<div class="form-group form-inline">
				<label for="ddns_check" id="ddns_check_label"><%~ ChItv %>:</label>
				<input type="text" class="form-control" id="ddns_check"  size="8" onkeyup="proofreadNumeric(this)"/>
				<em><%~ minutes %></em>
			</div>

			<div class="form-group form-inline">
				<label for="ddns_force" id="ddns_force_label"><%~ FUItv %>:</label>
				<input type="text" class="form-control" id="ddns_force"  size="8" onkeyup="proofreadNumeric(this)"/>
				<em><%~ days %></em>
			</div>

			<div>
				<button id="add_service_button" class="btn btn-info" onclick="addDdnsService()"><%~ AddDDNS %></button>
			</div>

			<div>
				<span id="ddns_1_txt">
					<p><%~ HelpCI %></p>
					<p><%~ HelpFI %></p>
				</span>
				<a onclick="setDescriptionVisibility('ddns_1')"  id="ddns_1_ref" href="#ddns_1"><%~ Hide %></a>
			</div>

		</div>

		<div id="internal_divider1" class="internal_divider"></div>

		<span id="add_ddns_label"><p><%~ DYSect %>:</p></span>

		<div id="ddns_table_container" class="table-responsive"></div>

	</div>
</div>
</div>
</div>
	<div id="bottom_button_container">
		<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
		<button id="reset_button" class="btn btn-danger" onclick="resetData()"><%~ Reset %></button>
	</div>

	<span id="update_container" ><%~ WaitSettings %></span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "dyndns"
%>
