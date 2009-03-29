#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "system" -p "reboot" -c "internal.css" -j "reboot.js"
?>

<form>
	<fieldset>
		<legend class="sectionheader">Reboot</legend>
		<center><input type='button' value='Reboot Now' id="reboot_button" class="bottom_button" onclick='reboot()' /></center>

	</fieldset>
</form>
<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<?
	gargoyle_header_footer -f -s "system" -p "reboot"
?>
