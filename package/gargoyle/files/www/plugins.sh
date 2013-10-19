#!/usr/bin/haserl
<%
	# Copyright © 2012-2013 Eric Bishop, © 2011 Cezary Jackiewicz <cezary@eko.one.pl>
	# and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "plugins" -c "internal.css" -j "table.js plugins.js" -z "plugins.js" gargoyle
%>
<script>

<%

	if [ ! -d '/plugin_root' ] && [ ! -h '/plugin_root' ] ; then
		mkdir -p /plugin_root
		ln -s /etc /plugin_root/etc
		ln -s /tmp /plugin_root/tmp
		ln -s /var /plugin_root/var
	fi
	if [ ! -d '/var/opkg-lists' ] ; then
		mkdir -p '/var/opkg-lists'
	fi
	plug_root_dest=$( awk '$1 == "dest" && $3 == "/plugin_root" ' /etc/opkg.conf 2>/dev/null )
	if [ -z "$plug_root_dest" ] ; then
		echo "dest plugin_root /plugin_root" >>/etc/opkg.conf
	fi

	pkg_info=$(gpkg info -v 'Install-Destination,Required-Size,Required-Depends,Description,Will-Fit,User-Installed' -d plugin_root -o 'js' -r /^plugin\-gargoyle/)
	gpkg dest-info -o 'js'
	if [ -n "$pkg_info" ] ; then
		printf "%s\n" "$pkg_info" 
	fi

	echo "var pluginSources = [];"
	awk  '$0 ~ /^src.gz/  { print "pluginSources.push([\"" $2 "\", \"" $3 "\"])" ; }' /etc/opkg.conf 2>/dev/null

	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

%>

</script>
<form>

	<fieldset id="plugin_options">
		<legend class="sectionheader"><%~ plugins.PgOpt %></legend>

		<div>
			<span class="narrowleftcolumn"><%~ PRoot %>:</span>
			<span id="plugin_root_static" class="widerightcolumn">/plugin_root</span>
			<input type="text" id="plugin_root_text" class="widerightcolumn" style="display:none" />
		</div>
		<div>
			<span id="plugin_root_drive_static" class="widerightcolumnonly" for="plugin_root_drive_select"><%~ RDrv %></span>
			<select id="plugin_root_drive_select" class="widerightcolumnonly" onchange="updatePluginRootDisplay()" style="display:none"></select>
		</div>
		<div id="plugin_root_change_container" style="display:none" >
			<span class="widerightcolumnonly" >
				<input type="button" class="default_button" value="<%~ Chroot %>" onclick="changePluginRoot()" />
			</span>
		</div>
		<br/>

		<div>
			<span class="leftcolumn"><%~ PgSrc %>:</span>
		</div>
		<div id="package_source_table_container" style="margin-left:5px;" ></div>
		<div class="indent">
			<div>
				<label class="narrowleftcolumn" for="add_source_name"><%~ ANam %>:</label>
				<input type="text" class="widerightcolumn" id="add_source_name" onkeyup="proofreadSourceName(this)" style="width:325px;"/>
			</div>
			<div>
				<label class="narrowleftcolumn" for="add_source_url"><%~ Aurl %>:</label>
				<input type="text" class="widerightcolumn" id="add_source_url" style="width:325px;"/>
			</div>

			<span class="leftcolumn"><input type="button" class="default_button" id="add_source_button" value="<%~ APSrc %>" onclick="addPluginSource()" /></span>

		</div>
	</fieldset>

	<fieldset id="plugin_list">
		<legend class="sectionheader"><%~ PList %></legend>
		<div>
			<div id="languages_table_container" style="margin-left:5px" ></div>
		</div>
		<div>
			<div id="themes_table_container" style="margin-left:5px" ></div>
		</div>
		<div>
			<div id="packages_table_container" style="margin-left:5px" ></div>
		</div>
		<div id="no_packages" style='display:none;'>
			<%~ NoPkg %>
		</div>
		<div id="bottom_button_container">
			<input type='button' value='<%~ RfshP %>' id="update_button" class="bottom_button" onclick='updatePackagesList()' />
		</div>
	</fieldset>

</form>

<script>
	resetData();

</script>

<%
	gargoyle_header_footer -f -s "system" -p "plugins"
%>
