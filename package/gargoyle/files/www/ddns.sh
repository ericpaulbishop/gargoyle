#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "dyndns" -c "internal.css" -j "table.js ddns.js" gargoyle ddns_gargoyle

?>

<script>
<!--
<?
	echo "providerData = new Array();"
	awk '{gsub(/"/,"'\''"); print "providerData.push(\""$0"\");" ;}' /etc/ddns_providers.conf

	updates=$(ls /var/last_ddns_updates 2>/dev/null )
	echo "updateTimes = new Array();"
	for update in $updates ; do
		echo "updateTimes[\"$update\"] = " $(cat /var/last_ddns_updates/$update) ";"
	done

?>
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader">Dynamic DNS Services</legend>

		<span id="add_ddns_label"><p>Add Dynamic DNS Service:</p></span>

		<div>
			<div class="indent">
				<label class='leftcolumn' for='ddns_provider' id='ddns_provider_label'>Service Provider:</label>
				<select class='rightcolumn' id="ddns_provider" onchange="setProvider()"></select>
			</div>
		</div>

		<div class="indent" id="ddns_variable_container">

		</div>

		<div>
			<div class='indent'>
				<label class='leftcolumn' for='ddns_check' id='ddns_check_label'>Check Interval:</label>
				<input type='text' class='rightcolumn' id='ddns_check'  size='8' onkeyup='proofreadNumeric(this)'/>
				<em>minutes</em>
			</div>
			<div class='indent'>
				<label class='leftcolumn' for='ddns_force' id='ddns_force_label'>Force Update Interval:</label>
				<input type='text' class='rightcolumn' id='ddns_force'  size='8' onkeyup='proofreadNumeric(this)'/>
				<em>days</em>
			</div>

			<div class='indent'>
				<input type="button" id="add_service_button" class="default_button" value="Add DDNS Service" onclick="addDdnsService()" />
			</div>

			<div class='indent'>
				<span id='ddns_1_txt'>
					<p>
					<em>Check Interval</em> specifies how often the router will check whether your current IP matches the one currently associated with your
					domain name.  This check is performed without connecting to your dynamic DNS service provider, which means that this will not cause 
					problems with providers that ban users who connect too frequently (e.g. dyndns.com).  However, a network connection is established 
					to perform this check, so this value should not be too low.  A check interval between 10 and 20 minutes is usually appropriate.
					</p>

					<p>
					<em>Force Update Interval</em> specifies how often the router will connect to your dynamic DNS service provider and update their records,
					even if your IP has not changed.  Service providers will ban users who update too frequently, but may close accounts of users who do
					not update for over a month.  It is recommended that this parameter be set between 3 and 7 days.
					</p>
				</span>
				<a onclick='setDescriptionVisibility("ddns_1")'  id="ddns_1_ref" href="#ddns_1">Hide Text</a>
			</div>
		</div>

		<div id='internal_divider1' class='internal_divider'></div>

		<span id="add_ddns_label"><p>Dynamic DNS Services:</p></span>

		<div id="ddns_table_container"></div>

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
	gargoyle_header_footer -f -s "connection" -p "dyndns"
?>
