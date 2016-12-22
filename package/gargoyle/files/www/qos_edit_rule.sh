#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )
	gargoyle_header_footer -m -c "internal.css" -j "qos.js" -z "qos.js"
%>
<div id="edit_container" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ qos.QERulClass %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_source_ip" onclick="enableAssociatedField(this,'source_ip', '')" />
						<label id="source_ip_label" for="source_ip"><%~ SrcIP %>:</label>
					</span>
					<span class="col-xs-7"><input type="text" id="source_ip" class="form-control" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" /></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_source_port" onclick="enableAssociatedField(this,'source_port', '')"/>
						<label id="source_port_label" for="source_port"><%~ SrcPort %>:</label>
					</span>
					<span class="col-xs-7"><input type="text" id="source_port" class="form-control" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" /></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_dest_ip" onclick="enableAssociatedField(this,'dest_ip', '')" />
						<label id="dest_ip_label" for="dest_ip"><%~ DstIP %>:</label>
					</span>
					<span class="col-xs-7"><input type="text" id="dest_ip" class="form-control" onkeyup="proofreadIpRange(this)" size="17" maxlength="31" /></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_dest_port" onclick="enableAssociatedField(this,'dest_port', '')" />
						<label id="dest_port_label" for="dest_port"><%~ DstPort %>:</label>
					</span>
					<span class="col-xs-7"><input type="text" id="dest_port" class="form-control" onkeyup="proofreadPortOrPortRange(this)" size="17" maxlength="11" /></span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_max_pktsize" onclick="enableAssociatedField(this,'max_pktsize', '')" />
						<label id="max_pktsize_label" for="max_pktsize"><%~ MaxPktLen %>:</label>
					</span>
					<span class="col-xs-7">
						<input type="text" id="max_pktsize" class="form-control" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" />
						<em><%~ byt %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_min_pktsize" onclick="enableAssociatedField(this,'min_pktsize', '')" />
						<label id="min_pktsize_label" for="min_pktsize"><%~ MinPktLen %>:</label>
					</span>
					<span class="col-xs-7">
						<input type="text" id="min_pktsize" class="form-control" onkeyup="proofreadNumericRange(this,1,1500)" size="17" maxlength="4" />
						<em><%~ byt %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_transport_protocol" onclick="enableAssociatedField(this,'transport_protocol', '')" />
						<label id="transport_protocol_label" for="transport_protocol"><%~ TrProto %>:</label>
					</span>
					<span class="col-xs-7">
						<select id="transport_protocol" class="form-control"/>
							<option value="TCP">TCP</option>
							<option value="UDP">UDP</option>
							<option value="ICMP">ICMP</option>
							<option value="GRE">GRE</option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_connbytes_kb" onclick="enableAssociatedField(this,'connbytes_kb', '')" />
						<label id="connbytes_kb_label" for="connbytes_kb"><%~ Conreach %>:</label>
					</span>
					<span class="col-xs-7">
						<input type="text" id="connbytes_kb" class="form-control" onkeyup="proofreadNumeric(this)" size="17" maxlength="28" />
						<em><%~ KBy %></em>
					</span>
				</div>

				<div class="row form-group">
					<span class="col-xs-5">
						<input type="checkbox" id="use_app_protocol" onclick="enableAssociatedField(this,'app_protocol', '')" />
						<label id="app_protocol_label" for="app_protocol"><%~ AppProto %>:</label>
					</span>
					<span class="col-xs-7">
						<select id="app_protocol" class="form-control">
						<%
						sed -e '/^#/ d' -e "s/\([^ ]* \)\(.*\)/<option value='\1'>\2<\/option>/" /etc/l7-protocols/l7index
						%>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" id="classification_label" for="class_name" ><%~ SetClass %>:</label>
					<span class="col-xs-7"><select id="classification" class="form-control"></select></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default"></div>

</body>
</html>

