#!/usr/bin/haserl --upload-limit=4096 --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "backup" -j "backup.js" -z "backup.js" network
%>
<h1 class="page-header"><%~ backup.mBackup %></h1>
<div class="row">
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ backup.CurrC %></h3>
			</div>
			<div id="backup_section" class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><button id="backup_button" class="btn btn-success btn-lg" onclick="getBackup()"><%~ GetBackup %></button></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ RestC %></h3>
			</div>
			<div id="restore_section" class="panel-body">
				<form id="restore_form" enctype="multipart/form-data" method="post" action="utility/do_restore.sh" target="do_restore">
					<div class="row form-group">
						<label class="col-xs-5"><%~ SelOF %>:</label>
						<span class="col-xs-7"><input type="file" id="restore_file" name="restore_file" /></span>
						<input id="restore_hash" name="hash" type="hidden" value=""/>
					</div>
					<br />
				</form>
				<div class="row form-group">
					<span class="col-xs-12"><button id="restore_button" class="btn btn-warning btn-lg" onclick="doRestore()"><%~ RestoreConfig %></button></span>
				</div>
				<iframe id="do_restore" name="do_restore" src="#" style="display:none"></iframe>
			</div>
		</div>
	</div>

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DfltC %></h3>
			</div>
			<div id="restore_original_section" class="panel-body">
				<form id="restore_original_form" enctype="multipart/form-data" method="post" action="utility/do_restore_original.sh" target="do_restore_original">
					<input id="restore_original_hash" name="hash" type="hidden" value=""/>
				</form>
				<div class="row form-group">
					<span class="col-xs-12"><button id="restore_original_button" class="btn btn-danger btn-lg" onclick="doDefaultRestore()"><%~ RestoreDefault %></button></span>
				</div>
				<iframe id="do_restore_original" name="do_restore_original" src="#" style="display:none"></iframe>
			</div>
		</div>
	</div>

	<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>
</div>
<%
	gargoyle_header_footer -f -s "system" -p "backup"
%>

