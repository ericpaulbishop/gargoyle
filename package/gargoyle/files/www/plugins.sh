#!/usr/bin/haserl
<%
	# Copyright © 2012-2013 Eric Bishop, © 2011 Cezary Jackiewicz <cezary@eko.one.pl>
	# and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "plugins" -j "table.js plugins.js" -z "plugins.js" -i gargoyle
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

    du -s $plugin_root_dest | awk '{ print "var pluginRootSize="$1*1000";" }' 2>/dev/null
%>

</script>

<h1 class="page-header"><%~ plugins.mPlugins %></h1>
<div id="plugin_options" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ plugins.PgOpt %></h3>
			</div>

			<div class="panel-body">
				<div class="row">
					<div class="col-lg-6">
						<div class="alert alert-info" role="alert">
							<div class="row form-group">
								<span class="col-xs-5"><%~ PRoot %>:</span>
								<span class="col-xs-7" id="plugin_root_static">/plugin_root</span>
								<span class="col-xs-7"><input type="text" id="plugin_root_text" style="display:none" /></span>
							</div>

							<div class="row form-group">
								<span class="col-xs-12" id="plugin_root_drive_static" for="plugin_root_drive_select"><%~ RDrv %></span>
								<span class="col-xs-12"><select id="plugin_root_drive_select" class="form-control" onchange="updatePluginRootDisplay()" style="display:none"></select></span>
							</div>

							<div id="plugin_root_change_container" class="row form-group" style="display:none" >
								<span class="col-xs-12"><button class="btn btn-default" onclick="changePluginRoot()"><%~ Chroot %></button></span>
							</div>
						</div>
					</div>
				</div>

				<div class="row">
					<div class="col-lg-12">
						<div class="row form-group">
							<h3 class="col-xs-12"><%~ PgSrc %>:</h3>
						</div>

						<div id="package_source_table_container" class="table-responsive"></div>

						<div class="row form-group">
							<label class="col-xs-5" for="add_source_name"><%~ ANam %>:</label>
							<span class="col-xs-7"><input type="text" id="add_source_name" class="form-control" onkeyup="proofreadSourceName(this)"/></span>
						</div>

						<div class="row form-group">
							<label class="col-xs-5" for="add_source_url"><%~ Aurl %>:</label>
							<span class="col-xs-7"><input type="text" id="add_source_url" class="form-control" /></span>
						</div>

						<button id="add_source_button" class="btn btn-default btn-add" onclick="addPluginSource()"><%~ APSrc %></button>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="plugin_list" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ PList %></h3>
			</div>

			<div class="panel-body">
				<div class="row form-group">
					<span class="col-xs-12"><button id="update_button" class="btn btn-primary btn-lg" onclick="updatePackagesList()"><%~ RfshP %></button></span>
				</div>

				<div class="row form-group">
					<span id="wan-warn" class="alert alert-warning" role="alert" style="display:none;"><%~ NoWan %></span>
				</div>

				<div class="row form-group">
					<div class="col-lg-12">
						<div id="languages_table_container" class="table-responsive"></div>
					</div>
				</div>

				<div class="row form-group">
					<div class="col-lg-12">
						<div id="themes_table_container" class="table-responsive"></div>
					</div>
				</div>

				<div class="row form-group">
					<div class="col-lg-12">
						<div id="packages_table_container" class="table-responsive"></div>
					</div>
				</div>

				<div id="no_packages" style="display:none;">
					<%~ NoPkg %>
				</div>
			</div>
		</div>
	</div>
</div>

<script>
	resetData();
</script>

<%
	gargoyle_header_footer -f -s "system" -p "plugins"
%>
