#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	gargoyle_header_footer -h -s "firewall" -p "restriction" -c "internal.css" -j "table.js restrictions.js" gargoyle restricter_gargoyle

?>

<script>
<!--
<?
	restricter_enabled=$(ls /etc/rc.d/*restricter_gargoyle* 2>/dev/null)
	if [ -n "$restricter_enabled" ] ; then
		echo "var restricterEnabled = true;"
	else
		echo "var restricterEnabled = false;"
	fi
	echo "var uci = uciOriginal.clone();"
?>
//-->
</script>



<form>
	<fieldset>
		<legend class="sectionheader">Access Restrictions</legend>
	
		<span id="add_restriction_label"><p style="text-decoration:underline">New Restriction Rule:</p></span>	
		
		<div>
			<? cat templates/restriction_template ?>
			<div>
				<input type="button" id="add_restriction_button" class="default_button" value="Add New Rule" onclick="addNewRule()" />
			</div>	
		</div>
	
	
		<div id='internal_divider1' class='internal_divider'></div>
	


		<span id="current_restrictions_label"><p>Current Restrictions:</p></span>	

		<div id="restriction_table_container"></div>
		
	</fieldset>
	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>


	<span id="update_container" >Please wait while new settings are applied. . .</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "firewall" -p "restriction"
?>
