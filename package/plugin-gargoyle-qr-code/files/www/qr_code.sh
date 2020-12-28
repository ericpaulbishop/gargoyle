#!/usr/bin/haserl
<%
	# This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )
	gargoyle_header_footer -h -s "system" -p "qr_code" -c "qr_code.css" -j "drawdown.js qrcodegen.js optgroup.js qr_code_common.js qr_code.js" -z "qr_code.js" -i gargoyle system dhcp uhttpd firewall network wireless mjpg-streamer qr_code_gargoyle
%>

<%in templates/client_server_template %>
<h1 class="page-header"><%~ qr_code.QrCode %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 id="title" class="panel-title"><%~ QrCodeEditor %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-3" for="application"><%~ Application %>:</label>
					<span class="col-xs-9">
						<select class="form-control" id="application" disabled="disabled">
							<option value="" hidden="hidden"><%~ SelectApplication %></option>
						</select>
					</span>
				</div>
				<div id="print_container" class="row form-group" style="display: none">
					<span class="col-xs-3"><%~ Print %>:</span>
					<span class="col-xs-9">
						<div id="print_subject_container" class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="print_subject"/>
								<label id="print_subject_label" for="print_subject"></label>
							</span>
						</div>
						<div id="print_secrets_container" class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="print_secrets"/>
								<label id="print_secrets_label" for="print_secrets"></label>
							</span>
						</div>
						<div id="print_comment_container" class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="print_comment"/>
								<label id="print_comment_label" for="print_comment"><%~ Comment %></label>
							</span>
						</div>
					</span>
				</div>
				<div id="comment_container" class="row form-group" style="display: none">
					<span class="col-xs-3"><%~ Comment %>:</span>
					<span class="col-xs-9">
						<textarea class="form-control markdown" id="comment" placeholder="<%~ Markdown %>"></textarea>
					</span>
				</div>
				<div id="embed_container" class="row form-group" style="display: none">
					<span class="col-xs-3"><%~ EmbedIn %>:</span>
					<span class="col-xs-9">
						<div class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="embed_via_lan"/>
								<label id="embed_via_lan_label" for="embed_via_lan"><%~ LocalLoginPage %></label>
							</span>
						</div>
						<div class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="embed_via_wan"/>
								<label id="embed_via_wan_label" for="embed_via_wan"><%~ RemoteLoginPage %></label>
							</span>
						</div>
					</span>
				</div>
				<div id="qr_code_help" class="row form-group" style="display: none">
					<span class="col-xs-12"><a id="qr_code_help_ref" href="javascript:setDescriptionVisibility('qr_code_help')"><%~ MoreInfo %></a></span>
					<span class="col-xs-12" id="qr_code_help_txt">
						<p id="wifi_help"><%~ WifiHelp %></p>
						<p id="web_access_help"><%~ WebAccessHelp %></p>
						<p id="webcam_help"><%~ WebcamHelp %></p>
					</span>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-6">

<%in templates/qr_code_viewer_template %>
	</div>
</div>
<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg"><%~ Reset %></button>
</div>

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "system" -p "qr_code"
%>
