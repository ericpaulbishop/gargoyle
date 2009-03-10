#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	gargoyle_header_footer -m  -c "internal.css" -j "port_forwarding.js table.js"
?>
<fieldset id="edit_container">
	<legend class="sectionheader">Edit Port Forward</legend>

	<? cat templates/single_forward_template ?>

</fieldset>
<div id="bottom_button_container"></div>


</body>
</html>
