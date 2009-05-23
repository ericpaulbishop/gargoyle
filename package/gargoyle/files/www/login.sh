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
		firstboot=$( uci get gargoyle.global.is_first_boot 2>/dev/null )
		echo "HTTP/1.1 301 Moved Permanently" 
		if [ "$firstboot" = "1" ] ; then
			echo "Location: firstboot.sh"
		else
			echo "Location: overview.sh"
		fi
		exit
	fi
	dump_quotas
	quotas=$(uci show firewall | grep "=quota$" | sed 's/^.*\.//g' | sed 's/=.*$//g' )
	allq=""
	ipq=""
	for q in $quotas ; do
		ip=$(uci get firewall.$q.ip 2>/dev/null)
		if [ "$ip" = "ALL" ] ; then
			allq="$q"
		fi
		if [ "$ip" = "$REMOTE_ADDR" ] ; then
			ipq="$q"
		fi
	done

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
	if [ "$FORM_logout" = "1" ] ; then
		echo "var loggedOut = true;"
	else
		echo "var loggedOut = false;"
	fi
	echo "var allq= [];"
	echo "var ipq= [];"
	if [ -n "$allq" ] ; then
		uci show "firewall.$allq" | sed 's/^/allq.push("'/g | sed 's/$/");/g'
	fi
	if [ -n "$ipq" ] ; then
		uci show "firewall.$ipq" | sed 's/^/ipq.push("'/g | sed 's/$/");/g'
	fi
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
		<input class="rightcolumn" type='password' onchange="doLogin()"  onkeyup='proofreadLengthRange(this,1,999)' id='password' size='25' maxlength='35' />
	</div>
	<div>
		<span class="leftcolumn"><input class="default_button" type="button" value="Login" onclick="doLogin()" /></span>
	</div>
	
</fieldset>
<fieldset id="your_quota" style="display:none">
	<legend class="sectionheader">Your Quota</legend>
	<div class="nocolumn" id="up_your_quota_container"><p id="up_your_quota"></p></div>
	<div class="nocolumn" id="down_your_quota_container"><p id="down_your_quota"></p></div>
	<div class="nocolumn" id="combined_your_quota_container"><p id="combined_your_quota"></p></div>

</fieldset>
<fieldset id="network_quota" style="display:none">
	<legend class="sectionheader">Entire Network Quota</legend>
	<div class="nocolumn" id="up_all_quota_container"><p id="up_all_quota"></p></div>
	<div class="nocolumn" id="down_all_quota_container"><p id="down_all_quota"></p></div>
	<div class="nocolumn" id="combined_all_quota_container"><p id="combined_all_quota"></p></div>

</fieldset>


<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	setStatusAndQuotas();
//-->
</script>
	

<?
	gargoyle_header_footer -f 
?>
