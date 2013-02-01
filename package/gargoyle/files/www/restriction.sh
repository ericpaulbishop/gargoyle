#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "restriction" -c "internal.css" -j "table.js restrictions.js" gargoyle firewall

?>

<script>
<!--
	var uci = uciOriginal.clone();
//-->
</script>



<form>
	<fieldset>
		<legend class="sectionheader">Access Restrictions</legend>

		<span id="add_rule_label" style="text-decoration:underline" >New Restriction Rule:</span>

		<div>
			<?
		       sed -e '/<L7OPTIONS>/,$ d' templates/restriction_template
		       sed -e '/^#/ d'  -e "s/\([^ ]* \)\(.*\)/<option value='\1'>\2<\/option>/" /etc/l7-protocols/l7index
		       sed -e '1,/<L7OPTIONS>/ d' templates/restriction_template
                     ?>
			<div>
				<input type="button" id="add_restriction_button" class="default_button" value="Add New Rule" onclick='addNewRule("restriction_rule", "rule_")' />
			</div>
		</div>


		<div id='internal_divider1' class='internal_divider'></div>



		<span id="current_rule_label" style="text-decoration:underline" >Current Restrictions:</span>

		<div id="rule_table_container"></div>

	</fieldset>

		<fieldset>
		<legend class="sectionheader">Exceptions (White List)</legend>

		<span id="add_exception_label" style="text-decoration:underline" >New Exception:</span>

		<div>
			<?
		       sed -e '/<L7OPTIONS>/,$ d' templates/whitelist_template
		       sed -e '/^#/ d'  -e "s/\([^ ]* \)\(.*\)/<option value='\1\2'>\2<\/option>/" /etc/l7-protocols/l7index
		       sed -e '1,/<L7OPTIONS>/ d' templates/whitelist_template
		       ?>

			<div>
				<input type="button" id="add_restriction_button" class="default_button" value="Add New Rule" onclick='addNewRule("whitelist_rule", "exception_")' />
			</div>
		</div>


		<div id='internal_divider1' class='internal_divider'></div>



		<span id="current_exceptions_label" style="text-decoration:underline" >Current Exceptions:</span>

		<div id="exception_table_container"></div>

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
