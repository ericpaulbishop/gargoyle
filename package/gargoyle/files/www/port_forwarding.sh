#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -c "internal.css" -j "gs_sortable.js port_forwarding.js table.js" -z "port.js" gargoyle -i firewall network dropbear upnpd
%>

<script>
<!--
<%
	upnp_config_enabled=$(uci get upnpd.config.enable_upnp 2>/dev/null)
	if [ -h /etc/rc.d/S95miniupnpd ] && [ -n "$upnp_config_enabled" ] && [ "$upnp_config_enabled" != "0" ] ; then
		echo "var upnpdEnabled = true;"
	else
		echo "var upnpdEnabled = false;"
	fi
	if [ -z "$upnp_config_enabled" ] ; then
		echo "var haveUpnpd = false;"
	else
		echo "var haveUpnpd = true;"
	fi
%>
//-->
</script>

<h1 class="page-header">Port Forwarding</h1>
<div class="row">

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ port.PISect %></h3>
			</div>

			<div class="panel-body">
				<div id="portf_add_heading_container" class="form-group form-inline">
					<label id="portf_add_heading_label"><%~ ForIPort %>:</label>
				</div>

				<div class="form-group form-inline">
					<div id="portf_add_container">
						<%in templates/single_forward_template %>
					</div>
				</div>

				<div id="portf_table_container" class="table-responsive"></div>
			</div>

		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ PRSect %></h3>
			</div>

			<div class="panel-body">
				<div id="portfrange_add_heading_container" class="form-group form-inline">
					<label id="portf_add_heading_label"><%~ ForRPort %>:</label>
				</div>

				<div class="form-group form-inline">
					<div id="portfrange_add_container">
						<%in templates/multi_forward_template %>
					</div>
				</div>

				<div id="portfrange_table_container" class="table-responsive"></div>
			</div>

		</div>
	</div>

</div>

<div class="row">

	<div id="upnp_fieldset" class="col-lg-4">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ UP_NAT %></h3>
			</div>

			<div class="panel-body">
				<div id="upnp_enabled_container" class="form-group form-inline">
					<input type="checkbox" id="upnp_enabled" onclick="setUpnpEnabled()" />
					<label id="upnp_enabled_label" for="upnp_enabled"><%~ UPNAT_En %></label>
				</div>

				<div id="upnp_table_heading_container" class="form-group form-inline">
					<span><%~ APFor %>:</span>
				</div>

				<br/>

				<div class="form-group form-inline">
					<div id="upnp_table_container" class="bottom_gap"></div>
				</div>

				<div id="upnp_up_container" class="form-group form-inline">
					<label for="upnp_up" id="upnp_up_label"><%~ USpd %>:</label>
					<input type="text" id="upnp_up" class="form-control" onkeyup="proofreadNumeric(this)" size="5" maxlength="5" />
					<em><%~ KBs %></em>
				</div>

				<div id="upnp_down_container" class="form-group form-inline">
					<label for="upnp_down" id="upnp_down_label"><%~ DSpd %>:</label>
					<input type="text" id="upnp_down" class="form-control" onkeyup="proofreadNumeric(this)" size="5" maxlength="5" />
					<em><%~ KBs %></em>
				</div>

				<div id="upnp_help" class="form-group form-inline">
					<span id="upnp_help_txt"><%~ UPHelp %></span>
					<a id="upnp_help_ref" onclick="setDescriptionVisibility('upnp_help')" href="#upnp_help"><%~ Hide %></a>
				</div>
			</div>

		</div>
	</div>

	<div class="col-lg-4">
		<div class="panel panel-default">

			<div class="panel-heading">
				<h3 class="panel-title"><%~ DMZ %></h3>
			</div>

			<div class="panel-body">
				<div id="dmz_enabled_container" class="form-group form-inline">
					<input type="checkbox" id="dmz_enabled" onclick="setDmzEnabled()" />
					<label id="dmz_enabled_label" for="dmz_enabled"><%~ UseDMZ %></label>
				</div>

				<div id="dmz_ip_container" class="form-group form-inline">
					<label for="dmz_ip" id="dmz_ip_label"><%~ DMZIP %>:</label>
					<input type="text" class="form-control" name="dmz_ip" id="dmz_ip" onkeyup="proofreadIp(this)" size="20" maxlength="15" />
				</div>
			</div>

		</div>
	</div>

</div>

<div id="bottom_button_container">
	<button id="save_button" class="btn btn-primary" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning" onclick="resetData()"><%~ Reset %></button>
</div>
<span id="update_container" ><%~ WaitSettings %></span>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "portforwarding"
%>
