#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "bdist" -j "table.js bdist.js" -z "bandwidth.js" -n -i gargoyle
%>

<script>
<!--
<%
	echo 'monitorNames = new Array();'
	mnames=$(cat /tmp/bw_backup/do_bw_backup.sh 2>/dev/null | egrep "bw_get" | sed 's/^.*\-i \"//g' | sed 's/\".*$//g')
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
<h1 class="page-header"><%~ bandwidth.BDst %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ bandwidth.DOpSect %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5" for="time_frame" id="time_frame_label"><%~ DTFrm %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="time_frame" onchange="resetTimeFrame()">
							<option value="bdist1"><%~ minutes %></option>
							<option value="bdist2"><%~ qhour %></option>
							<option value="bdist3"><%~ hours %></option>
							<option value="bdist4"><%~ days %></option>
							<option value="bdist5"><%~ mnths %></option>
						</select>
					</span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="time_interval" id="time_interval_label"><%~ DtbI %>:</label>
					<span class="col-xs-7"><select class="form-control" id="time_interval" onchange="resetDisplayInterval()"></select></span>
				</div>

				<div class="row form-group">
					<label class="col-xs-5" for="host_display" id="time_interval_label"><%~ HDsp %>:</label>
					<span class="col-xs-7">
						<select class="form-control" id="host_display" onchange="resetTimeFrame()">
							<option value="hostname"><%~ DspHn %></option>
							<option value="ip"><%~ DspHIP %></option>
						</select>
					</span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">

<div class="col-lg-12">
	<div class="panel panel-default">
		<div class="panel-heading">
			<h3 class="panel-title"><%~ BDst %>:</h3>
		</div>
		<div class="panel-body">
			<span class="col-lg-8 col-xs-12"><embed id="pie_chart" src="multi_pie.svg"  type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/"></embed></span>
		</div>
	</div>
</div>

</div>

<div class="row">

<div class="col-lg-12">
	<div class="panel panel-default">
		<div class="panel-heading">
			<h3 class="panel-title"><%~ BDtbl %></h3>
		</div>
		<div class="panel-body">
			<div id="bandwidth_distribution_table_container" class="table-responsive"></div>
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
	gargoyle_header_footer -f -s "status" -p "bdist"
%>
