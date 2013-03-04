/*
 * This program is copyright © 2013 BashfulBladder and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

/*
TODO:
• handle (not currently used, but some user might manually edit) crontabs with   * / 2   1,2,3,4,5   1-4,5 hours or days
• add a monthly crontab to delete /tmp/cron-*.backup this script generates
• wifi_status is barely glanced at in the wifi status

ENHANCEMENTS:
2 buttons with manual WiFi up & WiFi down (toggle disabled)
A disclosure triangle to show actual crontabs - I don't think is needed.
  It would just confuse new users when the summary presents a natural language representation of the crontabs.
*/

/* in this table:
	+minutes represent an hour where wifi is up UNTIL that minute, where it goes down for the remainder of the hour
	-minutes represent an hour where wifi is down UNTIL that minute, where it goes up for the remainder of the hour
	 (- for when the hour starts wifi-off and some minutes later turns on)
	 (+ for when the hour starts wifi-on and some minutes later turns on)
	
	all tables *display* positive minutes, but their .value may be positive or negative or 0 (fully off for the hour) or 60 (fully on)
	an hour can only have negative minutes coming after an hour that was off (meaning this hour will go up) OR
	 or after an hour that has positive minutes (@12:+05 goes down, up @ 13:-45)
	
	an hour can only have positive minutes coming after an hour that was on (meaning this hour will go down at some minutes) OR
	 or after an hour that has positive minutes (was up at 23:+05, down @ 00:-55)
*/

