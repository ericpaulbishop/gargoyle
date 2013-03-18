#!/usr/bin/haserl
<?
	# This webpage is copyright ¬© 2013 by BashfulBladder 
	# There is not much to this page, so this is public domain 
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "wifi_survey" -c "internal.css" -j "table.js  wifi_survey.js" gargoyle

?>

<script>
<!--
<?
	if [ -e /tmp/survey_data.txt ] ; then
		cat /tmp/survey_data.txt
	else
		echo "var sdata = [];"
	fi
	echo "var curr_time=\"`date \"+%Y%m%d%H%M\"`\";"
	
	if [ -e /tmp/OUIs.js ] ; then
		cat /tmp/OUIs.js
	fi
?>

var station_data = new Array();
for (sd in sdata) {
	station_data.push(sdata[sd]);
}

//-->
</script>

<style type="text/css">

.backer{ display:block; width:90px; height:8px; background:#ddd; border-radius:4px; margin: 0 0 8px 0;}
.dbacker{ display:block; width:90px; height:8px; background:#aaa; border-radius:4px; margin: 0 0 8px 0;}

.gfiller{ display:block; height:8px; background:#00ff00; border-radius:4px; }
.yfiller{ display:block; height:8px; background:#ffff00; border-radius:4px; }
.rfiller{ display:block; height:8px; background:#ff0000; border-radius:4px; }
	
</style>

<fieldset id="wifi_survey">
	<legend class="sectionheader">WiFi Survey</legend>
		
	<div>
		<div id="station_table_container"</div>
	</div>

	<div id="notes">
		<span id='note_txt'></span>
	</div>
</fieldset>

<script>
<!--
	InitSurvey();
//-->
</script>

<?
	gargoyle_header_footer -f -s "system" -p "wifi_survey"
?>
