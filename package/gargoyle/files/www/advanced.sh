#!/usr/bin/haserl
<%
	# This program is copyright © 2022 Mcihael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "advanced" -j "table.js advanced.js" -z "advanced.js basic.js" -i wireless usteer network
%>
<script>
<!--
<%
	if [ -e /tmp/cached_detected_country ] ; then
		echo "var geoip=\"$(sed -n 's/ip: //p' /tmp/cached_detected_country)\";"
		echo "var geocode=\"$(sed -n 's/country_code: //p' /tmp/cached_detected_country)\";"
	else
		echo "var geoip='';"
		echo "var geocode='';"
	fi

	echo "var countryLines = new Array();"
	if [ -e ./data/countrylist.txt ] ; then
		awk '{gsub(/"/, "\\\""); print "countryLines.push(\""$0"\");"}' ./data/countrylist.txt	
	fi

	echo "var num_cpus=$(grep -c processor /proc/cpuinfo);"
%>
//-->
</script>
<h1 class="page-header"><%~ advanced.mAdvanced %></h1>
<div id="no_settings" style="display:none;" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-body">
				<em><span><%~ NoSet %></span></em>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6" id="wireless_container" style="display:none">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ basic.Wrlss %></h3>
			</div>
			<div class="panel-body">
				<div id="wireless_country_container" class="row form-group">
					<span class="alert alert-warning col-xs-12" role="alert"><%~ WifiCountryWarning %></span>
					<label class="col-xs-5" for="wireless_country"><%~ WifiCountry %>:</label>
					<div class="col-xs-7">
						<select class="form-control" id="wireless_country"></select>
					</div>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6" id="wan_container" style="display:none">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ basic.WANSec %></h3>
			</div>
			<div class="panel-body">
				<div id="modem_network_container" class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_modem_network" onclick="enableAssociatedField(this, 'modem_network_ip', '192.168.0.2');enableAssociatedField(this, 'modem_network_mask', '255.255.255.0')">
						<label class="short-left-pad" for="use_modem_network" id="modem_network_ip_label"><%~ ModemNet %>:</label>
					</span>
					<span class="col-xs-7"><input type="text" name="modem_network_ip" id="modem_network_ip" class="form-control disabled" oninput="proofreadIp(this)" disabled><em><%~ ModemIP %></em></span>
					<span class="col-xs-offset-5 col-xs-7"><input type="text" name="modem_network_mask" id="modem_network_mask" class="form-control disabled" oninput="proofreadMask(this)" disabled><em><%~ ModemSMsk %></em></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6" id="lan_container" style="display:none">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ basic.LANSec %></h3>
			</div>
			<div class="panel-body">
				<div id="usteer_comms_container" class="row form-group">
					<label class="col-xs-5" for="usteer_comms"><%~ ShrAPClnts %>:</label>
					<div class="col-xs-7">
						<select class="form-control" id="usteer_comms">
							<option value="0"><%~ Disabled %></option>
							<option value="1"><%~ Enabled %></option>
						</select>
					</div>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6" id="netopt_container" style="display:none">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ NetOptTitle %></h3>
			</div>
			<div class="panel-body">
				<div id="pktsteer_opt_container" class="row form-group">
					<label class="col-xs-5" for="pktsteer_opt"><%~ PktSteer %>:</label>
					<div class="col-xs-7">
						<select class="form-control" id="pktsteer_opt">
							<option value="0"><%~ Disabled %></option>
							<option value="1"><%~ Enabled %></option>
							<option value="2"><%~ Enabled (All CPUs) %></option>
						</select>
					</div>
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
<!--
	resetData();
//-->
</script>
<%
	gargoyle_header_footer -f -s "connection" -p "advanced"
%>

