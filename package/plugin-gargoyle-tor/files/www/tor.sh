#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "tor" -c "internal.css" -j "tor.js" -z "tor.js" tor httpd_gargoyle dropbear firewall
%>

<script>
<%
	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

	gpkg dest-info -o 'js'
%>
</script>


	<fieldset>
		<legend class="sectionheader"><%~ tor.TorAC %></legend>
		
		<div id='tor_client_mode_container'>
			<label  class='wideleftcolumn' for='tor_client_mode' id='tor_client_mode_label' ><%~ TorC %>:</label>
			<select class='rightcolumn' id="tor_client_mode" onchange='setTorVisibility()' >
				<option value="2"><%~ EnByHost %></option>
				<option value="1"><%~ EnAll %></option>
				<option value="3"><%~ HidAcc %></option>
				<option value="0"><%~ Disabled %></option>
			</select>
			<br/>
			<em><span class="farrightcolumnonly" id="mode_description"></span></em>
			<br/>
		</div>

		<div id='tor_client_connect_container'>
			<label  class='wideleftcolumn' for='tor_client_connect' id='tor_client_mode_label' ><%~ ConVia %>:</label>
			<select class='rightcolumn' id="tor_client_connect" onchange='setTorVisibility()' >
				<option value="relay"><%~ TRly %></option>
				<option value="bridge"><%~ TBrg %></option>
				<option value="obfsproxy"><%~ TBrgOb %></option>
			</select>
		</div>
		<div class="indent">
			<div id='tor_client_bridge_ip_container'>
				<label  class='wideleftcolumn' for='tor_client_bridge_ip' id='tor_client_bridge_ip_label'><%~ BrIP %>:</label>
				<input type="text" class="rightcolumn" id='tor_client_bridge_ip' onkeyup='proofreadIp(this)'/>
			</div>
			<div id='tor_client_bridge_port_container'>
				<label  class='wideleftcolumn' for='tor_client_bridge_port' id='tor_client_bridge_port_label'><%~ BrPrt %>:</label>
				<input type="text" class="rightcolumn" id='tor_client_bridge_port' onkeyup='proofreadPort(this)'/>
			</div>
		</div>

		<div id='tor_other_proto_container'>
			<label  class='wideleftcolumn' for='tor_other_proto' id='tor_other_proto_label'><%~ OProto %>:</label>
			<select class='rightcolumn' id="tor_other_proto">
				<option value="0"><%~ Ignr %></option>
				<option value="1"><%~ Blck %></option>
			</select>
		</div>

		<div id='tor_hidden_subnet_container'>
			<label  class='wideleftcolumn' for='tor_hidden_subnet' id='tor_hidden_subnet_label'><%~ HSSub %>:</label>
			<input type="text" class="rightcolumn" id='tor_hidden_subnet' onkeyup='proofreadIp(this)' />
		</div>
		<div id='tor_hidden_mask_container'>
			<label  class='wideleftcolumn' for='tor_hidden_mask' id='tor_hidden_mask_label'><%~ HSMsk %>:</label>
			<input type="text" class="rightcolumn" id='tor_hidden_mask' onkeyup='proofreadMask(this)'/>
		</div>

	</fieldset>

	<fieldset>
		<legend class="sectionheader"><%~ TorAS %></legend>

		<div id='tor_relay_mode_container'>
			<label  class='wideleftcolumn' for='tor_relay_mode' id='tor_relay_mode_label' ><%~ TorS %>:</label>
			<select class='rightcolumn' id="tor_relay_mode" onchange='setTorVisibility()' >
				<option value="1"><%~ EnBr %></option>
				<option value="3"><%~ EnBrO %></option>
				<option value="2"><%~ EnRly %></option>
				<option value="0"><%~ Disabled %></option>
			</select>
			<br/>
			<em><span class="farrightcolumnonly" id="mode_description"></span></em>
			<br/>
		</div>

		<div id='tor_relay_port_container'>
			<label  class='wideleftcolumn' for='tor_relay_port' id='tor_relay_port_label'><%~ BrRPrt %>:</label>
			<input type="text" class="rightcolumn" id='tor_relay_port' size='9' onkeyup='proofreadPort(this)' />
		</div>

		<div id='tor_obfsproxy_port_container'>
			<label  class='wideleftcolumn' for='tor_obfsproxy_port' id='tor_obfsproxy_port_label'><%~ ObfPrt %>:</label>
			<input type="text" class="rightcolumn" id='tor_obfsproxy_port' size='9' onkeyup='proofreadPort(this)' />
		</div>

		<div id='tor_relay_max_bw_container'>
			<label  class='wideleftcolumn' for='tor_relay_max_bw' id='tor_relay_max_bw_label'><%~ MaxRB %>:</label>
			<span class="rightcolumn"><input type="text" id='tor_relay_max_bw' size='9' onkeyup='proofreadNumeric(this)' /><em>&nbsp;&nbsp;<%~ KBs %></em></span>
		</div>

		<div id='tor_relay_publish_container'>
			<label  class='wideleftcolumn' for='tor_relay_publish' id='tor_relay_publish_label'><%~ PubBrDB %>:</label>
			<select class="rightcolumn" id="tor_relay_publish">
				<option value="1"><%~ PubBr %></option>
				<option value="0"><%~ NoPub %></option>
			</select>
		</div>

		<div id='tor_relay_nickname_container'>
			<label  class='wideleftcolumn' for='tor_relay_nickname' id='tor_relay_nickname_label'><%~ Nick %>:</label>
			<input type="text" class="rightcolumn" id='tor_relay_nickname' />
		</div>

		<div id='tor_relay_contact_container'>
			<label  class='wideleftcolumn' for='tor_relay_contact' id='tor_relay_contact_label'><%~ Mail %>:</label>
			<textarea class="rightcolumn" id='tor_relay_contact' ></textarea>
		</div>

		<div id='tor_relay_status_link_container'>
			<span class='nocolum'><%~ VisMsg %> <a href="http://torstatus.blutmagie.de/"><%~ GlbMsg %></a></span>
		</div>

	</fieldset>

	<fieldset id="tor_data_dir_section">
		<legend class="sectionheader"><%~ TDDir %></legend>
		<div>
			<span class="narrowleftcolumn"><%~ TDDir %>:</span>
			<span id="tor_dir_ramdisk_static" class="widerightcolumn">/var/tor</span>
			<span id="tor_dir_root_static" class="widerightcolumn">/usr/lib/tor</span>
			<input type="text" id="tor_dir_text" class="widerightcolumn" style="display:none" />
		</div>
		<div>
			<span class="narrowleftcolumn"><%~ TDDrv %>:</span>
			<select id="tor_dir_drive_select" class="widerightcolumn" onchange="setTorVisibility()" ></select>
		</div>
		<div>
			<div class="widerightcolumnonly">
				<em><%~ CacheWarn %></em>
			</div>
		</div>
	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button"  onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button"  onclick='resetData()'/>
	</div>

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "connection" -p "tor"
%>
