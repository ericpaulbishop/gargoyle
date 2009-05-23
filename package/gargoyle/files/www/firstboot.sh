#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" )
	gargoyle_header_footer -h -c "internal.css" -j "firstboot.js"
?>

<script>
<!--
<?
	echo "var httpUserAgent = \"$HTTP_USER_AGENT\";"
	echo "var remoteAddr = \"$REMOTE_ADDR\";"
?>
//-->
</script>



<fieldset>
	<legend class="sectionheader">Set Administrator Password</legend>
	<p><strong>Please enter a new password now:</strong></p>
	<div>
		<label class='leftcolumn' for='password1' id='password1_label'>New Password:</label>
		<input type='password' class='rightcolumn' id='password1'  size='25' />
	</div>
	<div>
		<label class='leftcolumn' for='password2' id='password2_label'>Confirm Password:</label>
		<input type='password' class='rightcolumn' id='password2'  size='25' />
	</div>
	
	<div>
		<span class="leftcolumn"><input class="default_button" type="button" value="Set Password" onclick="setPassword()" /></span>
	</div>
</fieldset>



<?
	gargoyle_header_footer -f 
?>
