#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -c "internal.css" -j "port_forwarding.js table.js" -z "port.js" gargoyle -i firewall network dropbear upnpd
%>

<script>
<!--
<%
	upnp_config_enabled=$(uci get upnpd.config.enable_upnp)
	if [ -h /etc/rc.d/S95miniupnpd ] && [ "$upnp_config_enabled" != "0" ] ; then
		echo "var upnpdEnabled = true;"
	else
		echo "var upnpdEnabled = false;"
	fi
%>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader"><%~ port.PISect %></legend>

		<div id='portf_add_heading_container'>
			<label class='nocolumn' id='portf_add_heading_label'><%~ ForIPort %>:</label>
		</div>
		<div class='bottom_gap'>
			<div id='portf_add_container'>
				<%in templates/single_forward_template %>
			</div>
		</div>

		<div id='portf_table_container' class="bottom_gap"></div>
	</fieldset>

	<fieldset>
		<legend class="sectionheader"><%~ PRSect %></legend>

		<div id='portfrange_add_heading_container'>
			<label class='nocolumn' id='portf_add_heading_label'><%~ ForRPort %>:</label>
		</div>

		<div class='bottom_gap'>
			<div id='portfrange_add_container'>
				<%in templates/multi_forward_template %>
			</div>
		</div>

		<div id='portfrange_table_container' class="bottom_gap"></div>
	</fieldset>

	<fieldset>
		<legend class="sectionheader"><%~ DMZ %></legend>
		<div id='dmz_enabled_container'>
			<input type='checkbox' id='dmz_enabled' onclick="setDmzEnabled()" />
			<label id='dmz_enabled_label' for='dmz_enabled'><%~ UseDMZ %></label>
		</div>
		<div id="dmz_ip_container" class="indent">
			<label class='leftcolumn' for='dmz_ip' id='dmz_ip_label'><%~ DMZIP %>:</label>
			<input type='text' class='rightcolumn' name='dmz_ip' id='dmz_ip' onkeyup='proofreadIp(this)' size='20' maxlength='15' />
		</div>
	</fieldset>

	<fieldset>
		<legend class="sectionheader"><%~ UP_NAT %></legend>
		<div id='upnp_enabled_container'>
			<input type='checkbox' id='upnp_enabled' onclick="setUpnpEnabled()" />
			<label id='upnp_enabled_label' for='upnp_enabled'><%~ UPNAT_En %></label>
		</div>

		<div id='upnp_table_heading_container'>
			<span class='nocolumn'><%~ APFor %>:</span>
		</div>
 
		<br>

		<div class='indent'>
			<div id='upnp_table_container' class="bottom_gap"></div>
		</div>

		<div id='upnp_up_container'>
			<label class='leftcolumn' for='upnp_up' id='upnp_up_label'><%~ USpd %>:</label>
			<span class = 'rightcolumn'>
				<input type='text' class='rightcolumn' id='upnp_up' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em><%~ KBs %></em>
			</span>
		</div>

		<div id='upnp_down_container'>
			<label class='leftcolumn' for='upnp_down' id='upnp_down_label'><%~ DSpd %>:</label>
			<span class='rightcolumn'>
				<input type='text' id='upnp_down' onkeyup='proofreadNumeric(this)' size='5' maxlength='5' />
				<em><%~ KBs %></em>
			</span>

		<div id="upnp_help" class="indent">
		<span id='upnp_help_txt'>

		<p><%~ UPHelp %></p>

		</span>
		<a id="upnp_help_ref" onclick='setDescriptionVisibility("upnp_help")'  href="#upnp_help"><%~ Hide %></a>

		</div>

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
	gargoyle_header_footer -f -s "firewall" -p "portforwarding"
%>
