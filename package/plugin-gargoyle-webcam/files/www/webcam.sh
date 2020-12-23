#!/usr/bin/haserl
<%
	# This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "webcam" -j "webcam.js" -z "webcam.js" mjpg-streamer firewall dropbear uhttpd
%>
<%in templates/client_server_template %>
<script>
var webcams = [];
<!--
<%
	devices=$(ls -1 /sys/class/video4linux 2>/dev/null)

	for d in $devices; do
		echo "webcams['/dev/$d'] = [];"
		echo "webcams['/dev/$d']['res'] = [];"
		webcaminfo -d "/dev/$d"
	done
%>
//-->
</script>

<h1 class="page-header"><%~ webcam.WebC %></h1>
<div id="nowebcam" class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ webcam.WebC %></h3>
			</div>

			<div class="panel-body">
				<em><span><%~ NoWebC %>.</span></em>
			</div>
		</div>
	</div>
</div>

<div id="webcam" class="row">
	<div id="webcam_fieldset" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ webcam.WebC %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="webcam_enable" type="checkbox" onchange='updateWebcamWanAccess()'/>
						<label id="webcam_enable_label" for="webcam_enable"><%~ EnWebC %></label>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input id="webcam_wan_access" type="checkbox"/>
						<label id="webcam_wan_access_label" for="webcam_wan_access"><%~ EnRAWebC %>:</label>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="webcam_device_label" for="webcam_device"><%~ WebCDev %>:</label>
					<span class="col-xs-7"><select class="form-control" id='webcam_device' onchange='fillRes(this.value)'></select></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id='webcam_res_label' for='webcam_res'><%~ WebCRes %>:</label>
					<span class="col-xs-7"><select class="form-control" id='webcam_res'></select></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="webcam_fps_label" for="webcam_fps"><%~ WebCFPS %>:</label>
					<span class="col-xs-7"><input id="webcam_fps" class="form-control" type="text" size='20' maxlength='2' oninput='proofreadNumericRange(this,1,59)'/></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<input id="webcam_yuv" type="checkbox"/>
						<label id="webcam_yuv_label" for="webcam_yuv"><%~ WebCYUYV %>:</label>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="webcam_port_label" for="webcam_port"><%~ WebCPort %>:</label>
					<span class="col-xs-7"><input id="webcam_port" class="form-control" type="text" size='20' maxlength='5' oninput='proofreadPort(this)'/></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="webcam_username_label" for="webcam_username"><%~ WebCUName %>:</label>
					<span class="col-xs-7">
						<input id="webcam_username" class="form-control" type="text" size='20'/>
						<em>(<%~ WebCOpt %>)</em>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="webcam_password_label" for="webcam_password"><%~ WebCPass %>:</label>
					<span class="col-xs-7">
						<input id="webcam_password" class="form-control" type="text" size='20'/>
						<em>(<%~ WebCOpt %>)</em>
					</span>
				</div>
			</div>
		</div>
	</div>
	<div id="webcam_preview" class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ PrevWebC %></h3>
			</div>

			<div class="panel-body text-center">
				<div class="row form-group">
					<span class="col-xs-12">
						<a id="webcam_snapshot" href="" target="_blank"><%~ WebCSnapshot %></a>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="webcam_snapshot_url" class="form-control text-center" style="width: 100%" type="text" readonly="readonly" onclick="copy(this)"/>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<a id="webcam_stream" href="" target="_blank"><%~ WebCStream %></a>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input id="webcam_stream_url" class="form-control text-center" style="width: 100%" type="text" readonly="readonly" onclick="copy(this)"/>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<a id="webcam_frame" href="" target="_blank">
							<img alt="<%~ PrevWebC %>" src="" style="max-width: 100%"/>
						</a>
						<div id="webcam_alert" class="alert alert-info" role="alert">
							<%~ WebCAlert %>
						</div>
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
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "webcam"
%>
