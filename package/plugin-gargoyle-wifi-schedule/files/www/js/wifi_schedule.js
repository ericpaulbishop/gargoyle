/*
 * This program is copyright © 2013 BashfulBladder and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

/*

/*
TODO:
• handle (not currently used, but some user might manually edit) crontabs with   * / 2   1,2,3,4,5   1-4,5 hours or days

ENHANCEMENTS:
show current wifi state in a text box above the tables - the only way to see if wifi is up is via ifconfig/iwconfig
2 buttons with manual WiFi up & WiFi down (toggle disabled)
A disclosure triangle to show actual crontabs
*/


//var showCronTabs=true; //comment this in to show the raw crontabs
var showCronTabs=false  //comment this in to not show the raw crontabs

var increment=15;
var timerMode=0;
var hour_red="#ff0000";
var hour_partial_green="#90c050";
var hour_green="#00ff00";
var garCronWIFI = "/usr/lib/gargoyle/scheduled_wifi.sh";
var new_cron_tabs = [];
var stripped_cron_tabs = [];
var found_wifi_cron_tabs = [];
var weekdayperiod = [];

var weeklyPeriod = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
var week511Period = ["Sunday", "Monday-Friday", "Saturday"];
var dailyPeriod = ["Daily"];

//   Summary text functions
function InitSummaryText() {
	document.getElementById("summary_container").className = 'tabField';
	var textSpan=document.getElementById("summary_txt");
	
	textSpan.innerHTML="<strong>Summary:</strong><br />\n";
}

function AddSummaryText(more_text) {
	document.getElementById("summary_txt").innerHTML+=more_text;
}

function generateCronTabStr(min, hour, day) {
	var a_cron_string = "";
	var day_string="" ;
	var previous_WiFi_state="";
	
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
	
	if (new_cron_tabs.length > 0) {
		previous_WiFi_state=new_cron_tabs[ new_cron_tabs.length-1 ].split(" ")[6];
	}
	
	if (min == 0) {
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "down";
	} else if (min == 60) {
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "up";
	} else {
		a_cron_string="" + min + " " + hour + " * * " + day_string + " " + garCronWIFI + " " + (previous_WiFi_state.search("up") >= 0 ? "down" : "up");
	}
	
	new_cron_tabs.push(a_cron_string);
}

function CronTabCull() {
	var culledTabs = [];
	var previous_WiFi_state=new_cron_tabs[ new_cron_tabs.length-1 ].split(" ")[6];
	
	for(var i = 0; i < new_cron_tabs.length; i++) {
		//AddSummaryText(previous_WiFi_state + " " + new_cron_tabs[i].split(" ")[6] + " = match: " + new_cron_tabs[i].split(" ")[6].match(previous_WiFi_state) + "<br />\n");
		if ( !(new_cron_tabs[i].split(" ")[6].match(previous_WiFi_state))) {
			culledTabs.push(new_cron_tabs[i]);
			previous_WiFi_state=new_cron_tabs[i].split(" ")[6];
		} else if (timerMode == 3 && i == weekdayperiod[0]) {
			if ( new_cron_tabs[weekdayperiod[0]].split(" ")[6].match("up") && new_cron_tabs[weekdayperiod[1]-1].split(" ")[6].match("down") ) {
				culledTabs.push(new_cron_tabs[weekdayperiod[0]]);
			}
		}
	}
	//comment out this to view the original hourly crontabs before culling
	new_cron_tabs.length = 0;
	for(var j = 0; j < culledTabs.length; j++) {
		new_cron_tabs.push(culledTabs[j]);
	}
	culledTabs.length - 0;
}

