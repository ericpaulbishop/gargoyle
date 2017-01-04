#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "qosdownload" -c "internal.css" -j "qos.js table.js" -z "qos.js" qos_gargoyle firewall gargoyle dhcp -i qos_gargoyle
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

<form>
	<fieldset>
		<legend class="sectionheader"><%~ qos.DRSection %></legend>

		<div id='qos_enabled_container' class='nocolumn'>
			<input type='checkbox' id='qos_enabled' onclick="setQosEnabled()" />
			<label id='qos_enabled_label' for='qos_enabled'><%~ DEnable %></label>
		</div>
		<div class="indent">
			<p><%~ QoSAbout %>
			</p>
		</div>
		<div class="internal_divider"></div>

		<div id='qos_rule_table_container' class="bottom_gap"></div>
		<div>
			<label class="leftcolumn" id="default_class_label" for="default_class"><%~ ServClass %>:</label>
			<select class="rightcolumn" id="default_class"></select>
		</div>

		<div id="qos_down_1" class="indent">
			<span id='qos_down_1_txt'>
				<p><%~ PackAbout %></p>

				<p><%~ DefServClassAbout %></p>

			</span>
			<a onclick='setDescriptionVisibility("qos_down_1")'  id="qos_down_1_ref" href="#qos_down_1"><%~ Hide %></a>
		</div>


		<div class="internal_divider">
			<p>
		</div>

		<div><strong><%~ AddNewClassRule %>:</strong></div>
		<div class="indent">
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_source_ip' onclick='enableAssociatedField(this,"source_ip", "")' />
					<label id="source_ip_label" for='source_ip'><%~ SrcIP %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='source_ip' onkeyup='proofreadIpRange(this)' size='17' maxlength='31' />
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_source_port' onclick='enableAssociatedField(this,"source_port", "")'/>
					<label id="source_port_label" for='source_port'><%~ SrcPort %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='source_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_dest_ip' onclick='enableAssociatedFields(this,["dest_ip","group"], ["",""])' />
					<label id="dest_ip_label" for='dest_ip'><%~ DstIPor %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='dest_ip' onkeyup='proofreadIpRangeOrGroup(this)' size='17' maxlength='31' />
				<span id="dest_group_container">
					<select id="group" disabled onchange="groupSelected('dest_ip')">
						<option value=""><%~ SelGrp %></option>
					</select>
				</span>
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_dest_port' onclick='enableAssociatedField(this,"dest_port", "")'  />
					<label id="dest_port_label" for='dest_port'><%~ DstPort %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='dest_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_max_pktsize' onclick='enableAssociatedField(this,"max_pktsize", "")'  />
					<label id="max_pktsize_label" for='max_pktsize'><%~ MaxPktLen %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='max_pktsize' onkeyup='proofreadNumericRange(this,1,1500)' size='17' maxlength='4' />
				<em><%~ byt %></em>
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_min_pktsize' onclick='enableAssociatedField(this,"min_pktsize", "")'  />
					<label id="min_pktsize_label" for='min_pktsize'><%~ MinPktLen %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='min_pktsize' onkeyup='proofreadNumericRange(this,1,1500)' size='17' maxlength='4' />
				<em><%~ byt %></em>
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_transport_protocol' onclick='enableAssociatedField(this,"transport_protocol", "")'  />
					<label id="transport_protocol_label" for='transport_protocol'><%~ TrProto %>:</label>
				</div>
				<select class='rightcolumn' id="transport_protocol"/>
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
					<option value="ICMP">ICMP</option>
					<option value="GRE">GRE</option>
				</select>
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_connbytes_kb' onclick='enableAssociatedField(this,"connbytes_kb", "")'  />
					<label id="connbytes_kb_label" for='connbytes_kb'><%~ Conreach %>:</label>
				</div>
				<input class='rightcolumn' type='text' id='connbytes_kb' onkeyup='proofreadNumericRange(this,0,4194303)' size='17' maxlength='28' />
				<em><%~ KBy %></em>
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_app_protocol' onclick='enableAssociatedField(this,"app_protocol", "")' />
					<label id="app_protocol_label" for='app_protocol'><%~ AppProto %>:</label>
				</div>
				<select class='rightcolumn' id="app_protocol">
				<%
				sed -e "s/#.*//" -e "s/\([^ ]* \)\(.*\)/<option value='\1'>\2<\/option>/" /etc/l7-protocols/l7index
				%>
				</select>
			</div>

			<div>
				<label class='leftcolumn' id="classification_label" for='classification' ><%~ SetClass %>:</label>
				<select class='rightcolumn' id="classification">
				</select>
			</div>


			<div id="add_rule_container">
				<input type="button" id="add_rule_button" class="default_button" value="<%~ AddRule %>" onclick="addClassificationRule()" />
			</div>
	</div>
	</fieldset>

	<fieldset>

		<legend class="sectionheader"><%~ DCSection %></legend>
		<div id='qos_class_table_container' class="bottom_gap"></div>

		<div>
			<label class="leftcolumn" id="total_bandwidth_label" for="total_bandwidth"><%~ DTotBand %>:</label>
			<input type="text" class="rightcolumn" id="total_bandwidth" class="rightcolumn" onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
			<em><%~ Kbs %></em>

		</div>

		<div id="qos_down_2" class="indent">
			<span id='qos_down_2_txt'>
				<p><%~ DSCAbout %></p>

				<p><%~ PerBandAboutD %></p>

				<p><%~ DMinBandAbout %></p>

				<p><%~ MaxBandAbout %></p>

				<p><%~ RTTAbout %></p>

				<p><%~ DTotBandAbout %></p>

				<p><%~ MinSpeedWarn %></p>
			</span>


			<a onclick='setDescriptionVisibility("qos_down_2")'  id="qos_down_2_ref" href="#qos_down_2"><%~ Hide %></a>

		</div>

		<div class="internal_divider"></div>


		<div><strong><%~ AddNewServiceRule %>:</strong></div>
		<div class="indent">

			<div>
				<label class='leftcolumn' id="class_name_label" for='class_name'><%~ SrvClassName %>:</label>
				<input class='rightcolumn' type='text' id='class_name' onkeyup="proofreadLengthRange(this,1,10)" size='12' maxlength='10' />
			</div>

			<div>
				<label class='leftcolumn' id="percent_bandwidth_label" for='percent_bandwidth'><%~ PerBandCap %>:</label>
				<div class='rightcolumn'>
					<input type='text' id='percent_bandwidth' onkeyup="proofreadNumericRange(this,1,100)"  size='5' maxlength='3' />
					<em>%</em>
				</div>
			</div>

			<div class='nocolumn'><%~ BandMin %>:</div>
			<div class='indent'>
				<div class='nocolumn'>
					<input type='radio' name="min_radio" id='min_radio1' onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
					<label for='min_radio1'><%~ BandMinNo %></label>
				</div>
				<div>
					<span class='leftcolumn'>
						<input type='radio' name="min_radio" id="min_radio2" onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
						<label id="min_bandwidth_label" for='min_radio2'><%~ BandMin %>:</label>
					</span>
					<span class='rightcolumn'>
						<input type='text' class="rightcolumn" id='min_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
						<em><%~ Kbs %></em>
					</span>
				</div>
			</div>

			<div class='nocolumn'><%~ BandMax %>:</div>
			<div class='indent'>
				<div class='nocolumn'>
					<input type='radio' name="max_radio" id='max_radio1' onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
					<label for='max_radio1'><%~ BandMaxNo %></label>
				</div>
				<div>
					<span class='leftcolumn'>
						<input type='radio' name="max_radio" id="max_radio2" onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
						<label id="max_bandwidth_label" for='max_radio2'><%~ BandMax %>:</label>
					</span>
					<span class='rightcolumn'>
						<input type='text' class="rightcolumn" id='max_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
						<em><%~ Kbs %></em>
					</span>
				</div>
			</div>

			<div class='nocolumn'><%~ MinRTT %>:</div>
			<div class='indent'>
				<div class='nocolumn'>
					<input type='radio' name="rtt_radio" id='rtt_radio1'/>
					<label for='max_radio1'><%~ ActRTT %></label>
				</div>
				<div>
					<span class='leftcolumn'>
						<input type='radio' name="rtt_radio" id="rtt_radio2" />
						<label for='max_radio2'><%~ OptiWAN %></label>
					</span>
				</div>
			</div>

			<div id="add_class_container">
				<input type="button" id="add_class_button" class="default_button" value="<%~ AddSvcCls %>" onclick="addServiceClass()" />
			</div>
		</div>
	</fieldset>

	<fieldset>

		<legend class="sectionheader"><%~ DACCSect %></legend>

		<div id='qos_monitor_container' class='nocolumn'>
			<input type='checkbox' id='qos_monenabled' onclick="setQosEnabled()"/>
			<label id='qos_monenabled_label' for='qos_monenabled'><%~ ACCOn %></label>
		</div>

		<div>
			<span class='indent'>
				<input type='checkbox' id='use_ptarget_ip' onclick='enableAssociatedField(this, "ptarget_ip", currentWanGateway)'/>&nbsp;&nbsp;
				<label for='ptarget_ip' id='ptarget_ip_label'><%~ ACC_Pt %>:</label>
				<input type='text' name='ptarget_ip' id='ptarget_ip' onkeyup='proofreadIpRange(this)' size='17' maxlength='31' />
			</span>
		</div>

		<div>
			<span class='indent'>
				<input type='checkbox' id='use_auto_pinglimit' onclick='enableAssociatedField(this, "pinglimit", 85)'/>&nbsp;&nbsp;
				<label for='pinglimit' id='pinglimit_label'><%~ ACC_con %>:</label>
				<input type='text' name='pinglimit' id='pinglimit' onkeyup='proofreadNumericRange(this, 10, 250)' size='4' maxlength='4' />
			</span>
		</div>

		<div id="qos_down_3" class="indent">
		<span id='qos_down_3_txt'>
			<%~ AbACC %>
		</span>
		<a onclick='setDescriptionVisibility("qos_down_3")'  id="qos_down_3_ref" href="#qos_down_3"><%~ Hide %></a>

		</div>
		<div class="internal_divider"></div>

		<div class="indent">
		<table>
		<tr><td><strong><%~ ACC_Stat %></strong></td></tr>
		<tr><td><span id='qstate'></span></td></tr>
		<tr><td><span id='qllimit'></span></td></tr>
		<tr><td><span id='qollimit'></span></td></tr>
		<tr><td><span id='qload'></span></td></tr>
		<tr><td><span id='qpinger'></span></td></tr>
		<tr><td><span id='qpingtime'></span></td></tr>
		<tr><td><span id='qpinglimit'></span></td></tr>
		<tr><td><span id='qactivecnt'></span></td></tr>
		</table>
		</div>

		<div id="qos_down_4" class="indent">
			<span id='qos_down_4_txt'>
				<table>
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
			<a onclick='setDescriptionVisibility("qos_down_4")'  id="qos_down_4_ref" href="#qos_down_4"><%~ Hide %></a>
		</div>

	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>
	<span id="update_container" ><%~ WaitSettings %></span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "firewall" -p "qosdownload"
%>
