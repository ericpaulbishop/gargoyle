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
		<legend class="sectionheader">Rebooting</legend>
		<p><em>System is now rebooting . . .</em></p>	
	</fieldset>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<script>
<!--
	reboot();
//-->
</script>


<?
	gargoyle_header_footer -f -s "system" -p "reboot"
?>
