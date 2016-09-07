/*
 * This program is copyright © 2015 dpint and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 *
 * Cron configuration code was derived from wifi_schedule plugin, which was written by BashfulBladder.
 *
 */

var email = new Object;
var showCronTabs = false;
var increment = 15;
var timerMode = 0;
var hour_red = "#ff0000";
var hour_partial_green = "#ff6868";
var hour_green = "#00ff00";
var garCron = "/usr/lib/gargoyle/email.sh";
var new_cron_tabs = [];
var stripped_cron_tabs = [];
var found_cron_tabs = [];
var weeklyPeriod = [];
var week511Period = [];
var dailyPeriod = [];
var shellvarsupdater = null;
var current_time = [];
var Email = 0;
var cloned_crontab_table = [];

function InitSummaryText() {
	document.getElementById("summary_container").className = 'tabField';
	var textSpan=document.getElementById("summary_txt");
	
	textSpan.innerHTML="<strong>"+email.Smmy+":</strong><br />\n";
}

function AddSummaryText(more_text) {
    document.getElementById("summary_txt").innerHTML+=more_text;
}

function generateCronTabStr(min, hour, day, extra) {
	var a_cron_string = "";
	var day_string="" ;
	
	if (timerMode == 1) {
		day_string="*";
	} else if (timerMode == 3) {
		if (day == 0) { day_string = "0"; }
		if (day == 1) { day_string = "1-5"; }
		if (day == 2) { day_string = "6"; }
	} else if (timerMode == 7) {
		day_string = day.toString();
	} else {
		return;
	}
	
	if (min == 60) {
		return;
	}else if(min == 0){
		a_cron_string = "0 " + hour + " * * " + day_string + " " + garCron;
	} else {
		a_cron_string = "" + Math.abs(min) + " " + hour + " * * " + day_string + " " + garCron;
	}
	
	new_cron_tabs.push(a_cron_string);
}

function CronTabCull() {
	var culledTabs = [];
	
	for(var i = 0; i < new_cron_tabs.length; i++) {
		if ( (new_cron_tabs[i].split(" ")[5].match(garCron))) {
			culledTabs.push(new_cron_tabs[i]);
		}
	}
	
	//comment out this to view the original hourly crontabs before culling
	new_cron_tabs.length = 0;
	for(var j = 0; j < culledTabs.length; j++) {
		if (culledTabs[j].search("keep") > 0 ) {
			new_cron_tabs.push(culledTabs[j].split("keep")[0]);
		} else {
			new_cron_tabs.push(culledTabs[j]);
		}
	}
	
	culledTabs.length - 0;
}

function scanSettings() {
    new_cron_tabs.length = 0;
	
    for (var i = 0; i < timerMode; i++) {
        var aTable = document.getElementById("tab" + (1 + eval(i)) + "_timeTable");
        if (i == 0) {
            preceedingState = aTable.rows[1].cells[0].value
        }
        for (var j = 0; j < 24; j++) {
            var acell = aTable.rows[j < 12 ? 1 : 4].cells[j < 12 ? j : j - 12];
            generateCronTabStr(acell.value, j, i, "");
        }
    }
}

function UpdateSummary() {
    if (timerMode > 0) { 
		scanSettings(); 
	}
	
    AddSummaryText(email.SelTM + ": ");
	
    if (timerMode == 0) { 
		AddSummaryText(email.SumDis + "<br />\n")
    }
    if (timerMode == 1) {
        AddSummaryText(email.SumDly + "<br />\n")
    }
    if (timerMode == 3) {
        AddSummaryText(email.SumSwS + "<br />\n")
    }
    if (timerMode == 7) {
        AddSummaryText(email.SumWky + "<br />\n")
    }
    if (showCronTabs) {
        for (var e = 0; e < new_cron_tabs.length; e++) {
            AddSummaryText(new_cron_tabs[e] + "<br />\n")
        }
        AddSummaryText("<br />\n")
    }
	
    for (var t = 0; t < new_cron_tabs.length; t++) {
		var aCronTab = new_cron_tabs[t];
		var minCronText = aCronTab.split(" ")[0];
		var hourCronText = aCronTab.split(" ")[1];
		var dayCronText = aCronTab.split(" ")[4];
		var minuteStr = (minCronText < 10 ? '0' + minCronText : (minCronText == 60 ? '00' : minCronText));
		var day_string="";
		if (dayCronText == "*") {
			day_string = email.STDly
		}
		if (dayCronText == "0") {
			day_string = email.STSunday
		}
		if (dayCronText == "1") {
			day_string = email.STMonday
		}
		if (dayCronText == "2") {
			day_string = email.STTuesday
		}
		if (dayCronText == "3") {
			day_string = email.STWednesday
		}
		if (dayCronText == "4") {
			day_string = email.STThursday
		}
		if (dayCronText == "5") {
			day_string = email.STFriday
		}
		if (dayCronText == "6") {
			day_string = email.STSaturday
		}
		if (dayCronText == "1-5") {
			day_string = email.STMonFri
		}
        AddSummaryText(email.SumGo + " " + " - " + day_string + " " + email.SumAt + " " + (hourCronText < 10 ? "0" + hourCronText : hourCronText) + ":" + minuteStr + "<br />\n")
    }
}

