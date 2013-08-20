#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "connlimits" -c "internal.css" -j "connlimits.js" -z "connlimits.js"
%>

<script>
<%
	max=$(cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null )

	if [ -n "$max" ] ; then
		echo "var maxConnections = $max;";
	else
		echo "var maxConnections = 4096;";
	fi

	tcp=$(cat /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established 2>/dev/null)
	udp=$(cat /proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream 2>/dev/null)
	if [ -z "$tcp" ] ; then tcp=180 ; fi
	if [ -z "$udp" ] ; then udp=180 ; fi
	echo "var tcpTimeout = $tcp;"
	echo "var udpTimeout = $udp;"
%>
</script>

<fieldset>
	<legend class="sectionheader"><%~ connlimits.CLSect %></legend>
	<div>
		<label class='narrowleftcolumn' for='max_connections' id='max_connections_label'><%~ MaxC %>:</label>
		<input type='text' class='rightcolumn' onkeyup='proofreadNumericRange(this,1,16384)' id='max_connections' size='10' maxlength='5' />
		<em>(<%~ max %> 16384)</em>
	</div>
	<div>
		<label class='narrowleftcolumn' for='tcp_timeout' id='tcp_timeout_label'><%~ TTout %>:</label>
		<input type='text' class='rightcolumn' onkeyup='proofreadNumericRange(this,1,3600)' id='tcp_timeout' size='10' maxlength='4' />
		<em><%~ seconds %> (<%~ max %> 3600)</em>
	</div>
	<div>
		<label class='narrowleftcolumn' for='udp_timeout' id='udp_timeout_label'><%~ UTout %>:</label>
		<input type='text' class='rightcolumn' onkeyup='proofreadNumericRange(this,1,3600)' id='udp_timeout' size='10' maxlength='4' />
		<em><%~ seconds %> (<%~ max %> 3600)</em>
	</div>
</fieldset>
<div id="bottom_button_container">
	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
</div>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "connlimits"
%>
