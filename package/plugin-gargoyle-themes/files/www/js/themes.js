/*
 *     Copyright (c) 2011 Cezary Jackiewicz <cezary@eko.one.pl>
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *     MA 02110-1301, USA.
 */

function createUseButton()
{
	var useButton = createInput("button");
	useButton.value = "Select";
	useButton.className="default_button";
	useButton.onclick = useTheme;
	return useButton;
}

function resetData()
{
	var columnNames = ['Theme', '', ''];
	var TableData = new Array();

	for (idx=0; idx < themes.length; idx++)
	{
		var current = (themes.indexOf(currentTheme) == idx)?"*":"";
		TableData.push([ themes[idx], current, createUseButton() ]);
	}

	var Table = createTable(columnNames, TableData, "themes_table", false, false);
	var tableContainer = document.getElementById('themes_table_container');
	if(tableContainer.firstChild != null)
	{
		tableContainer.removeChild(tableContainer.firstChild);
	}
	tableContainer.appendChild(Table);
}

function useTheme(row, action)
{
	var row = this.parentNode.parentNode;
	var theme = row.firstChild.firstChild.data;

	var cmd = [];
	cmd.push("rm /www/themes/default");
	cmd.push("ln -s /www/themes/\"" + theme + "\" /www/themes/default");
	cmd.push("sleep 1");

	commands = cmd.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, "Please wait...");

	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			location.reload(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
