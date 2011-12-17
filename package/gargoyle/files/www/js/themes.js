/*
 * Copyright (c) 2011 Eric Bishop and Cezary Jackiewicz <cezary@eko.one.pl>  
 * and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 *
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
	var theme = uciOriginal.get("gargoyle", "global", "theme");
	var current = "";

	for (idx=0; idx < themes.length; idx++)
	{
		current = (themes[idx] == theme) ? "*" : "";
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
	cmd.push("uci set gargoyle.global.theme=\"" + theme + "\"");
	cmd.push("uci commit");
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
