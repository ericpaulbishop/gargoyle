#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	valid=$( eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" ) | grep "Set-Cookie" )
	require=$(uci get gargoyle.global.require_web_password)
	if [ "$require" = "0" ] ; then
		eval $( gargoyle_session_validator -g -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -t $(uci get gargoyle.global.session_timeout) )
		valid="1"
	fi
	if [ -n "$valid" ] ; then
		firstboot=$( uci get gargoyle.global.is_first_boot 2>/dev/null )
		echo "Status: 302 Found"
		if [ "$firstboot" = "1" ] ; then
			echo "Location: /firstboot.sh"
		else
			echo "Location: /overview.sh"
		fi
		echo ""
		echo ""
		exit
	fi

	web_root=$(uci get gargoyle.global.web_root 2>/dev/null)

	# Append shared login hook resources as whitespace separated strings to default resources.
	css=""
	js="login.js"
	i18n="$js"
	pkg="gargoyle"
	# Iterate over section array where [$i].option is simply appended to the $hook string.
	i=0
	hook="uci -q get gargoyle.@login_hook"
	while [ -n "$($hook[$i])" ]; do
		css="$css $($hook[$i].css)"
		js="$js $($hook[$i].js)"
		i18n="$i18n $($hook[$i].i18n)"
		[ "$($hook[$i].mac)" = "1" ] && mac="-i"
		[ "$($hook[$i].host)" = "1" ] && host="-n"
		pkg="$pkg $($hook[$i].pkg)"
		i=$((i+1))
	done
	if [ -d "$web_root/hooks/login" ] ; then
		js_hooks=$(ls "$web_root/hooks/login/js" | sort | awk " \$1 ~ /js\$/ { print \"../hooks/login/js/\"\$1  }")
		js_hooks=$(echo $js_hooks)
		js="$js $js_hooks"
		i18n_hooks=$(echo "$js" | awk -F '[ /]' '{ for(i = 1; i <= NF; i++) {  print $i; } }' | awk '/.js/')
		i18n="$i18n $(echo $i18n_hooks)"
	fi

	gargoyle_header_footer -h -c "$css" -j "$js" -z "$i18n" "$mac" "$host" -- $pkg
%>


<script>
<!--

var passInvalid = false;
<%
	if [ "$FORM_expired" = "1" ] ; then
		echo "var sessionExpired = true;"
	else
		echo "var sessionExpired = false;"
	fi
	if [ "$FORM_logout" = "1" ] ; then
		echo "var loggedOut = true;"
	else
		echo "var loggedOut = false;"
	fi
	echo "var connectedIp = \"$REMOTE_ADDR\";"
	print_quotas

	. /usr/lib/gargoyle/current_time.sh
%>
//-->
</script>

<h1 class="page-header"><%~ login.LSect %></h1>
<div id="login_status" class="alert alert-danger" role="alert" style="display:none;"></div>

<div class="row">
	<div class="col-lg-12">
		<div class="panel panel-default"> 
			<div class="panel-heading"> 
				<h3 class="panel-title"><%~ LSect %></h3> 
			</div> 
			<div class="panel-body">
				<div class="row form-group">
					<label class="sr-only" for="password" id="password_label"><%~ EAdmP %></label>
					<span class="col-xs-12">
						<input id="password" class="form-control" type="password" oninput="proofreadLengthRange(this,1,999)" onkeydown="checkKey(event)" size="25" placeholder="<%~ EAdmP %>" autocomplete="current-password"/>
						<button class="btn btn-default" onclick="doLogin()" ><%~ LSect %></button>
					</span>
				</div>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-4">
		<div id="current_time" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ CTime %></h3>
			</div>

			<div class="panel-body">
				<div id="current_time_date"></div>
			</div>
		</div>
	</div>

	<div class="col-lg-4">
		<div id="current_ip" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ CIP %></h3>
			</div>

			<div class="panel-body">
				<div id="current_connected_ip"><%~ CIPs %><div id="current_connect_ip"></div></div>
			</div>
		</div>
	</div>
</div>
<div class="row">
	<div class="col-lg-4">
		<div id="local_quotas" class="panel panel-default" style="display:none">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ YQot %></h3>
			</div>
			<div class="panel-body"></div>
		</div>
	</div>

	<div class="col-lg-4">
		<div id="global_quotas" class="panel panel-default" style="display:none">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ NQot %></h3>
			</div>
			<div class="panel-body"></div>
		</div>
	</div>
</div>
<%din hooks/login/ %>

<script>
<!--
	document.getElementById('password').focus();
	setStatusAndQuotas();
	document.getElementById("current_connect_ip").innerHTML=connectedIp;
//-->
</script>

<%
	gargoyle_header_footer -f
%>
