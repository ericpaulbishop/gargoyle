#!/usr/bin/haserl
<%
	# This webpage is copyright © 2013 by BashfulBladder 
	# There is not much to this page, so this is public domain 
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "wifi_schedule" -c "internal.css" -j "wifi_schedule.js" -z "wifi_schedule.js"
%>

<script>
<!--
<%
	echo "var cron_data = new Array();"
	if [ -e /etc/crontabs/root ] ; then
		awk '{gsub(/"/, "\\\""); print "cron_data.push(\""$0"\");" }' /etc/crontabs/root
	fi
	echo "var weekly_time=\"`date \"+%w-%H-%M\"`\";"
		
	echo "var wifi_status = new Array();"
	iwconfig 2>&1 | grep -v 'wireless' | sed '/^$/d;s/"//g' | awk -F'\n' '{print "wifi_status.push(\""$0"\");" }'

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
	
</style>

<fieldset id="wifi_schedule">
	<legend class="sectionheader"><%~ wifi_schedule.Wisch %></legend>
	<div id='wlan_stat'>
		<label class='leftcolumn'><%~ Rstat %>:</label>
		<span class='rightcolumn' id='wlan_status'></span>
	</div>
	
	<div id='wifi_action' style="margin-top:15px">
		<label class='leftcolumn' style="margin-top:5px"><%~ StStR %></label>
		<span class='rightcolumn'>
			<input type='button' class='default_button' id='wifi_up_button' value="<%~ RadOn %>" onclick='GetWifiUpdate("up")'/>
			<input type='button' class='default_button' id='wifi_down_button' value="<%~ RadOf %>" onclick='GetWifiUpdate("down")'/>
		</span>
	</div>

	<div class="internal_divider"></div>

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
</div>

<script>
<!--
	LoadCrontabs();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "wifi_schedule"
%>
