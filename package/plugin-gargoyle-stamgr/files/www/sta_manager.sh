#!/usr/bin/haserl
<%
	# This program is copyright © 2019 Michael Gray and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "connection" -p "stamanager" -j "table.js sta_manager.js" -z "sta_manager.js" -i wireless gargoyle_stamgr
%>

<script>
<!--
<%
		echo "var radioList = [];"
		radio0=$(uci -q get wireless.radio0.hwmode)
		if [ "$radio0" = "11g" ] ; then
			echo "radioList['radio0'] = '2.4GHz';"
		elif [ "$radio0" = "11a" ] ; then
			echo "radioList['radio0'] = '5GHz';"
		else
			echo "radioList['radio0'] = '';"
		fi
		radio1=$(uci -q get wireless.radio1.hwmode)
		if [ "$radio1" = "11g" ] ; then
			echo "radioList['radio1'] = '2.4GHz';"
		elif [ "$radio1" = "11a" ] ; then
			echo "radioList['radio1'] = '5GHz';"
		else
			echo "radioList['radio1'] = '';"
		fi
		
		if [ -e "/tmp/gargoyle_stamgr.json" ] ; then
			json="$(cat /tmp/gargoyle_stamgr.json)"
		fi
		echo "var runtime_status = '$json';"
%>
//-->
</script>
<style>
	.wireless_station_table_column_6
	{
		display: none;
	}
</style>
<h1 class="page-header"><%~ sta_manager.mStaMgr %></h1>
<div class="row">
	<div class="col-lg-6">
			<div class="panel panel-default">
				<div class="panel-heading">
					<h3 class="panel-title"><%~ StaMgrSettings %></h3>
				</div>
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="enablestamgr"/>
						<label id="enablestamgr_label" for="enablestamgr"><%~ StaMgrEn %></label>
					</span>
				</div>
				
				<div class="row form-group">
					<label class="col-xs-5" for="maxretry" id="maxretry_label"><%~ MaxConnAttempts %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id="maxretry"/></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="maxwait" id="maxwait_label"><%~ MaxWait %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id="maxwait"/><em> (<%~ seconds %>)</em></span>
				</div>
				
				<div class="row form-group">
					<label class="col-xs-5" for="disconnectqualthresh" id="disconnectqualthresh_label"><%~ DisconnectQualThresh %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id="disconnectqualthresh"/><em> %</em></span>
				</div>
				
				<div class="row form-group">
					<label class="col-xs-5" for="connectqualthresh" id="connectqualthresh_label"><%~ ConnectQualThresh %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id="connectqualthresh"/><em> %</em></span>
				</div>
				
				<div class="row form-group">
					<label class="col-xs-5" for="blacklisttimer" id="blacklisttimer_label"><%~ BlacklistTimer %>:</label>
					<span class="col-xs-7"><input type="text" class="form-control" id="blacklisttimer"/><em> (<%~ seconds %>)</em></span>
				</div>
				
				<div class="row form-group">
					<span class="col-xs-12"><button id="add_button" class="btn btn-default btn-add" onclick="addAPModal()"><%~ Add %> AP</button></span>
				</div>
			</div>
		</div>
	</div>
	
	<div class="col-lg-6">
		<div class="panel panel-default" id="runtimepanelheader">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Status %></h3>
			</div>
			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ LastUpdate %>:</span>
						<span id="lastupdate"></span>
					</li>
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ CurrAP %>:</span>
						<span id="currentsection"></span>
					</li>
					<div class="indent" id="currentsection_div">
						<li class="list-group-item">
							<span class="list-group-item-title"><%~ Connected %>:</span>
							<span id="connected"></span>
						</li>
						<li class="list-group-item">
							<span class="list-group-item-title">ID:</span>
							<span id="currentid"></span>
						</li>
						<li class="list-group-item">
							<span class="list-group-item-title"><%~ Band %>:</span>
							<span id="currentradio"></span>
						</li>
						<li class="list-group-item">
							<span class="list-group-item-title">SSID:</span>
							<span id="currentssid"></span>
						</li>
						<li class="list-group-item">
							<span class="list-group-item-title">BSSID:</span>
							<span id="currentbssid"></span>
						</li>
						<li class="list-group-item">
							<span class="list-group-item-title"><%~ Encryption %>:</span>
							<span id="currentencryption"></span>
						</li>
					</div>
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ BlacklistAP %></span>
						<span id="blacklistsection"></span>
					</li>
					<div class="indent" id="blacklistsection_div">
					</div>
				</ul>
			</div>
		</div>
	</div>
</div>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ WirelessAPs %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<div id="wireless_station_container" class="table-responsive col-xs-12"></div>
				</div>
			</div>
		</div>
	</div>
</div>
<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="stamgr_ap_modal" aria-hidden="true" aria-labelledby="static_ip_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="stamgr_ap_modal_title" class="panel-title"><%~ AddAP %></h3>
			</div>
			<div class="modal-body">
				<%in templates/stamgr_ap_template %>
			</div>
			<div class="modal-footer" id="stamgr_ap_modal_button_container">
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
	gargoyle_header_footer -f -s "connection" -p "stamanager"
%>

