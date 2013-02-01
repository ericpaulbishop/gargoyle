#!/usr/bin/haserl
<?
	# Copyright © 2012 Eric Bishop, © 2011 Cezary Jackiewicz <cezary@eko.one.pl>
	# and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "plugins" -c "internal.css" -j "table.js plugins.js" gargoyle
?>
<script>

<?

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

	opkg_defs=$(opkg-more --packages-matching /^plugin\-gargoyle/  --install-destination --required-size --required-depends --description --version --will-fit plugin_root --javascript 2>/dev/null)
	if [ -z "$opkg_defs" ] ; then
		opkg-more --packages not_a_real_package --will-fit root --javascript
	else
		printf "%s\n" "$opkg_defs" 
	fi

	echo "var pluginSources = [];"
	awk  '$0 ~ /^src.gz/  { print "pluginSources.push([\"" $2 "\", \"" $3 "\"])" ; }' /etc/opkg.conf 2>/dev/null

	echo "var storageDrives = [];"
	awk '{ print "storageDrives.push([\""$1"\",\""$2"\",\""$3"\",\""$4"\", \""$5"\", \""$6"\"]);" }' /tmp/mounted_usb_storage.tab 2>/dev/null

?>

</script>
<form>
	
	<fieldset id="plugin_options">
		<legend class="sectionheader">Plugin Options</legend>
		
		<div>
			<span class="narrowleftcolumn">Plugin Root:</span>
			<span id="plugin_root_static" class="widerightcolumn">/plugin_root</span>
			<input type="text" id="plugin_root_text" class="widerightcolumn" style="display:none" />
		</div>
		<div>
			<span id="plugin_root_drive_static" class="widerightcolumnonly" for="plugin_root_drive_select">Root Drive</span>
			<select id="plugin_root_drive_select" class="widerightcolumnonly" onchange="updatePluginRootDisplay()" style="display:none"></select>
		</div>
		<div id="plugin_root_change_container" style="display:none" >
			<span class="widerightcolumnonly" >
				<input type="button" class="default_button" value="Change Plugin Root" onclick="changePluginRoot()" />
			</span>
		</div>
		<br/>


		<div>
			<span class="leftcolumn">Plugin Sources:</span>
		</div>
		<div id="package_source_table_container" style="margin-left:5px;" ></div>
		<div class="indent">
			<div>
				<label class="narrowleftcolumn" for="add_source_name">Add Name:</label>
				<input type="text" class="widerightcolumn" id="add_source_name" onkeyup="proofreadSourceName(this)" style="width:325px;"/>
			</div>
			<div>
				<label class="narrowleftcolumn" for="add_source_url">Add URL:</label>
				<input type="text" class="widerightcolumn" id="add_source_url" style="width:325px;"/>
			</div>
			
			<span class="leftcolumn"><input type="button" class="default_button" id="add_source_button" value="Add Plugin Source" onclick="addPluginSource()" /></span>
			
		</div>
	</fieldset>


	<fieldset id="plugin_list">
		<legend class="sectionheader">Plugin List</legend>
		<div>
			<div id="packages_table_container" style="margin-left:5px" ></div>
		</div>
		<div id="no_packages" style='display:none;'>
			Packages not found. Refresh plugins list.
		</div>
		<div id="bottom_button_container">
			<input type='button' value='Refresh Plugins' id="update_button" class="bottom_button" onclick='updatePackagesList()' />
		</div>
	</fieldset>

</form>

<script>
	resetData();

</script>

<?
	gargoyle_header_footer -f -s "system" -p "plugins"
?>
