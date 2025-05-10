#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "portforwarding" -j "gs_sortable.js port_forwarding.js table.js" -z "port.js" -i firewall gargoyle network dropbear upnpd
%>

<script>
<!--
<%
	upnp_config_enabled=$(uci get upnpd.config.enabled 2>/dev/null)
	have_miniupnpd=$(gpkg list-installed miniupnpd-nftables 2>&1)
	if [ -h /etc/rc.d/S94miniupnpd ] && [ -n "$upnp_config_enabled" ] && [ "$upnp_config_enabled" != "0" ] ; then
		echo "var upnpdEnabled = true;"
	else
		echo "var upnpdEnabled = false;"
	fi
	if [ -z "$have_miniupnpd" ] ; then
		echo "var haveUpnpd = false;"
	else
		echo "var haveUpnpd = true;"
	fi
%>
//-->
</script>

<h1 class="page-header"><%~ port.mPFwding %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ port.PISect %></h3>
			</div>

			<div class="panel-body">
				<div id="portf_add_heading_container" class="row form-group">
					<label class="col-xs-12" id="portf_add_heading_label" style="text-decoration:underline"><%~ ForIPort %>:</label>
				</div>

				<div class="row form-group">
					<div id="portf_add_container" class="col-xs-12">
						<button type="button" id="add_button" class="btn btn-default btn-add" onclick="addPortFModal(true)"><%~ Add %></button>
					</div>
				</div>

				<div id="portf_table_container" class="col-xs-12 table-responsive"></div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ PRSect %></h3>
			</div>

			<div class="panel-body">
				<div id="portfrange_add_heading_container" class="row form-group">
					<label class="col-xs-12" id="portf_add_heading_label" style="text-decoration:underline"><%~ ForRPort %>:</label>
				</div>

				<div class="row form-group">
					<div id="portfrange_add_container" class="col-xs-12">
						<button type='button' id='addr_button' class='btn btn-default btn-add' onclick='addPortFModal(false)'><%~ Add %></button>
					</div>
				</div>

				<div id="portfrange_table_container" class="col-xs-12 table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ port.POISect %></h3>
			</div>

			<div class="panel-body">
				<div id="porto_add_heading_container" class="row form-group">
					<label class="col-xs-12" id="porto_add_heading_label" style="text-decoration:underline"><%~ OpeIPort %>:</label>
				</div>

				<div class="row form-group">
					<div id="porto_add_container" class="col-xs-12">
						<button type="button" id="addo_button" class="btn btn-default btn-add" onclick="addPortOModal(true)"><%~ Add %></button>
					</div>
				</div>

				<div id="porto_table_container" class="col-xs-12 table-responsive"></div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ port.PORSect %></h3>
			</div>

			<div class="panel-body">
				<div id="portorange_add_heading_container" class="row form-group">
					<label class="col-xs-12" id="portorange_add_heading_label" style="text-decoration:underline"><%~ OpeRPort %>:</label>
				</div>

				<div class="row form-group">
					<div id="portorange_add_container" class="col-xs-12">
						<button type="button" id="addor_button" class="btn btn-default btn-add" onclick="addPortOModal(false)"><%~ Add %></button>
					</div>
				</div>

				<div id="portorange_table_container" class="col-xs-12 table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div id="upnp_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ UP_NAT %></h3>
			</div>

			<div class="panel-body">
				<div id="upnp_no_miniupnpd" class="alert alert-danger" role="alert" style="display: none;"><%~ NoMiniupnpdErr %></div>
				<div id="upnp_enabled_container" class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="upnp_enabled" onclick="setUpnpEnabled()" />
						<label id="upnp_enabled_label" for="upnp_enabled"><%~ UPNAT_En %></label>
					</span>
				</div>

				<div id="upnp_table_heading_container" class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ APFor %>:</span>
				</div>

				<br/>

				<div class="row form-group">
					<div id="upnp_table_container" class="col-xs-12 bottom_gap table-responsive"></div>
				</div>

				<div id="upnp_up_container" class="row form-group">
					<label class="col-xs-5" for="upnp_up" id="upnp_up_label"><%~ USpd %>:</label>
					<span class="col-xs-7">
						<input type="text" id="upnp_up" class="form-control" oninput="proofreadNumeric(this)" size="5" maxlength="5" />
						<em><%~ KBs %></em>
					</span>
				</div>

				<div id="upnp_down_container" class="row form-group">
					<label class="col-xs-5" for="upnp_down" id="upnp_down_label"><%~ DSpd %>:</label>
					<span class="col-xs-7">
						<input type="text" id="upnp_down" class="form-control" oninput="proofreadNumeric(this)" size="5" maxlength="5" />
						<em><%~ KBs %></em>
					</span>
				</div>

				<div id="upnp_help" class="row form-group">
					<span class="col-xs-12"><a id="upnp_help_ref" href="javascript:setDescriptionVisibility('upnp_help')"><%~ Hide %></a></span>
					<span class="col-xs-12" id="upnp_help_txt">
						<p><%~ UPHelp %></p>
					</span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DMZ %></h3>
			</div>

			<div class="panel-body">
				<div id="dmz_enabled_container" class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="dmz_enabled" onclick="setDmzEnabled()" />
						<label id="dmz_enabled_label" for="dmz_enabled"><%~ UseDMZ %></label>
					</span>
				</div>

				<div id="dmz_ip_container" class="row form-group">
					<label class="col-xs-5" for="dmz_ip" id="dmz_ip_label"><%~ DMZIP %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" name="dmz_ip" id="dmz_ip" oninput="proofreadIp(this)" size="20" maxlength="15" /></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="single_forward_modal" aria-hidden="true" aria-labelledby="single_forward_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="single_forward_modal_title" class="panel-title"><%~ ForIPort %></h3>
			</div>
			<div class="modal-body">
				<%in templates/single_forward_template %>
			</div>
			<div class="modal-footer" id="single_forward_modal_button_container">
			</div>
		</div>
	</div>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="multi_forward_modal" aria-hidden="true" aria-labelledby="multi_forward_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="multi_forward_modal_title" class="panel-title"><%~ ForIPort %></h3>
			</div>
			<div class="modal-body">
				<%in templates/multi_forward_template %>
			</div>
			<div class="modal-footer" id="multi_forward_modal_button_container">
			</div>
		</div>
	</div>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="single_open_modal" aria-hidden="true" aria-labelledby="single_open_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="single_open_modal_title" class="panel-title"><%~ OpeIPort %></h3>
			</div>
			<div class="modal-body">
				<%in templates/single_open_template %>
			</div>
			<div class="modal-footer" id="single_open_modal_button_container">
			</div>
		</div>
	</div>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="multi_open_modal" aria-hidden="true" aria-labelledby="multi_open_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="multi_open_modal_title" class="panel-title"><%~ OpeIPort %></h3>
			</div>
			<div class="modal-body">
				<%in templates/multi_open_template %>
			</div>
			<div class="modal-footer" id="multi_open_modal_button_container">
			</div>
		</div>
	</div>
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "portforwarding"
%>
