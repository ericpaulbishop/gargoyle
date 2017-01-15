#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "usb_storage" -c "internal.css" -j "gs_sortable.js table.js usb_storage.js" -z "usb_storage.js" gargoyle network firewall nfsd samba vsftpd share_users

%>

<script>
<!--
<%
	echo "var driveSizes = [];"

	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$7"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

	echo "var physicalDrives = [];"

	#note that drivesWithNoMounts, refers to drives
	# with no mounts on the OS, not lack of network mounts
	echo "var drivesWithNoMounts = [];"

	#ugly one-liner
	#unmounted_drives=$( drives=$(cat /tmp/drives_found.txt | grep "dev" | sed 's/[0-9]:.*$//g' | uniq) ; for d in $drives ; do mounted=$(cat /proc/mounts | awk '$1 ~ /dev/ { print $1 }' | uniq |  grep "$d") ; if [ -z "$mounted" ] ; then echo "$d" ; fi  ; done )

	drives="$(awk  -F':' '$1 ~ /^\/dev\// { sub(/[0-9]+$/, "", $1); arr[$1]; } END { for (x in arr) { print x; } }' /tmp/drives_found.txt)"
	for d in ${drives}; do
		if awk -v devpath="^${d}[0-9]+" '$1 ~ devpath { is_mounted = "yes"} END { if (is_mounted == "yes") { exit 1; } }' /proc/mounts; then
			size=$(( 1024 * $(fdisk -s "$d") ))
			DRIVE=$(echo ${d##/*/})
			if [ -e "/sys/class/block/$DRIVE/device/model" ]; then
				V=$(cat /sys/class/block/$DRIVE/device/vendor | xargs)
				P=$(cat /sys/class/block/$DRIVE/device/model | xargs)
				DRIVE="$V $P"
			else
				DRIVE=$d
			fi
			echo "drivesWithNoMounts.push( [ \"$d\", \"$size\", \"$DRIVE\" ] );"
		fi
	done
	echo "var extroot_enabled=\""$(mount | grep "/dev/sd.*on /overlay" | wc -l)"\";"
	echo "var extroot_drive=\""$(mount | awk '/\/dev\/sd.*on \/overlay/ {print $1}')"\";"
%>
//-->
</script>

<h1 class="page-header"><%~ usb_storage.mUSB %></h1>
<div id="no_disks" style="display:none;" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ usb_storage.SDisk %></h3>
			</div>

			<div class="panel-body">
				<em><span><%~ Nomdsk %></span></em>
			</div>
		</div>
	</div>
</div>

<div id="shared_disks" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SDisk %></h3>
			</div>

			<div class="panel-body">
				<div id='ftp_wan_access_container' class="row form-group">
					<span class="col-xs-1"><input type='checkbox' id='ftp_wan_access' onclick='updateWanFtpVisibility()'/></span>
					<label class="col-xs-11" id='ftp_wan_access_label' for='ftp_wan_access'><%~ WFTP %></label>
				</div>

				<div id='ftp_pasv_container' class="row form-group">
					<span class="col-xs-1"><input type='checkbox' id='ftp_wan_pasv' onclick='updateWanFtpVisibility()'/></span>
					<span class="col-xs-11">
						<label id='ftp_wan_access_label' for='ftp_wan_pasv'><%~ WpFTP %></label>&nbsp;
						<input class="form-control" type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)' id='pasv_min_port' />&nbsp;-&nbsp;
						<input class="form-control" type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)' id='pasv_max_port' />
					</span>
				</div>

				<div id="ftp_wan_spacer" style="height:15px;"></div>

				<div id="cifs_workgroup_container" class="row form-group" style="margin-bottom:20px;">
					<label class="col-xs-5" id="cifs_workgroup_label" for="cifs_workgroup"><%~ CFWkg %>:</label>
					<span class="col-xs-7"><input id="cifs_workgroup" class="form-control" type="text" size='30'/></span>
				</div>

				<div id="user_container" class="row form-group">
					<label class="col-xs-12" id="cifs_user_label"><%~ CFUsr %>:</label>
					<label class="col-xs-5" id="user_label" for="new_user"><%~ NewU %>:</label>
					<span class="col-xs-7"><input id="new_user" class="form-control" type="text"/></span>
				</div>

				<div id="user_pass_container" class="row form-group">
					<label class="col-xs-5" id="user_pass_label" for="user_pass"><%~ Pasw %>:</label>
					<span class="col-xs-7"><input id="user_pass" class="form-control" type="password"/></span>
				</div>
				<div id="user_pass_confirm_container" class="row form-group">
					<label class="col-xs-5" id="user_pass_confirm_label" for="user_pass_confirm"><%~ CfPass %>:</label>
					<span class="col-xs-7"><input id="user_pass_confirm" class="form-control" type="password"/></span>
				</div>
				<div id="user_pass_container" class="row form-group">
					<span class="col-xs-12"><button id="add_user" class="btn btn-default" onclick="addUser()"><%~ AddU %></button></span>
				</div>

				<div style="margin-bottom:20px;" class="row form-group">
					<div id="user_table_container" class="col-xs-offset-5 col-xs-7"></div>
				</div>

				<div id="sharing_add_heading_container" class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ ADir %>:</span>
				</div>

				<div id="sharing_add_controls_container" class="row form-group">
					<%in templates/usb_storage_template %>
					<span class="col-xs-12">
						<button id="add_share_button" class="btn btn-default" onclick="addNewShare()"><%~ ADsk %></button>
					</span>
				</div>
				<div class="internal_divider"></div>

				<div id="sharing_current_heading_container" class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ CShare %>:</span>
				</div>
				<div id="sharing_mount_table_container" class="table-responsive"></div>

				<div class="internal_divider"></div>

				<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
				<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
			</div>
		</div>
	</div>

	<div id="disk_unmount" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Umnt %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><button id="unmount_usb_button" class="btn btn-warning btn-lg" onclick="unmountAllUsb()"><%~ UmntB %></button></span>
					<span class="col-xs-12 alert alert-warning"><em><%~ UmntWarn %></em></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="disk_format" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ FDisk %></h3>
			</div>

			<div class="panel-body">
				<div id="no_unmounted_drives" class="row form-group">
					<em><span class="col-xs-12"><%~ NoUmntDev %></span></em>
				</div>

				<div id="format_warning" class="row form-group">
					<em><span class="col-xs-12 alert alert-danger"><%~ FmtWarn %></span></em>
				</div>

				<div id="format_disk_select_container" class="row form-group">
					<label class="col-xs-5" id="format_disk_select_label" for="format_disk_select"><%~ DskForm %>:</label>
					<span class="col-xs-7"><select class="form-control" id="format_disk_select"></select></span>
					<br/>
					<span class="col-xs-12" id="format_warning"></span>
				</div>

				<div id="swap_percent_container" class="row form-group">
					<label class="col-xs-5" id="swap_percent_label" for="swap_percent"><%~ PSwap %>:</label>
					<span class="col-xs-7">
						<input id="swap_percent" class="form-control" type="text" onkeyup="updateFormatPercentages(this.id)" /><em><span id="swap_size"></span></em>
					</span>
				</div>

				<div id="storage_percent_container" class="row form-group">
					<label class="col-xs-5" id="storage_percent_label" for="storage_percent"><%~ PStor %>:</label>
					<span class="col-xs-7">
						<input id="storage_percent" class="form-control" type="text" onkeyup="updateFormatPercentages(this.id)" /><em><span id="storage_size"></span></em>
					</span>
				</div>

				<div id="extroot_container" class="row form-group">
					<span class="col-xs-1"><input type="checkbox" id="extroot" name="extroot"/></span>
					<label class="col-xs-11" id="extroot_label" for="extroot" style="vertical-align:middle"><%~ MExtr %></label>
					<div class="col-xs-12 alert alert-warning">
						<em><%~ ExtrWarn %></em>
					</div>
				</div>

				<div id="usb_format_button_container" class="row form-group">
					<span class="col-xs-12"><button id="usb_format_button" class="btn btn-danger btn-lg" onclick="formatDiskRequested()"><%~ FmtNow %></button></span>
				</div>
			</div>
		</div>
	</div>

	<div id="extroot_fieldset" style="display:none;" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ExtrS %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><button id="extroot_button" class="btn btn-danger btn-lg" onclick="disableExtroot();"><%~ ExtrOff %></button></span>
					<span class="col-xs-12 alert alert-warning"><em><%~ ExtDt %> <strong><span id="extroot_drive"></span></strong>.</em></span>
				</div>
			</div>
		</div>
	</div>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none"></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "usb_storage"
%>
