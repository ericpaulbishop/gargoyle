#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "reboot" -j "reboot.js" -z "reboot.js"
%>

<script>
<%
	echo "var cronLine = \"\";"
	if [ -e /etc/crontabs/root ] ; then
		awk ' $0 ~ /usr\/lib\/gargoyle\/reboot.sh/ {print "cronLine=\""$0"\";"};' /etc/crontabs/root
	fi

	prAM=$(i18n pAM)
	prPM=$(i18n pPM)
	hrAM=$(i18n hAM)
	hrPM=$(i18n hPM)
	tfmt=$(uci -q get gargoyle.global.hour_style)
	tabs=""

	otime() {
		for h in $(seq 0 23); do
			tstr=""
			[ -z $tfmt ] || [ $tfmt = "12" ] && {
				hr=$(date -u -d $h:00 +"%I:%M")
				[ $h -lt 12 ] && {
					tstr="$prAM $hr $hrAM"
				} || {
					tstr="$prPM $hr $hrPM"
				}
			} || {
				tstr=$(date -u -d $h:00 +"%H:%M")
			}
			echo -e "$tabs<option value=\"$h\">$tstr</option>"
			[ $h = 0 ] && tabs="$1"
		done
	}
%>
</script>

<h1 class="page-header"><%~ reboot.RbSect %></h1>
<div class="row">
	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ reboot.RbSect %></h3>
			</div>

			<div class="panel-body">
				<button id="reboot_button" class="btn btn-danger btn-lg" onclick="reboot()"><%~ Reboot %></button>
			</div>
		</div>
	</div>

	<div class="col-lg-4">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ SchRb %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12">
						<select id="sched_reboot" class="form-control" onchange="setVisibility()">
							<option value="none"><%~ NoSch %></option>
							<option value="scheduled"><%~ RbSch %></option>
						</select>
					</span>
				</div>

				<div id="schedule_reboot_container">
					<div class="row form-group">
						<label class="col-xs-5" for="reboot_interval"><%~ WillR %>:</label>
						<span class="col-xs-7">
							<select id="reboot_interval" class="form-control" onchange="setVisibility()">
								<option value="day"><%~ EDay %></option>
								<option value="week"><%~ EWek %></option>
								<option value="month"><%~ EMnh %></option>
							</select>
						</span>
					</div>

					<div id="reboot_day_container" class="row form-group">
						<label class="col-xs-5" id="reboot_day_label" for="reboot_day"><%~ RDay %>:</label>
						<span class="col-xs-7"><select id="reboot_day" class="form-control"></select></span>
					</div>

					<div id="reboot_hour_container" class="row form-group">
						<label class="col-xs-5" id="reboot_hour_label" for="reboot_hour"><%~ RHr %>:</label>
						<span class="col-xs-7">
							<select id="reboot_hour" class="form-control">
								<% otime "\t\t\t\t" %>
							</select>
						</span>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="bottom_button_container" class="panel panel-default">
	<button id="save_button" class="btn btn-primary btn-lg" onclick="saveChanges()"><%~ SaveChanges %></button>
	<button id="reset_button" class="btn btn-warning btn-lg" onclick="resetData()"><%~ Reset %></button>
</div>

<iframe id="reboot_test" onload="reloadPage()" style="display:none" ></iframe>


<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "system" -p "reboot"
%>
