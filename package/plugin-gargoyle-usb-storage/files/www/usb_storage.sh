#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2010 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "usb_storage" -c "internal.css" -j "table.js usb_storage.js" gargoyle network nfsd samba

?>

<script>
<!--
<?
	echo "var driveSizes = [];"
	
	echo "var storageDrives = [];"
	cat /tmp/mounted_usb_storage.tab 2>/dev/null | awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\"]);" }' 

	echo "var physicalDrives = [];"

?>
//-->
</script>



<fieldset id="shared_disks">
	<legend class="sectionheader">Shared Disks</legend>
	
	<div id="cifs_workgroup_container">
		<label id="cifs_workgroup_label" class="leftcolumn" for="cifs_workgroup">CIFS Workgroup:</label>
		<input id="cifs_workgroup" class="rightcolumn" type="text" size='30'/>
	</div>

	
	<div id="cifs_policy_container">
		<label id="cifs_policy_label" class="leftcolumn">CIFS Access Policy:</label>
		<select class="rightcolumn" id="cifs_policy" onchange="setPolicyVisibility()">
			<option value="share">Allow Anonymous Access</option>
			<option value="user">Require User &amp; Password</option>
		</select>
	</div>
	<div class="rightcolumnonly" id="cifs_user_container">
		<label id="cifs_user_label" for="cifs_user" style="float:left;width:100px;">User:</label>
		<input id="cifs_user" type="text" />
	</div>
	<div class="rightcolumnonly" style="margin-bottom:20px;" id="cifs_pass_container">
		<label id="cifs_pass_label" for="cifs_pass" style="float:left;width:100px;">Password:</label>
		<input id="cifs_pass" type="text" />
	</div>
	<div id="nfs_policy_container">
		<label id="nfs_policy_label" class="leftcolumn">NFS Access Policy:</label>
		<select class="rightcolumn" id="nfs_policy" onchange="setPolicyVisibility()" >
			<option value="share">Allow Anonymous Access</option>
			<option value="ip">Allow Only The Following IPs</option>
		</select>
	</div>

	<div id="nfs_ip_container">
		<div class="rightcolumnonly" id="nfs_ip_table_container"></div>
		<div class="rightcolumnonly">
			<input type='text' id='nfs_ip'  size='30' onkeyup='proofreadIp(this)'  />
			<input type="button" class="default_button" id="add_nfs_ip" value="Add" onclick='addAddressesToTable(document,"nfs_ip","nfs_ip_table_container","nfs_ip_table",false, 2, true, 250)' />
		</div>
		<div class="rightcolumnonly">
			<em>Specify an IP or Subnet</em>
		</div>
	</div>

	<div id="sharing_add_heading_container">
		<span class="nocolumn" style="text-decoration:underline;">Add Shared Disk:</span>
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
</fieldset>

<fieldset id="no_disks" style="display:none;">
	<legend class="sectionheader">Shared Disks</legend>
	No USB disks connected.
</fieldset>

<div id="bottom_button_container">
	<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
</div>



<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "system" -p "usb_storage"
?>


