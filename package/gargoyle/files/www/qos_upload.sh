#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "qosupload" -c "internal.css" -j "qos.js table.js" qos_gargoyle firewall gargoyle -i qos_gargoyle
?>


<script>
<!--
       var direction = "upload";
       protocolMap = new Object;
<?
       sed -e '/^#/ d' -e 's/\([^ ]*\) \(.*\)/protocolMap\["\1"\]="\2";/' /etc/l7-protocols/l7index


	if [ -h /etc/rc.d/S50qos_gargoyle ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi
?>

//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader">QoS (Upload) -- Classification Rules</legend>

		<div id='qos_enabled_container' class='nocolumn'>
			<input type='checkbox' id='qos_enabled' onclick="setQosEnabled()" />
			<label id='qos_enabled_label' for='qos_enabled'>Enable Quality of Service (Upload Direction)</label>
		</div>

		<div class="indent">
			<p>Quality of Service (QoS) provides a way to control how available bandwidth is allocated.  Connections are classified into
			different &ldquo;service classes,&rdquo; each of which is allocated a share of the available bandwidth.  QoS should be applied
			in cases where you want to divide available bandwidth between competing requirements.  For example if you want
			your VoIP phone to work correctly while downloading videos.  Another case would be if you want your bit torrents
			throttled back when you are web surfing.</p>
		</div>
		<div class="internal_divider"></div>

		<div id='qos_rule_table_container' class="bottom_gap"></div>
		<div>
			<label class="leftcolumn" id="default_class_label" for="default_class">Default Service Class:</label>
			<select class="rightcolumn" id="default_class"></select>
		</div>

		<div id="qos_up_1" class="indent">
			<span id='qos_up_1_txt'>
				<p>Packets are tested against the rules in the order specified -- rules toward the top have priority.
				As soon as a packet matches a rule it is classified, and the rest of the rules are ignored.  The order of
				the rules can be altered using the arrow controls.</p>

				<p>The <em>Default Service Class</em> specifies how packets that do not match any rule should be classified.</p>

			</span>
			<a onclick='setDescriptionVisibility("qos_up_1")'  id="qos_up_1_ref" href="#qos_up_1">Hide Text</a>
		</div>


		<div class="internal_divider">
			<p>
		</div>

		<div><strong>Add New Classification Rule:</strong></div>
		<div class="indent">
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_source_ip' onclick='enableAssociatedField(this,"source_ip", "")' />
					<label id="source_ip_label" for='source_ip'>Source IP:</label>
				</div>
				<input class='rightcolumn' type='text' id='source_ip' onkeyup='proofreadIpRange(this)' size='17' maxlength='31' />
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_source_port' onclick='enableAssociatedField(this,"source_port", "")'/>
					<label id="source_port_label" for='source_port'>Source Port(s):</label>
				</div>
				<input class='rightcolumn' type='text' id='source_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_dest_ip' onclick='enableAssociatedField(this,"dest_ip", "")' />
					<label id="dest_ip_label" for='dest_ip'>Destination IP:</label>
				</div>
				<input class='rightcolumn' type='text' id='dest_ip' onkeyup='proofreadIpRange(this)' size='17' maxlength='31' />
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_dest_port' onclick='enableAssociatedField(this,"dest_port", "")'  />
					<label id="dest_port_label" for='dest_port'>Destination Port(s):</label>
				</div>
				<input class='rightcolumn' type='text' id='dest_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_max_pktsize' onclick='enableAssociatedField(this,"max_pktsize", "")'  />
					<label id="max_pktsize_label" for='max_pktsize'>Maximum Packet Length:</label>
				</div>
				<input class='rightcolumn' type='text' id='max_pktsize' onkeyup='proofreadNumericRange(this,1,1500)' size='17' maxlength='4' />
				<em>bytes</em>
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_min_pktsize' onclick='enableAssociatedField(this,"min_pktsize", "")'  />
					<label id="min_pktsize_label" for='min_pktsize'>Minimum Packet Length:</label>
				</div>
				<input class='rightcolumn' type='text' id='min_pktsize' onkeyup='proofreadNumericRange(this,1,1500)' size='17' maxlength='4' />
				<em>bytes</em>
			</div>


			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_transport_protocol' onclick='enableAssociatedField(this,"transport_protocol", "")'  />
					<label id="transport_protocol_label" for='transport_protocol'>Transport Protocol:</label>
				</div>
				<select class='rightcolumn' id="transport_protocol"/>
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
					<option value="ICMP">ICMP</option>
				</select>
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_connbytes_kb' onclick='enableAssociatedField(this,"connbytes_kb", "")'  />
					<label id="connbytes_kb_label" for='connbytes_kb'>Connection bytes reach:</label>
				</div>
				<input class='rightcolumn' type='text' id='connbytes_kb' onkeyup='proofreadNumericRange(this,0,4194303)' size='17' maxlength='28' />
				<em>kBytes</em>
			</div>

			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_app_protocol' onclick='enableAssociatedField(this,"app_protocol", "")' />
					<label id="app_protocol_label" for='app_protocol'>Application (Layer7) Protocol:</label>
				</div>
				<select class='rightcolumn' id="app_protocol">
				<?
				sed -e "s/#.*//" -e "s/\([^ ]* \)\(.*\)/<option value='\1'>\2<\/option>/" /etc/l7-protocols/l7index
				?>
				</select>
			</div>



			<div>
				<label class='leftcolumn' id="classification_label" for='class_name' >Set Service Class To:</label>
				<select class='rightcolumn' id="classification">
				</select>
			</div>


			<div id="add_rule_container">
				<input type="button" id="add_rule_button" class="default_button" value="Add Rule" onclick="addClassificationRule()" />
			</div>
	</div>
	</fieldset>

	<fieldset>

		<legend class="sectionheader">QoS (Upload) -- Service Classes</legend>
		<div id='qos_class_table_container' class="bottom_gap"></div>

		<div>
			<label class="leftcolumn" id="total_bandwidth_label" for="total_bandwidth">Total (Upload) Bandwidth:</label>
			<input type="text" class="rightcolumn" id="total_bandwidth" class="rightcolumn" onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
			<em>kbit/s</em>

		</div>

		<div id="qos_up_2" class="indent">
			<span id='qos_up_2_txt'>
				<p>Each service class is specified by three parameters: percent bandwidth at capacity, minimum bandwidth and maximum bandwidth.</p>

				<p><em>Percent bandwidth at capacity</em> is the percentage of the total available bandwidth that should be allocated to this class
				when all available bandwidth is being used.  If unused bandwidth is available, more can (and will) be allocated.
				The percentages can be configured to equal more (or less) than 100, but when the settings are applied the percentages will be adjusted
				proportionally so that they add to 100.</p>

				<p><em>Minimum bandwidth</em> specifies the minimum service this class will be allocated when the link is at capacity.
				For certain applications like VoIP or online gaming it is better to specify a minimum service in bps rather than a percentage.
				QoS will satisfiy the minimum service of all classes first before allocating the remaining service to other waiting classes.
				</p>

				<p><em>Maximum bandwidth</em> specifies an absolute maximum amount of bandwidth this class will be allocated in kbit/s.  
				Even if unused bandwidth iavailable, this service class will never be permitted to use more than this amount of bandwidth.</p>

				<p><em>Total Upload Bandwidth</em> should be set to around 95% of your available upload bandwidth.  
				Entering a number which is too high will result in QoS not meeting its class requirements.  
				Entering a number which is too low will needlessly penalize your upload speed.
				Note that bandwidth is specified in kilobit/s.  There are 8 kilobits per kilobyte.</p>
			</span>
			<a onclick='setDescriptionVisibility("qos_up_2")'  id="qos_up_2_ref" href="#qos_up_2">Hide Text</a>
		</div>

		<div class="internal_divider"></div>


		<div><strong>Add New Service Class:</strong></div>
		<div class="indent">

			<div>
				<label class='leftcolumn' id="class_name_label" for='class_name'  >Service Class Name:</label>
				<input class='rightcolumn' type='text' id='class_name' onkeyup="proofreadLengthRange(this,1,10)" size='12' maxlength='10' />
			</div>

			<div>
				<label class='leftcolumn' id="percent_bandwidth_label" for='percent_bandwidth' >Percent Bandwidth At Capacity:</label>
				<div class='rightcolumn'>
					<input type='text' id='percent_bandwidth' onkeyup="proofreadNumericRange(this,1,100)"  size='5' maxlength='3' />
					<em>%</em>
				</div>
			</div>
			<div class='nocolumn'>Bandwidth Minimum:</div>
			<div class='indent'>
				<div class='nocolumn'>
					<input type='radio' name="min_radio" id='min_radio1' onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
					<label for='min_radio1'>No Bandwidth Minimum</label>
				</div>
				<div>
					<span class='leftcolumn'>
						<input type='radio' name="min_radio" id="min_radio2" onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
						<label id="min_bandwidth_label" for='min_radio2'>Bandwidth Minimum:</label>
					</span>
					<span class='rightcolumn'>
						<input type='text' class="rightcolumn" id='min_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
						<em>kbit/s</em>
					</span>
				</div>
			</div>

			<div class='nocolumn'>Bandwidth Maximum:</div>
			<div class='indent'>
				<div class='nocolumn'>
					<input type='radio' name="max_radio" id='max_radio1' onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
					<label for='max_radio1'>No Bandwidth Maximum</label>
				</div>
				<div>
					<span class='leftcolumn'>
						<input type='radio' name="max_radio" id="max_radio2" onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
						<label id="max_bandwidth_label" for='max_radio2'>Bandwidth Maximum:</label>
					</span>
					<span class='rightcolumn'>
						<input type='text' class="rightcolumn" id='max_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
						<em>kbit/s</em>
					</span>
				</div>
			</div>

			<div id="add_class_container">
				<input type="button" id="add_class_button" class="default_button" value="Add Service Class" onclick="addServiceClass()" />
			</div>
		</div>
	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>
	<span id="update_container" >Please wait while new settings are applied. . .</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "firewall" -p "qosupload"
?>
