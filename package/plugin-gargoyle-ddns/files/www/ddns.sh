#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "dyndns" -j "table.js ddns.js" -z "ddns.js" gargoyle ddns_gargoyle

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

	echo "extScripts = new Array();"
	external_scripts=$(ls /usr/lib/ddns-gargoyle 2&>1)
	for script in $external_scripts ; do
		echo "extScripts.push(\""$script"\");"
	done

%>
//-->
</script>

<h1 class="page-header"><%~ ddns.mDDNS %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ddns.DYSect %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><button id="add_service_button" class="btn btn-default btn-add" onclick="addDDNSModal()"><%~ AddDDNS %></button></span>
				</div>

				<div id="internal_divider1" class="internal_divider"></div>

				<span style="text-decoration:underline" id="add_ddns_label"><p><%~ DYSect %>:</p></span>

				<div id="ddns_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="ddns_service_modal" aria-hidden="true" aria-labelledby="ddns_service_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="ddns_service_modal_title" class="panel-title"><%~ AddDy %></h3>
			</div>
			<div class="modal-body">
				<%in templates/ddns_service_template %>
			</div>
			<div class="modal-footer" id="ddns_service_modal_button_container">
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
	gargoyle_header_footer -f -s "connection" -p "dyndns"
%>
