#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "tor" -j "tor.js" -z "tor.js" tor uhttpd dropbear firewall
%>

<script>
<%
	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

	gpkg dest-info -o 'js'
%>
</script>

<h1 class="page-header"><%~ tor.mTor %></h1>
<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ tor.TorAC %></h3>
			</div>

			<div class="panel-body">
				<div id='tor_client_mode_container' class="row form-group">
					<span class="col-xs-5">
						<select class="form-control" id="tor_client_mode" onchange='setTorVisibility()' >
							<option value="2"><%~ EnByHost %></option>
							<option value="1"><%~ EnAll %></option>
							<option value="3"><%~ HidAcc %></option>
							<option value="0"><%~ Disabled %></option>
						</select>
					</span>
					<em><span class="col-xs-7" id="mode_description"></span></em>
				</div>

				<div id='tor_client_connect_container' class="row form-group">
					<label class="col-xs-5" for='tor_client_connect' id='tor_client_mode_label'><%~ ConVia %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="tor_client_connect" onchange='setTorVisibility()' >
							<option value="relay"><%~ TRly %></option>
							<option value="bridge"><%~ TBrg %></option>
							<option value="obfsproxy"><%~ TBrgOb %></option>
						</select>
					</span>
				</div>
				<div class="indent">
					<div id='tor_client_bridge_ip_container' class="row form-group">
						<label class="col-xs-5" for='tor_client_bridge_ip' id='tor_client_bridge_ip_label'><%~ BrIP %>:</label>
						<span class="col-xs-7"><input type="text" class="form-control" id='tor_client_bridge_ip' onkeyup='proofreadIp(this)'/></span>
					</div>
					<div id='tor_client_bridge_port_container' class="row form-group">
						<label class="col-xs-5" for='tor_client_bridge_port' id='tor_client_bridge_port_label'><%~ BrPrt %>:</label>
						<span class="col-xs-7"><input type="text" class="form-control" id='tor_client_bridge_port' onkeyup='proofreadPort(this)'/></span>
					</div>
				</div>

				<div id='tor_other_proto_container' class="row form-group">
					<label class="col-xs-5" for='tor_other_proto' id='tor_other_proto_label'><%~ OProto %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="tor_other_proto">
							<option value="0"><%~ Ignr %></option>
							<option value="1"><%~ Blck %></option>
						</select>
					</span>
				</div>

				<div id='tor_hidden_subnet_container' class="row form-group">
					<label class="col-xs-5" for='tor_hidden_subnet' id='tor_hidden_subnet_label'><%~ HSSub %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_hidden_subnet' onkeyup='proofreadIp(this)' /></span>
				</div>
				<div id='tor_hidden_mask_container' class="row form-group">
					<label class="col-xs-5" for='tor_hidden_mask' id='tor_hidden_mask_label'><%~ HSMsk %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_hidden_mask' onkeyup='proofreadMask(this)' /></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ TorAS %></h3>
			</div>

			<div class="panel-body">
				<div id='tor_relay_mode_container' class="row form-group">
					<span class="col-xs-5">
						<select class="form-control" id="tor_relay_mode" onchange='setTorVisibility()' >
							<option value="1"><%~ EnBr %></option>
							<option value="3"><%~ EnBrO %></option>
							<option value="2"><%~ EnRly %></option>
							<option value="0"><%~ Disabled %></option>
						</select>
					</span>
					<em><span class="col-xs-7" id="mode_description"></span></em>
				</div>

				<div id='tor_relay_port_container' class="row form-group">
					<label class="col-xs-5" for='tor_relay_port' id='tor_relay_port_label'><%~ BrRPrt %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_relay_port' size='9' onkeyup='proofreadPort(this)' /></span>
				</div>

				<div id='tor_obfsproxy_port_container' class="row form-group">
					<label class="col-xs-5" for='tor_obfsproxy_port' id='tor_obfsproxy_port_label'><%~ ObfPrt %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_obfsproxy_port' size='9' onkeyup='proofreadPort(this)' /></span>
				</div>

				<div id='tor_relay_max_bw_container' class="row form-group">
					<label class="col-xs-5" for='tor_relay_max_bw' id='tor_relay_max_bw_label'><%~ MaxRB %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_relay_max_bw' size='9' onkeyup='proofreadNumeric(this)' /><em>&nbsp;&nbsp;<%~ KBs %></em></span>
				</div>

				<div id='tor_relay_publish_container' class="row form-group">
					<label class="col-xs-5" for='tor_relay_publish' id='tor_relay_publish_label'><%~ PubBrDB %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="tor_relay_publish">
							<option value="1"><%~ PubBr %></option>
							<option value="0"><%~ NoPub %></option>
						</select>
					</span>
				</div>

				<div id='tor_relay_nickname_container' class="row form-group">
					<label class="col-xs-5" for='tor_relay_nickname' id='tor_relay_nickname_label'><%~ Nick %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id='tor_relay_nickname' /></span>
				</div>

				<div id='tor_relay_contact_container' class="row form-group">
					<label class="col-xs-5" for='tor_relay_contact' id='tor_relay_contact_label'><%~ Mail %>:</label>
					<span class="col-xs-7"><textarea class="form-control" id='tor_relay_contact' ></textarea></span>
				</div>

				<div id='tor_relay_status_link_container' class="row form-group">
					<span class='col-xs-12'><%~ VisMsg %> <a href="http://torstatus.blutmagie.de/"><%~ GlbMsg %></a></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row" id="tor_data_dir_section">
	<div class="col-lg-12 ">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ TDDir %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-5"><%~ TDDir %>:</span>
					<span class="col-xs-7" id="tor_dir_ramdisk_static">/var/tor</span>
					<span class="col-xs-7" id="tor_dir_root_static">/usr/lib/tor</span>
					<span class="col-xs-7"><input type="text" id="tor_dir_text" style="display:none" /></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-5"><%~ TDDrv %>:</span>
					<span class="col-xs-7"><select id="tor_dir_drive_select" onchange="setTorVisibility()" class="form-control"></select></span>
				</div>
				<div>
					<span class="col-xs-12">
						<em><%~ CacheWarn %></em>
					</span>
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
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "tor"
%>
