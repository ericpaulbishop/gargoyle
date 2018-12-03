#!/usr/bin/haserl
<%
#
# This program is copyright © 2015 dpint and is distributed under the terms of the GNU GPL
# version 2.0 with a special clarification/exception that permits adapting the program to
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL.
# See http://gargoyle-router.com/faq.html#qfoss for more information
#
# Cron configuration code was derived from wifi_schedule plugin, which was written by BashfulBladder.
#
eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )
gargoyle_header_footer -h -s "system" -p "email_notifications" -j "email.js" -z "email.js" -i email
%>

<script>
<!--
<%
	echo "var cron_data = new Array();"
	if [ -e /etc/crontabs/root ] ; then
		awk '{gsub(/"/, "\\\""); print "cron_data.push(\""$0"\");" }' /etc/crontabs/root
	fi
	echo 'var msmtprc='"'$(cat /etc/msmtprc | tr '\n' ' ')'"';';
	echo "var weekly_time=\"`date \"+%w-%H-%M\"`\";"
	echo 'var TLSsupport='"'$(msmtp --version | grep OpenSSL)'"';';
	webmon_enabled=$(ls /etc/rc.d/*webmon_gargoyle* 2>/dev/null)
	if [ -n "$webmon_enabled" ] ; then
		echo "var webmonEnabled=true;"
	else
		echo "var webmonEnabled=false;"
	fi
%>
var raw_cron_data = new Array();
for (tab_idx in cron_data) {
	raw_cron_data.push(cron_data[tab_idx]);
}
//-->
</script>

<style type="text/css">
	div.tabField.hidden { display: none; }
	div.tabField.blank { }
</style>

<h1 class="page-header"><%~ email.Emailsettings %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ email.SmtpSettings %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label class="col-xs-5 col-sm-3" for='serverip'><%~ email.ServerIP %></label>
					<span class="col-xs-7 col-sm-9"><input class="form-control" type='text' id='serverip' size='35' /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3' for='serverport'><%~ email.ServerPort %></label>
					<span class="col-xs-7 col-sm-9"><input type='text' class='form-control' id='serverport' size='35' /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3'><%~ email.Encryption %></label>
					<span class="col-xs-7 col-sm-9">
						<input type='radio' value="plain" id='plain' name='encryption'/>
						<label for='plain'><%~ email.None %></label>
						<input type='radio' value="tls" id='TLS' name='encryption'/>
						<label for='TLS'>TLS</label>
					</span>
				</div>
				<br>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type="checkbox" id="auth" onclick="Visibility()"/>
						<label for="auth"><%~ email.Auth %></label>
					</span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3' for='username'><%~ email.Username %></label>
					<span class="col-xs-7 col-sm-9"><input type='text' class='form-control' id='username' size='35' disabled/></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3' for='password'><%~ email.Password %></label>
					<span class="col-xs-7 col-sm-9">
						<input type='password' class='form-control' id='password' size='35' disabled/>
						<input type="checkbox" id="show_pass" onclick="togglePass('password')"/>
						<label for="show_pass" id="show_pass_label"><%~ email.rvel %></label>
					</span>
				</div>
			</div>
		</div>
	</div>
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ email.Emailsettings %></h3>
			</div>
		
			<div class="panel-body">
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3' for='sender'><%~ email.Sender %></label>
					<span class="col-xs-7 col-sm-9"><input type='text' class='form-control' id='sender' size='35' /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5 col-sm-3' for='receiver'><%~ email.Receipment %></label>
					<span class="col-xs-7 col-sm-9"><input type='text' class='form-control' id='receiver' size='35' /></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><button type="button" id="testEmail" class="btn btn-default" onclick="testMail()"><%~ email.Test %></button></span>
				</div>
			</div>
		</div>

	</div>
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ email.DataSettings %></h3>
			</div>
		
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<span style="text-decoration:underline"><%~ email.Include %></span>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb1'/>
						<label for='cb1'><%~ email.recentWebsites %></label>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb2'/>
						<label for='cb2'><%~ email.recentSearches %></label>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb3'/>
						<label for='cb3'><%~ email.Logs %></label>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb4'/>
						<label for='cb4'><%~ email.DHCP %></label>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb5'/>
						<label for='cb5'><%~ email.ARP %></label>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12">
						<input type='checkbox' name='content' id='cb6' onclick="intervalVisibility(this)"/>
						<label for='cb6'><%~ email.Bandwidth %></label>
					</span>
				</div>
				<div class="row form-group">
					<label class="col-xs-5 col-sm-3" id="bandwidthIntervalLabel" for="bandwidthIntervalSelect"><%~ email.BandwidthInterval %></label>
					<span class="col-xs-7 col-sm-9">
						<select id="bandwidthIntervalSelect" class="form-control" disabled>
							<option value="minute"><%~ email.minutes %></option>
							<option value="900"><%~ email.quarterhours %></option>
							<option value="hour"><%~ email.hours %></option>
							<option value="day"><%~ email.days %></option>
						</select>
					</span>
	
				</div>
				<div class="row form-group">
					<label class="col-xs-5 col-sm-3" id="bandwidthIntervalLabel" for="count"><%~ email.Count %></label>
					<span class="col-xs-7 col-sm-9">
						<input type='number' class='form-control' id='count' style="width:50px" min="0" disabled/>
					</span>
				</div>
	
			</div>
		</div>
	</div>
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ email.Time %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<label for="timer_mode" class="col-xs-5 col-sm-3"><%~ TPer %>:</label>
					<span class="col-xs-7 col-sm-9">
						<select id="timer_mode" class="form-control" onchange="SetTimerMode(this.value)">
							<option selected="" value="0"><%~ NoTm %></option>
							<option value="1"><%~ Dly %></option>
							<option value="3"><%~ Wkd %></option>
							<option value="7"><%~ Wkly %></option>
						</select>
					</span>
	
				</div>
				<div class="row form-group" id="div_timer_increment" style="display:none;">
					<label for="timer_increment" class="col-xs-5 col-sm-3"><%~ TInc %>:</label>
					<span class="col-xs-7 col-sm-9">
						<select id="timer_increment" class="form-control" onchange="SetTimerIncrement(this)">
							<option value="5">5 <%~ minutes %></option>
							<option value="10">10 <%~ minutes %></option>
							<option selected="" value="15">15 <%~ minutes %></option>
							<option value="30">30 <%~ minutes %></option>
							<option value="60">60 <%~ minutes %></option>
						</select>
					</span>
				</div>
				<div class="form-group" id="tabs">
					<ul class="nav nav-tabs" id="tab_ulist">
					  <li id="tab_li_1" style="display:none;"></li>
					  <li id="tab_li_2" style="display:none;"></li>
					  <li id="tab_li_3" style="display:none;"></li>
					  <li id="tab_li_4" style="display:none;"></li>
					  <li id="tab_li_5" style="display:none;"></li>
					  <li id="tab_li_6" style="display:none;"></li>
					  <li id="tab_li_7" style="display:none;"></li>
					</ul>
				</div>
				<div class="tabField" id="tab_1">
					<table id="tab1_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_2">
					<table id="tab2_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_3">
					<table id="tab3_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_4">
					<table id="tab4_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_5">
					<table id="tab5_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_6">
					<table id="tab6_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<div class="tabField" id="tab_7">
					<table id="tab7_timeTable" class="table-responsive" style="width:100%; height:100%; text-align: center;"></table>
				</div>
				<br>
				<div id="summary_container">
					<span id="summary_txt"></span>
				</div>
			</div>
		</div>
	</div>



</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="setTimerMode(0)"><%~ Reset %></button>
</div>

<script>
<!--
	LoadData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "email_notifications"
%>
