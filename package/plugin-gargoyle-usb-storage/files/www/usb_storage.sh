#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "usb_storage" -c "internal.css" -j "table.js usb_storage.js" -z "usb_storage.js" gargoyle network firewall nfsd samba vsftpd share_users

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

<fieldset id="no_disks" style="display:none;">
	<legend class="sectionheader"><%~ usb_storage.SDisk %></legend>
	<em><span class="nocolumn"><%~ Nomdsk %></span></em>
</fieldset>

<fieldset id="shared_disks">
	<legend class="sectionheader"><%~ SDisk %></legend>

	<div id='ftp_wan_access_container' >
		<span class="nocolumn">
			<input class="aligned_check" type='checkbox' id='ftp_wan_access' onclick='updateWanFtpVisibility()' />&nbsp;
			<label class="aligned_check_label" id='ftp_wan_access_label' for='ftp_wan_access'><%~ WFTP %></label>
		</span>
	</div>

	<div id='ftp_pasv_container' class="indent" >
		<span class="nocolumn">
			<input class="aligned_check" type='checkbox' id='ftp_wan_pasv' onclick='updateWanFtpVisibility()' />&nbsp;
			<label class="aligned_check_label" id='ftp_wan_access_label' for='ftp_wan_pasv'><%~ WpFTP %></label>&nbsp;
			<input type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)' id='pasv_min_port'>&nbsp;-&nbsp;<input type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)'  id='pasv_max_port'>
		</span>
	</div>

	<div id="ftp_wan_spacer" style="height:15px;"></div>

	<div id="cifs_workgroup_container" style="margin-bottom:20px;" >
		<label id="cifs_workgroup_label" class="leftcolumn" for="cifs_workgroup"><%~ CFWkg %>:</label>
		<input id="cifs_workgroup" class="rightcolumn" type="text" size='30'/>
	</div>

	<div id="user_container">
		<label id="cifs_user_label" class="leftcolumn"><%~ CFUsr %>:</label>
		<span class="rightcolumnonly" id="user_container">
			<label id="user_label" for="new_user" style="float:left;width:120px;"><%~ NewU %>:</label>
			<input id="new_user" type="text" />
		</span>
	</div>
	<div class="rightcolumnonly" id="user_pass_container">
		<label id="user_pass_label" for="user_pass" style="float:left;width:120px;"><%~ Pasw %>:</label>
		<input id="user_pass" type="password" />
	</div>
	<div class="rightcolumnonly" id="user_pass_confirm_container">
		<label id="user_pass_confirm_label" for="user_pass_confirm" style="float:left;width:120px;"><%~ CfPass %>:</label>
		<input id="user_pass_confirm" type="password" />
	</div>
	<div class="rightcolumnonly" id="user_pass_container">
		<input id="add_user" type="button"  class="default_button" value="<%~ AddU %>" onclick="addUser()" style="margin-left:0px;" />
	</div>

	<div class="rightcolumnonly" style="margin-bottom:20px;" id="user_table_container">
	</div>

	<div id="sharing_add_heading_container">
		<span class="nocolumn" style="text-decoration:underline;"><%~ ADir %>:</span>
	</div>
	<div id="sharing_add_controls_container" class="indent">
		<%in templates/usb_storage_template %>
		<div>
			<input type="button" id="add_share_button" class="default_button" value="<%~ ADsk %>" onclick="addNewShare()" />
		</div>
	</div>
	<div class="internal_divider"></div>
	<div id="sharing_current_heading_container">
		<span class="nocolumn" style="text-decoration:underline;"><%~ CShare %>:</span>
	</div>
	<div id="sharing_mount_table_container">
	</div>

	<div class="internal_divider"></div>

	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
</fieldset>

<fieldset id="disk_unmount">
	<legend class="sectionheader"><%~ Umnt %></legend>
	<div>
		<span class="leftcolumn"  style="margin-bottom:60px;margin-left:0px;"><input type='button' value="<%~ UmntB %>" id="unmount_usb_button" class="default_button" onclick="unmountAllUsb()"></span>
		<span class="rightcolumn"><em><%~ UmntWarn %></em></span>
	</div>
</fieldset>

<fieldset id="disk_format">
	<legend class="sectionheader"><%~ FDisk %></legend>

	<div id="no_unmounted_drives">
		<em><span class="nocolumn"><%~ NoUmntDev %></span></em>
	</div>

	<div id="format_warning">
		<em><span class="nocolumn"><%~ FmtWarn %></span></em>
	</div>

	<div id="format_disk_select_container">
		<label id="format_disk_select_label" class="leftcolumn" for="format_disk_select"><%~ DskForm %>:</label>
		<select class="rightcolumn" id="format_disk_select" ></select>
		<br/>
		<span id="format_warning" class="right_column_only"></span>
	</div>
	<div id="swap_percent_container">
		<label class="leftcolumn" id="swap_percent_label" for="swap_percent" ><%~ PSwap %>:</label>
		<span  class="rightcolumn"><input id="swap_percent" type="text" onkeyup="updateFormatPercentages(this.id)" /></span>%&nbsp;&nbsp;<em><span id="swap_size"></span></em>
	</div>
	<div id="storage_percent_container">
		<label class="leftcolumn" id="storage_percent_label" for="storage_percent" ><%~ PStor %>:</label>
		<span  class="rightcolumn"><input id="storage_percent" type="text" onkeyup="updateFormatPercentages(this.id)" /></span>%&nbsp;&nbsp;<em><span id="storage_size"></span></em>
	</div>
	<div id="extroot_container">
		<span class="rightcolumnonly">
			<input type="checkbox" id="extroot" name="extroot" style="padding:0;margin:0px;vertical-align:middle;overflow:hidden;" />
			<label id="extroot_label" for="extroot" style="vertical-align:middle"><%~ MExtr %></label>
		</span>
		<div class="rightcolumnonly">
			<em><%~ ExtrWarn %></em>
		</div>
	</div>
	<div id="usb_format_button_container">
		<span class="leftcolumn" style="margin-left:0px;" ><input type="button" value="<%~ FmtNow %>" id="usb_format_button" class="default_button" onclick="formatDiskRequested()" /></span>
	</div>
</fieldset>

<fieldset id="extroot_fieldset" style="display:none;">
	<legend class="sectionheader"><%~ ExtrS %></legend>
	<span class="leftcolumn"  style="margin-left:0px;"><input type="button" value="<%~ ExtrOff %>" id="extroot_button" class="default_button" onclick="disableExtroot();" /></span>
	<span class="rightcolumn"><em><%~ ExtDt %> <b><span id="extroot_drive"></span></b>.</em></span>
</fieldset>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "usb_storage"
%>
