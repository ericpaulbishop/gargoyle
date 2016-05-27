#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "printers" -c "internal.css"
%>

<script>
<%
	printer_id=$(uci get p910nd.@p910nd[0].printer_name 2>/dev/null)
	ipaddr=$(uci get network.lan.ipaddr 2>/dev/null)
	echo "var printerId=\"$printer_id\";"
	echo "var routerIp=\"$ipaddr\";"
%>
	window.onload = function()
	{
		document.getElementById("no_printer_div").style.display = printerId == "" ? "block" : "none"
		document.getElementById("printer_found_div").style.display = printerId == "" ? "none" : "block"
		setChildText( "printer_id", printerId )
		setChildText( "router_ip",  routerIp  )
	}

</script>

	<fieldset>
		<legend class="sectionheader"><%~ print.Attch %></legend>
		<div id="no_printer_div">
			<p><em><%~ NoPrnt %></em></p>
		</div>
		<div id="printer_found_div">

			<p><strong><span id="printer_id"></span> <%~ ConnU %></strong></p>
			
			<p><%~ ConnIP %> <span id="router_ip"></span> <%~ JetProto %></p>
		</div>
		
	
	</fieldset>

<%
	gargoyle_header_footer -f -s "system" -p "printers"
%>
