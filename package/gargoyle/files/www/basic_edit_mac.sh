#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m -c "internal.css" -j "basic.js table.js"
?>

<fieldset id="edit_container">
	<legend class="sectionheader">Configure Wireless MAC Filter</legend>
	<div>
		<p>Be aware that mac filtering applies to all wireless interfaces, 
		including those in client mode.  If you are using a MAC filter
		and are in client mode either set the policy to Deny only 
		listed MACs, or include include the MAC of the AP you are
		connecting to in the MAC list below.</p>
	</div>
	<div class="internal_divider"></div>
	<div>
		<label class='leftcolumn' for='filter_policy'>MAC Filter Policy:</label>
		<select class='rightcolumn' id='filter_policy'>
			<option value='allow'>Allow Only MACs Listed Below</option>
			<option value='deny' >Deny Only MACs Listed Below</option>
		</select>
	</div>
	<div>
		<span class='leftcolumn'><input type="button" class="default_button" id="add_mac_button" value="Add" onclick="addMacToFilter(document)" /></span>
		<input type='text' id='add_mac' class='rightcolumn' onkeyup='proofreadMac(this)' size='20' maxlength='17' />
	</div>

	<div id="mac_table_container"></div>
</fieldset>

<div id="bottom_button_container"></div>

</body>
</html>
