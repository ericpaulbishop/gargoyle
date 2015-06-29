#!/usr/bin/haserl
<%
	# This program is copyright © 2015 Michael Gray based on the work of teffalump and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "adblock" -c "internal.css" -j "adblock.js" -z "adblock.js" gargoyle adblock
%>


<fieldset id="adblock">
	<legend class="sectionheader"><%~ adblock.Adblock %></legend>

	<div class="nocolumn">
		<input id="adblock_enable" type="checkbox" />
		<label id="adblock_enable_label" for="adblock_enable"><%~ ADBLOCKEn %></label>
	</div>

	<div class="rightcolumnonly">
		<input type ="button" value="<%~ ADBLOCKupdate %>" id="adblock_update" class="default_button" onclick='adblockUpdate()' />
	</div>

	<div>
		<label id="adblock_wireless_label" class="leftcolumn" for="adblock_wireless"><%~ ADBLOCKWireless %>:</label>
		<input id="adblock_wireless" class="rightcolumn" type="checkbox" />
	</div>

	<div>
		<label id="adblock_transparent_label" class="leftcolumn" for="adblock_transparent"><%~ ADBLOCKTrans %>:</label>
		<input id="adblock_transparent" class="rightcolumn" type="checkbox" />
	</div>

	<div id="adblock_help" class="indent">
	<span id="adblock_help_txt">

	<p><%~ ADBLOCKHelp %></p>

	</span>
	<a id="adblock_help_ref" onclick='setDescriptionVisibility("adblock_help")' href="#adblock_help"><%~ Hide %></a>

	</div>

	<div class="internal_divider"></div>

	<div>
		<label id="adblock_exempten_label" class="leftcolumn" for="adblock_exempten"><%~ ADBLOCKExemptEn %>:</label>
		<input id="adblock_exempten" class="rightcolumn" type="checkbox" />
	</div>
	<div>
		<label class="leftcolumn" id="adblock_exempt_labels" for="adblock_exempts"><%~ ADBLOCKExempts %>:</label>
		<input id="adblock_exempts" class="rightcolumn" type="text" size='15' />
	</div>
	<div>
		<label class="leftcolumn" id="adblock_exempt_labelf" for="adblock_exemptf"><%~ ADBLOCKExemptf %>:</label>
		<input id="adblock_exemptf" class="rightcolumn" type="text" size='15' />
	</div>

</fieldset>

<div id="bottom_button_container">
	<input type="button" value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type="button" value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()' />
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "adblock"
%>
