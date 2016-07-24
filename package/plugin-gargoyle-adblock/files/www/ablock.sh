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
	cat /usr/lib/adblock/black.list | awk '{print "blacklistlines.push(\""$0"\");"}'
	echo "var whitelistlines = new Array();"
	cat /usr/lib/adblock/white.list | awk '{print "whitelistlines.push(\""$0"\");"}'
%>
</script>

<h1 class="page-header"><%~ ablock.Adblock %></h1>
<div class="row">
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-body">
				<div class="form-group">
					<label id="adblock_enable_label" for="adblock_enable"><%~ ADBLOCKEn %></label>
					<input id="adblock_enable" type="checkbox" />
				</div>

				<div class="form-group">
					<button id="adblock_update" class="btn btn-default" onclick='adblockUpdate()'><%~ ADBLOCKupdate %></button>
					<label id="adblock_lastrun"><%~ ADBLOCKLstrn %>: </label>
					<label id="adblock_lastrunval"></label>
				</div>

				<div class="form-group">
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

				<div class="form-group">
					<label id="adblock_exempten_label" class="leftcolumn" for="adblock_exempten"><%~ ADBLOCKExemptEn %>:</label>
					<input id="adblock_exempten" class="rightcolumn" type="checkbox" />
				</div>
				<div class="form-group">
					<div class="form-group">
						<label class="leftcolumn" id="adblock_exempt_labels" for="adblock_exempts"><%~ ADBLOCKExempts %>:</label>
						<input id="adblock_exempts" class="form-control" type="text" size='15' />
					</div>
					<div class="form-group">
						<label class="leftcolumn" id="adblock_exempt_labelf" for="adblock_exemptf"><%~ ADBLOCKExemptf %>:</label>
						<input id="adblock_exemptf" class="form-control" type="text" size='15' />
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row" id="list_gui">
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKBlocklist %></h3>
			</div>
			<div class="panel-body">
				<div>
					<label id="adblock_displayed_count" class="leftcolumn">-</label>
				</div>
				<div class="form-group">
					<select id="adblock_blocklist_list" multiple class="form-control"></select>
					<input type="button" value="-->" id="adblock_transfer_button" class="btn btn-warning" onclick="transferwhiteList();" />
				</div>
				<div class="form-group">
					<input id="adblock_blocklist_search" class="form-control" type="text" />
					<button class="btn btn-primary" onclick="searchBlocklist();"><%~ ADBLOCKSearch %></button>
				</div>
				<div id="adblock_help2" class="indent">
					<span id="adblock_help2_txt">
						<p><%~ ADBLOCKHelp2 %></p>
					</span>
					<a id="adblock_help2_ref" onclick='setDescriptionVisibility("adblock_help2")' href="#adblock_help2"><%~ Hide %></a>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKWhitelist %></h3>
			</div>
			<div class="panel-body">
				<div class="form-group">
					<select id="adblock_whitelist_list" multiple class="form-control"></select>
					<button id="adblock_whitelist_delete_button" class="btn btn-danger" onclick="deleteList(adblock_whitelist_list);"><%~ ADBLOCKBlackdel %></button>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKBlacklist %></h3>
			</div>
			<div class="panel-body">
				<div class="form-group">
					<select id="adblock_blacklist_list" multiple class="form-control"></select>
					<button id="adblock_blacklist_delete_button" class="btn btn-danger" onclick="deleteList(adblock_blacklist_list);"><%~ ADBLOCKBlackdel %></button>
				</div>
				<div>
					<input id="adblock_blacklist_add" class="form-control" type="text" />
				</div>
				<div>
					<button class="btn btn-primary" onclick="addBlacklist();"><%~ ADBLOCKBlackadd %></button>
				</div>
			</div>
		</div>
	</div>
</div>
	
	

<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick='saveChanges()'><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick='resetData()'><%~ Reset %></button>
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
