#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	gargoyle_header_footer -h -s "firewall" -p "quotas" -c "internal.css" -j "table.js quotas.js" gargoyle restricter_gargoyle

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

	q=$(uci show restricter_gargoyle | grep "=quota$" | sed ' s/^.*\.//g ' | sed 's/=.*$//g')
	quotas=$(echo $q)
	echo "var quotaNames = \"$quotas\";"
	if [ -n "$restricter_enabled" ] ; then
		quota_data=$(restricter_gargoyle -m $quotas)
		echo "var quotaData = \"$quota_data\";"
	else
		echo "var quotaData = \"\";"
	fi

	echo "var uci = uciOriginal.clone();"
?>
//-->
</script>




<form>
	<fieldset>
		<legend class="sectionheader">Bandwidth Quotas</legend>
	
		<span id="add_quota_label" style="text-decoration:underline" >Add New Quota:</span>	
		
		<div>

			<? cat templates/quotas_template ?>

			<div>
				<input type="button" id="add_quota_button" class="default_button" value="Add New Quota" onclick="addNewQuota()" />
			</div>	
		</div>
	
	
		<div id='internal_divider1' class='internal_divider'></div>
	


		<span id="active_quotas_label" style="text-decoration:underline" >Active Quotas:</span>	

		<div id="quota_table_container"></div>
		
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
	gargoyle_header_footer -f -s "firewall" -p "quotas"
?>
