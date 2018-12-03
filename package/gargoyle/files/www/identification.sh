#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "ident" -j "identification.js" -z "ident.js" system dhcp network wireless
%>

<script>
	var havePrinterScript=false;
<%
	if [ -e /usr/lib/gargoyle/configure_printer.sh ] ; then echo "havePrinterScript=true;" ; fi
%>
</script>

<h1 class="page-header"><%~ ident.IdSect %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ IdSect %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" for="hostname" id="hostname_label"><%~ HsNm %></label>
					<span class="col-xs-7"><input type="text" id="hostname" class="form-control" onkeyup="proofreadLengthRange(this,1,999)"  size="35" maxlength="25"/></span>
				</div>
				
				<div id="domain_container" class="row form-group">
					<label class="col-xs-5" for="domain" id="domain_label"><%~ Domn %></label>
					<span class="col-xs-7"><input type="text" id="domain" class="form-control" onkeyup="proofreadLengthRange(this,1,999)" size="35" maxlength="100"/></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>



<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "ident"
%>
