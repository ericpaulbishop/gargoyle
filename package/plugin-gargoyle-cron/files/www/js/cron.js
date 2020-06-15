/*
 * This program is copyright © 2020 Michael Gray and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 *
 * Originally written by Saski (c) 2013 under GPL. Rewritten for modern Gargoyle.
 */

var cronStr=new Object;
var gargoyleDefault = [
	'* * * * * if [ -z "$(ifconfig | grep tun 2>/dev/null)" ] ; then logger "openvpn stopped, restarting" ; /etc/init.d/openvpn restart ; fi',
	'0,1,11,21,31,41,51 * * * * /usr/bin/set_kernel_timezone >/dev/null 2>&1',
	'0 0,4,8,12,16,20 * * * /tmp/bw_backup/do_bw_backup.sh',
	'0 0,4,8,12,16,20 * * * /tmp/bw_backup/do_openvpn_bw_backup.sh',
	'0 0,4,8,12,16,20 * * * /tmp/do_webmon_backup.sh',
	'0 0,4,8,12,16,20 * * * /tmp/bw_backup/do_tor_bw_backup.sh',
	'* * * * * /usr/sbin/update_tor_ipset',
	'0 4 * * 0 sh /plugin_root/usr/lib/adblock/runadblock.sh',
	'0 0,4,8,12,16,20 * * * /usr/bin/backup_quotas',
	'0 0,4,8,12,16,20 * * * /tmp/bw_backup/do_wireguard_bw_backup.sh',
	'0,15,30,45 * * * * /usr/bin/wireguard_watchdog'
];

