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
					<span class="col-xs-12" id="qos_down_1_txt">
						<p><%~ PackAbout %></p>

						<p><%~ DefServClassAbout %></p>

					</span>
					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_down_1')"  id="qos_down_1_ref" href="#qos_down_1"><%~ Hide %></a></span>
				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewClassRule %>:</strong></div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_source_ip" onclick="enableAssociatedField(this,'source_ip', '')" />
						<label id="source_ip_label" for="use_source_ip"><%~ SrcIP %>:</label>
					</span>
					<span class="col-xs-7"><input class="form-control" type="text" id="source_ip" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" aria-labelledby="source_ip_label"/></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_source_port" onclick="enableAssociatedField(this,'source_port', '')"/>
						<label id="source_port_label" for="use_source_port"><%~ SrcPort %>:</label>
					</span>
					<span class="col-xs-7"><input class="form-control" type="text" id="source_port" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" aria-labelledby="source_port_label"/></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_dest_ip" onclick="enableAssociatedField(this,'dest_ip', '')" />
						<label id="dest_ip_label" for="use_dest_ip"><%~ DstIP %>:</label>
					</span>
					<span class="col-xs-7"><input class="form-control" type="text" id="dest_ip" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" aria-labelledby="dest_ip_label"/></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_dest_port" onclick="enableAssociatedField(this,'dest_port', '')"  />
						<label id="dest_port_label" for="use_dest_port"><%~ DstPort %>:</label>
					</span>
					<span class="col-xs-7"><input class="form-control" type="text" id="dest_port" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" aria-labelledby="dest_port_label"/></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_max_pktsize" onclick="enableAssociatedField(this,'max_pktsize', '')"  />
						<label id="max_pktsize_label" for="use_max_pktsize"><%~ MaxPktLen %>:</label>
					</span>
					<span class="col-xs-7">
						<input type="text" id="max_pktsize" class="form-control" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" aria-labelledby="max_pktsize_label"/>
						<em><%~ byt %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_min_pktsize" onclick="enableAssociatedField(this,'min_pktsize', '')"  />
						<label id="min_pktsize_label" for="use_min_pktsize"><%~ MinPktLen %>:</label>
					</span>
					<span class="col-xs-7">
						<input type="text" id="min_pktsize" class="form-control" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" aria-labelledby="min_pktsize_label"/>
						<em><%~ byt %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_transport_protocol" onclick="enableAssociatedField(this,'transport_protocol', '')"  />
						<label id="transport_protocol_label" for="use_transport_protocol"><%~ TrProto %>:</label>
					</span>
					<span class="col-xs-7">
						<select id="transport_protocol" class="form-control" aria-labelledby="transport_protocol_label">
							<option value="TCP">TCP</option>
							<option value="UDP">UDP</option>
							<option value="ICMP">ICMP</option>
							<option value="GRE">GRE</option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_connbytes_kb" onclick="enableAssociatedField(this,'connbytes_kb', '')"  />
						<label id="connbytes_kb_label" for="use_connbytes_kb"><%~ Conreach %>:</label>
					</span>
					<span class="col-xs-7">
						<input class="form-control" type="text" id="connbytes_kb" onkeyup="proofreadNumericRange(this,0,4194303)" size="17" maxlength="28" aria-labelledby="connbytes_kb_label"/>
						<em><%~ KBy %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_app_protocol" onclick="enableAssociatedField(this,'app_protocol', '')" />
						<label id="app_protocol_label" for="use_app_protocol"><%~ AppProto %>:</label>
					</span>
					<span class="col-xs-7">
						<select id="app_protocol" class="form-control" aria-labelledby="app_protocol_label">
						<%
						sed -e "s/#.*//" -e "s/\([^ ]* \)\(.*\)/<option value='\1'>\2<\/option>/" /etc/l7-protocols/l7index
						%>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_comment_rule" onclick="enableAssociatedField(this,'comment_rule', '')"  />
						<label id="comment_rule_label" for="use_comment_rule"><%~ Comment %>:</label>
					</span>
					<span class="col-xs-7">
						<input class="form-control" type="text" id="comment_rule" size="17" maxlength="25" aria-labelledby="comment_rule_label"/>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="classification_label" for="classification" ><%~ SetClass %>:</label>
					<span class="col-xs-7"><select id="classification" class="form-control"></select></span>
				</div>


				<div id="add_rule_container" class="row form-group">
					<span class="col-xs-12"><button id="add_rule_button" class="btn btn-default btn-add" onclick="addClassificationRule()" ><%~ AddRule %></button></span>
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
						<input type="text" id="total_bandwidth" class="form-control" onkeyup="proofreadNumeric(this)" size="10" maxlength="10" />
						<em><%~ Kbs %></em>
					</span>
				</div>

				<div id="qos_down_2" class="row form-group">
					<span id="qos_down_2_txt" class="col-xs-12">
						<p><%~ DSCAbout %></p>

						<p><%~ PerBandAboutD %></p>

						<p><%~ DMinBandAbout %></p>

						<p><%~ MaxBandAbout %></p>

						<p><%~ RTTAbout %></p>

						<p><%~ DTotBandAbout %></p>

						<p><%~ MinSpeedWarn %></p>
					</span>

					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_down_2')"  id="qos_down_2_ref" href="#qos_down_2"><%~ Hide %></a></span>
				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewServiceRule %>:</strong></div>

				<div class="row form-group">
					<label class="col-xs-5" id="class_name_label" for="class_name"><%~ SrvClassName %>:</label>
					<span class="col-xs-7"><input type="text" id="class_name" class="form-control" onkeyup="proofreadLengthRange(this,1,10)" size="12" maxlength="10" /></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="percent_bandwidth_label" for="percent_bandwidth"><%~ PerBandCap %>:</label>
					<span class="col-xs-7">
						<input type="text" id="percent_bandwidth" class="form-control" onkeyup="proofreadNumericRange(this,1,100)"  size="5" maxlength="3" />
						<em>%</em>
					</span>
				</div>

				<div><%~ BandMin %>:</div>
				<div class="indent">
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" name="min_radio" id="min_radio1" onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" />
							<label for="min_radio1"><%~ BandMinNo %></label>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="radio" name="min_radio" id="min_radio2" onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" />
							<label id="min_bandwidth_label" for="min_radio2"><%~ BandMin %>:</label>
						</span>

						<span class="col-xs-7">
							<input type="text" id="min_bandwidth" class="form-control" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" aria-labelledby="min_bandwidth_label"/>
							<em><%~ Kbs %></em>
						</span>
					</div>
				</div>

				<div><%~ BandMax %>:</div>
				<div class="indent">
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" name="max_radio" id="max_radio1" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" />
							<label for="max_radio1"><%~ BandMaxNo %></label>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="radio" name="max_radio" id="max_radio2" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" />
							<label id="max_bandwidth_label" for="max_radio2"><%~ BandMax %>:</label>
						</span>
						<span class="col-xs-7">
							<input type="text" id="max_bandwidth" class="form-control" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" aria-labelledby="max_bandwidth_label"/>
							<em><%~ Kbs %></em>
						</span>
					</div>
				</div>

				<div><%~ MinRTT %>:</div>
				<div class="indent">
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" name="rtt_radio" id="rtt_radio1"/>
							<label for="rtt_radio1"><%~ ActRTT %></label>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-12">
							<input type="radio" name="rtt_radio" id="rtt_radio2" />
							<label for="rtt_radio2"><%~ OptiWAN %></label>
						</span>
					</div>
				</div>

				<div class="row form-group" id="add_class_container">
					<span class="col-xs-12"><button id="add_class_button" class="btn btn-default btn-add" onclick="addServiceClass()" ><%~ AddSvcCls %></button></span>
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
						<input type="text" name="ptarget_ip" id="ptarget_ip" class="form-control" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" aria-labelledby="ptarget_ip_label"/>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-6">
						<input type="checkbox" id="use_auto_pinglimit" onclick="enableAssociatedField(this, 'pinglimit', 85)"/>
						<label for="use_auto_pinglimit" id="pinglimit_label"><%~ ACC_con %>:</label>
					</span>
					<span class="col-xs-6">
						<input type="text" name="pinglimit" id="pinglimit" class="form-control" onkeyup="proofreadNumericRange(this, 10, 250)" size="4" maxlength="4" aria-labelledby="pinglimit_label"/>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-12">
						<button id="reset_FLL" class="btn btn-default" onclick="resetFairLinkLimit()"><%~ resetFLL %></button>
					</span>
				</div>

				<div id="qos_down_3" class="row form-group">
					<span class="col-xs-12" id="qos_down_3_txt">
						<%~ AbACC %>
					</span>
					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_down_3')" id="qos_down_3_ref" href="#qos_down_3"><%~ Hide %></a></span>
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
					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_down_4')"  id="qos_down_4_ref" href="#qos_down_4"><%~ Hide %></a></span>
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
	gargoyle_header_footer -f -s "firewall" -p "qosdownload"
%>
