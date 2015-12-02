#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2015 John Brown and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "devices" -c "internal.css" -j "table.js device.js" -z "device.js" network dhcp known
	subnet=$(ifconfig br-lan | awk 'BEGIN {FS=":"}; $0 ~ /inet.addr/ {print $2}' | awk 'BEGIN {FS="."}; {print $1"\."$2"\."$3"\."}')
%>

<script>
<!--
<%
	echo "var hostData = new Array();"
	if [ -e /etc/hosts ] ; then
		awk ' $0 ~ /^[\t ]*[0-9]/ {print "hostData.push([\""$1"\",\""$2"\"]);"};' /etc/hosts
	fi

	echo "";
	echo "var etherData = new Array();";
	if [ -e /etc/ethers ] ; then
		awk ' $0 ~ /^[\t ]*[0-9abcdefABCDEF]/ {print "etherData.push([\""$1"\",\""$2"\"]);"};' /etc/ethers
	fi

	echo "";
	echo "var leaseData = new Array();";
	if [ -e /tmp/dhcp.leases ] ; then
		awk ' $0 ~ /[a-z,A-Z,0-9]+/ {print "leaseData.push([\""$2"\",\""$3"\",\""$4"\"]);"};' /tmp/dhcp.leases
	fi

%>
//-->
</script>

<form>

	<fieldset>
		<legend class="sectionheader"><%~ device.KnDev %></legend>

		<div id='device_add_heading_container'>
			<label class='nocolumn' id='device_add_heading_label' style='text-decoration:underline'><%~ AdKnDev %>:</label>
		</div>
		<div class='bottom_gap'>
			<div id='device_add_container'>
				<%in templates/device_template %>
			</div>
			<div>
				<select id="mac_list" onchange="macSelected()" >
					<option value="none"><%~ SelM %></option>
				</select>
			</div>
		</div>

		<div id='device_table_heading_container'>
			<span class='nocolumn'><%~ KnDev %>:</span>
		</div>
		<div class='indent'>
			<div id='device_table_container' class="bottom_gap"></div>
		</div>
	</fieldset>


		<fieldset>
			<legend class="sectionheader"><%~ DevGp %></legend>

			<div id='group_add_heading_container'>
				<label class='nocolumn' id='group_add_heading_label' style='text-decoration:underline'><%~ AdGp %>:</label>
			</div>
			<div class='bottom_gap'>
				<div id='group_add_container'>
					<%in templates/group_template %>
				</div>
				<div>
					<select id="group_list" onchange="groupSelected()" >
						<option value="none"><%~ SelG %></option>
					</select>
					<select id="device_list" onchange="deviceSelected()" >
						<option value="none"><%~ SelD %></option>
					</select>
				</div>
			</div>

			<div id='group_table_heading_container'>
				<span class='nocolumn'><%~ DevGp %>:</span>
			</div>
			<div class='indent'>
				<div id='group_table_container' class="bottom_gap"></div>
			</div>
		</fieldset>

	<div id="firefox3_bug_correct" style="display:none">
		<input type='text' value='firefox3_bug' />
	</div>

	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button"  onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button"  onclick='resetData()'/>
	</div>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "devices"
%>
