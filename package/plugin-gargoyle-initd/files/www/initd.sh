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
	gargoyle_header_footer -h -s "system" -p "initd" -j "table.js initd.js" -z "initd.js"
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

<h1 class="page-header"><%~ initd.Services %></h1>
<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ Services %></h3>
			</div>
			<div class="panel-body">
				<div id="initd_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick='saveChanges()'><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick='resetData()'><%~ Reset %></button>
</div>


<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "initd"
%>
