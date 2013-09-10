#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bandwidth" -c "internal.css" -j "table.js bandwidth.js" -z "bandwidth.js" -n -i gargoyle qos_gargoyle
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

<form>
	<fieldset>
		<legend class="sectionheader"><%~ bandwidth.GOpSect %></legend>
		<div>
			<label for='plot_time_frame' id='time_frame_label'><%~ TFrm %>:</label>
			<select id="plot_time_frame" onchange="resetPlots()">
				<option value="1">15 <%~ minutes %></option>
				<option value="2"> 6 <%~ hours %></option>
				<option value="3">24 <%~ hours %></option>
				<option value="4">30 <%~ days %></option>
				<option value="5"> 1 <%~ year %></option>
			</select>
		</div>

		<div id="control_column_container">
			<div id="plot1_control_column">
				<div><span  id="plot1_title"><%~ Plot %> 1</span></div>
				<div><select id="plot1_type" onchange="resetPlots()" ><option value="total"><%~ TBdw %></option></select></div>
				<div><select id="plot1_id" onchange="resetPlots()"></select></div>

			</div>
			<div id="plot2_control_column">
				<div><span   id="plot2_title"><%~ Plot %> 2</span></div>
				<div><select id="plot2_type" onchange="resetPlots()" ><option value="none"><%~ None %></option></select></div>
				<div><select id="plot2_id" onchange="resetPlots()"></select></div>
			</div>
			<div id="plot3_control_column">
				<div><span   id="plot3_title"><%~ Plot %> 3</span></div>
				<div><select id="plot3_type" onchange="resetPlots()"><option value="none"><%~ None %></option></select></div>
				<div><select id="plot3_id" onchange="resetPlots()"></select></div>
			</div>
		</div>

		<div>
			<input type="checkbox" id="use_high_res_15m" onclick="highResChanged()">&nbsp;
			<label id="use_high_res_15m_label" for="use_high_res_15m"><%~ HRInf %></label>
			<br/>
			<em><%~ HRWrn %></em>
		</div>

		<br/><%~ UsInf %>  
		<br/><%~ LclTrff %>
	</fieldset>

	<fieldset id="bandwidth_graphs">
		<legend class="sectionheader"><%~ BGrSect %></legend>
		<span class="bandwidth_title_text"><strong><%~ Dnld %></strong> (<span onclick='expand("<%~ Dnld %>")' class="pseudo_link"><%~ expd %></span>)</span>
		<span class="bandwidth_title_text"><strong><%~ Upld %></strong> (<span onclick='expand("<%~ Upld %>")' class="pseudo_link"><%~ expd %></span>)</span>
		<br/>
		<embed id="download_plot" style="margin-left:0px; margin-right:5px; float:left; width:240px; height:180px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/' />
		<embed id="upload_plot" style="margin-left:0px; margin-right:5px; float:left; width:240px; height:180px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/' />
		<br/>
		<span class="bandwidth_title_text"><strong><%~ Totl %></strong> (<span onclick='expand("<%~ Totl %>")' class="pseudo_link"><%~ expd %></span>)</span>
		<br/>
		<embed id="total_plot" style="margin-left:0px; width:480px; height:360px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/' />
	</fieldset>
	<fieldset id="total_bandwidth_use">
		<legend class="sectionheader"><%~ BUTab %></legend>
		<div>
			<label for='table_time_frame' class="narrowleftcolumn" id='table_time_frame_label'><%~ DspI %>:</label>
			<select id="table_time_frame" class="rightcolumn" onchange="resetPlots()">
				<option value="1"><%~ minutes %></option>
				<option value="2"><%~ qhour %></option>
				<option value="3"><%~ hours %></option>
				<option value="4"><%~ days %></option>
				<option value="5"><%~ mnths %></option>
			</select>
		</div>
		<div>
			<label for='table_type' class="narrowleftcolumn" id='total_type_label'><%~ DspT %>:</label>
			<select id="table_type" class="rightcolumn" onchange="resetPlots()">
				<option value="total"><%~ TBdw %></option>
			</select>
		</div>
		<div id="table_id_container" style="display:none" >
			<label for='table_id' class="narrowleftcolumn" id='total_id_label'><%~ DspID %>:</label>
			<select id="table_id" class="rightcolumn" onchange="resetPlots()"></select>
		</div>
		<div class="bottom_gap">
			<label for='table_units' class="narrowleftcolumn" id='table_units_label'><%~ TbUnt %>:</label>
			<select id="table_units" class="rightcolumn" onchange="resetPlots()">
				<option value="mixed"><%~ AutoM %></option>
				<option value="KBytes"><%~ KBy %></option>
				<option value="MBytes"><%~ MBy %></option>
				<option value="GBytes"><%~ GBy %></option>
				<option value="TBytes"><%~ TBy %></option>
			</select>
		</div>

		<div id="bandwidth_table_container"></div>

		<div>
			<center><input type='button' id='delete_data_button' class='big_button' value='<%~ DelD %>' onclick='deleteData();' /></center>
		</div>

	</fieldset>

	<fieldset id="download_bandwidth_data" >
		<legend class="sectionheader"><%~ DBData %></legend>
		<div>
			<span style='text-decoration:underline'><%~ DSep %>:</span>
			<br/>
			<em><%~ DFmt %></em>
			<br/>
		</div>
		<div>
			<center><input type='button' id='download_data_button' class='big_button' value='<%~ DNow %>' onclick='window.location="bandwidth.csv";' /></center>
		</div>
	</fieldset>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	initializePlotsAndTable();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "bandwidth"  
%>
