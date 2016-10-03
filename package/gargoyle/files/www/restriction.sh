#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "restriction" -c "internal.css" -j "table.js restrictions.js" -z "restrictions.js" gargoyle firewall

%>

<script>
<!--
	var uci = uciOriginal.clone();
//-->
</script>


<h1 class="page-header"><%~ restrictions.mRestrict %></h1>
<div class="row">

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ restrictions.ARSect %></h3>
			</div>

			<div class="panel-body">
				<span id="add_rule_label" style="text-decoration:underline" ><%~ NRRule %>:</span>
				<div>
					<%in templates/restriction_template %>
					<div>
						<button id="add_restriction_button" class="btn btn-default" onclick="addNewRule('restriction_rule', 'rule_')"><%~ ANRule %></button>
					</div>
				</div>

				<div id="internal_divider1" class="internal_divider"></div>
				<span id="current_rule_label" style="text-decoration:underline" ><%~ CRestr %>:</span>
				<div id="rule_table_container" class="table-responsive"></div>
			</div>

		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ EWSect %></h3>
			</div>
			<div class="panel-body">

				<span id="add_exception_label" style="text-decoration:underline" ><%~ NExcp %>:</span>

				<div>
					<%in templates/whitelist_template %>

					<div>
						<button id="add_restriction_button" class="btn btn-default" onclick="addNewRule('whitelist_rule', 'exception_')"><%~ ANRule %></button>
					</div>
				</div>

				<div id="internal_divider1" class="internal_divider"></div>
				<span id="current_exceptions_label" style="text-decoration:underline" ><%~ CExcp %>:</span>
				<div id="exception_table_container" class="table-responsive"></div>
			</div>

		</div>
	</div>

</div>

<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick="resetData()"><%~ Reset %></button>
</div>
<span id="update_container" ><%~ WaitSettings %></span>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "restriction"
%>
