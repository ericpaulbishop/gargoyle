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
	var columnNames = [intS.Lang, intS.Actv, ''];
	var tableData = new Array();


	var curLangPkgName = "plugin-gargoyle-i18n-" + uciOriginal.get("gargoyle", "global", "language")

	for(pkgName in pkg_info)
	{
		for (ver in pkg_info[pkgName])
		{
			//since firstboot.sh only comes up on a pristine router, the first version *should* be the most current - except in a case of failsafe...
			var pStatus=pkg_info[pkgName][ver]["Status"].split(" ");
			var descriptionSpan = document.createElement("span");
			var descriptionStr =  pkg_info[pkgName][ver]["Description"]==null ? "" : pkg_info[pkgName][ver]["Description"] 
			descriptionSpan.id = pkgName;
			descriptionSpan.appendChild(document.createTextNode(descriptionStr));
			tableData.push([
					descriptionSpan,
					pStatus[2] == "not-installed" ? "" : createUseCheck( pkgName == curLangPkgName ? 0 : 1 ),
					createInstallButton(pStatus[2] == "not-installed" ? 1 : 0)
					]
					);
			break;
		}
	}

	var langTable = createTable(columnNames, tableData, "lang_table", false, false);
	
	return langTable;

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

function installLang()
{
	var pkg = this.parentNode.parentNode.firstChild.firstChild.id;
	var cmd = [ "sh /usr/lib/gargoyle/install_gargoyle_package.sh " + pkg ];
	execute(cmd);
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
		inButton.style.marginLeft = "0px";
		inButton.value = UI.Install;
		inButton.className="default_button";
		inButton.onclick = installLang;
	}
	else
	{
		inButton = document.createTextNode(intS.Instd);
	}
	return inButton;
}

function createUseCheck(type)
{
	var useCheck = createInput("checkbox");
	useCheck.checked = type == 1 ? false : true;
	useCheck.onclick=activateLang;
	return useCheck;
}

function resetFirstBootData()
{
	document.getElementById("upload_lang_button").value = intS.Upld +" \u21e7";
	var langTable = genLangTable();
	var tableContainer = document.getElementById("lang_table_container");
	tableContainer.appendChild(langTable);
	
}


function resetLanguagesData()
{
	var langTable = genLangTable();
	var tableContainer = document.getElementById("lang_table_container");
	tableContainer.appendChild(langTable);

}

function activateLang()
{
	var lang = this.parentNode.parentNode.firstChild.firstChild.id;

	var cmd = [];
	cmd.push(". /usr/lib/gargoyle/i18nServices.sh");
	cmd.push("change_menu_language \"" + lang + "\"");

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
