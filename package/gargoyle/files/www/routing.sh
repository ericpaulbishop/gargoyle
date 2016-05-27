#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "routing" -c "internal.css" -j "table.js routing.js" -z "routing.js" network
	subnet=$(ifconfig br-lan | awk 'BEGIN {FS=":"}; $0 ~ /inet.addr/ {print $2}' | awk 'BEGIN {FS="."}; {print $1"\."$2"\."$3"\."}')
%>

<script>
<!--
<%
	lan_iface=$(uci -P /var/state get network.lan.ifname)
	wan_iface=$(uci -q -P /var/state get network.wan.ifname)
	echo "var lanIface=\"$lan_iface\";"
	echo "var wanIface=\"$wan_iface\";"
	echo "var routingData = new Array();"
	route | awk ' {print "routingData.push(\""$0"\");"};'
%>

//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader"><%~ routing.ARSect %></legend>
		<div id="active_route_table_container"></div>
	</fieldset>

	<fieldset>
		<legend class="sectionheader"><%~ SRSect %></legend>

		<div id='static_route_add_heading_container'>
			<label class='nocolumn' id='staticroute_add_heading_label' style='text-decoration:underline'><%~ ASRte %>:</label>
		</div>
		<div class='bottom_gap'>
			<div id='static_route_add_container'>
				<%in templates/static_route_template %>
			</div>
		</div>

		<div id='static_route_table_heading_container'>
			<span class='nocolumn'><%~ CSRSect %>:</span>
		</div>
		<div class='indent'>
			<div id='static_route_table_container' class="bottom_gap"></div>
		</div>
	</fieldset>

	<div id="firefox3_bug_correct" style="display:none">
		<input type='text' value='firefox3_bug' />
	</div>

	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button"  onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button"  onclick='resetData()'/>
	</div>
	<span id="update_container" ><%~ WaitSettings %></span>
</form>
<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "routing"
%>
