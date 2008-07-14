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
			<label class='leftcolumn'>Select Configuration File to Restore From:</label>
			<input class='rightcolumn' type="file" id="restore_file" name="restore_file" />
		</form>
	</div>
	<div>
		<span class='leftcolumn'><input id="restore_button" type='button' class="default_button" value="Restore Configuration Now" onclick="doRestore()"/></span>
	</div>

	<iframe id="do_restore" name="do_restore" src="#" style="width:0;height:0;border:0px solid #fff;"></iframe> 

</fieldset>


<fieldset  id="restore_in_progress" style="display:none">
	<legend class="sectionheader">Restore In Progress</legend>
	<div>
		</p>Configuration file uploaded successfully.  Please wait while your new settings are applied.</p>
	</div>
</fieldset>



<?
	gargoyle_header_footer -f -s "system" -p "backup" 
?>
