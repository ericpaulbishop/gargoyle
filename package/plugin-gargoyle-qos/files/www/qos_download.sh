#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "qosdownload" -j "qos.js table.js" -z "qos.js" -i firewall gargoyle qos_gargoyle
%>


<script>
<!--
	var direction = "download";
	protocolMap = new Object;
<%
	sed -e '/^#/ d' -e 's/\([^ ]*\) \(.*\)/protocolMap\["\1"\]="\2";/' /etc/l7-protocols/l7index

	if [ -h /etc/rc.d/S50qos_gargoyle ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi
%>

//-->
</script>

<h1 class="page-header"><%~ qos.mQDl %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ qos.DRSection %></h3>
			</div>

			<div class="panel-body">
				<div id="qos_enabled_container" class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="qos_enabled" onclick="setQosEnabled()" />
						<label id="qos_enabled_label" for="qos_enabled"><%~ DEnable %></label>
					</span>
				</div>

				<div class="row form-group">
					<p class="col-xs-12"><%~ QoSAbout %></p>
				</div>

				<div class="internal_divider"></div>

				<div id="qos_rule_table_container" class="bottom_gap table-responsive"></div>

				<div class="row form-group">
					<label class="col-xs-5" id="default_class_label" for="default_class"><%~ ServClass %>:</label>
					<span class="col-xs-7"><select id="default_class" class="form-control"></select></span>
				</div>

				<div id="qos_down_1" class="row form-group">
					<span class="col-xs-12"><a id="qos_down_1_ref" href="javascript:setDescriptionVisibility('qos_down_1')"><%~ Hide %></a></span>
					<span class="col-xs-12" id="qos_down_1_txt">
						<p><%~ PackAbout %></p>

						<p><%~ DefServClassAbout %></p>

					</span>
				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewClassRule %>:</strong></div>
				<div id="add_rule_container" class="row form-group">
					<span class="col-xs-12"><button id="add_rule_button" class="btn btn-default btn-add" onclick="addRuleModal()" ><%~ AddRule %></button></span>
				</div>
			</div>
		</div>
	</div>
</div>


<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DCSection %></h3>
			</div>

			<div class="panel-body">
				<div id="qos_class_table_container" class="bottom_gap table-responsive"></div>

				<div class="row form-group">
					<label class="col-xs-5" id="total_bandwidth_label" for="total_bandwidth"><%~ DTotBand %>:</label>
					<span class="col-xs-7">
						<input type="text" id="total_bandwidth" class="form-control" oninput="proofreadNumeric(this)" size="10" maxlength="10" />
						<em><%~ Kbs %></em>
					</span>
				</div>

				<div id="qos_down_2" class="row form-group">
					<span class="col-xs-12"><a id="qos_down_2_ref" href="javascript:setDescriptionVisibility('qos_down_2')"><%~ Hide %></a></span>
					<span id="qos_down_2_txt" class="col-xs-12">
						<p><%~ DSCAbout %></p>

						<p><%~ PerBandAboutD %></p>

						<p><%~ DMinBandAbout %></p>

						<p><%~ MaxBandAbout %></p>

						<p><%~ RTTAbout %></p>

						<p><%~ DTotBandAbout %></p>

						<p><%~ MinSpeedWarn %></p>
					</span>

				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewServiceRule %>:</strong></div>
				<div class="row form-group" id="add_class_container">
					<span class="col-xs-12"><button id="add_class_button" class="btn btn-default btn-add" onclick="addClassModal()" ><%~ AddSvcCls %></button></span>
				</div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DACCSect %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group" id="qos_monitor_container">
					<span class="col-xs-12">
						<input type="checkbox" id="qos_monenabled" onclick="setQosEnabled()"/>
						<label id="qos_monenabled_label" for="qos_monenabled"><%~ ACCOn %></label>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-6">
						<input type="checkbox" id="use_ptarget_ip" onclick="enableAssociatedField(this, 'ptarget_ip', currentWanGateway)"/>
						<label for="use_ptarget_ip" id="ptarget_ip_label"><%~ ACC_Pt %>:</label>
					</span>
					<span class="col-xs-6">
						<input type="text" name="ptarget_ip" id="ptarget_ip" class="form-control" oninput="proofreadIpRange(this)" size="17" maxlength="31" aria-labelledby="ptarget_ip_label"/>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-6">
						<input type="checkbox" id="use_auto_pinglimit" onclick="enableAssociatedField(this, 'pinglimit', 85)"/>
						<label for="use_auto_pinglimit" id="pinglimit_label"><%~ ACC_con %>:</label>
					</span>
					<span class="col-xs-6">
						<input type="text" name="pinglimit" id="pinglimit" class="form-control" oninput="proofreadNumericRange(this, 10, 250)" size="4" maxlength="4" aria-labelledby="pinglimit_label"/>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<button id="reset_FLL" class="btn btn-default" onclick="resetFairLinkLimit()"><%~ resetFLL %></button>
					</span>
				</div>

				<div id="qos_down_3" class="row form-group">
					<span class="col-xs-12"><a id="qos_down_3_ref" href="javascript:setDescriptionVisibility('qos_down_3')"><%~ Hide %></a></span>
					<span class="col-xs-12" id="qos_down_3_txt">
						<%~ AbACC %>
					</span>
				</div>

				<div class="internal_divider"></div>

				<div class="row form-group">
					<span class="col-xs-6">
						<table class="table table-responsive">
							<tr><td><strong><%~ ACC_Stat %></strong></td></tr>
							<tr><td><span id="qstate"></span></td></tr>
							<tr><td><span id="qllimit"></span></td></tr>
							<tr><td><span id="qollimit"></span></td></tr>
							<tr><td><span id="qload"></span></td></tr>
							<tr><td><span id="qpinger"></span></td></tr>
							<tr><td><span id="qpingtime"></span></td></tr>
							<tr><td><span id="qpinglimit"></span></td></tr>
							<tr><td><span id="qactivecnt"></span></td></tr>
						</table>
					</span>
				</div>

				<div class="row form-group" id="qos_down_4">
					<span class="col-xs-12"><a id="qos_down_4_ref" href="javascript:setDescriptionVisibility('qos_down_4')"><%~ Hide %></a></span>
					<span class="col-xs-8" id="qos_down_4_txt">
						<table class="table table-responsive">
							<tr><td><strong>Status Help</strong></td></tr>
							<tr><td>CHECK</td><td><%~ ACC_L_Ck %></td></tr>
							<tr><td>INIT</td><td><%~ ACC_L_In %></td></tr>
							<tr><td>ACTIVE</td><td><%~ ACC_L_Act %></td></tr>
							<tr><td>MINRTT</td><td><%~ ACC_L_Min %></td></tr>
							<tr><td>IDLE</td><td><%~ ACC_L_Id %></td></tr>
							<tr><td>DISABLE</td><td><%~ ACC_L_Dis %></td></tr>
							<tr><td>Link Limit</td><td><%~ ACC_L_Lim %></td></tr>
							<tr><td>Fair Link Limit</td><td><%~ ACC_L_Fr %></td></tr>
							<tr><td>Link Load</td><td><%~ ACC_L_Ld %></td></tr>
							<tr><td>Ping</td><td><%~ ACC_L_pg %></td></tr>
							<tr><td>Filtered Ping</td><td><%~ ACC_L_Flt %></td></tr>
							<tr><td>Ping Time Limit</td><td><%~ ACC_L_plim %></td></tr>
							<tr><td>Active Classes</td><td><%~ ACC_L_AC %></td></tr>
						</table>
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

<div class="modal fade" tabindex="-1" role="dialog" id="qos_rule_modal" aria-hidden="true" aria-labelledby="qos_rule_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="qos_rule_modal_title" class="panel-title"><%~ AddNewClassRule %></h3>
			</div>
			<div class="modal-body">
				<%in templates/qos_rule_template %>
			</div>
			<div class="modal-footer" id="qos_rule_modal_button_container">
			</div>
		</div>
	</div>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="qos_class_modal" aria-hidden="true" aria-labelledby="qos_class_modal_title">
	<div class="modal-dialog" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="qos_class_modal_title" class="panel-title"><%~ AddNewServiceRule %></h3>
			</div>
			<div class="modal-body">
				<%in templates/qos_class_template %>
			</div>
			<div class="modal-footer" id="qos_class_modal_button_container">
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
	gargoyle_header_footer -f -s "firewall" -p "qosdownload"
%>