function PreviousCell(aday, ahour) {
	if (ahour == 0) {
		if (aday == 0) {
			aday = timerMode-1;
		} else {
			aday--;
		}
		ahour = 23;
	} else {
		ahour--;
	}
	return document.getElementById("tab" + (1+aday) + "_timeTable").rows[ (ahour < 12 ? 1 : 4) ].cells[ (ahour < 12 ? ahour : ahour-12) ];
}

function ThisCell(aday,ahour) {
	return document.getElementById("tab" + (1+aday) + "_timeTable").rows[ (ahour < 12 ? 1 : 4) ].cells[ (ahour < 12 ? ahour : ahour-12) ];
}

function ToggleTime(cell) {
    var day_tab = -1;
    var hour_cell = -1;
    for (var i = 0; i < timerMode; i++) {
        var aTabTable = document.getElementById("tab" + (1 + eval(i)) + "_timeTable");
        if (aTabTable.style.display == "") {
            day_tab = eval(aTabTable.id.charAt(3));
            break
        }
    }
	
    hour_cell = cell.id.split("timer_ID_")[1];
	
	if(cell.value < 0){
		cell.value = cell.value * -1;
	}
	
	cell.value+=increment;
	
	if (cell.value > 60) {
		cell.value = 0;
	}
	
    ToggleTimerColor(cell);
    InitSummaryText();
    UpdateSummary()
}

function CleanTable(table) {
	for(var i = table.rows.length; i > 0; i--) {
		table.deleteRow(i-1);
	}
}

function PurgeTables() {
	for (var i = 0; i < 7; i++) {
		CleanTable( document.getElementById("tab" + (1+eval(i)) + "_timeTable") ); //tab1_timeTable
	}
}

function CellGradient(cell) {
    cell.style.backgroundColor = hour_partial_green
}

function SetCellContents(cell, text, value, color) {
	if (value != null) { 
		cell.value = value; 
	}
	
	cell.innerHTML = text;
	cell.style.backgroundColor = color;
	cell.style.backgroundImage = "";
}

function ToggleTimerColor(cell) {
	if (cell.value == 0) {
		SetCellContents(cell, "&nbsp;", null, hour_red);
	} else if (cell.value < 60) {
		CellGradient(cell);
		cell.innerHTML = Math.abs(cell.value);
	} else {
		SetCellContents(cell, "&nbsp;", null, hour_green);
	}
}

function DisableSelection(an_object) {
	//http://www.mindfiresolutions.com/Using-of-onselectstart-and-its-alternative-for-Firefox-239.php
	if (typeof an_object.onselectstart!="undefined") { //For IE/Safari This code will work
		an_object.onselectstart=function(){return false;}
	//For Firefox this code will work
    } else if (typeof an_object.style.MozUserSelect!="undefined") {
    	an_object.style.MozUserSelect="none"
    } else {
    //all other  (ie: Opera) This code will work   
    	an_object.onmousedown=function(){return false}
    	an_object.style.cursor = "default"
    }
}

