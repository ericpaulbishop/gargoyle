#!/usr/bin/haserl
<?
	# This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "minidlna" -c "internal.css" -j "table.js minidlna.js" minidlna
?>
<script>
<!--
<?
	lan_ip=$(uci -p /tmp/state get network.lan.ipaddr 2>/dev/null)
	echo "currentLanIp=\"$lan_ip\";"
	echo "var storageDrives = [];"
	df /overlay | head -n2 | awk '/overlay/ {printf "storageDrives.push([\"Root Disk\",\"/\",\"/\",\"jffs2\", \"%.0f\", \"%.0f\"]);" , $2 * 1024, $4 * 1024}'
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null
?>
//-->
</script>
<fieldset id="dlna">
	<legend class="sectionheader">DLNA</legend>

	<div>
		<label id="dlna_enable_label" class="leftcolumn" for="webcam_enable">Enabled:</label>
		<input id="dlna_enable" class="rightcolumn" type="checkbox" />&nbsp;&nbsp;<span id="dlna_status"></span>
	</div>

	<div class="rightcolumnonly">
		<input type='button' value='DLNA Status' id="status_button" class="default_button" onclick='statusDlna()' />
	</div>
	<div class="rightcolumnonly">
		<input type='button' value='Rescan media' id="rescan_button" class="default_button" onclick='rescanMedia()' />
	</div>

	<div>
		<label id="dlna_name_label" class="leftcolumn" for="dlna_name">Server name:</label>
		<input id="dlna_name" class="rightcolumn" type="text" size='30' />
	</div>

	<div>
		<label id="dlna_strict_label" class="leftcolumn" for="dlna_strict">Strict DLNA:</label>
		<input id="dlna_strict" class="rightcolumn" type="checkbox" />
	</div>

	<div class="internal_divider"></div>

	<div>
		<label class="leftcolumn" id="media_type_label" for="media_type">Media type:</label>
		<select id="media_type" class="rightcolumn">
		<option value="">All</a>
		<option value="A">Audio</a>
		<option value="V">Video</a>
		<option value="P">Image</a>
		</select>
	</div>

	<div>
		<label class="leftcolumn" id="drive_select_label" for="drive_select">Drive:</label>
		<select id="drive_select" class="rightcolumn"></select>
	</div>

	<div>
		<label class="leftcolumn" id="media_dir_label" for="media_dir">Media folder:</label>
		<input type="text" id="media_dir" class="rightcolumn" size='30'/>
	</div>

	<div>
		<input type="button" id="add_share_button" class="default_button" value="Add" onclick="addNewMediaDir()" />
	</div>

	<div class="internal_divider"></div>

	<div id="sharing_current_heading_container">
		<span class="nocolumn" style="text-decoration:underline;">Folders list:</span>
	</div>

	<div id="media_table_container"></div>

</fieldset>

<div id="bottom_button_container">
	<input type='button' value='Save changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
</div>

<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "system" -p "minidlna"
?>
