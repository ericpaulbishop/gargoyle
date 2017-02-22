#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m -c "internal.css" -j "qos.js" -z "qos.js"
%>
<div id="edit_container" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ qos.QESrvClass %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" id="class_name_label" for="class_name" ><%~ QESrvName %>:</label>
					<span class="col-xs-7"><input class="form-control" type="text" id="class_name" onkeyup="proofreadLengthRange(this,1,10)"  size="12" maxlength="10" /></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="percent_bandwidth_label" for="percent_bandwidth" ><%~ PerBandCap %>:</label>
					<span class="col-xs-7"><input type="text" id="percent_bandwidth" class="form-control" onkeyup="proofreadNumericRange(this,1,100)" size="5" maxlength="3" /><em>%</em></span>
				</div>

				<div><label><%~ BandMin %>:</label></div>
				<div class="indent">
					<div class="row form-group">
						<span class="col-xs-1"><input type="radio" name="min_radio" id="min_radio1"  onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" /></span>
						<label class="col-xs-11" for="min_radio1"><%~ BandMinNo %></label>
					</div>

					<div class="row form-group">
						<span class="col-xs-1"><input type="radio" name="min_radio"  id="min_radio2" onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" /></span>
						<span class="col-xs-11">
							<label id="min_bandwidth_label" for="min_radio2"><%~ BandMin %>:</label>
							<input type="text" class="form-control" id="min_bandwidth" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" />
							<em><%~ Kbs %></em>
						</span>
					</div>
				</div>

				<br/>
				<div><label><%~ BandMax %>:</label></div>
				<div class="indent">
					<div class="row form-group">
						<span class="col-xs-1"><input type="radio" name="max_radio"  id="max_radio1" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" /></span>
						<label class="col-xs-11" for="max_radio1"><%~ BandMaxNo %></label>
					</div>

					<div class="row form-group">
						<span class="col-xs-1"><input type="radio" name="max_radio"  id="max_radio2" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" /></span>
						<span class="col-xs-11">
							<label id="max_bandwidth_label" for="max_radio2"><%~ BandMax %>:</label>
							<input type="text" class="form-control" id="max_bandwidth" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" />
							<em><%~ Kbs %></em>
						</span>
					</div>
				</div>

				<div id="rttdiv">
					<br>
					<div><label><%~ MinRTT %>:</label></div>
					<div class="indent">
						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="rtt_radio" id="rtt_radio1"/></span>
							<label class="col-xs-11" for="rtt_radio1"><%~ ActRTT %></label>
						</div>

						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="rtt_radio" id="rtt_radio2" /></span>
							<label class="col-xs-11" for="rtt_radio2"><%~ OptiWAN %></label>
						</div>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default"></div>

<%
	echo '</body>'
	echo '</html>'
%>
