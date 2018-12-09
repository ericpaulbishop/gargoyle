#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m -c "common.css" -j "ddns.js" -z "ddns.js"
%>


<div id="edit_container" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ ddns.EDSect %></h3>
			</div>
			<div class="panel-body">
				<div id="ddns_no_script" class="alert alert-danger" role="alert" style="display: none;"><%~ NoScriptErr %></div>
				<div class="row form-group">
					<label class="col-xs-5" for="ddns_provider" id="ddns_provider_label"><%~ SvPro %>:</label>
					<span class="col-xs-7" id="ddns_provider_text"></span>
				</div>

				<div id="ddns_variable_container"></div>

				<div class="row form-group">
					<label class="col-xs-5" for="ddns_check" id="ddns_check_label"><%~ ChItv %>:</label>
					<span class="col-xs-7">
						<input type="text" class="form-control" id="ddns_check" size="8" onkeyup="proofreadNumeric(this)"/>
						<em><%~ minutes %></em>
					</span>
					<span class="col-xs-12">
						<p><%~ HelpCI %></p>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="ddns_force" id="ddns_force_label"><%~ FUItv %>:</label>
					<span class="col-xs-7">
						<input type="text" class="form-control" id="ddns_force" size="8" onkeyup="proofreadNumeric(this)"/>
						<em><%~ days %></em>
					</span>
					<span class="col-xs-12">
						<p><%~ HelpFI %></p>
					</span>
				</div>

			</div>
		</div>
	</div>
</div>
<div id="bottom_button_container" class="panel panel-default"></div>
</body>
</html>