function GenerateCellData(table, row, column, col_label) {
	//label hours (single digits get prepended with a 0 -> 1 becomes 01
	table.rows[row].insertCell(-1);	
	table.rows[row].cells[column].innerHTML = (col_label < 10 ? '0' + col_label : col_label);
	table.rows[row].cells[column].title = col_label + ":00-" + col_label + ":59";
	table.rows[row].cells[column].style.border = "1px solid black";
	
	//timer cells
	table.rows[row+1].insertCell(-1);
	table.rows[row+1].cells[column].id = "timer_ID_" + col_label;
	table.rows[row+1].cells[column].innerHTML = "&nbsp;";
	table.rows[row+1].cells[column].onclick=function(){ToggleTime(this)};
	table.rows[row+1].cells[column].value=60;
	table.rows[row+1].cells[column].style.border = "1px solid black";
	ToggleTimerColor(table.rows[row+1].cells[column]);
	
	DisableSelection(table);
}

function SetupTimeTable(targetTable) { //targetTable comes in 0-6; tabs are 1-7
	var table = document.getElementById("tab" + (1+eval(targetTable)) + "_timeTable");
	
	for(var i = 0; i < 5; i++) {
		table.insertRow(-1);
	}
	
	table.rows[1].style.cursor = "pointer";
	table.rows[4].style.cursor = "pointer";
	
	for(var i = 0; i < 24; i++) {
		(i < 12 ? GenerateCellData(table, 0, i, i) : GenerateCellData(table,3,i-12, i));
	}
	
	table.rows[2].insertCell(-1);
	table.rows[2].cells[0].height = "20 px";
	table.rows[2].cells[0].style.border = "none";
}

//   timer Tab functions
function PurgeTabs() {
	for ( var i = 1; i <= 7; i++ ) {
		var atab_li_item = document.getElementById("tab_li_" + i);
		
		if ( atab_li_item.nodeName == "LI" && atab_li_item.childNodes.length) {
			//alert("Removing " + atab_li_item.childNodes.length + " child noes for node " + i);
			atab_li_item = atab_li_item.removeChild(atab_li_item.lastChild);
		}
	}
}

function ShowTabField(tabNum) {
	for ( var i = 0; i < timerMode; i++ ) {
		var aTabTable = document.getElementById("tab" + (1+eval(i)) + "_timeTable");
		
		if (i == tabNum) {
			aTabTable.style.display = '';
		} else {
			aTabTable.style.display = 'none';
		}
	}
}


function ShowTab(achor_tab) {
	var days = 0;
	var tab_li_items = document.getElementById("tab_ulist").childNodes;
	for ( var i = 0; i < tab_li_items.length; i++ ) {
		if ( tab_li_items[i].nodeName == "LI" ) {
			days++;
			for (var j = 0; j < tab_li_items[i].childNodes.length; j++) {
				if (tab_li_items[i].childNodes[j].nodeName == "A" ) {
					tab_li_items[i].childNodes[j].className = 'deselected';
				}
			}
			if(days > timerMode) {
				for (var k = 0; k < tab_li_items[k].childNodes.length; k++) {
					if (tab_li_items[i].childNodes[k].nodeName == "A" ) {
						tab_li_items[i].childNodes[k].style.display = 'none';
					}
				}
				tab_li_items[i].style.display = 'none';
			}
		}
	}
	ShowTabField((achor_tab.id).split("tab_ID_")[1]);
	achor_tab.className = 'selected';
}

function SetupTabs(timer_style) {
	var tab_li_items = document.getElementById("tab_ulist").childNodes;
	var daycount = 0;
	var periodicity = [];
	
	if (timer_style == 1) {
		periodicity = dailyPeriod;
	} else if (timer_style == 3) {
		periodicity = week511Period;
	} else if (timer_style == 7) {
		periodicity = weeklyPeriod;
	}
	
	for ( var i = 0; i < tab_li_items.length; i++ ) {
		if ( tab_li_items[i].nodeName == "LI" ) {
			var anchorTag = document.createElement('a'); //add anchor tag dynamically
			anchorTag.onclick=function(){ShowTab(this)};
			anchorTag.id = "tab_ID_" + daycount;
			anchorTag.style.cursor = "default"
  			
			if (daycount < timer_style) {
				anchorTag.innerHTML = periodicity[daycount];
			} else {
				anchorTag.innerHTML = "aa";
			}
			tab_li_items[i].style.display = "inline-block";
			tab_li_items[i].style.textAlign="center"

			tab_li_items[i].appendChild(anchorTag); //and attach the anchor tag
			
			if (daycount < timer_style) {
				SetupTimeTable(daycount);
			}
			daycount++;
		}
	}
	DisableSelection(document.getElementById("tabs"));
	ShowTab(document.getElementById("tab_ID_0"));
}

