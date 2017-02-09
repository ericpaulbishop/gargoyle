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
gargoyle_header_footer -h -s "system" -p "email_notifications" -c "internal.css" -j "email.js" -z "email.js" -i email
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

	#tabs ul { padding: 0px; margin: 30px 0 0 0; list-style-type:none; padding: 0 0 0 2px; height: 20px; }
	#tabs ul li { display: inline-block; clear: none; height: 20px; }
	#tabs ul li a { color: #42454a; background-color: #dedbde; outline: 2px solid #dedbde; border: 1px solid #dedbde; padding: 0 4px 0 4px; text-decoration: none; border-bottom: none; display: block; }
	#tabs ul li a.selected { color: #000; background-color: #f1f0ee; font-weight: bold; padding: 4px 5px 0 7px; outline: 1px solid #c6c6c6; border: 1px solid #f1f0ee;}
	#tabs ul li a.deselected { color: #000; background-color: #dedbde; font-weight: normal; padding: 0 4px 0 4px; }
	div.tabField { background-color: #f1f0ee; width: 500px; }
	div.tabField.hidden { display: none; }
	div.tabField.blank { }
	#encryptionText, #encryptionButton { display: none; }
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
					<label class="col-xs-5" ><%~ email.ServerIP %></label>
					<span class="col-xs-7"><input class="form-control" type='text' id='serverip' size='35' /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.ServerPort %></label>
					<span class="col-xs-7"><input type='text' class='form-control' id='serverport' size='35' /></control>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.Encryption %></label>
					<span class="col-xs-7">
						<input type='radio' value="plain" id='plain' name='encryption'/> <%~ email.None %>
						<input type='radio' value="tls" id='TLS' name='encryption'/> TLS
					</span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.Auth %></label>
					<span class="col-xs-7"><input type='checkbox' id='auth' onclick="Visibility()" /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.Username %></label>
					<span class="col-xs-7"><input type='text' class='form-control' id='username' size='35' disabled/></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.Password %></label>
					<span class="col-xs-7">
						<input type='password' class='form-control' id='password' size='35' disabled/>&nbsp;&nbsp;
						<input type="checkbox" id="show_pass" onclick="togglePass('password')">
						<label for="show_pass" id="show_pass_label" class="rightcolumn"><%~ email.rvel %></label>
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
					<label class='col-xs-5'><%~ email.Sender %></label>
					<span class="col-xs-7"><input type='text' class='form-control' id='sender' size='35' /></span>
				</div>
				<div class="row form-group">
					<label class='col-xs-5'><%~ email.Recipient %></label>
					<span class="col-xs-7"><input type='text' class='form-control' id='receiver' size='35' /></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-7"><input type="button" id="testEmail" value="<%~ email.Test %>" class="default_button" onclick="testMail()"></span>
				</div>
			</div>
		</div>

	</div>
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ email.Datasettings %></h3>
			</div>
		
			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><%~ email.Include %></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><input type='checkbox' name='content' /><%~ email.recentWebsites %></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><input type='checkbox' name='content' /><%~ email.recentSearches %></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><input type='checkbox' name='content' /><%~ email.Logs %></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><input type='checkbox' name='content' /><%~ email.DHCP %></span>
				</div>
				<div class="row form-group">
					<span class="col-xs-12"><input type='checkbox' name='content' /><%~ email.ARP %></span>
				</div>
				<div class="row form-group">
					<label class="col-xs-5" id="bandwidthIntervalLabel"><%~ email.BandwidthInterval %></label>
					<span class="col-xs-7">
						<select id="bandwidthIntervalSelect" class="form-control" disabled>
							<option value="minutes"><%~ email.minutes %></option>
							<option value="quarter hours"><%~ email.quarterhours %></option>
							<option value="hours"><%~ email.hours %></option>
							<option value="days"><%~ email.days %></option>
						</select>
					</span>
	
				</div>
				<div class="row form-group">
					<label class="col-xs-5" id="bandwidthIntervalLabel"><%~ email.Count %></label>
					<span class="col-xs-7">
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
					<label for="timer_mode" class="col-xs-5"><%~ TPer %>:</label>
					<span class="col-xs-7">
						<select id="timer_mode" class="form-control" onchange="SetTimerMode(this.value)">
							<option selected="" value="0"><%~ NoTm %></option>
							<option value="1"><%~ Dly %></option>
							<option value="3"><%~ Wkd %></option>
							<option value="7"><%~ Wkly %></option>
						</select>
					</span>
	
				</div>
				<div class="row form-group" id="div_timer_increment" style="display:none;">
					<label for="timer_increment" class="col-xs-5"><%~ TInc %>:</label>
					<span class="col-xs-7">
						<select id="timer_increment" onchange="SetTimerIncrement(this)">
							<option value="5">5 <%~ minutes %></option>
							<option value="10">10 <%~ minutes %></option>
							<option selected="" value="15">15 <%~ minutes %></option>
							<option value="30">30 <%~ minutes %></option>
							<option value="60">60 <%~ minutes %></option>
						</select>
					</span>
				</div>
				<div class="row form-group">
					<span class="col-xs-7">
		
						<div id="tabs">
							<ul id="tab_ulist">
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
							<table id="tab1_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_2">
							<table id="tab2_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_3">
							<table id="tab3_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_4">
							<table id="tab4_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_5">
							<table id="tab5_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_6">
							<table id="tab6_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
						<div class="tabField" id="tab_7">
							<table id="tab7_timeTable" style="width:100%; height:100%; text-align: center;"></table>
						</div>
					</span>
				
				</div>
				<div class="row form-group" id="summary_container">
					<span id="summary_txt" class="col-xs-12"></span>
				</div>
			</div>
		</div>
	</div>



</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="setTimerMode(0)"><%~ Reset %></button>
</div>
<span id="update_container" >fg</span>

<script>
<!--
	LoadData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "email_notifications"
%>
