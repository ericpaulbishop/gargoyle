#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bandwidth" -j "table.js bandwidth.js" -z "bandwidth.js" -n -i gargoyle qos_gargoyle
%>

<script>
<!--
<%
	echo 'var monitorNames = new Array();'
	mnames=$(cat /tmp/bw_backup/*.sh 2>/dev/null | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
	for m in $mnames ; do
		echo "monitorNames.push(\"$m\");"
	done

	tz_hundred_hours=$(date "+%z" | sed 's/^+0//' | sed 's/^-0/-/g')
	tz_h=$(($tz_hundred_hours/100))
	tz_m=$(($tz_hundred_hours-($tz_h*100)))
	tz_minutes=$((($tz_h*60)+$tz_m))
	echo "var tzMinutes = $tz_minutes;";
%>
//-->
</script>
<h1 class="page-header"><%~ bandwidth.mBandwidth %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ bandwidth.GOpSect %></h3>
			</div>
			<div class="panel-body">
				<div class="row">
					<div class="col-sm-4">
						<div id="plot_time_column" class="row form-group">
							<label class="col-xs-5" for="plot_time_frame" id="time_frame_label"><%~ TFrm %>:</label>
							<span class="col-xs-7">
								<select id="plot_time_frame" class="form-control" onchange="resetPlots()">
									<option value="1">15 <%~ minutes %></option>
									<option value="2">6 <%~ hours %></option>
									<option value="3">24 <%~ hours %></option>
									<option value="4">30 <%~ days %></option>
									<option value="5">1 <%~ year %></option>
								</select>
							</span>
						</div>
					</div>
				</div>

				<div class="row">
					<div class="col-sm-4">
						<div id="plot1_control_column" class="row form-group">
							<label class="col-xs-5" id="plot1_title"><%~ Plot %> 1</label>
							<span class="brk-sm"></span>
							<span class="col-xs-7">
								<select id="plot1_type" class="form-control" onchange="resetPlots()">
									<option value="total"><%~ TBdw %></option>
								</select>
								<div class="second_row_right_column">
									<select id="plot1_id" class="form-control" onchange="resetPlots()"></select>
								</div>
							</span>
						</div>
					</div>

					<div class="col-sm-4">
						<div id="plot2_control_column" class="row form-group">
							<label class="col-xs-5" id="plot2_title"><%~ Plot %> 2</label>
							<span class="brk-sm"></span>
							<span class="col-xs-7">
								<select id="plot2_type" class="form-control" onchange="resetPlots()">
									<option value="none"><%~ None %></option>
								</select>
								<div class="second_row_right_column">
									<select id="plot2_id" class="form-control" onchange="resetPlots()"></select>
								</div>

							</span>
						</div>
					</div>

					<div class="col-sm-4">
						<div id="plot3_control_column" class="row form-group">
							<label class="col-xs-5" id="plot3_title"><%~ Plot %> 3</label>
							<span class="brk-sm"></span>
							<span class="col-xs-7">
								<select id="plot3_type" class="form-control" onchange="resetPlots()">
									<option value="none"><%~ None %></option>
								</select>
								<div class="second_row_right_column">
									<select id="plot3_id" class="form-control" onchange="resetPlots()"></select>
								</div>

							</span>
						</div>
					</div>

				</div>

				<div class="row">
					<div class="col-lg-12">
						<div class="row form-group">
							<span class="col-xs-12">
								<input type="checkbox" id="use_high_res_15m" onclick="highResChanged()">
								<label id="use_high_res_15m_label" for="use_high_res_15m"><%~ HRInf %></label>
								<span class="col-xs-12"><em><%~ HRWrn %></em></span>
							</span>
						</div>
						<span class="alert alert-warning col-xs-12" role="alert"><%~ UsInf %><br /><%~ LclTrff %></span>
					</div>
				</div>

				<div id="bandwidth_graphs" class="row">
					<h1 class="page-header"><%~ BGrSect %></h1>
					<div class="col-lg-4 col-md-12 col-sm-8 col-xs-12">
						<div class="col-lg-12 col-md-6 col-xs-12">
							<span class="bandwidth_title_text"><strong><%~ Dnld %></strong> (<span onclick="expand('<%~ Dnld %>')" class="pseudo_link"><%~ expd %></span>)</span>
							<embed id="download_plot" style="margin-left:0px; margin-right:5px; float:left; width:100%; height:100%;" src="bandwidth.svg" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/" />
						</div>
						<div class="col-lg-12 col-md-6 col-xs-12">
							<span class="bandwidth_title_text"><strong><%~ Upld %></strong> (<span onclick="expand('<%~ Upld %>')" class="pseudo_link"><%~ expd %></span>)</span>
							<embed id="upload_plot" style="margin-left:0px; margin-right:5px; float:left; width:100%; height:100%;" src="bandwidth.svg" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/" />
						</div>
					</div>
					<div class="col-lg-8 col-xs-12">
						<span class="bandwidth_title_text"><strong><%~ Totl %></strong> (<span onclick="expand('<%~ Totl %>')" class="pseudo_link"><%~ expd %></span>)</span>
						<br/>
						<embed id="total_plot" style="width:100%; height:100%;" src="bandwidth.svg" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/" />
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
				<h3 class="panel-title"><%~ BUTab %></h3>
			</div>
			<div id="total_bandwidth_use" class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" for="table_time_frame" id="table_time_frame_label"><%~ DspI %>:</label>
					<span class="col-xs-7">
						<select id="table_time_frame" class="form-control" onchange="resetPlots()">
							<option value="1"><%~ minutes %></option>
							<option value="2"><%~ qhour %></option>
							<option value="3"><%~ hours %></option>
							<option value="4"><%~ days %></option>
							<option value="5"><%~ mnths %></option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="table_type" id="total_type_label"><%~ DspT %>:</label>
					<span class="col-xs-7">
						<select id="table_type" class="form-control" onchange="resetPlots()">
							<option value="total"><%~ TBdw %></option>
						</select>
					</span>
				</div>

				<div id="table_id_container" style="display:none" class="row form-group">
					<label class="col-xs-5" for="table_id" id="total_id_label"><%~ DspID %>:</label>
					<span class="col-xs-7"><select id="table_id" class="form-control" onchange="resetPlots()"></select></span>
				</div>

				<div class="bottom_gap row form-group">
					<label class="col-xs-5" for="table_units" id="table_units_label"><%~ TbUnt %>:</label>
					<span class="col-xs-7">
						<select id="table_units" class="form-control" onchange="resetPlots()">
							<option value="mixed"><%~ AutoM %></option>
							<option value="KBytes"><%~ KBy %></option>
							<option value="MBytes"><%~ MBy %></option>
							<option value="GBytes"><%~ GBy %></option>
							<option value="TBytes"><%~ TBy %></option>
						</select>
					</span>
				</div>
				<div id="bandwidth_table_container" class="table-responsive"></div>
				<div class="form-group">
					<button id="delete_data_button" class="btn btn-danger btn-lg" onclick="deleteData();"><%~ DelD %></button>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ BMonSet %></h3>
			</div>
			<div id="custom_bwmon_settings" class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="enable_custom_bwmon" onclick="setCustBWMonVisibility()">
						<label id="enable_custom_bwmon_label" for="enable_custom_bwmon"><%~ EnCustBMon %></label>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="custom_time_frame" id="custom_time_frame_label"><%~ BMonIntvl %>:</label>
					<span class="col-xs-7">
						<select id="custom_time_frame" class="form-control" disabled="true">
							<option value="1"><%~ mnths %></option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="custom_reset_day" id="custom_reset_day_label"><%~ BMonReset %>:</label>
					<span class="col-xs-7">
						<select id="custom_reset_day" class="form-control" onchange="setCustBWMonVisibility()"></select>
					</span>
				</div>

				<div class="row form-group">
					<span class="alert alert-danger col-xs-12" role="alert"><%~ CustBMonWarn %></span>
				</div>

				<div class="form-group">
					<button id="bwcustSaveChanges" class="btn btn-primary btn-lg" onclick="bwcustSaveChanges()" disabled="true"><%~ SaveChanges %></button>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-6">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ DBData %></h3>
			</div>
			<div id="download_bandwidth_data" class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12" style="text-decoration:underline"><%~ DSep %>:</span>
					<br/>
					<code class="col-xs-12"><%~ DFmt %></code>
				</div>
				<br/>
				<div class="form-group">
					<button id="download_data_button" class="btn btn-success btn-lg" onclick="window.location='bandwidth.csv';"><%~ DNow %></button>
				</div>
			</div>
		</div>
	</div>
</div>


<script>
<!--
	initializePlotsAndTable();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "bandwidth"
%>