function scanSettings() {
	//this function will loop through the tabs & tables & generate a crontab for every hour
	var preceedingState=0;
	new_cron_tabs.length = 0;
	needs_initial_state=0;
	weekdayperiod = [];
		
	for (var i = 0; i < timerMode; i++) {
		var aTable = document.getElementById("tab" + (1+eval(i)) + "_timeTable"); //tab1_timeTable
		if (i == 0) {
			preceedingState=aTable.rows[1].cells[0].value; //always generate initial state of period
		}
		if (timerMode == 3 && i == 2) {
			weekdayperiod.push(new_cron_tabs.length); //end of weekdays
		}
		//step through the table
		for( var j = 0; j < 24; j++ ) {
			var aCell=aTable.rows[ (j < 12 ? 1 : 4) ].cells[ (j < 12 ? j : j-12) ].value;
			if (j == 0 && i == 0 && (aCell > 0 && aCell < 60)) { needs_initial_state = 1; }
			generateCronTabStr( aCell, j, i);
			preceedingState=aCell;
		}
		if (timerMode == 3 && i == 0) {
			weekdayperiod.push(new_cron_tabs.length); //start of weekdays
		}
	}
	if (needs_initial_state == 1) { //we need an intial 0000 wifi state (found from terminal wifi state) to preceeed a 00XX crontab event
		generateCronTabStr( 0, 0, 0);
		var initial_crontab = new_cron_tabs.pop();
		new_cron_tabs.splice(0, 0, initial_crontab);
		if (timerMode == 3) {
			weekdayperiod[0]++;
			weekdayperiod[1]++;
		}
	}
	CronTabCull();
}

function UpdateSummary() {  //summary is dynamically generated from parsed crontab text
	if (timerMode > 0) { scanSettings(); }
	
	AddSummaryText("Selected timer mode: ");
	if (timerMode == 0) { AddSummaryText("disabled (no schedule).<br />\n"); }
	if (timerMode == 1) { AddSummaryText("daily schedule (every day is the same).<br />\n"); }
	if (timerMode == 3) { AddSummaryText("Sun/weekday/Sat schedule (every weekday is the same).<br />\n"); }
	if (timerMode == 7) { AddSummaryText("weekly schedule (separate timer for each day of the week; each week is the same).<br />\n"); }
	
	if (showCronTabs) {
		for(var i = 0; i < new_cron_tabs.length; i++) {
			AddSummaryText(new_cron_tabs[i] + "<br />\n");
		}
		AddSummaryText("<br />\n");
	}
	
	for(var ii = 0; ii < new_cron_tabs.length; ii++) {
		
		var aCronTab = new_cron_tabs[ii];
		var minCronText = aCronTab.split(" ")[0];
		var hourCronText = aCronTab.split(" ")[1];
		var dayCronText = aCronTab.split(" ")[4];
		var wifiCronCMD = aCronTab.split(" ")[6];
		var minuteStr = (minCronText < 10 ? '0' + minCronText : (minCronText == 60 ? '00' : minCronText));
		var day_string="";
		
		//dayCronText could be 0,2,4-6 or */2 but this script won't generate that, so no need to parse it
		if (dayCronText == "*") { day_string = "Daily"; }
		if (dayCronText == "0") { day_string = "Sunday"; }
		if (dayCronText == "1") { day_string = "Monday"; }
		if (dayCronText == "2") { day_string = "Tuesday"; }
		if (dayCronText == "3") { day_string = "Wednesday"; }
		if (dayCronText == "4") { day_string = "Thursday"; }
		if (dayCronText == "5") { day_string = "Friday"; }
		if (dayCronText == "6") { day_string = "Saturday"; }
		if (dayCronText == "1-5") { day_string = "Monday-Friday"; }		
		
		AddSummaryText("Wifi will go " + (wifiCronCMD.search("up") >= 0 ? "&nbsp;&nbsp;up&nbsp;&nbsp;" : "down") + " - " +  day_string + " at " + (hourCronText < 10 ? '0' + hourCronText : hourCronText) + ":" + minuteStr + "<br />\n");
	}
}

