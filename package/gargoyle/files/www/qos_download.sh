#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -h -s "firewall" -p "qosdownload" -c "internal.css" -j "qos.js table.js" qos_gargoyle gargoyle
?>


<script>
<!--
<?
	if [ -h /etc/rc.d/S50qos_gargoyle ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi

	echo "var direction = \"download\";"
?>
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader">QoS (Download) -- Classification Rules</legend>
		
		<div id='qos_enabled_container' class='nocolumn'>
			<input type='checkbox' id='qos_enabled' onclick="setQosEnabled()" />
			<label id='qos_enabled_label' for='dhcp_enabled'>Enable Quality of Service (Download Direction)</label>
		</div>
		<div class="indent">
			<p>Quality of Service (QoS) provides a way to control how available bandwidth is allocated.  Connections are classified into
			different &ldquo;service classes,&rdquo; each of which is allocated a share of the available bandwidth.</p>
		</div>	
		<div class="internal_divider"></div>	
		
		<div id='qos_rule_table_container' class="bottom_gap"></div>
		<div>
			<label class="leftcolumn" id="total_bandwidth_label" for="total_bandwidth">Default Service Class:</label>
			<select class="rightcolumn" id="default_class"></select>
		</div>

		<div>
			<label class="leftcolumn" id="total_bandwidth_label" for="total_bandwidth">Total (Download) Bandwidth:</label>
			<input class="rightcolumn" id="total_bandwidth" class="rightcolumn" onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' /> 
			<em>kbit/s</em>

		</div>
		
		<div id="qos_down_1" class="indent">
			<span id='qos_down_1_txt'>
				<p> Be sure that the specified total bandwidth is as accurate as possible.  QoS ensures that this bandwidth is never exceeded.   
				Therefore if you specify less bandwidth than is actually available you will artificially slow down your connection.  However, if
				you specify a value that is too high, QoS will not be able to accurately allocate bandwidth because of network congestion. Note 
				that bandwidth is specified in kilobit/s.  There are 8 kilobits per kilobyte.</p>
				
				<p>The default service class specifies how packets that do not match any rule should be classified.</p>
	
				<p>Packets are tested against the rules in the order specified -- rules toward the top are tested first.
				As soon as a packet matches a rule it is classified, and the rest of the rules are ignored.  The order of 
				the rules can be altered using the arrow controls.</p>
			</span>
			<a onclick='setDescriptionVisibility("qos_down_1")'  id="qos_down_1_ref" href="#qos_down_1">Hide Text</a>
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
				</select>
			</div>
			<div>
				<div class='leftcolumn'>
					<input type='checkbox'  id='use_app_protocol' onclick='enableAssociatedField(this,"app_protocol", "")' />
					<label id="app_protocol_label" for='app_protocol'>Application (Layer7) Protocol:</label>
				</div>	
				<select class='rightcolumn' id="app_protocol">
					<option>HTTP</option>
					<option>FTP</option>
					<option>SSL</option>
					<option>POP3</option>
					<option>SMTP</option>
					<option>Ident</option>
					<option>NTP</option>
					<option>VNC</option>
					<option>IRC</option>
					<option>Jabber</option>
					<option>MSN Messenger</option>
					<option>AIM</option>
					<option>FastTrack</option>
					<option>BitTorrent</option>
					<option>Gnutella</option>
					<option>eDonkey</option>
					<option>Any P2P</option>
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
		
		<legend class="sectionheader">QoS (Download) -- Service Classes</legend>
		<div id='qos_class_table_container' class="bottom_gap"></div>

		<div id="qos_down_2" class="indent">
			<span id='qos_down_2_txt'>
				<p>Each service class is specified by three parameters: percent bandwidth at capacity, maximum bandwidth and minimize delay.</p>
				
				<p><em>Percent bandwidth at capcity</em> is the percentage of the total available bandwidth that should be allocated to this class of 
				connection when all available bandwidth is being used.  If unused bandwidth is available, more can (and will) be allocated.
				The percentages can be configured to equal more (or less) than 100, but when the settings are applied the percentages will be adjusted
				proportionally so that they add to 100.</p>
		
				<p><em>Maximum bandwidth</em> specifies an absolute maximum amount of bandwidth a service class will be allocated in kbit/s.  Even if unused bandwidth
				is available, this service class will never use more than this amount of bandwidth.</p>

				<p><em>Minimize delay</em> is an option that should be selected if it is important to minimize the delay of individual packets of this service class.
				This ensures that so long as the average bandwidth being used by this service class is less than the maximum allowed, individual packets will not spend a
				significant amount of time waiting in a queue. For example it may be important that the packets of a VoIP connection be delivered as soon as they are produced,
		       		while all that matters for an FTP is the average speed.  Therefore you might select to minimize the packet delay of the VoIP connection, but not the 
				FTP connection.</p>
			</span>
			<a onclick='setDescriptionVisibility("qos_down_2")'  id="qos_down_2_ref" href="#qos_down_2">Hide Text</a>

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
			
	
			<div>
				<label class= 'leftcolumn' id="minimize_delay_label" for='minimize_delay'>Minimize Delay</label>
				<select id='minimize_delay' class='rightcolumn'><option>yes</option><option>no</option></select>
			</div>
		
			<div class='nocolumn'>Bandwidth Maximum:</div>
			<div class='indent'>
				<div class='nocolumn'>
				<input type='radio' name="max_radio" id='max_radio1' onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
					<label for='max_radio1'>No Bandwidth Maximum</label>
				</div>	
				<div class='leftcolumn'>
					<input type='radio' name="max_radio" id="max_radio2" onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
					<label id="max_bandwidth_label" for='max_radio2'>Bandwidth Maximum:</label>
				</div>
				<input type='text' class="rightcolumn" id='max_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' /> 
				<em>kbit/s</em>
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
	gargoyle_header_footer -f -s "firewall" -p "qosdownload"
?>