// drop-down menu functions
function SetTimerIncrement(timer_option) {
	increment=eval(timer_option.value);
}

function SetTimerMode(mode_option) {
	timerMode=mode_option;
	new_cron_tabs=[];
	
	if (mode_option == 0) {
		document.getElementById("timer_mode").selectedIndex = 0;
	}
	
	PurgeTables();
	PurgeTabs();
	
	if (mode_option > 0) {
		SetupTabs(timerMode);
		document.getElementById('div_timer_increment').style.display = 'block';
	} else {
		document.getElementById('div_timer_increment').style.display = 'none';
	}
	
	InitSummaryText();
	UpdateSummary();
}

function FinalizeTables() {
	var initial_crontab = new Array();
	var previous_email_state = 0;
	
    for (var n = 0; n < timerMode; n++) {
        for (var r = 0; r < 24; r++) {
            var acell = document.getElementById("tab" + (1 + n) + "_timeTable").rows[r < 12 ? 1 : 4].cells[r < 12 ? r : r - 12];
			
            if (acell.value != 60 && initial_crontab.length == 0) {
                initial_crontab[0] = n;
                initial_crontab[1] = r;
                previous_email_state = acell.value > 0 ? 1 : -1
            }
			
            if (acell.value == 1e3) {
                SetCellContents(acell, "&nbsp;", 0, hour_red);
                previous_email_state = 1
            } else if (acell.value == -1e3) {
                SetCellContents(acell, "&nbsp;", 0, hour_red);
                previous_email_state = -1
            } else if (acell.value == 60) {
                if (previous_email_state != 0 && initial_crontab.length > 0) {
                    if (previous_email_state > 0) {
                        SetCellContents(acell, "&nbsp;", 60, hour_green);
                        previous_email_state = 1
                    } else {
                        SetCellContents(acell, "&nbsp;", 0, hour_red);
                        previous_email_state = -1
                    }
                }
            } else {
                if (acell.value > 0) {
                    acell.innerHTML = acell.value;
                    previous_email_state = -1
                } else {
                    acell.innerHTML = Math.abs(acell.value);
                    previous_email_state = 1
                }
                CellGradient(acell)
            }
        }
    }
	
    if (initial_crontab.length > 0) {
        var s = PreviousCell(0, 0).value;
        for (var o = 0; o < timerMode; o++) {
            for (var u = 0; u < 24; u++) {
                if (o == initial_crontab[0] && u == initial_crontab[1]) {
                    o = 20;
                    break
                }
                var a = document.getElementById("tab" + (1 + o) + "_timeTable").rows[u < 12 ? 1 : 4].cells[u < 12 ? u : u - 12];
                if (s == 60) {
                    SetCellContents(a, "&nbsp;", 60, hour_green)
                } else if (s == 0) {
                    SetCellContents(a, "&nbsp;", 0, hour_red)
                } else if (s > 0) {
                    SetCellContents(a, "&nbsp;", 0, hour_red)
                } else if (s < 0) {
                    SetCellContents(a, "&nbsp;", 60, hour_green)
                }
            }
        }
    }
}

function SeatCronData(cron_minute, cron_hour, cron_day, cron_cmd) {
    var ecron_minute = eval(cron_minute);
    var ecron_hour = eval(cron_hour);
    var ecron_day = eval(cron_day);
    var day_table = document.getElementById("tab" + (1 + ecron_day) + "_timeTable");
    var ecell = day_table.rows[ecron_hour < 12 ? 1 : 4].cells[ecron_hour < 12 ? ecron_hour : ecron_hour - 12];
    if (ecron_minute > 0 && ecron_minute < 60) {
        ecell.value = ecron_minute;
        if (cron_cmd.search(garCron) >= 0) {
            ecell.value = ecell.value * -1
		}
        CellGradient(ecell);
        ecell.innerHTML = ecell.value
    }else if (cron_cmd.search(garCron) >= 0) {
		SetCellContents(ecell, "60", 1000, hour_green);
	} 
}

