/*
 * This program is copyright © 2013 BashfulBladder and Eric Bishop and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var intS=new Object();


//
//  genLangTable uses the javascript output of gpkg to provide info on available packages & builds a table from the multi-dimensional
//  associative array.
//
function genLangTable()
{
	var columnNames = [intS.Lang, intS.Desc, ''];
	var tableData = new Array();

	for(pkgName in pkg_info)
	{
		for (ver in pkg_info[pkgName])
		{
			//since firstboot.sh only comes up on a pristine router, the first version *should* be the most current - except in a case of failsafe...
			var pStatus=pkg_info[pkgName][ver]["Status"].split(" ");
			tableData.push([pkgName,
							pkg_info[pkgName][ver]["Description"]==null?"":pkg_info[pkgName][ver]["Description"],
							pStatus[2]=="not-installed"?createInstallButton(1):createInstallButton(0)]);
			break;
		}
	}

	var langTable = createTable(columnNames, tableData, "lang_table", false, false);
	
	var tableContainer = document.getElementById("lang_table_container");
	tableContainer.appendChild(langTable);
}


function do_get_lfile()
{
	var fname=document.getElementById('lfile').value;
	
	if (fname.length > 0 )
	{
		if (fname.match(/plugin-gargoyle-i18n/) && fname.substring(fname.lastIndexOf('.') + 1) == 'ipk')
		{
			setControlsEnabled(false, true, intS.UpMsg);

			document.getElementById('lfile_fname').value = fname;
			document.getElementById('lfile_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
			document.getElementById('lfile_form').submit();
		}
	}
}

function InstallLang()
{
	console.log(this.parentNode.parentNode.firstChild.innerHTML);
	var pkg = this.parentNode.parentNode.firstChild.innerHTML;
	var cmd = [ "sh /usr/lib/gargoyle/remove_gargoyle_package.sh " + pkg ];
	//execute(cmd);
}

function ldone()
{
	setControlsEnabled(true);
	window.location.reload(true);
}

function createInstallButton(type)
{
	var inButton = createInput("button");
	if (type == 1)
	{
		inButton.value = UI.Install;
		inButton.className="default_button";
		inButton.onclick = InstallLang;
	}
	else
	{
		inButton.value = intS.Instd;
		inButton.className="default_button_disabled";
		inButton.onclick = ""
	}
	return inButton;
}

function createUseButton(type)
{
	var useButton = createInput("button");
	if (type == 1)
	{
		useButton.value = intS.Acv8;
		useButton.className="default_button";
		useButton.onclick = activateLang;
	}
	else
	{
		useButton.value = intS.Actv;
		useButton.className="default_button_disabled";
		useButton.onclick = ""
	}
	return useButton;
}

function resetData()
{
	
	var tableContainer = document.getElementById('lang_table_container');
	tableContainer.appendChild(GenLangDiv(2));
	
	var columnNames = ['', '', ''];
	var TableData = new Array();
	var active_lang = uciOriginal.get("gargoyle", "global", "language");

	for (idx=0; idx < langs.length; idx++)
	{
		if (langs[idx] == "universal" || langs[idx] == "graphics") { continue; }
		TableData.push([ langs[idx], (langs[idx] == active_lang ? "*" : ""), createUseButton( (langs[idx]==active_lang?0:1) ) ]);
	}

	var Table = createTable(columnNames, TableData, "lang_table", false, false);
	//if(tableContainer.firstChild != null)
	//{
	//	tableContainer.removeChild(tableContainer.firstChild);
	//}
	tableContainer.appendChild(Table);
	
}

function activateLang(row, action)
{
	var row = this.parentNode.parentNode;
	var lang = row.firstChild.firstChild.data;

	var cmd = [];
	//cmd.push("uci set gargoyle.global.language=\"" + lang + "\"");
	cmd.push(". /usr/lib/gargoyle/i18nServices.sh");
	cmd.push("change_menu_language \"" + lang + "\"");
	cmd.push("uci commit");

	commands = cmd.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, UI.WaitSettings);

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
