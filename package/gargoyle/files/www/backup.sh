#!/usr/bin/haserl --upload-limit=4096 --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "backup" -c "internal.css" -j "backup.js" -z "backup.js" network
%>

<fieldset id="backup_section">
	<legend class="sectionheader"><%~ backup.CurrC %></legend>
	<div class='form-group'>
		<span class='leftcolumn'>
			<button id="backup_button" class="default_button btn btn-default" onclick="getBackup()"><%~ GetBackup %></button>
		</span>
	</div>
</fieldset>

<fieldset id="restore_section">
	<legend class="sectionheader"><%~ RestC %></legend>

		<form id='restore_form' enctype="multipart/form-data" method="post" action="utility/do_restore.sh" target="do_restore">
		<div class='form-group'>
			<label class='leftcolumn'><%~ SelOF %>:</label>
			<input class='rightcolumn form-control' type="file" id="restore_file" name="restore_file" />
			<input id='restore_hash' name="hash" type='hidden' value='' />
			</div>
		</form>

	<div class='form-group'>
		<span class='leftcolumn'>
			<button id="restore_button" class="default_button btn btn-default" onclick="doRestore()"/><%~ RestoreConfig %></button>
		</span>
	</div>
	<iframe id="do_restore" name="do_restore" src="#" style="display:none"></iframe>

</fieldset>

<fieldset id="restore_original_section">
	<legend class="sectionheader"><%~ DfltC %></legend>
	<form id='restore_original_form' enctype="multipart/form-data" method="post" action="utility/do_restore_original.sh" target="do_restore_original">
			<input id='restore_original_hash' name="hash" type='hidden' value='' />
	</form>
	<div class="form-group">
		<span class='leftcolumn'>
			<button id="restore_original_button" class="btn btn-default" onclick="doDefaultRestore()"><%~ RestoreDefault %></button>
		</span>
	</div>
	<iframe id="do_restore_original" name="do_restore_original" src="#" style="display:none"></iframe>
</fieldset>
<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<%
	gargoyle_header_footer -f -s "system" -p "backup"
%>
