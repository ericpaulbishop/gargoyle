#!/usr/bin/haserl
<%
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "qosupload" -c "internal.css" -j "qos.js table.js" -z "qos.js" qos_gargoyle firewall gargoyle -i qos_gargoyle
%>

<script>
<!--
	var direction = "upload";
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

<h1 class="page-header"><%~ qos.mQUl %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ qos.URSection %></h3>
			</div>
			<div class="panel-body">
				<div id="qos_enabled_container" class="row form-group">
					<span class="col-xs-1"><input type="checkbox" id="qos_enabled" onclick="setQosEnabled()" /></span>
					<label class="col-xs-11" id="qos_enabled_label" for="qos_enabled"><%~ UEnable %></label>
				</div>

				<div class="row form-group">
					<p class="col-xs-12"><%~ QoSAbout %></p>
				</div>
				<div class="internal_divider"></div>

				<div id="qos_rule_table_container" class="bottom_gap table-responsive"></div>
				<div class="row form-group">
					<label class="col-xs-5" id="default_class_label" for="default_class"><%~ ServClass %>:</label>
					<span class="col-xs-7"><select class="form-control" id="default_class"></select></span>
				</div>

				<div id="qos_up_1" class="row form-group">
					<span class="col-xs-12" id="qos_up_1_txt">
						<p><%~ PackAbout %></p>

						<p><%~ DefServClassAbout %></p>

					</span>
					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_up_1')" id="qos_up_1_ref" href="#qos_up_1"><%~ Hide %></a></span>
				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewClassRule %>:</strong></div>
				<div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox" id="use_source_ip" onclick="enableAssociatedField(this,'source_ip', '')" />
							<label id="source_ip_label" for="source_ip"><%~ SrcIP %>:</label>
						</span>
						<span class="col-xs-7"><input class="form-control" type="text" id="source_ip" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" /></span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox" id="use_source_port" onclick="enableAssociatedField(this,'source_port', '')"/>
							<label id="source_port_label" for="source_port"><%~ SrcPort %>:</label>
						</span>
						<span class="col-xs-7"><input type="text" id="source_port" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" /></span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_dest_ip" onclick="enableAssociatedField(this,'dest_ip', '')" />
							<label id="dest_ip_label" for="dest_ip"><%~ DstIP %>:</label>
						</span>
						<span class="col-xs-7"><input type="text" id="dest_ip" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" /></span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_dest_port" onclick="enableAssociatedField(this,'dest_port', '')"  />
							<label id="dest_port_label" for="dest_port"><%~ DstPort %>:</label>
						</span>
						<span class="col-xs-7"><input type="text" id="dest_port" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" /></span>
					</div>

					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_max_pktsize" onclick="enableAssociatedField(this,'max_pktsize', '')"  />
							<label id="max_pktsize_label" for="max_pktsize"><%~ MaxPktLen %>:</label>
						</span>
						<span class="col-xs-7">
							<input type="text" id="max_pktsize" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" />
							<em><%~ byt %></em>
						</span>
					</div>
					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_min_pktsize" onclick="enableAssociatedField(this,'min_pktsize', '')"  />
							<label id="min_pktsize_label" for="min_pktsize"><%~ MinPktLen %>:</label>
						</span>
						<span class="col-xs-7">
							<input type="text" id="min_pktsize" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" />
							<em><%~ byt %></em>
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_transport_protocol" onclick="enableAssociatedField(this,'transport_protocol', '')"  />
							<label id="transport_protocol_label" for="transport_protocol"><%~ TrProto %>:</label>
						</span>
						<span class="col-xs-7">
							<select class="rightcolumn" id="transport_protocol"/>
								<option value="TCP">TCP</option>
								<option value="UDP">UDP</option>
								<option value="ICMP">ICMP</option>
								<option value="GRE">GRE</option>
							</select>
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_connbytes_kb" onclick="enableAssociatedField(this,'connbytes_kb', '')"  />
							<label id="connbytes_kb_label" for="connbytes_kb"><%~ Conreach %>:</label>
						</span>
						<span class="col-xs-7">
							<input class="rightcolumn" type="text" id="connbytes_kb" onkeyup="proofreadNumericRange(this,0,4194303)" size="17" maxlength="28" />
							<em><%~ KBy %></em>
						</span>
					</div>

					<div class="row form-group">
						<span class="col-xs-5">
							<input type="checkbox"  id="use_app_protocol" onclick="enableAssociatedField(this,'app_protocol', '')" />
							<label id="app_protocol_label" for="app_protocol"><%~ AppProto %>:</label>
						</span>
						<span class="col-xs-7">
							<select class="rightcolumn" id="app_protocol">
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
							<input class="form-control" type="text" id="comment_rule" size="17" maxlength="25" />
						</span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" id="classification_label" for="classification" ><%~ SetClass %>:</label>
						<span class="col-xs-7"><select class="rightcolumn" id="classification"></select></span>
					</div>

					<div id="add_rule_container" class="row form-group">
						<span class="col-xs-12"><button id="add_rule_button" class="btn btn-default" onclick="addClassificationRule()" ><%~ AddRule %></button></span>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ UCSection %></h3>
			</div>
			<div class="panel-body">
				<div id="qos_class_table_container" class="bottom_gap table-responsive"></div>

				<div class="row form-group">
					<label class="col-xs-5" id="total_bandwidth_label" for="total_bandwidth"><%~ UTotBand %>:</label>
					<span class="col-xs-7">
						<input type="text" class="rightcolumn" id="total_bandwidth" class="rightcolumn" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" />
						<em><%~ Kbs %></em>
					</span>
				</div>

				<div id="qos_up_2" class="row form-group">
					<span class="col-xs-12" id="qos_up_2_txt">
						<p><%~ USCAbout %></p>

						<p><%~ PerBandAboutU %></p>

						<p><%~ UMinBandAbout %></p>

						<p><%~ MaxBandAbout %></p>

						<p><%~ UTotBandAbout %></p>
					</span>
					<span class="col-xs-12"><a onclick="setDescriptionVisibility('qos_up_2')"  id="qos_up_2_ref" href="#qos_up_2"><%~ Hide %></a></span>
				</div>

				<div class="internal_divider"></div>

				<div><strong><%~ AddNewServiceRule %>:</strong></div>
				<div>
					<div class="row form-group">
						<label class="col-xs-5" id="class_name_label" for="class_name"><%~ SrvClassName %>:</label>
						<span class="col-xs-7"><input type="text" id="class_name" onkeyup="proofreadLengthRange(this,1,10)" size="12" maxlength="10" /></span>
					</div>

					<div class="row form-group">
						<label class="col-xs-5" id="percent_bandwidth_label" for="percent_bandwidth"><%~ PerBandCap %>:</label>
						<span class="col-xs-7">
							<input type="text" id="percent_bandwidth" onkeyup="proofreadNumericRange(this,1,100)" size="5" maxlength="3" />
							<em>%</em>
						</span>
					</div>
					<div><%~ BandMin %>:</div>
					<div class="indent">
						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="min_radio" id="min_radio1" onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" /></span>
							<label class="col-xs-11" for="min_radio1"><%~ BandMinNo %></label>
						</div>
						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="min_radio" id="min_radio2" onclick="enableAssociatedField(document.getElementById('min_radio2'),'min_bandwidth', '')" /></span>
							<span class="col-xs-11">
								<label id="min_bandwidth_label" for="min_radio2"><%~ BandMin %>:</label>
								<input type="text" class="rightcolumn" id="min_bandwidth" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" />
								<em><%~ Kbs %></em>
							</span>
						</div>
					</div>

					<div><%~ BandMax %>:</div>
					<div class="indent">
						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="max_radio" id="max_radio1" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" /></span>
							<label class="col-xs-11" for="max_radio1"><%~ BandMaxNo %></label>
						</div>
						<div class="row form-group">
							<span class="col-xs-1"><input type="radio" name="max_radio" id="max_radio2" onclick="enableAssociatedField(document.getElementById('max_radio2'),'max_bandwidth', '')" /></span>
							<span class="col-xs-11">
								<label id="max_bandwidth_label" for="max_radio2"><%~ BandMax %>:</label>
								<input type="text" class="rightcolumn" id="max_bandwidth" onkeyup="proofreadNumeric(this)"  size="10" maxlength="10" />
								<em><%~ Kbs %></em>
							</span>
						</div>
					</div>

					<div class="row form-group" id="add_class_container">
						<span class="col-xs-12"><button id="add_class_button" class="btn btn-default" onclick="addServiceClass()"><%~ AddSvcCls %></button></span>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-info btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>
<span id="update_container"><%~ WaitSettings %></span>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id="output"></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "qosupload"
%>
