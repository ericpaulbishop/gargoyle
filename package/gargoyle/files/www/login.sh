#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL 
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
		echo "HTTP/1.1 301 Moved Permanently" 
		if [ "$firstboot" = "1" ] ; then
			echo "Location: firstboot.sh"
		else
			echo "Location: overview.sh"
		fi
		exit
	fi

	web_root=$(uci get gargoyle.global.web_root 2>/dev/null)
	
	js="login.js"
	if [ -d "$web_root/hooks/login" ] ; then
		sh_hooks=$(ls "$web_root/hooks/login/"*.sh | sort )
		js_hooks=$(ls "$web_root/hooks/login" | sort | awk " \$1 ~ /js\$/ { print \"../hooks/login/\"\$1  }")
		js_hooks=$(echo $js_hooks)
		js="$js $js_hooks"
	fi


	gargoyle_header_footer -h  -c "internal.css" -j "$js"
?>


<script>
<!--

var passInvalid = false;
<?
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

	dateformat=$(uci get gargoyle.global.dateformat 2>/dev/null)
	if [ "$dateformat" == "iso" ]; then
		current_time=$(date "+%Y/%m/%d %H:%M %Z")
	elif [ "$dateformat" == "australia" ]; then
		current_time=$(date "+%d/%m/%y %H:%M %Z")
	else
		current_time=$(date "+%D %H:%M %Z")
	fi
	timezone_is_utc=$(uci get system.@system[0].timezone | grep "^UTC" | sed 's/UTC//g')
	if [ -n "$timezone_is_utc" ] ; then
		current_time=$(echo $current_time | sed "s/UTC/UTC-$timezone_is_utc/g" | sed 's/\-\-/+/g')
	fi
	echo "var currentTime = \"$current_time\";"
	
?>
//-->
</script>
	


<fieldset>
	<legend class="sectionheader">Login</legend>
	<span class="leftcolumn" >
		<p>
			<strong>
				<span id="login_status"></span>
			</strong>
		</p>
	</span>

	<div>
		<label class="leftcolumn" for='password' id='password_label'>Enter Admin Password:</label>
		<input class="rightcolumn" type='password' onchange="doLogin()"  onkeyup='proofreadLengthRange(this,1,999)' onkeydown='checkKey(event)' id='password' size='25' />
	</div>
	<div>
		<span class="leftcolumn"><input class="default_button" type="button" value="Login" onclick="doLogin()" /></span>
	</div>
	
</fieldset>
<fieldset id="local_quotas" style="display:none">
	<legend class="sectionheader">Your Quota</legend>
</fieldset>



<fieldset id="global_quotas" style="display:none">
	<legend class="sectionheader">Entire Network Quota</legend>
</fieldset>

<fieldset id="current_time" style="display:block">
	<legend class="sectionheader">Current Date & Time</legend>
	<div class="nocolumn" id="current_time_date"></div>
</fieldset>

<?
	for h in $sh_hooks ; do
		haserl $h
	done
?>

<script>
<!--
	document.getElementById('password').focus();
	setStatusAndQuotas();
//-->
</script>
	

<?
	gargoyle_header_footer -f 
?>
