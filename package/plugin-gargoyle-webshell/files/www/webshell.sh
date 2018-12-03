#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "webshell" -j "webshell.js"
%>

<h1 class="page-header"><%~ webshell.Webs %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Webs %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" for='cmd' id='cmd_label'><%~ Cmd %>:</label>
					<span class="col-xs-7">
						<input id="cmd" class="form-control" onkeydown='checkKey(event)' type="text" size='80'/>
						<button class='btn btn-primary' id='cmd_button' onclick='runCmd()'><%~ Exe %></button>
					</span>
					<span class="col-xs-12"> 
						<div class=" alert alert-warning"><em><%~ CmdWarn %></em></div>
					</span>
				</div>
				<textarea style="width:100%" rows=30 id='output'></textarea>
			</div>
		</div>
	</div>
</div>

<%
	gargoyle_header_footer -f -s "system" -p "webshell"
%>
