#!/usr/bin/haserl
<%
	# This program is copyright © 2020 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	#
	# Originally written by Saski (c) 2013 under GPL. Rewritten for modern Gargoyle.
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "cron" -j "table.js cron.js" -z "cron.js"
%>

<h1 class="page-header"><%~ cron.Tasks %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Tasks %></h3>
			</div>
			<div class="panel-body">
				<div id="task">
					<div id="task_table_container" class="table-responsive"></div>
				</div>

				<div id="no_task" style="display:none">
					<em><span class="col-md-12"><%~ NoTasks %></span></em>
				</div>

				<div class="row form-group">
					<span class="col-xs-12"><button id="add_cron_button" class="btn btn-default btn-add" onclick="addCronModal()"><%~ AddTasks %></button></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="cron_task_modal" aria-hidden="true" aria-labelledby="cron_task_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="cron_task_modal_title" class="panel-title"><%~ cron.AddTasks %></h3>
			</div>
			<div class="modal-body">
				<%in templates/cron_task_template %>
			</div>
			<div class="modal-footer" id="cron_task_modal_button_container">
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
	gargoyle_header_footer -f -s "system" -p "cron"
%>