function CronTabsToTables() {
    SetupTabs(timerMode);
	
    for (var i = 0; i < found_cron_tabs.length; i++) {
        var aFetchedCronTab = found_cron_tabs[i];
        var aCronMinute = aFetchedCronTab.split(" ")[0];
        var aCronHour = aFetchedCronTab.split(" ")[1];
        var aCronDay = aFetchedCronTab.split(" ")[4];
        var aCronemailCmd = aFetchedCronTab.split(" ")[5];
		
        if (timerMode == 1) {
            aCronDay = "0"
        } else if (timerMode == 3) {
            if (aCronDay == "1-5") {
                aCronDay = "1"
            } else if (aCronDay == "6") {
                aCronDay = "2"
            }
        }
		
        if (eval(aCronHour) > 23 || eval(aCronHour) < 0) {
            aCronHour = 0
        }
		
        if (eval(aCronMinute) > 59 || eval(aCronMinute) < 0) {
            aCronMinute = 0
        }
		
        for (sch_days = 0; sch_days < aCronDay.split(",").length; sch_days++) {
            for (sch_hours = 0; sch_hours < aCronHour.split(",").length; sch_hours++) {
                SeatCronData(aCronMinute, aCronHour.split(",")[sch_hours], aCronDay.split(",")[sch_days], aCronemailCmd)
            }
        }
    }
	
    FinalizeTables()
}

function ParseCurrentTime(shell_vars) {
    var globbed_time = shell_vars == null ? weekly_time : shell_vars;
    current_time.length = 0;
    current_time = globbed_time.split("-");
	
    for (var i = 0; i < 3; i++) {
        current_time[i] = eval(current_time[i])
    }
	
    if (timerMode == 1) {
        current_time[3] = 0
    } else if (timerMode == 3) {
        if (current_time[0] > 0 && current_time[0] < 6) {
            current_time[3] = 1
        } else if (current_time[0] == 6) {
            current_time[3] = 2
        } else {
            current_time[3] = 0
        }
    } else {
        current_time[3] = current_time[0]
    }
}

function CloneTable() {
	cloned_crontab_table.length=0;
	for (var i = 0; i < timerMode; i++) {
		cloned_crontab_table[i] = new Array();
		for (var j = 0; j < 24; j++ ) {
			cloned_crontab_table[i].push( ThisCell(i, j).value );
			//AddSummaryText("i=" + i + " d=" + j + "v=" + cloned_crontab_table[i][j]);
		}
	}
}

function LoadCrontabs() {
    stripped_cron_tabs.length = 0;
    var foundDailySched = 0;
    var found511Sched = 0;
    var foundWeekend = 0;
    var foundWeekday = 0;
    var foundWeeklySched = 0;
    dailyPeriod = [email.Dly];
    week511Period = email.WDayA;
    weeklyPeriod = email.WeekA;
    InitSummaryText();
	
    for (var s = 0; s < raw_cron_data.length; s++) {
        if (raw_cron_data[s].search(garCron) > 0) {
            found_cron_tabs.push(raw_cron_data[s]);
            if (showCronTabs) {
                AddSummaryText("email: " + found_cron_tabs[found_cron_tabs.length - 1] + "<br />\n")
            }
        } else if (raw_cron_data[s].length > 0) {
            stripped_cron_tabs.push(raw_cron_data[s]);
            if (showCronTabs) {
                AddSummaryText("system: " + stripped_cron_tabs[stripped_cron_tabs.length - 1] + "<br />\n")
            }
        }
    }
	
    for (var o = 0; o < found_cron_tabs.length; o++) {
        if (found_cron_tabs[o].split(" ")[4] == "*") {
            foundDailySched++
        } else if (found_cron_tabs[o].split(" ")[4] == "1-5") {
            found511Sched++;
            foundWeekday++
        } else if (found_cron_tabs[o].split(" ")[4].match(/[1-5]/g) >= 0) {
            foundWeeklySched++
        } else if (found_cron_tabs[o].split(" ")[4].match(/[0,6]/g) >= 0) {
            foundWeekend++
        }
    }
	
    if (foundDailySched && found511Sched == 0 && foundWeekend == 0 && foundWeeklySched == 0) {
        timerMode = 1;
        document.getElementById("timer_mode").selectedIndex = 1
    } else if (foundWeekday || (foundWeekend || found511Sched) && foundWeeklySched == 0) {
        timerMode = 3;
        document.getElementById("timer_mode").selectedIndex = 2
    } else if (foundWeeklySched || foundWeekend) {
        timerMode = 7;
        document.getElementById("timer_mode").selectedIndex = 3
    }
    if (timerMode > 0) {
        document.getElementById("div_timer_increment").style.display = "block";
        CronTabsToTables()
    }
	
    CloneTable();
    ParseCurrentTime(null);
	
    if (timerMode > 0) {
        ShowTab(document.getElementById("tab_ID_" + current_time[3]))
    }
	
    UpdateSummary()
}

