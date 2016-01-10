#!/usr/bin/haserl
<%
	# This program is copyright © 2015 Michael Gray based on the work of teffalump and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "adblock" -c "internal.css" -j "ablock.js" -z "ablock.js" gargoyle adblock
%>

<script>
<%
	echo "var blocklistlines = new Array();"
	cat /plugin_root/adblock/block.hosts | awk '{print "blocklistlines.push(\""$2"\");"}'
	echo "var blacklistlines = new Array();"
	cat /plugin_root/adblock/black.list | awk '{print "blacklistlines.push(\""$0"\");"}'
	echo "var whitelistlines = new Array();"
	cat /plugin_root/adblock/white.list | awk '{print "whitelistlines.push(\""$0"\");"}'
%>
</script>

<fieldset id="adblock-fieldset">
	<legend class="sectionheader"><%~ ablock.Adblock %></legend>

	<div class="nocolumn">
		<input id="adblock_enable" type="checkbox" />
		<label id="adblock_enable_label" for="adblock_enable"><%~ ADBLOCKEn %></label>
	</div>

	<div class="rightcolumnonly">
		<input type ="button" value="<%~ ADBLOCKupdate %>" id="adblock_update" class="default_button" onclick='adblockUpdate()' />
	</div>

	<div class="rightcolumnonly">
		<label id="adblock_lastrun"><%~ ADBLOCKLstrn %>: </label>
		<label id="adblock_lastrunval"></label>
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
	
	<div id="list_gui">
	<div class="internal_divider"></div>
	<div>
		<label id="adblock_blocklist_label" class="leftcolumn" for="adblock_blocklist"><%~ ADBLOCKBlocklist %></label>
		<label id="adblock_whitelist_label" class="rightcolumn" style="padding-left: 30px;" for="adblock_whitelist"><%~ ADBLOCKWhitelist %></label>
	</div>
	<div>
		<label id="adblock_displayed_count" class="leftcolumn">-</label>
	</div>
	<div>
		<select id="adblock_blocklist_list" multiple class="leftcolumn" style="width: 200px; height: 150px;"></select>
		<input id="adblock_transfer_button" type="button" value="-->" onclick="transferwhiteList();" />
		<select id="adblock_whitelist_list" multiple style="width: 200px; height: 150px;"></select>
		<input id="adblock_whitelist_delete_button" class="rightcolumn" type="button" value='<%~ ADBLOCKBlackdel %>' onclick="deleteList(adblock_whitelist_list);" />
	</div>
	<div>
		<input id="adblock_blocklist_search" class="leftcolumn" type="text" />
	</div>
	<div>
		<input type="button" value='<%~ ADBLOCKSearch %>' class="leftcolumn" onclick="searchBlocklist();" />
	</div>
	<div>
		<label id="adblock_blacklist_label" class="leftcolumn" style="margin-top: 10px; margin-bottom: 5px;" for="adblock_blacklist"><%~ ADBLOCKBlacklist %></label>
	</div>
	<div>
		<select id="adblock_blacklist_list" multiple class="leftcolumn" style="width: 200px; height: 150px;"></select>
		<input id="adblock_blacklist_delete_button" type="button" class="rightcolumn" value='<%~ ADBLOCKBlackdel %>' onclick="deleteList(adblock_blacklist_list);" />
	</div>
	<div>
		<input id="adblock_blacklist_add" class="leftcolumn" type="text" />
	</div>
	<div>
		<input type="button" class="leftcolumn" value='<%~ ADBLOCKBlackadd %>' onclick="addBlacklist();" />
	</div>
	<div id="adblock_help2" class="indent">
	<span id="adblock_help2_txt">

	<p><%~ ADBLOCKHelp2 %></p>

	</span>
	<a id="adblock_help2_ref" onclick='setDescriptionVisibility("adblock_help2")' href="#adblock_help2"><%~ Hide %></a>

	</div>
	</div>
	
</fieldset>

<div id="bottom_button_container">
	<input type="button" value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type="button" value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()' />
</div>

<script>
<!--
	document.getElementById('adblock_transfer_button').value = String.fromCharCode(8594);
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "adblock"
%>
