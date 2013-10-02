#!/usr/bin/haserl
<%
#
#       Copyright (c) 2010 Artur Wronowski <arteqw@gmail.com>
#       Copyright (c) 2011-2013 Cezary Jackiewicz <cezary@eko.one.pl>
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
	gargoyle_header_footer -h -s "system" -p "initd" -c "internal.css" -j "table.js initd.js" -z "initd.js"
%>

<script>
<!--
<%
	echo "allInitScripts = new Array();"
	ls /etc/init.d/ | awk '{print "allInitScripts.push(\""$0"\");" ;}'

	echo "enabledScripts = new Array();"
	ls /etc/rc.d/ | grep S | cut -b4- | awk '{print "enabledScripts.push(\""$0"\");" ;}'
%>
//-->
</script>

<form>
	<fieldset>
		<legend class="sectionheader"><%~ initd.Services %></legend>
		<div id="initd_table_container"></div>
	</fieldset>

	<div id="bottom_button_container">
		<input type='button' value='<%~ SaveChanges %>' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='<%~ Reset %>' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>

</form>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "initd"
%>