function Visibility() {
    if (!document.getElementById("auth").checked) {
		disableAuthOptions();
    } else {
		enableAuthOptions();
    }
}

function enableAuthOptions(){
	document.getElementById("auth").checked = true;
    document.getElementById("username").disabled = false;
    document.getElementById("password").disabled = false
	document.getElementById("show_pass").disabled = false;
	document.getElementById("show_pass_label").disabled = false;
}

function disableAuthOptions(){
	document.getElementById("auth").checked = false;
    document.getElementById("username").disabled = true;
    document.getElementById("password").disabled = true
	document.getElementById("show_pass").disabled = true;
	document.getElementById("show_pass_label").disabled = true;
}

function saveChanges() {
	
	var e = new Array();
    var command;
	var data = getData();
	
	if(!data['validate']){
		return;
	}
	
	setControlsEnabled(false, true, UI.WaitSettings);
	
	//MSMTPRC config
    command = 'echo -e "account default\\nhost ' + data['serverIP'] + "\\nport " + data['serverPort'] + "\\n from " + data['sender'] + "\\n timeout 30";
	
	if(data['auth']){
		command = command + '\\nauth plain\\nuser ' + data['username'] + '\\npassword ' + data['password'];
	}else{
		command = command + "\\nauth off";
	}
	
	if(data['encryption']){
		command = command + "\\ntls on";
	}else{
		command = command + "\\ntls off";
	}
	
	command = command + '" > /etc/msmtprc';
	e.push(command);
	
	//UCI config
	e.push("uci set email.@email[0].recipient='" + data['receiver'] + "'");
	e.push("uci set email.@email[0].data='" + data['include'] + "'");
	if(data['interval'] != null){
		e.push("uci set email.@email[0].bandwidthInterval='" + data['interval'] + "'");
		e.push("uci set email.@email[0].count='" + data['count'] + "'");
	}
	
	if(document.getElementById("TLS").checked){
		e.push("uci set email.@email[0].tls='1'");
	}else{
		e.push("uci set email.@email[0].tls='0'");
	}
	
	e.push("uci commit");
	
	//Crontabs
	e.push("mkdir -p /etc/crontabs");
    e.push("touch /etc/crontabs/root");
	e.push("sed -i '/\\/usr\\/lib\\/gargoyle\\/email.sh/d' /etc/crontabs/root");
	
    for (var h = 0; h < new_cron_tabs.length; h++) {
        e.push("echo '" + new_cron_tabs[h] + "' >> /etc/crontabs/root");
    }
	
	e.push("/etc/init.d/cron restart");
	
	var param = getParameterDefinition("commands", e.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));		
	
    var d = function(e) {
        if (e.readyState == 4) {
            setControlsEnabled(true)
        }
    };
	
    CloneTable();
    runAjax("POST", "utility/run_commands.sh", param, d)
}

function togglePass(e){
	password_field=document.getElementById(e),password_field.type=="password"?password_field.type="text":password_field.type="password"
}

