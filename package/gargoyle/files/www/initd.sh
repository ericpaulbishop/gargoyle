#!/usr/bin/haserl
<?
#	
# 	   Copyright (c) 2010 Artur Wronowski <arteqw@gmail.com>
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#      
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#      
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#      MA 02110-1301, USA.
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "initd" -c "internal.css" -j "table.js initd.js"
?>

<script>
<!--
<?	
	echo "allInitScripts= new Array();"
	ls /etc/init.d/ | grep -v rcS | awk '{print "allInitScripts.push(\""$0"\");" ;}'

	echo "enabledScripts = new Array();"
	ls /etc/rc.d/ | grep S | cut -b4- | awk '{print "enabledScripts.push(\""$0"\");" ;}'
	
?>
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader">Initscripts</legend>	
		<div class='indent'>
		<div id="initd_table_container"></div>
		</div>
		<br />
		<div class='indent'>
			<em>WARNING: Beacuse of safety reason some important services was blocked before accidental turn off at startup.</em>
		</div>
	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='Save changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>


	<span id="update_container" >Proszę czekać na wprowadzenie zmian...</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->


<script>
<!--
	resetData();
//-->
</script>

<?
	gargoyle_header_footer -f -s "system" -p "initd"
?>
