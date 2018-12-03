#!/usr/bin/haserl
<%
	# This program is copyright © 2012-2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "logread" -j "logread.js" -z "logread.js"

%>

<h1 class="page-header"><%~ logread.SLogs %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SLogs %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><textarea class="form-control" rows=30 id='output' style="width:100%;"></textarea></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="refresh_button" class="btn btn-primary btn-lg" onclick='resetData()'><%~ Rfsh %></button>
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "logread"
%>
