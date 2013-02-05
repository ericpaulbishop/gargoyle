#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m  -c "internal.css" -j "ddns.js"
?>
<fieldset id="edit_container">
	<legend class="sectionheader">Edit Dynamic DNS Service</legend>

	<div>
		<div>
			<label class='leftcolumn' for='ddns_provider' id='ddns_provider_label'>Service Provider:</label>
			<span class='rightcolumn' id="ddns_provider_text" ></span>
		</div>
	</div>

	<div id="ddns_variable_container"></div>

	<div>
		<div>
			<label class='leftcolumn' for='ddns_check' id='ddns_check_label'>Check Interval:</label>
			<input type='text' class='rightcolumn' id='ddns_check'  size='8' onkeyup='proofreadNumeric(this)'/>
			<em>minutes</em>
			<div class='indent'>
				<p>
				The check interval specifies how often the router will check whether your current IP matches the one currently associated with your
				domain name.  This check is performed without connecting to your dynamic DNS service provider, which means that this will not cause 
				problems with providers that ban users who connect too frequently (e.g. dyndns.com).  However, a network connection is established 
				to perform this check, so this value should not be too low.  A check interval between 10 and 20 minutes is usually appropriate.
				</p>
			</div>
		</div>
		<div>
			<label class='leftcolumn' for='ddns_force' id='ddns_force_label'>Force Update Interval:</label>
			<input type='text' class='rightcolumn' id='ddns_force'  size='8' onkeyup='proofreadNumeric(this)'/>
			<em>days</em>
			<div class='indent'>
				<p>
				The force update interval specifies how often the router will connect to your dynamic DNS service provider and update their records,
				even if your IP has not changed.  Service providers will ban users who update too frequently, but may close accounts of users who do
				not update for over a month.  It is recommended that this parameter be set between 3 and 7 days.
				</p>
			</div>
		</div>
	</div>

</fieldset>
<div id="bottom_button_container"></div>

</body>
</html>
