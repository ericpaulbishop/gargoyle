#!/usr/bin/haserl
<%
	# This program is copyright © 2013 Cezary Jackiewicz and Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "minidlna" -j "table.js minidlna.js" -z "minidlna.js" minidlna
%>
<script>
<!--
<%
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr 2>/dev/null)
	echo "currentLanIp=\"$lan_ip\";"
	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null
	df /overlay | head -n2 | awk -v root="$(i18n minidlna.RootD)" '/overlay/ {printf "storageDrives.push([\"%s\",\"/\",\"/\",\"jffs2\", \"%.0f\", \"%.0f\"]);" , root, $2 * 1024, $4 * 1024}'
%>
//-->
</script>

<h1 class="page-header"><%~ minidlna.DLNA %></h1>
<div id="dlna" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="dlna_enable" type="checkbox" />
						<label id="dlna_enable_label" for="dlna_enable"><%~ DLNAEn %></label>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12"><button id="status_button" class="btn btn-default" onclick='statusDlna()'><%~ StatDLNA %></button></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><button id="rescan_button" class="btn btn-default" onclick='rescanMedia()'><%~ RescanDLNA %></button></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="dlna_name_label" for="dlna_name"><%~ DLNASName %>:</label>
					<span class="col-xs-7"><input id="dlna_name" type="text" size='30' class="form-control" /></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input id="dlna_strict" type="checkbox" />
						<label id="dlna_strict_label" for="dlna_strict"><%~ DLNAStr %></label>
					</span>
				</div>

				<div class="internal_divider"></div>

				<div class="form-group">
					<div class="row form-group">
						<label class="col-xs-5" id="media_type_label" for="media_type"><%~ DLNAMType %>:</label>
						<span class="col-xs-7">
							<select id="media_type" class="form-control">
								<option value=""><%~ DLNAAll %></a>
								<option value="A"><%~ DLNAA %></a>
								<option value="V"><%~ DLNAV %></a>
								<option value="P"><%~ DLNAI %></a>
							</select>
						</span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" id="drive_select_label" for="drive_select"><%~ DLNADrive %>:</label>
						<span class="col-xs-7"><select id="drive_select" class="form-control"></select></span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" id="media_dir_label" for="media_dir"><%~ DLNAMFolder %>:</label>
						<span class="col-xs-7"><input type="text" id="media_dir" class="form-control" size='30'/></span>
					</div>

					<div class="row form-group">
						<span class="col-xs-12"><button id="add_share_button" class="btn btn-default btn-add" onclick="addNewMediaDir()"><%~ Add %></button></span>
					</div>
				</div>

				<div class="internal_divider"></div>

				<div id="sharing_current_heading_container">
					<span style="text-decoration:underline;"><%~ DLNAFolders %>:</span>
				</div>

				<div id="media_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick='saveChanges()'><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick='resetData()'><%~ Reset %></button>
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "minidlna"
%>
