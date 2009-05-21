#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	valid=$( eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR"  ) | grep "Set-Cookie" )
	require=$(uci get gargoyle.global.require_web_password)
	if [ "$require" = "0" ] ; then
		eval $( gargoyle_session_validator -g -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR"  ) 
		valid="1"
	fi
	if [ -n "$valid" ] ; then
		echo "HTTP/1.1 301 Moved Permanently" 
		echo "Location: overview.sh"
		exit
	fi

	gargoyle_header_footer -h  -c "internal.css" -j "login.js"
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
?>
//-->
</script>
	


<fieldset>
	<legend class="sectionheader">Login</legend>
	<span class="leftcolumn" style="color:red">
		<p>
			<strong>
				<span id="login_status"></span>
			</strong>
		</p>
	</span>
	<div>
		<label class="leftcolumn" for='password' id='password_label'>Enter Admin Password:</label>
		<input class="rightcolumn" type='password' onchange="doLogin()"  onkeyup='proofreadLengthRange(this,1,999)' id='password' size='25' maxlength='35' />
	</div>
	<div>
		<span class="leftcolumn"><input class="default_button" type="button" value="Login" onclick="doLogin()" /></span>
	</div>
	
</fieldset>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	setStatus();
//-->
</script>
	

<?
	gargoyle_header_footer -f 
?>
