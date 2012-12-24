#!/usr/bin/haserl
<?
#
#       Copyright (c) 2011 Cezary Jackiewicz <cezary@eko.one.pl>
#       Copyright (c) 2012 Eric Bishop <eric@gargoyle-router.com>
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#      MA 02110-1301, USA.

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "plugins" -c "internal.css" -j "table.js plugins.js" gargoyle
?>
<script>

<?
	opkg_defs=$(opkg-more --packages-matching /^plugin\-gargoyle/  --install-destination --required-size --required-depends --description --version --will-fit plugin_root --javascript 2>/dev/null)
	if [ -z "$opkg_defs" ] ; then
		echo "var opkg_info = [];"
		echo "var opkg_matching_packages = [];"
	else
		printf "%s\n" "$opkg_defs" 
	fi
	
	
	
?>

</script>
<form>
	
	<fieldset id="plugin_options">
		<legend class="sectionheader">Plugin Options</legend>
		
		<div>
			<span class="leftcolumn">Plugin Root:</span>
			<span id="plugin_root_static" class="rightcolumn">/plugin_root</span>
			<input type="text" id="plugin_root_text" class="rightcolumn" style="display:none" />
		</div>
		<div>
			<span id="plugin_root_drive_static" class="rightcolumnonly">Root Drive</span>
			<select id="plugin_root_drive_select" class="rightcolumnonly" style="display:none"></select>
		</div>
		<div>
			<span id="plugin_root_bytes_free" class="rightcolumnonly"></span>
		</div>
		<br/>


		<div>
			<span class="leftcolumn">Plugin Sources:</span>
		</div>	
		<div id="package_source_table_container" style="margin-left:15px;" ></div>

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