function getData(){
	
	var data = new Array();
	
    data['serverIP'] = document.getElementById("serverip").value.trim();
    data['serverPort'] = document.getElementById("serverport").value.trim();
    data['sender'] = document.getElementById("sender").value.trim();
    data['receiver'] = document.getElementById("receiver").value.trim();
	data['count'] = document.getElementById("count").value.trim();
	data['certpath'] = "/etc/ssl/certs/ca-certificates.crt";
	data['include'] = "";
	
	var e = document.getElementsByName('content');
	
	for (var i=0;i<e.length;i++){
		if (e[i].checked) {
			data['include'] = data['include'] + i;
			
			if(i == 5){
				var interval = document.getElementById("bandwidthIntervalSelect").value;
				switch(interval){
					case email.minutes:
						data['interval'] = "minute";
						break;
					case email.quarterhours:
						data['interval'] = "900";
						break;
					case email.hours:
						data['interval'] = "hour";
						break;
					case email.days:
						data['interval'] = "day";
						break
					default:
						break;
				}
			}
		}
	}
	
	if(document.getElementById('auth').checked){
		data['auth'] = true;
		data['username'] = document.getElementById("username").value.trim();
		data['password'] = document.getElementById("password").value.trim();
	}else{
		data['auth'] = false;
	}
	
	if(document.getElementsByName('encryption')[0].checked){
		data['encryption'] = false;
	}else if(document.getElementsByName('encryption')[1].checked){
		data['encryption'] = true;
	}
	
	if(validateInput(data)){
		data['validate'] = true;
	}else{
		data['validate'] = false;
	}
	
	return data;
}

function validateInput(data){
	
	if(data['encryption'] == null){
		alert(email.encryptionTypeError);
		return false;
	}
	
	if(!/\S+@\S+/.test(data['sender'])){
		alert(email.senderError);
		return false;
	}
	
	if(!/\S+@\S+/.test(data['receiver'])){
		alert(email.receiverError);
		return false;
	}
	
	if(data['include']==""){
		alert(email.noDataSelected);
		return false;
	}
	
	if(isNaN(data['serverPort'])){
		alert(email.portError);
		return false;
	}
	
	if(data['include'].indexOf("5") > -1){
		if(isNaN(data['count']) || data['count']==""){
			alert(email.countError);
			return false;
		}
	}
	
	if(/\s/g.test(data['serverIP']) || /\s/g.test(data['receiver']) || /\s/g.test(data['sender']) || /\s/g.test(data['username']) || /\s/g.test(data['password'])){
		alert(email.whitespaceError);
		return false;
	}
	
	return true;
}

function testMail() {

    var command;
	var data = getData();
	
	if(!data['validate']){
		return;
	}
	
	setControlsEnabled(false, true, email.testEmail);
	var command = "mv /etc/msmtprc /tmp/msmtprc.tmp && ";
	var body = 'echo -e "Subject: Gargoyle router TEST EMAIL\\r\\nFrom: '+data['sender']+'\\r\\nContent-Type: text/plain; charset=\'UTF-8\';\\r\\nThis is test email sent from Gargoyle router." | ';
	
    if (data['auth']) {
		if(!data['encryption']){
			//Authentication enabled and encryption disabled
			command = command + 'echo -e "account default\\nhost '+data['serverIP']+"\\nport "+data['serverPort']+"\\nauth plain\\nuser " + data['username'] + "\\npassword " + data['password'] + "\\nauto_from off\\nfrom "+data['sender']+'" > /etc/msmtprc && ';
		}else{
			command = command + 'echo -e "account default\\nhost '+data['serverIP']+"\\nport "+data['serverPort']+"\\ntls on"+"\\nauth plain\\nuser " + data['username'] + "\\npassword " + data['password'] + "\\nauto_from off\\nfrom "+data['sender']+'" > /etc/msmtprc && ';
		}
		command = command + body + 'sendmail '+data['receiver']+' --syslog --tls-trust-file '+data['certpath']+' --timeout 30 && rm /etc/msmtprc && mv /tmp/msmtprc.tmp /etc/msmtprc';
    } else {
		if(!data['encryption']){
			//Authentication and encryption disabled
			command = body + 'sendmail --host='+data['serverIP']+' --port='+data['serverPort']+' --from '+data['sender']+' '+data['receiver']+' --timeout 30 --syslog';		
		}else{
			command = body + 'sendmail --host='+data['serverIP']+' --port='+data['serverPort']+' --tls=on --tls-trust-file='+data['certpath']+' --from '+data['sender']+' '+data['receiver']+' --timeout 30 --syslog';		
		}
    }
	
    var w = function(e) {
        if (e.readyState == 4) {
			if(e.responseText == "Success\n"){
				var a = function(e) {
					if (e.readyState == 4) {
							alert(e.responseText.replace("Success\n", ""));
							setControlsEnabled(true);
					}
				}
				var command = getParameterDefinition("commands", "logread -l 1") + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/, "").replace(/[\t ;]+.*$/, ""));
				runAjax("POST", "utility/run_commands.sh", command, a);
			}else{
				alert(email.testEmailFail + e.responseText);
				setControlsEnabled(true);
			}
        }
    };
	
	var command = getParameterDefinition("commands", command) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/, "").replace(/[\t ;]+.*$/, ""));
    runAjax("POST", "utility/run_commands.sh", command, w);
}

