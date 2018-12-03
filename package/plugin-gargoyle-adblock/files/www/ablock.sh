#!/usr/bin/haserl
<%
	# This program is copyright © 2015 Michael Gray based on the work of teffalump and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "adblock" -j "ablock.js" -z "ablock.js" gargoyle adblock
%>

<script>
<%
	echo "var blocklistlines = new Array();"
	cat /plugin_root/adblock/block.hosts | awk '{print "blocklistlines.push(\""$2"\");"}'
	echo "var blacklistlines = new Array();"
	cat /plugin_root/usr/lib/adblock/black.list | awk '{print "blacklistlines.push(\""$0"\");"}'
	echo "var whitelistlines = new Array();"
	cat /plugin_root/usr/lib/adblock/white.list | awk '{print "whitelistlines.push(\""$0"\");"}'
%>
</script>

<h1 class="page-header"><%~ ablock.Adblock %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default"> 
			<div class="panel-heading"> 
				<h3 class="panel-title"><%~ Adblock %></h3> 
			</div> 
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="adblock_enable" type="checkbox" />
						<label id="adblock_enable_label" for="adblock_enable"><%~ ADBLOCKEn %></label>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-offset-5 col-xs-7"><button id="adblock_update" class="btn btn-default" onclick='adblockUpdate()'><%~ ADBLOCKupdate %></button></span>
					<span class="col-xs-offset-5 col-xs-7">
						<label id="adblock_lastrun"><%~ ADBLOCKLstrn %>: </label>
						<label id="adblock_lastrunval"></label>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input id="adblock_transparent" type="checkbox" />
						<label id="adblock_transparent_label" for="adblock_transparent"><%~ ADBLOCKTrans %></label>
					</span>
				</div>

				<div id="adblock_help" class="row">
					<span class="col-xs-12" id="adblock_help_txt">
						<p><%~ ADBLOCKHelp %></p>
					</span>
					<span class="col-xs-12"><a id="adblock_help_ref" onclick='setDescriptionVisibility("adblock_help")' href="#adblock_help"><%~ Hide %></a></span>
				</div>

				<div class="internal_divider"></div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input id="adblock_exempten" type="checkbox" />
						<label id="adblock_exempten_label" for="adblock_exempten"><%~ ADBLOCKExemptEn %>:</label>
					</span>
				</div>

				<div class="form-group">
					<div class="row form-group">
						<label class="col-xs-5" id="adblock_exempt_labels" for="adblock_exempts"><%~ ADBLOCKExempts %>:</label>
						<span class="col-xs-7"><input id="adblock_exempts" class="form-control" type="text" size='15' /></span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" id="adblock_exempt_labelf" for="adblock_exemptf"><%~ ADBLOCKExemptf %>:</label>
						<span class="col-xs-7"><input id="adblock_exemptf" class="form-control" type="text" size='15' /></span>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row" id="list_gui">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKBlocklist %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label id="adblock_displayed_count" class="col-xs-12">-</label>
				</div>
				<div class="row form-group">
					<span class="col-xs-10"><select id="adblock_blocklist_list" multiple class="form-control" style="min-height:300px;width:100%;"></select></span>
					<span class="col-xs-2"><input type="button" value="-->" id="adblock_transfer_button" class="btn btn-default" onclick="transferwhiteList();" /></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="adblock_blocklist_search" class="form-control" type="text" />
						<button class="btn btn-default" onclick="searchBlocklist();"><%~ ADBLOCKSearch %></button>
					</span>
				</div>
				<div id="adblock_help2" class="row">
					<span class="col-xs-12" id="adblock_help2_txt">
						<p><%~ ADBLOCKHelp2 %></p>
					</span>
					<span class="col-xs-12"><a id="adblock_help2_ref" onclick='setDescriptionVisibility("adblock_help2")' href="#adblock_help2"><%~ Hide %></a></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKWhitelist %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-10"><select id="adblock_whitelist_list" multiple class="form-control" style="min-height:100px;width:100%;"></select></span>
					<span class="col-xs-2"><button id="adblock_whitelist_delete_button" class="btn btn-danger" onclick="deleteList(adblock_whitelist_list);"><%~ ADBLOCKBlackdel %></button></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ADBLOCKBlacklist %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-10"><select id="adblock_blacklist_list" multiple class="form-control" style="min-height:100px;width:100%;"></select></span>
					<span class="col-xs-2"><button id="adblock_blacklist_delete_button" class="btn btn-danger" onclick="deleteList(adblock_blacklist_list);"><%~ ADBLOCKBlackdel %></button></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="adblock_blacklist_add" class="form-control" type="text" />
						<button class="btn btn-default" onclick="addBlacklist();"><%~ ADBLOCKBlackadd %></button>
					</span>
				</div>
			</div>
		</div>
	</div>
</div>
	
<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick='saveChanges()'><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick='resetData()'><%~ Reset %></button>
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
