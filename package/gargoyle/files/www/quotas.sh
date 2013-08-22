#!/usr/bin/haserl
<%
	# This program is copyright © 2008,2009 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "quotas" -c "internal.css" -j "table.js quotas.js" -z "quotas.js" gargoyle firewall qos_gargoyle

%>


<script>
<!--
<%
	echo "var qosMarkList = [];"
	if [ -h /etc/rc.d/S50qos_gargoyle ] && [ -e /etc/qos_class_marks ]  ; then
		echo "var fullQosEnabled = true;"
		awk '{ print "qosMarkList.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\"]);" }' /etc/qos_class_marks
	else
		echo "var fullQosEnabled = false;"
	fi

	print_quotas
%>
	var uci = uciOriginal.clone();
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader"><%~ quotas.Section %></legend>

		<span id="add_quota_label" style="text-decoration:underline" ><%~ AddQuota %>:</span>

		<div>

			<%in templates/quotas_template %>

			<div>
				<input type="button" id="add_quota_button" class="default_button" value="<%~ AddQuota %>" onclick="addNewQuota()" />
			</div>
		</div>

		<div id='internal_divider1' class='internal_divider'></div>

		<span id="active_quotas_label" style="text-decoration:underline" ><%~ ActivQuotas %>:</span>

		<div id="quota_table_container"></div>

	</fieldset>
	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>

	<span id="update_container" ><%~ WaitSettings %></span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "quotas"
%>
