#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -h -s "status" -p "webmon" -c "internal.css" -j "webmon.js table.js" webmon_gargoyle gargoyle
?>


<script>
<!--
<?
	webmon_enabled=$(ls /etc/rc.d/*webmon_gargoyle* 2>/dev/null)
	if [ -n "$webmon_enabled" ] ; then
		echo "var webmonEnabled=true;"
	else
		echo "var webmonEnabled=false;"
	fi

	echo "var includeData=[];"
	echo "var includeExists = false;"
	include_file=$(uci get webmon_gargoyle.webmon.include_ip_file 2>/dev/null)
	if [ -e "$include_file" ] ; then
                cat "$include_file" | awk ' $0 ~ /^[\t ]*[0-9]/ {print "includeData.push([\""$1"\"]);"};'
		echo "includeExists = true;"
        fi

	echo "var excludeData=[];"
	echo "var excludeExists = false;"
	exclude_file=$(uci get webmon_gargoyle.webmon.exclude_ip_file 2>/dev/null)
	if [ -e "$exclude_file" ] ; then
                cat "$exclude_file" | awk ' $0 ~ /^[\t ]*[0-9]/ {print "excludeData.push([\""$1"\"]);"};'
		echo "excludeExists = true;"
        fi
?>
//-->
</script>


<form>
	<fieldset>
		<legend class="sectionheader">Web Monitor Preferences</legend>
		<div>
			<input type='checkbox' id='webmon_enabled' onclick="setWebmonEnabled()" />
			<label id='webmon_enabled_label' for='webmon_enabled'>Enable Web Usage Monitor</label>
		</div>
		<div class="indent">
			<div>
				<label class='leftcolumn' for='num_records' id='num_records_label'>Number of Pages to Save:</label>
				<input type='text' class='rightcolumn' id='num_records' onkeyup='proofreadNumericRange(this,1,9999)' size='6' maxlength='4' />
			</div>
			<div>
				<select id="include_exclude" onchange="setIncludeExclude()">
					<option value="all">Monitor All IP Addresses</option>
					<option value="include">Monitor Only IP Addresses Below</option>
					<option value="exclude">Exclude IP Addresses Below From Monitoring</option>
				</select>
			</div>
			<div class="indent" id="add_ip_container">
				<div>
					<input type='text' id='add_ip' onkeyup='proofreadIp(this)' size='20' maxlength='17' />
					<input type="button" class="default_button" id="add_ip_button" value="Add IP To List" onclick="addIp()" />
				</div>
				<div id="ip_table_container"></div>
			</div>
		</div>
		
		<div class="internal_divider"></div>

		<div id="bottom_button_container">
			<input type='button' value='Save Changes' id="save_button" class="default_button" onclick='saveChanges()' />
			<input type='button' value='Reset' id="reset_button" class="default_button" onclick='resetData()'/>
			<input type='button' value='Clear History' id="clear_history" class="default_button" onclick='clearHistory()'/>
		</div>
		<span id="update_container" >Please wait while new settings are applied. . .</span>
	</fieldset>
	<fieldset>
		<legend class="sectionheader">Recent Web Usage</legend>
		<div id="webmon_table_container"></div>
	</fieldset>
	<fieldset id="download_web_usage_data" >
		<legend class="sectionheader">Download Web Usage Data</legend>
		<div>
			<span style='text-decoration:underline'>Data is comma separated:</span>
			<br/>
			<em>[Time of Last Visit],[Local IP],[IP Visited],[Domain Visted]</em>
			<br/>
		</div>
		<div>
		<center><input type='button' id='download_data_button' class='big_button' value='Download Now' onclick='window.location="webmon.csv";' /></center>
			
		</div>
	</fieldset>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "status" -p "webmon"
?>
