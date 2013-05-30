#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "usb_storage" -c "internal.css" -j "table.js usb_storage.js" gargoyle network firewall nfsd samba vsftpd share_users

?>

<script>
<!--
<?
	echo "var driveSizes = [];"

	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

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
			echo "drivesWithNoMounts.push( [ \"$d\", \"$size\" ] );"
		fi
	done
	echo "var extroot_enabled=\""$(mount | grep "/dev/sd.*on /overlay" | wc -l)"\";"
	echo "var extroot_drive=\""$(mount | awk '/\/dev\/sd.*on \/overlay/ {print $1}')"\";"
?>
//-->
</script>

<fieldset id="no_disks" style="display:none;">
	<legend class="sectionheader">Shared Disks</legend>
	<em><span class="nocolumn">No mounted USB disks detected</span></em>
</fieldset>

<fieldset id="shared_disks">
	<legend class="sectionheader">Shared Disks</legend>

	<div id='ftp_wan_access_container' >
		<span class="nocolumn">
			<input class="aligned_check" type='checkbox' id='ftp_wan_access' onclick='updateWanFtpVisibility()' />&nbsp;
			<label class="aligned_check_label" id='ftp_wan_access_label' for='ftp_wan_access'>Allow Access to FTP From WAN</label>
		</span>
	</div>

	<div id='ftp_pasv_container' class="indent" >
		<span class="nocolumn">
			<input class="aligned_check" type='checkbox' id='ftp_wan_pasv' onclick='updateWanFtpVisibility()' />&nbsp;
			<label class="aligned_check_label" id='ftp_wan_access_label' for='ftp_wan_pasv'>Allow Passive FTP From WAN on ports</label>&nbsp;
			<input type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)' id='pasv_min_port'>&nbsp;-&nbsp;<input type="text" size='7' maxLength='5' onkeyup='proofreadPort(this)'  id='pasv_max_port'>
		</span>
	</div>

	<div id="ftp_wan_spacer" style="height:15px;"></div>

	<div id="cifs_workgroup_container" style="margin-bottom:20px;" >
		<label id="cifs_workgroup_label" class="leftcolumn" for="cifs_workgroup">CIFS Workgroup:</label>
		<input id="cifs_workgroup" class="rightcolumn" type="text" size='30'/>
	</div>

	<div id="user_container">
		<label id="cifs_user_label" class="leftcolumn">CIFS / FTP Users:</label>
		<span class="rightcolumnonly" id="user_container">
			<label id="user_label" for="new_user" style="float:left;width:120px;">New User:</label>
			<input id="new_user" type="text" />
		</span>
	</div>
	<div class="rightcolumnonly" id="user_pass_container">
		<label id="user_pass_label" for="user_pass" style="float:left;width:120px;">Password:</label>
		<input id="user_pass" type="password" />
	</div>
	<div class="rightcolumnonly" id="user_pass_confirm_container">
		<label id="user_pass_confirm_label" for="user_pass_confirm" style="float:left;width:120px;">Confirm Password:</label>
		<input id="user_pass_confirm" type="password" />
	</div>
	<div class="rightcolumnonly" id="user_pass_container">
		<input id="add_user" type="button"  class="default_button" value="Add User" onclick="addUser()" style="margin-left:0px;" />
	</div>

	<div class="rightcolumnonly" style="margin-bottom:20px;" id="user_table_container">
	</div>

	<div id="sharing_add_heading_container">
		<span class="nocolumn" style="text-decoration:underline;">Add Shared Disk / Directory:</span>
	</div>
	<div id="sharing_add_controls_container" class="indent">
		<? cat templates/usb_storage_template ?>
		<div>
			<input type="button" id="add_share_button" class="default_button" value="Add Shared Disk" onclick="addNewShare()" />
		</div>
	</div>
	<div class="internal_divider"></div>
	<div id="sharing_current_heading_container">
		<span class="nocolumn" style="text-decoration:underline;">Currently Shared Disks:</span>
	</div>
	<div id="sharing_mount_table_container">
	</div>

	<div class="internal_divider"></div>

	<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
</fieldset>

<fieldset id="disk_unmount">
	<legend class="sectionheader">Unmount</legend>
	<div>
		<span class="leftcolumn"  style="margin-bottom:60px;margin-left:0px;"><input type='button' value="Unmount All USB Disks" id="unmount_usb_button" class="default_button" onclick="unmountAllUsb()"></span>
		<span class="rightcolumn"><em>USB Disks should be unmounted before removal from the router. USB Disks still connected will be automatically remounted after next router reboot.</em></span>
	</div>
</fieldset>

<fieldset id="disk_format">
	<legend class="sectionheader">Format Disk</legend>

	<div id="no_unmounted_drives">
		<em><span class="nocolumn"><p>No attached, unmounted drives detected.</p><p>You must unmount drives before attempting to format them.</p></span></em>
	</div>

	<div id="format_warning">
		<em><span class="nocolumn">WARNING: Formatting a disk will permanently wipe out all contents of that disk.<p>Disk will be formatted for storage with EXT4 filesystem<br/>EXT4 may not be readable if USB drive is removed and attached to a Windows/Mac computer</p></span></em>
	</div>

	<div id="format_disk_select_container">
		<label id="format_disk_select_label" class="leftcolumn" for="format_disk_select">Disk to format:</label>
		<select class="rightcolumn" id="format_disk_select" ></select>
		<br/>
		<span id="format_warning" class="right_column_only"></span>
	</div>
	<div id="swap_percent_container">
		<label class="leftcolumn" id="swap_percent_label" for="swap_percent" >Percent Swap:</label>
		<span  class="rightcolumn"><input id="swap_percent" type="text" onkeyup="updateFormatPercentages(this.id)" /></span>%&nbsp;&nbsp;<em><span id="swap_size"></span></em>
	</div>
	<div id="storage_percent_container">
		<label class="leftcolumn" id="storage_percent_label" for="storage_percent" >Percent Storage:</label>
		<span  class="rightcolumn"><input id="storage_percent" type="text" onkeyup="updateFormatPercentages(this.id)" /></span>%&nbsp;&nbsp;<em><span id="storage_size"></span></em>
	</div>
	<div id="extroot_container">
		<span class="rightcolumnonly">
			<input type="checkbox" id="extroot" name="extroot" style="padding:0;margin:0px;vertical-align:middle;overflow:hidden;" />
			<label id="extroot_label" for="extroot" style="vertical-align:middle">Make extroot on this disk</label>
		</span>
	</div>
	<div id="usb_format_button_container">
		<span class="leftcolumn" style="margin-left:0px;" ><input type="button" value="Format Now" id="usb_format_button" class="default_button" onclick="formatDiskRequested()" /></span>
	</div>
</fieldset>

<fieldset id="extroot_fieldset" style="display:none;">
	<legend class="sectionheader">Extroot</legend>
	<span class="leftcolumn"  style="margin-left:0px;"><input type="button" value="Disable extroot" id="extoot_button" class="default_button" onclick="disableExtroot();" /></span>
	<span class="rightcolumn"><em>Extroot detected on <b><span id="extroot_drive"></span></b>.</em></span>
</fieldset>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "system" -p "usb_storage"
?>
