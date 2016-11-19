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


	<fieldset>
	<legend class="sectionheader"><%~ email.SmtpSettings %></legend>
		<div>
			<label class='narrowleftcolumn'><%~ email.ServerIP %></label>
			<input type='text' class='rightcolumn' id='serverip' size='35' />
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.ServerPort %></label>
			<input type='text' class='rightcolumn' id='serverport' size='35' />
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.Encryption %></label>
			<input type='radio' value="plain" id='plain' name='encryption'/> <%~ email.None %>
			<input type='radio' value="tls" id='TLS' name='encryption'/> TLS
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.Auth %></label>
			<input type='checkbox' id='auth' onclick="Visibility()" />
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.Username %></label>
			<input type='text' class='rightcolumn' id='username' size='35' disabled/>
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.Password %></label>
			<input type='password' class='rightcolumn' id='password' size='35' disabled/>&nbsp;&nbsp;
			<input type="checkbox" id="show_pass" onclick="togglePass('password')">
			<label for="show_pass" id="show_pass_label" class="rightcolumn"><%~ email.rvel %></label>
		</div>
	</fieldset>
	
		<fieldset>
	<legend class="sectionheader"><%~ email.Emailsettings %></legend>
		<div>
			<label class='narrowleftcolumn'><%~ email.Sender %></label>
			<input type='text' class='rightcolumn' id='sender' size='35' />
		</div>
		<div>
			<label class='narrowleftcolumn'><%~ email.Recipient %></label>
			<input type='text' class='rightcolumn' id='receiver' size='35' />
		</div>
		<input type="button" id="testEmail" value="<%~ email.Test %>" class="default_button" onclick="testMail()">
	</fieldset>
	<fieldset>
	<legend class="sectionheader"><%~ email.DataSettings %></legend>
	<%~ email.Include %>
	<p />
	<input type='checkbox' name='content' /><%~ email.recentWebsites %>
	<br />
	<input type='checkbox' name='content' /><%~ email.recentSearches %>
	<br />
	<input type='checkbox' name='content' /><%~ email.Logs %>
	<br />
	<input type='checkbox' name='content' /><%~ email.DHCP %>
	<br />
	<input type='checkbox' name='content' /><%~ email.ARP %>
	<br />
	<input type='checkbox' name='content' onclick="intervalVisibility(this)" /><%~ email.Bandwidth %>
	<br />
	<label class="narrowleftcolumn" id="bandwidthIntervalLabel"><%~ email.BandwidthInterval %></label>
	<select id="bandwidthIntervalSelect" class="rightcolumn" disabled>
				<option value="minutes"><%~ email.minutes %></option>
				<option value="quarter hours"><%~ email.quarterhours %></option>
				<option value="hours"><%~ email.hours %></option>
				<option value="days"><%~ email.days %></option>
			</select>
	<div>
		<label class='narrowleftcolumn'><%~ email.Count %></label>
		<input type='number' class='rightcolumn' id='count' style="width:50px" min="0" disabled/>
	</div>
</fieldset>
<fieldset>
	<legend class="sectionheader"><%~ email.Time %></legend>

	<div>
		<label for="timer_mode" class="narrowleftcolumn"><%~ TPer %>:</label>
		<select id="timer_mode" class="rightcolumn" onchange="SetTimerMode(this.value)">
			<option selected="" value="0"><%~ NoTm %></option>
			<option value="1"><%~ Dly %></option>
			<option value="3"><%~ Wkd %></option>
			<option value="7"><%~ Wkly %></option>
		</select>
		<br />
		<br />
		<div id="div_timer_increment" style="display:none;">
			<label for="timer_increment" class="narrowleftcolumn"><%~ TInc %>:</label>
			<select id="timer_increment" onchange="SetTimerIncrement(this)">
				<option value="5">5 <%~ minutes %></option>
				<option value="10">10 <%~ minutes %></option>
				<option selected="" value="15">15 <%~ minutes %></option>
				<option value="30">30 <%~ minutes %></option>
				<option value="60">60 <%~ minutes %></option>
			</select>
		</div>
	</div>
	
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

	<br/><br/>

	<div  id="summary_container">
		<span id='summary_txt'></span>
	</div>
</fieldset>

<div id="bottom_button_container">
	<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
	<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='SetTimerMode(0)'/>
	<span id="update_container" >fg</span>
</div>

<script>
<!--
	LoadData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "email_notifications"
%>
