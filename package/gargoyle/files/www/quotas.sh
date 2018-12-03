#!/usr/bin/haserl
<%
	# This program is copyright © 2008,2009 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "quotas" -j "gs_sortable.js table.js quotas.js" -z "quotas.js" gargoyle firewall qos_gargoyle

%>

<script>
<!--
<%
	echo "var qosMarkList = [];"
	if [ -h /etc/rc.d/S50qos_gargoyle ] && [ -e /etc/qos_class_marks ]  ; then
		echo "var fullQosEnabled = true;"
		awk '{ print "qosMarkList.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\"]);" }' /etc/qos_class_marks
	else
		echo "var fullQosEnabled = false;"
	fi

	echo "var tcInstalled=\""$(opkg list-installed 2>&1 | grep "^tc")"\";"

	print_quotas
%>
	var uci = uciOriginal.clone();
//-->
</script>

<h1 class="page-header"><%~ quotas.mQuotas %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ quotas.Section %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<span id="add_quota_label" style="text-decoration:underline"><%~ AddQuota %>:</span>
						<%in templates/quotas_template %>
						<button id="add_quota_button" class="btn btn-default btn-add" onclick="addNewQuota()"><%~ AddQuota %></button>
					</span>
				</div>

				<div id="internal_divider1" class="internal_divider"></div>

				<div class="row form-group">
					<span class="col-xs-12">
						<span id="active_quotas_label" style="text-decoration:underline"><%~ ActivQuotas %>:</span>
						<div id="quota_table_container" class="table-responsive"></div>
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
	gargoyle_header_footer -f -s "firewall" -p "quotas"
%>