function LoadData() {
	
	LoadCrontabs();
	
	var uci = uciOriginal.clone();
	var selector = uci.getAllSectionsOfType("email", "email");
	
	var config = readSendmailConfig();
	
	if(TLSsupport==''){
		document.getElementById("plain").checked = true;
		document.getElementById("TLS").disabled = true;
	}
	
	var el = document.getElementsByName('content');
	
	if(!webmonEnabled){
		el[0].disabled=true;
		el[1].disabled=true;
		el[0].checked=false;
		el[1].checked=false;
	}
	
	if(config['host'] == "mailhub.oursite.example"){
		return;
	}
	
	document.getElementById("serverip").value = config['host'];
	document.getElementById("serverport").value = config['port'];
	document.getElementById("sender").value = config['from'];
	document.getElementById("receiver").value = uciOriginal.get("email", selector[0], "recipient");
	document.getElementById("count").value = uciOriginal.get("email", selector[0], "count");
	
	var chars = uciOriginal.get("email", selector[0], "data").split('');
	
	for (var i=0;i<el.length;i++){
		for(var o = 0;o<chars.length;o++){
			if(parseInt(chars[o]) == i){
				el[i].checked=true;
				if(i == 5){
					document.getElementById("bandwidthIntervalSelect").disabled = false;
					document.getElementById("count").disabled = false;
					var interval = uciOriginal.get("email", selector[0], "bandwidthInterval");
					switch(interval){
						case "minute":
							document.getElementById("bandwidthIntervalSelect").value = email.minutes;
							break;
						case "900":
							document.getElementById("bandwidthIntervalSelect").value = email.quarterhours;
							break;
						case "hour":
							document.getElementById("bandwidthIntervalSelect").value = email.hours;
							break;
						case "day":
							document.getElementById("bandwidthIntervalSelect").value = email.days;
							break;
						default:
							break;
					}			
				}
				break;
			}
		}
	}
	
	switch(config['tls']){
		case "off":
			document.getElementById("plain").checked = true;
			break;
		case "on":
			document.getElementById("TLS").checked = true;
			break;
		default:
			document.getElementById("plain").checked = true;
			break;	
	}
	
	switch(config['auth']){
		case "off":
			disableAuthOptions();
			break;
		case "plain":
			enableAuthOptions();
			document.getElementById("auth").checked = true;
			document.getElementById("username").value = config['user'];
			document.getElementById("password").value = config['password'];
			break;
		default:
			disableAuthOptions();
			break;
	}
}

function intervalVisibility(obj){
	if(obj.checked){
		document.getElementById("bandwidthIntervalSelect").disabled = false;
		document.getElementById("count").disabled = false;
	}else{
		document.getElementById("bandwidthIntervalSelect").disabled = true;
		document.getElementById("count").disabled = true;
	}
}

function readSendmailConfig(){
	
	var array = msmtprc.split(' ');
	var data = new Array();
	
	for(var i = 0; i < array.length; i++){
		switch(array[i]){
			case "host":
				data['host'] = array[i+1];
				i++;
				break;
			case "port":
				data['port'] = array[i+1];
				i++;
				break;
			case "from":
				data['from'] = array[i+1];
				i++;
				break;
			case "tls":
				data['tls'] = array[i+1];
				i++;
				break;
			case "auth":
				data['auth'] = array[i+1];
				i++;
				break;
			case "user":
				data['user'] = array[i+1];
				i++;
				break;
			case "password":
				data['password'] = array[i+1];
				i++;
				break;
			default:
				break;
		}
	}
	
	return data;
}
