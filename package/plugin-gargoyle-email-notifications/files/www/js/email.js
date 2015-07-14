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
    document.getElementById("summary_container").className = "tabField";
    var e = document.getElementById("summary_txt");
    e.innerHTML = "<strong>" + email.Smmy + ":</strong><br />\n"
}

function AddSummaryText(e) {
    document.getElementById("summary_txt").innerHTML += e
}

function generateCronTabStr(e, t, n, r) {
    var i = "";
    var s = "";
    var o = "";
    if (timerMode == 1) {
        s = "*"
    } else if (timerMode == 3) {
        if (n == 0) {
            s = "0"
        }
        if (n == 1) {
            s = "1-5"
        }
        if (n == 2) {
            s = "6"
        }
    } else if (timerMode == 7) {
        s = n.toString()
    } else {
        return
    }
    if (e == 60) {
		return;
	}else if(e == 0){
        i = "0 " + t + " * * " + s + " " + garCron;
    } else {
        i = "" + Math.abs(e) + " " + t + " * * " + s + " " + garCron;
    }
    new_cron_tabs.push(i)
}

function CronTabCull() {
    var e = [];
    var t = garCron;
    for (var n = 0; n < new_cron_tabs.length; n++) {
        if (new_cron_tabs[n].split(" ")[5].match(t)) {
            e.push(new_cron_tabs[n]);
		}
    }
    new_cron_tabs.length = 0;
    for (var r = 0; r < e.length; r++) {
        if (e[r].search("keep") > 0) {
            new_cron_tabs.push(e[r].split("keep")[0])
        } else {
            new_cron_tabs.push(e[r])
        }
    }
    e.length - 0
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
    CronTabCull()
}

function CronWarning() {
    var e = ThisCell(current_time[3], current_time[1]);
    if (Email == 1 && e.value == 0 || Email == -1 && e.value == 60) {
        AddSummaryText("<br/>\n<strong>" + email.Warn + ":</strong><br/>\n");
        AddSummaryText(email.NextEv + "<br/>\n")
    } else if (e.value > 0 && e.value < 60 && current_time[2] > e.value && Email == 1) {
        AddSummaryText("<br/>\n<strong>" + email.Warn + ":</strong><br/>\n");
        AddSummaryText(email.NextEv + "<br/>\n")
    } else if (e.value < 0 && current_time[2] < Math.abs(e.value) && Email == 1) {
        AddSummaryText("<br/>\n<strong>" + email.Warn + ":</strong><br/>\n");
        AddSummaryText(email.NextEv + "<br/>\n")
    }
}

function UpdateSummary() {
    if (timerMode > 0) {
        scanSettings()
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
            var n = new_cron_tabs[t];
            var r = n.split(" ")[0];
            var i = n.split(" ")[1];
            var s = n.split(" ")[4];
            var o = n.split(" ")[6];
            var u = r < 10 ? "0" + r : r == 60 ? "00" : r;
            var a = "";
            if (s == "*") {
                a = email.STDly
            }
            if (s == "0") {
                a = email.STSunday
            }
            if (s == "1") {
                a = email.STMonday
            }
            if (s == "2") {
                a = email.STTuesday
            }
            if (s == "3") {
                a = email.STWednesday
            }
            if (s == "4") {
                a = email.STThursday
            }
            if (s == "5") {
                a = email.STFriday
            }
            if (s == "6") {
                a = email.STSaturday
            }
            if (s == "1-5") {
                a = email.STMonFri
            }
            AddSummaryText(email.SumGo + " " + " - " + a + " " + email.SumAt + " " + (i < 10 ? "0" + i : i) + ":" + u + "<br />\n")
    }
    if (timerMode > 0) {
        CronWarning()
    }
}

function PreviousCell(e, t) {
    if (t == 0) {
        if (e == 0) {
            e = timerMode - 1
        } else {
            e--
        }
        t = 23
    } else {
        t--
    }
    return document.getElementById("tab" + (1 + e) + "_timeTable").rows[t < 12 ? 1 : 4].cells[t < 12 ? t : t - 12]
}