/* version history
v1.0 	initial release
v1.1 	transition to storing positive/negative minutes in each table cell
		display wifi status based on iwconfig & evaluate current time to find the wifi status @ current time
		added diagonal gradients for cells with 1-59 minutes (Safari/Chrome/Firefox)
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
	
	if (min == 0) {
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "down";
	} else if (min == 60) {
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "up";
	} else {
		a_cron_string="" + Math.abs(min) + " " + hour + " * * " + day_string + " " + garCronWIFI + " " + (min > 0 ? "down" : "up");
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

function scanSettings() { //this function will loop through the tabs & tables & generate a crontab for every hour
	var preceedingState=0;
	new_cron_tabs.length = 0;
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
			generateCronTabStr( aCell, j, i);
			preceedingState=aCell;
		}
		if (timerMode == 3 && i == 0) {
			weekdayperiod.push(new_cron_tabs.length); //start of weekdays
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
	
function ToggleTime(cell){
	var day_tab = -1;
	var hour_cell = -1;
	for ( var i = 0; i < timerMode; i++ ) {
		var aTabTable = document.getElementById("tab" + (1+eval(i)) + "_timeTable");
		if ( aTabTable.style.display == '') {
			day_tab = eval( aTabTable.id.charAt(3) );
			break;
		}
	}
	hour_cell = cell.id.split("timer_ID_")[1];
	
	var previous_state=PreviousCell(day_tab-1, hour_cell).value;
	
	if ( previous_state == 60 || (previous_state > -60 && previous_state < 0) ) {
		cell.value+=increment;
		if (cell.value > 60) {
			cell.value = 0;
		}
	} else {
		if (cell.value == 60) {
			cell.value = 0;
		} else {
			cell.value-=increment;
		}
		if (cell.value <= -60) {
			cell.value = 60;
		}
	}
		
	ToggleTimerColor(cell);
    InitSummaryText();
	UpdateSummary();
	//AddSummaryText("previous to day: " + (day_tab-1) + ", hour:" + hour_cell + ", this was the value:" + PreviousCell(day_tab-1, hour_cell).value + "<br />\n");
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
	if (navigator.userAgent.match(/Safari/) || navigator.userAgent.match(/Chrome/)) {
   		if (cell.value > 0) {
			cell.style.backgroundImage = "-webkit-linear-gradient(70deg, #00ff00 0%,#44e664 45%,#e74c4c 55%,#ff0000 100%)";
		} else {
			cell.style.backgroundImage = "-webkit-linear-gradient(-70deg, #ff0000 0%,#e74c4c 45%,#44e664 55%,#00ff00 100%)";
		}	
	} else if (navigator.userAgent.match(/Firefox/)) {
   		if (cell.value > 0) {
			cell.style.backgroundImage = "-moz-linear-gradient(70deg, #00ff00 0%,#44e664 45%,#e74c4c 55%,#ff0000 100%)";
		} else {
			cell.style.backgroundImage = "-moz-linear-gradient(-70deg, #ff0000 0%,#e74c4c 45%,#44e664 55%,#00ff00 100%)";
		}	
	} else {
		cell.style.backgroundColor = hour_partial_green;
	}
}

function ToggleTimerColor(cell) {
	if (cell.value == 0) {
		cell.innerHTML = "&nbsp;";
		cell.style.backgroundColor = hour_red;
		cell.style.backgroundImage = "";
	} else if (cell.value < 60) {
		CellGradient(cell);
		cell.innerHTML = Math.abs(cell.value);
	} else {
		cell.style.backgroundColor = hour_green;
		cell.innerHTML = "&nbsp;";
		cell.style.backgroundImage = "";
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
}

//reading/parsing & encoding table data from crontabs
function FinalizeTables() {
	//loop through table, save the initial crontab event of the first 1to59 or -1to-59 or 1000 or -1000 minutes
	//loop through the table, filling in as needed
	//at the end, fill in from first cell to inital event the last state found at the end of the period
	var initial_crontab = new Array();
	var previous_wifi_state = 0;
	
	for (var i = 0; i < timerMode; i++) {
		for (var j = 0; j < 24; j++ ) {
			var acell = document.getElementById("tab" + (1+i) + "_timeTable").rows[ (j < 12 ? 1 : 4) ].cells[ (j < 12 ? j : j-12) ];
			//AddSummaryText("hour" + (j) + "value: " + acell.value + "<br />\n");
			if (acell.value != 60 && initial_crontab.length == 0 ) {
				initial_crontab[0]=i;
				initial_crontab[1]=j;
				previous_wifi_state=(acell.value > 0 ? 1 : -1);
				//AddSummaryText("Initial event: day:" + i + "-hour:" + j + "value: " + acell.value + "<br />\n");
			}
			
			if (acell.value == 1000) {
				acell.value = 60;
				acell.style.backgroundColor = hour_green;
				acell.innerHTML = "&nbsp;";
				previous_wifi_state = 1;
			} else if (acell.value == -1000) {
				acell.value = 0;
				acell.style.backgroundColor = hour_red;
				acell.innerHTML = "&nbsp;";
				previous_wifi_state = -1;
			} else if (acell.value == 60) { //default fill value
				if ( previous_wifi_state != 0 && initial_crontab.length > 0 ) {
					if (previous_wifi_state > 0) {
						acell.value = 60;
						acell.style.backgroundColor = hour_green;
						acell.innerHTML = "&nbsp;";
						previous_wifi_state = 1;
					} else {
						acell.value = 0;
						acell.style.backgroundColor = hour_red;
						acell.innerHTML = "&nbsp;";
						previous_wifi_state = -1;
					}
				}
			} else { //minutes
				if (acell.value > 0) {
					acell.innerHTML = acell.value;	
					previous_wifi_state = -1; //the hour started with uptime, but some minutes it, wifi went down;			
				} else {
					acell.innerHTML = Math.abs(acell.value);
					previous_wifi_state = 1;				
				}
				CellGradient(acell);
			}
		}
	}
	
	//fill in span from 0day,0hour to intial crontab
	if (initial_crontab.length > 0) {
		//work backward filling in the gaps
		var initial_state = PreviousCell(0, 0).value;
		for (var k = 0; k < timerMode; k++) {
			for (var m = 0; m < 24; m++ ) {
				if (k == initial_crontab[0] && m == initial_crontab[1]) { k=20; break; }
				var bcell = document.getElementById("tab" + (1+k) + "_timeTable").rows[ (m < 12 ? 1 : 4) ].cells[ (m < 12 ? m : m-12) ];
				if (initial_state == 60) {
					bcell.value = 60;
					bcell.style.backgroundColor = hour_green;
					bcell.innerHTML = "&nbsp;";
				} else if (initial_state == 0) {
					bcell.value = 0;
					bcell.style.backgroundColor = hour_red;
					bcell.innerHTML = "&nbsp;";
				} else if (initial_state > 0) { //it went down at the end of the cycle
					bcell.value = 0;
					bcell.style.backgroundColor = hour_red;
					bcell.innerHTML = "&nbsp;";
				} else if (initial_state < 0) { //it went up at the end of the cycle
					bcell.value = 60;
					bcell.style.backgroundColor = hour_green;
					bcell.innerHTML = "&nbsp;";
				}
			}
		}
	}
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
		if (cron_cmd.search("up")>=0) { ecell.value = ecell.value*-1; }
		CellGradient(ecell);
		ecell.innerHTML = ecell.value;
	} else if (cron_cmd.search("up")>=0) {
		ecell.value = 1000;
		ecell.style.backgroundColor = hour_green;
		ecell.innerHTML = "60";
	}  else if (cron_cmd.search("down")>=0) {
		ecell.value = -1000;
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
	FinalizeTables();
}

function LoadCrontabs() {
	stripped_cron_tabs.length=0;
	var foundDailySched = 0;
	var found511Sched = 0;
	var foundWeekend=0;
	var foundWeekday=0;
	var foundWeeklySched = 0;
	var current_wifi = []; // array members: 0=current day, 1=current hour, 2=current minute, 3-translated day

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
		 	foundWeekday++;
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
	} else if ( foundWeekday || ((foundWeekend || found511Sched) && (foundWeeklySched == 0)) ) {
		timerMode=3;
		document.getElementById("timer_mode").selectedIndex=2;
	} else if (foundWeeklySched || foundWeekend) {
		timerMode=7;
		document.getElementById("timer_mode").selectedIndex=3;
	} //else timerMode remains disabled
	
	if (timerMode > 0) {
		document.getElementById('div_timer_increment').style.display = 'block';
		CronTabsToTables();
	}
	
	//show wifi radio status relative to schedule
	current_wifi[0]=eval(weekly_time.split("-")[0]);
	current_wifi[1]=eval(weekly_time.split("-")[1]);
	current_wifi[2]=eval(weekly_time.split("-")[2]);
	
	if (timerMode == 1) {
		current_wifi[3] = 0;
	} else if (timerMode == 3) {
		if ( current_wifi[0] >0 && current_wifi[0] < 6) {
			current_wifi[3] = 1;
		} else if (current_wifi[0] == 6) {
			current_wifi[3] = 2;
		} else {
			current_wifi[3] = 0;
		}
	} else {
		current_wifi[3] = current_wifi[0];
	}
	if (found_wifi_cron_tabs.length > 0 && timerMode > 0) { 
		var acell = document.getElementById("tab" + (current_wifi[3]+1) + "_timeTable").rows[ (current_wifi[1] < 12 ? 1 : 4) ].cells[ (current_wifi[1] < 12 ? current_wifi[1] : current_wifi[1]-12) ];
		
		if (acell.value == 60 || acell.value == 0) {
			setChildText("wlan_status", (acell.value > 0 ? "active (scheduled)" : "disabled (scheduled)") );
		} else {
			if (acell.value > 0) {
				setChildText("wlan_status", (acell.value > current_wifi[2] ? "active (scheduled)" : "disabled (scheduled)") );
			} else {
				setChildText("wlan_status", (current_wifi[2] > Math.abs(acell.value) ? "active (scheduled)" : "disabled (scheduled)") );
			}
		}
	} else {
		setChildText("wlan_status", (wifi_status.toString().length > 15 ? "active" : "disabled") );
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
