#!/usr/bin/haserl --upload-limit=4096 --upload-target=/tmp/ --upload-dir=/tmp/
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "system" -p "backup" -c "internal.css" -j "backup.js" network 
?>

<fieldset  id="backup_section">
	<legend class="sectionheader">Backup Current Configuration</legend>
	<div>
		<span class='leftcolumn'><input id="backup_button" type='button' class="default_button" value="Get Backup Now" onclick="getBackup()"/></span>
	</div>
</fieldset>

<fieldset id="restore_section">
	<legend class="sectionheader">Restore Old Configuration</legend>
	<div>
		<form id='restore_form' enctype="multipart/form-data" method="post" action="utility/do_restore.sh" target="do_restore">
			<label class='leftcolumn'>Select Old Configuration File:</label>
			<input class='rightcolumn' type="file" id="restore_file" name="restore_file" />
		</form>
	</div>
	<div>
		<span class='leftcolumn'><input id="restore_button" type='button' class="default_button" value="Restore Configuration Now" onclick="doRestore()"/></span>
	</div>
	<iframe id="do_restore" name="do_restore" src="#" style="width:0;height:0;border:0px solid #fff;"></iframe> 


</fieldset>

<fieldset id="restore_original_section">
	<legend class="sectionheader">Restore Default Configuration</legend>
	<form id='restore_original_form' enctype="multipart/form-data" method="post" action="utility/do_restore_original.sh" target="do_restore_original">
	</form>
	<div>
		<span class='leftcolumn'><input id="restore_original_button" type='button' class="default_button" value="Restore Default Configuration Now" onclick="doDefaultRestore()"/></span>
	</div>
	<iframe id="do_restore_original" name="do_restore_original" src="#" style="width:0;height:0;border:0px solid #fff;"></iframe> 
</fieldset>
<iframe id="reboot_test" style="display:none" ></iframe>






<?
	gargoyle_header_footer -f -s "system" -p "backup" 
?>