function ThisCell(e, t) {
    return document.getElementById("tab" + (1 + e) + "_timeTable").rows[t < 12 ? 1 : 4].cells[t < 12 ? t : t - 12]
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

function CleanTable(e) {
    for (var t = e.rows.length; t > 0; t--) {
        e.deleteRow(t - 1)
    }
}

function PurgeTables() {
    for (var i = 0; i < 7; i++) {
        CleanTable(document.getElementById("tab" + (1 + eval(i)) + "_timeTable"))
    }
}

function CellGradient(e) {
    e.style.backgroundColor = hour_partial_green
}

function SetCellContents(e, t, n, r) {
    if (n != null) {
        e.value = n
    }
    e.innerHTML = t;
    e.style.backgroundColor = r;
    e.style.backgroundImage = ""
}

function ToggleTimerColor(e) {
    if (e.value == 0) {
        SetCellContents(e, "&nbsp;", null, hour_red)
    } else if (e.value < 60) {
        CellGradient(e);
        e.innerHTML = Math.abs(e.value)
    } else {
        SetCellContents(e, "&nbsp;", null, hour_green)
    }
}

function DisableSelection(e) {
    if (typeof e.onselectstart != "undefined") {
        e.onselectstart = function() {
            return false
        }
    } else if (typeof e.style.MozUserSelect != "undefined") {
        e.style.MozUserSelect = "none"
    } else {
        e.onmousedown = function() {
            return false
        };
        e.style.cursor = "default"
    }
}

function GenerateCellData(e, t, n, r) {
    e.rows[t].insertCell(-1);
    e.rows[t].cells[n].innerHTML = r < 10 ? "0" + r : r;
    e.rows[t].cells[n].title = r + ":00-" + r + ":59";
    e.rows[t].cells[n].style.border = "1px solid black";
    e.rows[t + 1].insertCell(-1);
    e.rows[t + 1].cells[n].id = "timer_ID_" + r;
    e.rows[t + 1].cells[n].innerHTML = "&nbsp;";
    e.rows[t + 1].cells[n].onclick = function() {
        ToggleTime(this)
    };
    e.rows[t + 1].cells[n].value = 60;
    e.rows[t + 1].cells[n].style.border = "1px solid black";
    ToggleTimerColor(e.rows[t + 1].cells[n]);
    DisableSelection(e)
}

function SetupTimeTable(targetTable) {
    var table = document.getElementById("tab" + (1 + eval(targetTable)) + "_timeTable");
    for (var i = 0; i < 5; i++) {
        table.insertRow(-1)
    }
    table.rows[1].style.cursor = "pointer";
    table.rows[4].style.cursor = "pointer";
    for (var i = 0; i < 24; i++) {
        i < 12 ? GenerateCellData(table, 0, i, i) : GenerateCellData(table, 3, i - 12, i)
    }
    table.rows[2].insertCell(-1);
    table.rows[2].cells[0].height = "20 px";
    table.rows[2].cells[0].style.border = "none"
}

function PurgeTabs() {
    for (var e = 1; e <= 7; e++) {
        var t = document.getElementById("tab_li_" + e);
        if (t.nodeName == "LI" && t.childNodes.length) {
            t = t.removeChild(t.lastChild)
        }
    }
}

function ShowTabField(tabNum) {
    for (var i = 0; i < timerMode; i++) {
        var aTabTable = document.getElementById("tab" + (1 + eval(i)) + "_timeTable");
        if (i == tabNum) {
            aTabTable.style.display = ""
        } else {
            aTabTable.style.display = "none"
        }
    }
}

function ShowTab(e) {
    var t = 0;
    var n = document.getElementById("tab_ulist").childNodes;
    for (var r = 0; r < n.length; r++) {
        if (n[r].nodeName == "LI") {
            t++;
            for (var i = 0; i < n[r].childNodes.length; i++) {
                if (n[r].childNodes[i].nodeName == "A") {
                    n[r].childNodes[i].className = "deselected"
                }
            }
            if (t > timerMode) {
                for (var s = 0; s < n[s].childNodes.length; s++) {
                    if (n[r].childNodes[s].nodeName == "A") {
                        n[r].childNodes[s].style.display = "none"
                    }
                }
                n[r].style.display = "none"
            }
        }
    }
    ShowTabField(e.id.split("tab_ID_")[1]);
    e.className = "selected"
}

function SetupTabs(e) {
    var t = document.getElementById("tab_ulist").childNodes;
    var n = 0;
    var r = [];
    if (e == 1) {
        r = dailyPeriod
    } else if (e == 3) {
        r = week511Period
    } else if (e == 7) {
        r = weeklyPeriod
    }
    for (var i = 0; i < t.length; i++) {
        if (t[i].nodeName == "LI") {
            var s = document.createElement("a");
            s.onclick = function() {
                ShowTab(this)
            };
            s.id = "tab_ID_" + n;
            s.style.cursor = "default";
            if (n < e) {
                s.innerHTML = r[n]
            } else {
                s.innerHTML = "aa"
            }
            t[i].style.display = "inline-block";
            t[i].style.textAlign = "center";
            t[i].appendChild(s);
            if (n < e) {
                SetupTimeTable(n)
            }
            n++
        }
    }
    DisableSelection(document.getElementById("tabs"));
    ShowTab(document.getElementById("tab_ID_0"))
}

function SetTimerIncrement(timer_option) {
    increment = eval(timer_option.value)
}

function SetTimerMode(e) {
    timerMode = e;
    new_cron_tabs = [];
    if (e == 0) {
        document.getElementById("timer_mode").selectedIndex = 0
    }
    PurgeTables();
    PurgeTabs();
    if (e > 0) {
        SetupTabs(timerMode);
        document.getElementById("div_timer_increment").style.display = "block"
    } else {
        document.getElementById("div_timer_increment").style.display = "none"
    }
    InitSummaryText();
    UpdateSummary()
}

function FinalizeTables() {
    var e = new Array;
    var t = 0;
    for (var n = 0; n < timerMode; n++) {
        for (var r = 0; r < 24; r++) {
            var i = document.getElementById("tab" + (1 + n) + "_timeTable").rows[r < 12 ? 1 : 4].cells[r < 12 ? r : r - 12];
            if (i.value != 60 && e.length == 0) {
                e[0] = n;
                e[1] = r;
                t = i.value > 0 ? 1 : -1
            }
            if (i.value == 1e3) {
                SetCellContents(i, "&nbsp;", 0, hour_red);
                t = 1
            } else if (i.value == -1e3) {
                SetCellContents(i, "&nbsp;", 0, hour_red);
                t = -1
            } else if (i.value == 60) {
                if (t != 0 && e.length > 0) {
                    if (t > 0) {
                        SetCellContents(i, "&nbsp;", 60, hour_green);
                        t = 1
                    } else {
                        SetCellContents(i, "&nbsp;", 0, hour_red);
                        t = -1
                    }
                }
            } else {
                if (i.value > 0) {
                    i.innerHTML = i.value;
                    t = -1
                } else {
                    i.innerHTML = Math.abs(i.value);
                    t = 1
                }
                CellGradient(i)
            }
        }
    }
    if (e.length > 0) {
        var s = PreviousCell(0, 0).value;
        for (var o = 0; o < timerMode; o++) {
            for (var u = 0; u < 24; u++) {
                if (o == e[0] && u == e[1]) {
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
    cloned_crontab_table.length = 0;
    for (var e = 0; e < timerMode; e++) {
        cloned_crontab_table[e] = new Array;
        for (var t = 0; t < 24; t++) {
            cloned_crontab_table[e].push(ThisCell(e, t).value)
        }
    }
}

function LoadCrontabs() {
    stripped_cron_tabs.length = 0;
    var e = 0;
    var t = 0;
    var n = 0;
    var r = 0;
    var i = 0;
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
            e++
        } else if (found_cron_tabs[o].split(" ")[4] == "1-5") {
            t++;
            r++
        } else if (found_cron_tabs[o].split(" ")[4].match(/[1-5]/g) >= 0) {
            i++
        } else if (found_cron_tabs[o].split(" ")[4].match(/[0,6]/g) >= 0) {
            n++
        }
    }
    if (e && t == 0 && n == 0 && i == 0) {
        timerMode = 1;
        document.getElementById("timer_mode").selectedIndex = 1
    } else if (r || (n || t) && i == 0) {
        timerMode = 3;
        document.getElementById("timer_mode").selectedIndex = 2
    } else if (i || n) {
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
    command = 'echo -e "account default\\nhost ' + data['serverIP'] + "\\nport " + data['serverPort'] + "\\n from " + data['sender'];
	
	if(data['auth']){
		command = command + '\\nauth plain\\nuser ' + data['username'] + '\\npassword ' + data['password'];
	}else{
		command = command + "\\nauth off";
	}
	
	if(data['encryption']){
		command = command + "\\ntls on\\ntls-cert-file "+data['certpath'];
	}else{
		command = command + "\\ntls off";
	}
	
	command = command + '" > /etc/msmtprc';
	e.push(command);
	
	//UCI config
	e.push("uci set email.@email[0].recipient='" + data['receiver'] + "'");
	e.push("uci set email.@email[0].data='" + data['include'] + "'");
	e.push("uci set email.@email[0].bandwidthInterval='" + data['interval'] + "'");
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
	
    data['serverIP'] = document.getElementById("serverip").value;
    data['serverPort'] = document.getElementById("serverport").value;
    data['sender'] = document.getElementById("sender").value;
    data['receiver'] = document.getElementById("receiver").value;
	data['certpath'] = "/etc/ssl/certs/ca-certificates.crt";
	data['include'] = "";
	
	var interval = document.getElementById("table_time_frame").value;
	switch(interval){
		case "minutes":
			data['interval'] = "minute";
			break;
		case "quarter hours":
			data['interval'] = "180";
			break;
		case "hours":
			data['interval'] = "hour";
			break;
		case "days":
			data['interval'] = "day";
			break
		default:
			break;
	}
	
	var e = document.getElementsByName('content');
	for (var i=0;i<e.length;i++){
		if (e[i].checked) {
			data['include'] = data['include'] + i;
		}
	}
	
	if(document.getElementById('auth').checked){
		data['auth'] = true;
		data['username'] = document.getElementById("username").value;
		data['password'] = document.getElementById("password").value;
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
	if(!/\S+@\S+/.test(data['sender'])){
		alert(email.senderError);
		return false;
	}
	if(!/\S+@\S+/.test(data['receiver'])){
		alert(email.receiverError);
		return false;
	}
	if(isNaN(data['serverPort'])){
		alert(email.portError);
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
	
	setControlsEnabled(false, true, "Sending test email...");
	var command = "mv /etc/msmtprc /tmp/msmtprc.tmp && ";
	var body = 'echo -e "Subject: Gargoyle router TEST EMAIL\r\nFrom: '+data['sender']+'\r\nContent-Type: text/plain; charset="UTF-8";\r\nThis is test email sent from Gargoyle router." | ';
	
    if (data['auth']) {
		if(!data['encryption']){
			command = command + 'echo -e "account default\\nhost '+data['serverIP']+"\\nport "+data['serverPort']+"\\nauth plain\\nuser " + data['username'] + "\\npassword " + data['password'] + "\\nauto_from off\\nfrom "+data['sender']+' '+data['receiver']+'" > /etc/msmtprc && ';
		}else{
			command = command + 'echo -e "account default\\nhost '+data['serverIP']+"\\nport "+data['serverPort']+"\\ntls on\\n tls_trust_file "+data['certpath']+"\\nauth plain\\nuser " + data['username'] + "\\npassword " + data['password'] + "\\nauto_from off\\nfrom "+data['sender']+' '+data['receiver']+'" > /etc/msmtprc && ';
		}
		command = command + body + 'sendmail '+data['receiver']+' --syslog && rm /etc/msmtprc && mv /tmp/msmtprc.tmp /etc/msmtprc';
    } else {
		if(!data['encryption']){
			command = body + 'sendmail --host='+data['serverIP']+' --port='+data['serverPort']+' --from '+data['sender']+' '+data['receiver']+' --syslog';		
		}else{
			command = body + 'sendmail --host='+data['serverIP']+' --port='+data['serverPort']+' --tls=on --tls-trust-file='+data['certpath']+' --from '+data['sender']+' '+data['receiver']+' --syslog';		
		}
    }
	
    var w = function(e) {
        if (e.readyState == 4) {
            setControlsEnabled(true)
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
	
	document.getElementById("serverip").value = config['host'];
	document.getElementById("serverport").value = config['port'];
	document.getElementById("sender").value = config['from'];
	document.getElementById("receiver").value = uciOriginal.get("email", selector[0], "recipient");
	
	var el = document.getElementsByName('content');
	var chars = uciOriginal.get("email", selector[0], "data").split('');
	
	for (var i=0;i<el.length;i++){
		for(var o = 0;o<chars.length;o++){
			if(i == 0){
				if(!webmonEnabled){
					el[i].disabled=true;
					el[i+1].disabled=true;
					el[i].checked=false;
					el[i+1].checked=false;
					i++;
				}
			}else{
				if(parseInt(chars[o]) == i){
					el[i].checked=true;
					if(i == 5){
						document.getElementById("table_time_frame").disabled = false;
						
						var interval = uciOriginal.get("email", selector[0], "bandwidthInterval");
						switch(interval){
							case "minute":
								document.getElementById("table_time_frame").value = "minutes";
								break;
							case "180":
								document.getElementById("table_time_frame").value = "quarter hours";
								break;
							case "hour":
								document.getElementById("table_time_frame").value = "hours";
								break;
							case "day":
								document.getElementById("table_time_frame").value = "days";
								break;
							default:
								break;
						}			
					}
					break;
				}
			}
		}
	}


	if(TLSsupport==''){
		document.getElementById("encryptionText").style.display = "block";
		document.getElementById("encryptionButton").style.display = "block";
		document.getElementById("plain").checked = true;
		document.getElementById("TLS").disabled = true;
	}else{
		switch(config['tls']){
			case "off":
				document.getElementById("plain").checked = true;
				document.getElementById("TLS").checked = false;
				break;
			case "on":
				document.getElementById("TLS").checked = true;
				document.getElementById("plain").checked = false;
				break;
			default:
				document.getElementById("plain").checked = true;
				document.getElementById("TLS").checked = false;
				break;
		}	
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
		document.getElementById("table_time_frame").disabled = false;
	}else{
		document.getElementById("table_time_frame").disabled = true;
	}
}

function installTLS(){

	setControlsEnabled(false, true, UI.WaitSettings);
	
	var command = "opkg remove msmtp-nossl --force-depends && opkg install msmtp && msmtp --version";
	
	var param = getParameterDefinition("commands", command) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));		
	
    var d = function(e) {
        if (e.readyState == 4) {
			if(e.responseText.indexOf("OpenSSL") > -1){
	            location.reload();			
			}else{
				reinstallMSMTP();
			}
        }
    };
	
    runAjax("POST", "utility/run_commands.sh", param, d)
}

function reinstallMSMTP(){

	setControlsEnabled(false, true, UI.WaitSettings);
	
	var command = "opkg install msmtp-nossl && msmtp --version";
	
	var param = getParameterDefinition("commands", command) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));		
	
    var d = function(e) {
        if (e.readyState == 4) {
			if(e.responseText.indexOf("msmtp version") > -1){
				alert("Installation failed.");
				setControlsEnabled(true)
			}else{
				alert("MSMTP reinstallation failed. Plugin may need reinstall.");
			}
        }
    };
	
    runAjax("POST", "utility/run_commands.sh", param, d)
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
			case "tls-trust-file":
				data['tls-trust-file'] = array[i+1];
				i++;
				break;
			case "timeout":
				data['timeout'] = array[i+1];
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