function saveChanges()
{
	taskTable = document.getElementById('task_table_container').firstChild;	
	tableData = getTableDataArray(taskTable, true, false);
	
	createCommands = [ "touch /etc/crontabs/root", "rm /etc/crontabs/root" ];
	taskTableData = new Array();
	for (rowIndex in tableData)
	{
		rowData = tableData[rowIndex];
		createCommands.push("echo '" + rowData.slice(0,2).join(" ").replace(/'/g,"'\\''") + "' >> /etc/crontabs/root");
	}
	var commands = createCommands.join("\n");
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	setControlsEnabled(false, true, UI.Wait);
        
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			setControlsEnabled(true);
			resetData();
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function addNewTask()
{
	var task_script = document.getElementById("task_script").value;
	if(task_script == "")
	{
		alert(cronStr.NoScript);
		return;
	}
	else
	{
		minute = getMultipleSelect("task_minute");
		hour = getMultipleSelect("task_hour");
		day = getMultipleSelect("task_day");
		month = getMultipleSelect("task_month");
		dayweek = getMultipleSelect("task_dayweek");
		values = new Array();
		values.push(minute + ' ' + hour + ' ' + day + ' ' + month + ' ' + dayweek);
		values.push(task_script);
		values.push(createEditButton());
		task_table = document.getElementById('task_table_container').firstChild;
		addTableRow(task_table, values, true, false, false);
		closeModalWindow('cron_task_modal');
	}
}

function getMultipleSelect(selectId)
{
	var selectEl = document.getElementById(selectId);
	if(selectEl)
	{
		if(selectEl.disabled)
		{
			return "*";
		}
		var options = selectEl.selectedOptions;
		if(options.length == 0)
		{
			return "*";
		}
		optionstr = "";
		for(var x = 0; x < options.length; x++)
		{
			if(optionstr != "")
			{
				optionstr = optionstr + ",";
			}
			optionstr = optionstr + options[x].value;
		}
		return optionstr;
	}
	else
	{
		return "*";
	}
}

function createEditButton()
{
	var editButton = createInput("button");
	editButton.textContent = UI.Edit;
	editButton.className="btn btn-default btn-edit";
	editButton.onclick = editCronModal;
	return editButton;
}

function editTask(editRow)
{
	var task_script = document.getElementById("task_script").value;
	if(task_script == "")
	{
		alert(cronStr.NoScript);
		return;
	}
	else
	{
		minute = getMultipleSelect("task_minute");
		hour = getMultipleSelect("task_hour");
		day = getMultipleSelect("task_day");
		month = getMultipleSelect("task_month");
		dayweek = getMultipleSelect("task_dayweek");
		editRow.childNodes[0].firstChild.data = minute + ' ' + hour + ' ' + day + ' ' + month + ' ' + dayweek;
		editRow.childNodes[1].firstChild.data = task_script;
	}
					
	closeModalWindow('cron_task_modal');
}
		
function resetData()
{
	var commands="cat /etc/crontabs/root 2>/dev/null";
	var param = getParameterDefinition("commands", commands) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	var stateChangeFunction = function(req)
	{
		if(req.readyState == 4)
		{
			var taskLines = req.responseText.split(/[\n\r]+/);
			var taskDataTable = new Array();
			var taskIndex;
			document.getElementById("task").style.display = taskLines[0].match(/^Success/) == null ? "block" : "none";
			document.getElementById("no_task").style.display = taskLines[0].match(/^Success/) == null ? "none" : "block";
			for(taskIndex=0; taskLines[taskIndex].match(/^Success/) == null; taskIndex++)
			{
				cronparts = taskLines[taskIndex].split(" ");
				crontime = cronparts.slice(0,5).join(" ");
				croncmd = cronparts.slice(5).join(" ");

				taskDataTable.push([crontime, croncmd, createEditButton()]);
			}
			var columnNames = [cronStr.dt,cronStr.cmd,''];
			var taskTable = createTable(columnNames, taskDataTable, "task_table", true, false);
			var tableContainer = document.getElementById('task_table_container');
			if(tableContainer.firstChild != null)
			{
				tableContainer.removeChild(tableContainer.firstChild);
			}	
			tableContainer.appendChild(taskTable)
			updateInProgress = false;
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
	
	var minute = gen(0, 59)
	setAllowableSelections("task_minute", minute, minute);
	var hour = gen(0, 23)
	setAllowableSelections("task_hour", hour, hour);
	var day = gen(1, 31)
	setAllowableSelections("task_day", day, day);
	var month = gen(1, 12)
	var month_name = new Array;
	var month_name = cronStr.Months
	setAllowableSelections("task_month", month, month_name);
	var dayweek = gen(0, 6)
	var dayweek_name = new Array(UI.Sunday, UI.Monday, UI.Tuesday, UI.Wednesday, UI.Thursday, UI.Friday, UI.Saturday);
	setAllowableSelections("task_dayweek", dayweek, dayweek_name);
}

function gen(min, max)
{
	tab = new Array
	var index = 0
	for(var lp=min; lp<=max; lp++)
	{	
		tab[index++] = lp;
	}
	return tab;
}

function addCronModal()
{
	modalButtons = [
		{"title" : UI.Add, "classes" : "btn btn-primary", "function" : addNewTask},
		"defaultDismiss"
	];

	var minute = "";
	var hour = "";
	var day = "";
	var month = "";
	var weekday = "";
	var script = "";

	modalElements = [
		{"id" : "task_minute", "value" : minute},
		{"id" : "task_hour", "value" : hour},
		{"id" : "task_day", "value" : day},
		{"id" : "task_month", "value" : month},
		{"id" : "task_dayweek", "value" : weekday},
		{"id" : "task_script", "value" : script}
	];

	cronControlIds = ["task_minute","task_hour","task_day","task_month","task_dayweek"];
	for(cronControlIndex=0; cronControlIndex < cronControlIds.length; cronControlIndex++)
	{
		checkbox =  document.getElementById( "use_" + cronControlIds[cronControlIndex]);
		checkbox.checked = false;
		enableAssociatedField( checkbox, cronControlIds[cronControlIndex], "");
	}
	document.getElementById("cron_gargoyle").style.display = "none";
	modalPrepare('cron_task_modal', cronStr.AddTasks, modalElements, modalButtons);
	openModalWindow('cron_task_modal');
}

function editCronModal()
{
	editRow=this.parentNode.parentNode;
	modalButtons = [
		{"title" : UI.CApplyChanges, "classes" : "btn btn-primary", "function" : function(){editTask(editRow);}},
		"defaultDiscard"
	];

	var crontime = editRow.childNodes[0].firstChild.data.split(" ");
	var script = editRow.childNodes[1].firstChild.data;

	var minute = crontime[0];
	var hour = crontime[1];
	var day = crontime[2];
	var month = crontime[3];
	var weekday = crontime[4];

	modalElements = [
		{"id" : "task_minute", "values" : minute},
		{"id" : "task_hour", "values" : hour},
		{"id" : "task_day", "values" : day},
		{"id" : "task_month", "values" : month},
		{"id" : "task_dayweek", "values" : weekday},
		{"id" : "task_script", "value" : script}
	];

	cronControlIds = ["task_minute","task_hour","task_day","task_month","task_dayweek"];
	cronVals = [minute,hour,day,month,weekday];
	for(cronControlIndex=0; cronControlIndex < cronControlIds.length; cronControlIndex++)
	{
		checkbox =  document.getElementById( "use_" + cronControlIds[cronControlIndex]);
		checkbox.checked = cronVals[cronControlIndex] == "*" ? false : true;
		enableAssociatedField( checkbox, cronControlIds[cronControlIndex], "");
	}
	document.getElementById("cron_gargoyle").style.display = gargoyleDefault.indexOf(crontime.join(" ")  + " " + script) > -1 ? "block" : "none";
	modalPrepare('cron_task_modal', cronStr.EdTask, modalElements, modalButtons);
	openModalWindow('cron_task_modal');
}
