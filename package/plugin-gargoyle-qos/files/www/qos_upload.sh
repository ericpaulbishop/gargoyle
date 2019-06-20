#!/usr/bin/haserl
<%
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "firewall" -p "qosupload" -j "qos.js table.js" -z "qos.js" -i firewall gargoyle qos_gargoyle
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
					<span class="col-xs-12">
						<input type="checkbox" id="qos_enabled" onclick="setQosEnabled()" />
						<label id="qos_enabled_label" for="qos_enabled"><%~ UEnable %></label>
					</span>
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
				<div class="row form-group" id="add_class_container">
					<span class="col-xs-12"><button id="add_class_button" class="btn btn-default btn-add" onclick="addClassModal()"><%~ AddSvcCls %></button></span>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<div class="modal fade" tabindex="-1" role="dialog" id="qos_rule_modal">
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

<div class="modal fade" tabindex="-1" role="dialog" id="qos_class_modal">
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
	gargoyle_header_footer -f -s "firewall" -p "qosupload"
%>
