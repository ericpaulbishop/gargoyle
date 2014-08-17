#!/usr/bin/haserl --upload-limit=8192 --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "update" -c "internal.css" -j "update.js" -z "update.js" network
%>

<script>
<!--
<%
	is_brcm=$(cat /proc/cpuinfo | grep Broadcom)
	is_atheros=$(cat /proc/cpuinfo | grep "system type" | grep "Atheros AR[1-6]")
	is_ar71xx=$(cat /proc/cpuinfo  | grep "system type" | grep "Atheros AR[7-9]")
	if [ -n "$is_brcm" ] || [ -e /lib/wifi/broadcom.sh ] ; then
		echo "var platform=\"broadcom\";"
	elif [ -n "$is_atheros" ] ; then
		echo "var platform=\"atheros\";"
	elif [ -n "$is_ar71xx" ] ; then
		echo "var platform=\"ar71xx\";"
	else
		echo "var platform=\"x86\";"
	fi

	gargoyle_version=$(cat data/gargoyle_version.txt)
	echo "var gargoyleVersion=\"$gargoyle_version\""
%>
//-->
</script>

<fieldset id="upgrade_section">
	<legend class="sectionheader"><%~ update.UpFrm %></legend>
	<div>
		<div>
			<p><%~ Warn %></p>
		</div>

		<div class="internal_divider"></div>

		<div>
			<span class='leftcolumn'><%~ CGV %>:</span><span id="gargoyle_version" class='rightcolumn'></span>
		</div>

		<div class="internal_divider"></div>

		<form id='upgrade_form' enctype="multipart/form-data" method="post" action="utility/do_upgrade.sh" target="do_upgrade">

			<div id="upgrade_file1_container">
				<label id="upgrade_label" class='leftcolumn' for="upgrade_file"><%~ SelF %>:</label>
				<input class='rightcolumn' type="file" id="upgrade_file" name="upgrade_file" />
				<br/>
				<em><span id="upgrade_text" class="rightcolumnonly"></span></em>
			</div>
			<div id="upgrade_preserve_container">
				<span class="rightcolumnonly">
					<input type="checkbox" id="upgrade_preserve" name="upgrade_preserve" style="padding:0;margin:0px;vertical-align:middle;overflow:hidden;" />
					<label id="upgrade_preserve_label" for="upgrade_preserve" style="vertical-align:middle"><%~ Prsv %></label>
				</span>
			</div>

			<input id='upgrade_hash' name="hash" type='hidden' value='' />
			<input id='upgrade_arch' name="arch" type='hidden' value='' />
		</form>
	</div>
	<div>
		<input id="upgrade_button" type='button' class="default_button" value="<%~ Upgrade %>" onclick="doUpgrade()" style="margin-left:0px;"/>
	</div>

	<iframe id="do_upgrade" name="do_upgrade" src="#" style="display:none"></iframe> 

<script>
<!--
	setUpgradeFormat();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "update" 
%>