//   Table functions	
function ToggleTime(cell){
	cell.value+=increment;
	if (cell.value > 60) {
		cell.value = 0;
	}
	ToggleTimerColor(cell);
    InitSummaryText();
	UpdateSummary();
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

function ToggleTimerColor(cell) {
	if (cell.value == 0) {
		cell.innerHTML = "&nbsp;";
		cell.style.backgroundColor = hour_red;
	} else if (cell.value < 60) {
		cell.style.backgroundColor = hour_partial_green;
		cell.innerHTML = cell.value;
	} else {
		cell.style.backgroundColor = hour_green;
		cell.innerHTML = "&nbsp;";
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
	AddSummaryText("raws" + raw_cron_tabs + "<br />\n");
}

//reading/parsing & encoding table data from crontabs
function FinalizeTables(initial_wifi_state) {
	var current_wifi_state = initial_wifi_state;
	for (var i = 0; i < timerMode; i++) {
		for (var j = 0; j < 24; j++ ) {
			var acell = document.getElementById("tab" + (1+i) + "_timeTable").rows[ (j < 12 ? 1 : 4) ].cells[ (j < 12 ? j : j-12) ];
			//AddSummaryText("hour" + (j) + "value: " + acell.value + "<br />\n");
			if (acell.value == 60) {
				if (current_wifi_state > 0) {
					acell.value = 60;
					acell.style.backgroundColor = hour_green;
					acell.innerHTML = "&nbsp;";
				} else {
					acell.value = 0;
					acell.style.backgroundColor = hour_red;
					acell.innerHTML = "&nbsp;";
				}
			} else if (acell.value > 0) { //hard crontab wifi up
				if (acell.value > 60) {
					acell.value = 60;
					acell.style.backgroundColor = hour_green;
					acell.innerHTML = "&nbsp;";
				} //minutes set on wifi up are already properly done
				current_wifi_state=1;
			} else if (acell.value < 0) { //hard crontab wifi down
				if (acell.value == -1) {
					acell.value = 0;
					acell.style.backgroundColor = hour_red;
					acell.innerHTML = "&nbsp;";
				} else {
					acell.value = acell.value*-1;
					acell.style.backgroundColor = hour_partial_green;
					acell.innerHTML = acell.value;
				}
				current_wifi_state=-1;
			}
		}
	}
}

function FindTerminalWifiState() {
	//loop backwards in the cycle; return final cell value
	for (var i = timerMode; i > 0; i--) {
		for (var j = 23; j >= 0; j--) {
			var acell = document.getElementById("tab" + i + "_timeTable").rows[ (j < 12 ? 1 : 4) ].cells[ (j < 12 ? j : j-12) ];
			if (acell.value != 60) {
				return acell.value;
			}
			//AddSummaryText("&nbsp;&nbsp;table: " + (1+i) + " hour "+ j + "<br />\n");
		}
	}
	return 0;
}

function SeatCronData(cron_minute, cron_hour, cron_day, cron_cmd) {
	//this function currently assumes cronhour is 0 <= integer <= 23
	var ecron_minute=eval(cron_minute);
	var ecron_hour=eval(cron_hour);
	var ecron_day=eval(cron_day);
	var day_table = document.getElementById("tab" + (1+ecron_day) + "_timeTable");
	var ecell=day_table.rows[ (ecron_hour < 12 ? 1 : 4) ].cells[ (ecron_hour < 12 ? ecron_hour : ecron_hour-12) ];
	
	if (ecron_minute > 0 && ecron_minute < 60) {
		ecell.value = ecron_minute;
		if (cron_cmd.search("down")>=0) { ecell.value = ecell.value*-1; }
		ecell.style.backgroundColor = hour_partial_green;
		ecell.innerHTML = ecell.value;
	} else if (cron_cmd.search("up")>=0) {
		ecell.value = 1000;
		ecell.style.backgroundColor = hour_green;
		ecell.innerHTML = "60";
	}  else if (cron_cmd.search("down")>=0) {
		ecell.value = -1;
		ecell.style.backgroundColor = hour_red;
		ecell.innerHTML = "-1";
	}
}

function CronTabsToTables() {
	SetupTabs(timerMode);
	for ( var i=0; i < found_wifi_cron_tabs.length; i++ ) {
		var aFetchedCronTab = found_wifi_cron_tabs[i];
		
		var aCronMinute = aFetchedCronTab.split(" ")[0];
		var aCronHour = aFetchedCronTab.split(" ")[1];
		var aCronDay = aFetchedCronTab.split(" ")[4];
		var aCronWifiCmd = aFetchedCronTab.split(" ")[6];
		
		if (timerMode == 1) {
			aCronDay = "0";
		} else if (timerMode == 3) {
			
			if ( aCronDay == "1-5") {
				aCronDay = "1";
			} else if (aCronDay == "6") {
				aCronDay = "2";
			}
		}
		//TODO: say that someone changes a crontab to have */2    1,2,3,4,5   1-4,5 hours or days		
		if (eval(aCronHour) > 23 || eval(aCronHour) < 0 ) { aCronHour=0; }
		if (eval(aCronMinute) > 59 || eval(aCronMinute) < 0 ) { aCronMinute=0; }
		
		for (sch_days=0; sch_days < aCronDay.split(",").length; sch_days++) {
			for (sch_hours=0; sch_hours < aCronHour.split(",").length; sch_hours++) {
				//AddSummaryText("&nbsp;&nbsp;Seat D-" + aCronDay.split(",")[sch_days] + "h-" + aCronHour.split(",")[sch_hours] + " cron: " + aFetchedCronTab + "<br />\n");
				SeatCronData(aCronMinute, aCronHour.split(",")[sch_hours], aCronDay.split(",")[sch_days], aCronWifiCmd);
			}
		}		
	}
	var end_state = FindTerminalWifiState(); //cell represents the terminal wifi state of the cycle
	//AddSummaryText("&nbsp;&nbsp;ending state: " + end_state + "<br />\n");
	FinalizeTables(end_state);
}

function LoadCrontabs() {
	stripped_cron_tabs.length=0;
	var foundDailySched = 0;
	var found511Sched = 0;
	var foundWeekend=0;
	var foundWeeklySched = 0;
	
	InitSummaryText();
	
	for ( var i=0; i < raw_cron_data.length; i++ ) {
		if (raw_cron_data[i].search(garCronWIFI) > 0) { 
			found_wifi_cron_tabs.push( raw_cron_data[i] );
			if (showCronTabs) { AddSummaryText("WiFi: " + found_wifi_cron_tabs[found_wifi_cron_tabs.length-1] + "<br />\n"); }
		} else if (raw_cron_data[i].length > 0) { 
			stripped_cron_tabs.push( raw_cron_data[i]);
			if (showCronTabs) { AddSummaryText("system: " + stripped_cron_tabs[stripped_cron_tabs.length-1] + "<br />\n"); }
		}
	}
	
		//figure out which timer mode to display (disabled, daily, 511 or weekly)
	for ( var j=0; j < found_wifi_cron_tabs.length; j++ ) {
		if (found_wifi_cron_tabs[j].split(" ")[4] == "*") {
			foundDailySched++; //Daily
			
		} else if (found_wifi_cron_tabs[j].split(" ")[4] == "1-5") {
		 	found511Sched++; //511 (weedkay + sat + sun
		} else if (  found_wifi_cron_tabs[j].split(" ")[4].match(/[1-5]/g) >= 0  ) {
			// there could also be this:   */2    1,2,3,4,5   1-4,5
			foundWeeklySched++; //weekly
		} else if (  found_wifi_cron_tabs[j].split(" ")[4].match(/[0,6]/g) >= 0  ) { //it can be either 511 or weekly
			foundWeekend++; //511 (weedkay + sat + sun)
		}
	}
	if (foundDailySched && (found511Sched == 0 && foundWeekend == 0 && foundWeeklySched == 0) ) {
		timerMode=1;
		document.getElementById("timer_mode").selectedIndex=1;
	} else if ( (foundWeekend || found511Sched) && (foundWeeklySched == 0) ) {
		timerMode=3;
		document.getElementById("timer_mode").selectedIndex=2;
	} else if (foundWeeklySched || foundWeekend) {
		timerMode=7;
		document.getElementById("timer_mode").selectedIndex=3;
	} //else timerMode remains disabled
	
	//AddSummaryText("Mode: " + timerMode + "<br />\n");
	
	if (timerMode > 0) {
		document.getElementById('div_timer_increment').style.display = 'block';
		CronTabsToTables();
	}
	UpdateSummary();
}

function saveChanges() { 	//follow reboot.sh somewhat
	var commands = [];
	setControlsEnabled(false, true, "Please Wait While Settings Are Applied");
	commands.push("mkdir -p /etc/crontabs"); //should fail gracefully
	commands.push("touch /etc/crontabs/root"); //no harm, no foul
	commands.push("cp /etc/crontabs/root /tmp/cron-" + current_time + ".backup"); //create timestamped backup so we don't clobber a good backup with a bad backup

	//these are pre-existing non-WiFi-schedule crontabs
	for (var i=0; i < stripped_cron_tabs.length; i++) {
		commands.push("echo \'" + stripped_cron_tabs[i] + "\' >> /tmp/cron.tmp");
	}
	for (var j=0; j < new_cron_tabs.length; j++) {
		commands.push("echo \'" + new_cron_tabs[j] + "\' >> /tmp/cron.tmp");
	}
	commands.push("mv /tmp/cron.tmp /etc/crontabs/root");
	commands.push("/etc/init.d/cron restart");
	
	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));		
	var stateChangeFunction = function(req) {
		if (req.readyState == 4) {
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}
