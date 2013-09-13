#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "connections" -c "internal.css" -j "conntrack.js table.js" -z "conntrack.js" -i -n httpd_gargoyle firewall qos_gargoyle
%>

<script>
<!--
<%
	qos_enabled=$(ls /etc/rc.d/*qos_gargoyle 2>/dev/null)
	if [ -n "$qos_enabled" ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi

	echo "var qosMarkList = [];"
	if [ -e /etc/qos_class_marks ] ; then
		awk '{ print "qosMarkList.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\"]);" }' /etc/qos_class_marks
	fi
%>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader"><%~ conntrack.CCSect %></legend>

		<div>
			<label for="refresh_rate" class="narrowleftcolumn"><%~ RRate %>:</label>
			<select id="refresh_rate" class="rightcolumn" >
				<option value="2000">2 <%~ seconds %></option>
				<option value="10000">10 <%~ seconds %></option>
				<option value="30000">30 <%~ seconds %></option>
				<option value="60000">60 <%~ seconds %></option>
				<option value="never"><%~ never %></option>
			</select>
		</div>
		<div>
			<label for="bw_units" class="narrowleftcolumn" onchange="updateConnectionTable()"><%~ BUnt %>:</label>
			<select id="bw_units" class="rightcolumn">
				<option value="mixed"><%~ AtMxd %></option>
				<option value="KBytes"><%~ KBy %></option>
				<option value="MBytes"><%~ MBy %></option>
				<option value="GBytes"><%~ GBy %></option>
			</select>
		</div>
		<div>
			<label for="host_display" class="narrowleftcolumn" onchange="updateConnectionTable()"><%~ HDsp %>:</label>
			<select id="host_display" class="rightcolumn">
				<option value="hostname"><%~ DspHn %></option>
				<option value="ip"><%~ DspHIP %></option>
			</select>
		</div>

		<div id="connection_table_container"></div>
		<div style="width:375px">
			<p><%~ CnWarn %></p>
		</div>
	</fieldset>
</form>

<script>
<!--
	initializeConnectionTable();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "connections"
%>
