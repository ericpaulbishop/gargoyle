/*
 * This program is copyright © 2013 BashfulBladder and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */
var Wsch=new Object(); //part of i18n

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

var weeklyPeriod = [];
var week511Period = [];
var dailyPeriod = [];

var shellvarsupdater = null;
var current_time = []; // array members: 0=current day, 1=current hour, 2=current minute, 3-translated day
var Wi_Fi = 0;
var cloned_crontab_table = [];

function ToggleWifiButtons() {
	if (Wi_Fi == 1) {
		setElementEnabled(document.getElementById("wifi_up_button"), false);
		setElementEnabled(document.getElementById("wifi_down_button"), true);
	} else if (Wi_Fi == -1) {
		setElementEnabled(document.getElementById("wifi_up_button"), true);
		setElementEnabled(document.getElementById("wifi_down_button"), false);
	}
}

//   Summary text functions
function InitSummaryText() {
	document.getElementById("summary_container").className = 'tabField';
	var textSpan=document.getElementById("summary_txt");
	
	textSpan.innerHTML="<strong>"+Wsch.Smmy+":</strong><br />\n";
}

function AddSummaryText(more_text) {
	document.getElementById("summary_txt").innerHTML+=more_text;
}

function generateCronTabStr(min, hour, day, extra) {
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
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "down" + extra;
	} else if (min == 60) {
		a_cron_string="0 " + hour + " * * " + day_string + " " + garCronWIFI + " " + "up" + extra;
	} else {
		a_cron_string="" + Math.abs(min) + " " + hour + " * * " + day_string + " " + garCronWIFI + " " + (min > 0 ? "down" : "up") + extra;
	}
	
	new_cron_tabs.push(a_cron_string);
}

function CronTabCull() {
	var culledTabs = [];
	var previous_WiFi_state=new_cron_tabs[ new_cron_tabs.length-1 ].split(" ")[6];
	
	for(var i = 0; i < new_cron_tabs.length; i++) {
		if ( !(new_cron_tabs[i].split(" ")[6].match(previous_WiFi_state))) {
			culledTabs.push(new_cron_tabs[i]);
			previous_WiFi_state=new_cron_tabs[i].split(" ")[6];
		} else if (new_cron_tabs[i].search("keep") > 0 ) {
			culledTabs.push(new_cron_tabs[i].split("keep")[0]);
			previous_WiFi_state=new_cron_tabs[i].split(" ")[6];
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

function scanSettings() { //this function will loop through the tabs & tables & generate a crontab for every hour
	var preceedingState=0;
	new_cron_tabs.length = 0;
		
	for (var i = 0; i < timerMode; i++) {
		var aTable = document.getElementById("tab" + (1+eval(i)) + "_timeTable"); //tab1_timeTable
		if (i == 0) {
			preceedingState=aTable.rows[1].cells[0].value; //always generate initial state of period
		}
		//step through the table
		for( var j = 0; j < 24; j++ ) {
			var acell=aTable.rows[ (j < 12 ? 1 : 4) ].cells[ (j < 12 ? j : j-12) ];
			var pre_cell = PreviousCell(i, j);
			
			//handle edge cases that will survive culling by appending "keep"
			if ( pre_cell.value >= 0 && pre_cell.value < 60 && acell.value > 0 && acell.value < 60) {
				generateCronTabStr( 60, j, i, " keep1"); //double down, needs an up event
			} else if ( pre_cell.value < 0 && pre_cell.value > -60 && acell.value < 0 && acell.value > -60) {
				generateCronTabStr( 0, j, i, " keep2"); //double up, needs an down event
			
			//handle weekday cycling oddiies
			} else if (timerMode == 3 && i == 1 && j == 0) {
				if (ThisCell(1,23).value == 60 && acell.value == 0) {
					if (pre_cell.value < 60 && pre_cell.value > 0) {
						generateCronTabStr( 0, j, i, " keep3");
					} else if (pre_cell.value == 0 ) {
						generateCronTabStr( 0, j, i, " keep4");
					}
				} else if (ThisCell(1,23).value < 0 && acell.value == 0) {
					generateCronTabStr( 0, j, i, " keep5");
				} else if (pre_cell.value == 60 && acell.value == 60 && ThisCell(1,23).value == 0) {
					generateCronTabStr( 60, j, i, " keep6");
				}
			}
			generateCronTabStr( acell.value, j, i, "");
			preceedingState=acell.value;
		}
	}
	CronTabCull();
}

function CronWarning() {
	//find current time in table; if our status differes from the schedule, alert user the schedule will take effect in on dayXhourY
	var thishour = ThisCell(current_time[3], current_time[1]);
	if ( (Wi_Fi == 1 && thishour.value == 0) || (Wi_Fi == -1 && thishour.value == 60) ) {
		AddSummaryText("<br/>\n<strong>"+Wsch.Warn+":</strong><br/>\n");
		AddSummaryText(Wsch.NextEv+"<br/>\n");
	} else if (thishour.value > 0 && thishour.value < 60 && current_time[2] > thishour.value && Wi_Fi == 1) {
		AddSummaryText("<br/>\n<strong>"+Wsch.Warn+":</strong><br/>\n");
		AddSummaryText(Wsch.NextEv+"<br/>\n");
	} else if (thishour.value < 0 && current_time[2] < Math.abs(thishour.value) && Wi_Fi == 1) {
		AddSummaryText("<br/>\n<strong>"+Wsch.Warn+":</strong><br/>\n");
		AddSummaryText(Wsch.NextEv+"<br/>\n");
	}
}

function UpdateSummary() {  //summary is dynamically generated from parsed crontab text
	if (timerMode > 0) { scanSettings(); }
	
	AddSummaryText(Wsch.SelTM+": ");
	if (timerMode == 0) { AddSummaryText(Wsch.SumDis+"<br />\n"); }
	if (timerMode == 1) { AddSummaryText(Wsch.SumDly+"<br />\n"); }
	if (timerMode == 3) { AddSummaryText(Wsch.SumSwS+"<br />\n"); }
	if (timerMode == 7) { AddSummaryText(Wsch.SumWky+"<br />\n"); }
	
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
		if (dayCronText == "*") { day_string = Wsch.STDly; }
		if (dayCronText == "0") { day_string = Wsch.STSunday; }
		if (dayCronText == "1") { day_string = Wsch.STMonday; }
		if (dayCronText == "2") { day_string = Wsch.STTuesday; }
		if (dayCronText == "3") { day_string = Wsch.STWednesday; }
		if (dayCronText == "4") { day_string = Wsch.STThursday; }
		if (dayCronText == "5") { day_string = Wsch.STFriday; }
		if (dayCronText == "6") { day_string = Wsch.STSaturday; }
		if (dayCronText == "1-5") { day_string = Wsch.STMonFri; }
		
		AddSummaryText(Wsch.SumGo+" " + (wifiCronCMD.search("up") >= 0 ? Wsch.SumUp : Wsch.SumDn) + " - " +  day_string + " "+Wsch.SumAt+" " + (hourCronText < 10 ? '0' + hourCronText : hourCronText) + ":" + minuteStr + "<br />\n");
	}
	
	if (timerMode > 0) { CronWarning(); }
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

function ThisCell(aday,ahour) {
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

function SetCellContents(cell, text, value, color) {
	if (value != null) { cell.value = value; }
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
			
			if (acell.value != 60 && initial_crontab.length == 0 ) {
				initial_crontab[0]=i;
				initial_crontab[1]=j;
				previous_wifi_state=(acell.value > 0 ? 1 : -1);
			}
			
			if (acell.value == 1000) {
				SetCellContents(acell, "&nbsp;", 60, hour_green);
				previous_wifi_state = 1;
			} else if (acell.value == -1000) {
				SetCellContents(acell, "&nbsp;", 0, hour_red);
				previous_wifi_state = -1;
			} else if (acell.value == 60) { //default fill value
				if ( previous_wifi_state != 0 && initial_crontab.length > 0 ) {
					if (previous_wifi_state > 0) {
						SetCellContents(acell, "&nbsp;", 60, hour_green);
						previous_wifi_state = 1;
					} else {
						SetCellContents(acell, "&nbsp;", 0, hour_red);
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
					SetCellContents(bcell, "&nbsp;", 60, hour_green);
				} else if (initial_state == 0) {
					SetCellContents(bcell, "&nbsp;", 0, hour_red);
				} else if (initial_state > 0) { //it went down at the end of the cycle
					SetCellContents(bcell, "&nbsp;", 0, hour_red);
				} else if (initial_state < 0) { //it went up at the end of the cycle
					SetCellContents(bcell, "&nbsp;", 60, hour_green);
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
	} else if (cron_cmd.search("up") >= 0) {
		SetCellContents(ecell, "60", 1000, hour_green);
	}  else if (cron_cmd.search("down") >= 0) {
		SetCellContents(ecell, "-1", -1000, hour_red);
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

function SetWifiStatus(shell_iwconfig) {
	var iwconfig = (shell_iwconfig == null ? wifi_status : shell_iwconfig);
	if (iwconfig.toString().length > 0) {
		Wi_Fi = 1;
	} else {
		Wi_Fi = -1;
	}
	if (found_wifi_cron_tabs.length > 0 && timerMode > 0) { //this causes a display hiccup when there are crontabs on the router, hit reset button (scheduled) disappears, but there is still a schedule
		var this_cron_hour = cloned_crontab_table[ current_time[3] ][current_time[1]];
		if (this_cron_hour == 60 || this_cron_hour == 0) {
			if (Wi_Fi > 0) {
				setChildText("wlan_status", (this_cron_hour == 60 ? Wsch.actv+" ("+Wsch.schd+")" : Wsch.actv+" ("+Wsch.nscd+")") );
			} else {
				setChildText("wlan_status", (this_cron_hour == 60 ? UI.disabled+" ("+Wsch.nscd+")" : UI.disabled+" ("+Wsch.schd+")") );
			}
		} else { //minutes on the hour
			if (this_cron_hour > 0) {
				if (Wi_Fi > 0) {
					setChildText("wlan_status", (this_cron_hour > current_time[2] ? Wsch.actv+" ("+Wsch.schd+")" : Wsch.actv+" ("+Wsch.nscd+")") );
				} else {
					setChildText("wlan_status", (this_cron_hour > current_time[2] ? UI.disabled+" ("+Wsch.nscd+")" : UI.disabled+" ("+Wsch.schd+")") );
				}
			} else {
				if (Wi_Fi > 0) {
					setChildText("wlan_status", (current_time[2] > Math.abs(this_cron_hour) ? Wsch.actv+" ("+Wsch.nscd+")" : Wsch.actv+" ("+Wsch.schd+")") );
				} else {
					setChildText("wlan_status", (current_time[2] > Math.abs(this_cron_hour) ? UI.disabled+" ("+Wsch.nscd+")" : UI.disabled+" ("+Wsch.schd+")") );
				}
			}
		}
	} else {
		setChildText("wlan_status", (Wi_Fi > 0 ? Wsch.actv : UI.disabled), (Wi_Fi > 0 ? "green" : "red") );
	}
}

function ParseCurrentTime(shell_vars) {
	var globbed_time = (shell_vars == null ? weekly_time : shell_vars);
	current_time.length=0;
	current_time=globbed_time.split("-");
	for (var i=0; i < 3; i++) {
		current_time[i] = eval(current_time[i]);
	}
	
	if (timerMode == 1) {
		current_time[3] = 0;
	} else if (timerMode == 3) {
		if ( current_time[0] >0 && current_time[0] < 6) {
			current_time[3] = 1;
		} else if (current_time[0] == 6) {
			current_time[3] = 2;
		} else {
			current_time[3] = 0;
		}
	} else {
		current_time[3] = current_time[0];
	}
	//AddSummaryText(globbed_time + "->" + current_time[0] + "." + current_time[1] + "." + current_time[2] + "<br/>\n");
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
	stripped_cron_tabs.length=0;
	var foundDailySched = 0;
	var found511Sched = 0;
	var foundWeekend=0;
	var foundWeekday=0;
	var foundWeeklySched = 0;
	
	dailyPeriod = [ Wsch.Dly ];
	week511Period = Wsch.WDayA;
	weeklyPeriod = Wsch.WeekA;

	InitSummaryText();
	shellvarsupdater = setInterval("GetWifiUpdate(null)", 5000);
	
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
	
	CloneTable();
	ParseCurrentTime(null);
	SetWifiStatus(null);
	ToggleWifiButtons();
	
	if (timerMode > 0) {
		ShowTab(document.getElementById("tab_ID_" + current_time[3]));
	}

	UpdateSummary();
}

function saveChanges() { 	//follow reboot.sh somewhat
	var commands = [];
	setControlsEnabled(false, true, UI.WaitSettings);
	commands.push("mkdir -p /etc/crontabs"); //should fail gracefully
	commands.push("touch /etc/crontabs/root"); //no harm, no foul
	commands.push("cat /etc/crontabs/root | grep -v -e '" + garCronWIFI + "' > /tmp/cron.backup");
	commands.push("cat /dev/null > /tmp/cron.tmp");
	
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
	CloneTable();
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

function GetWifiUpdate(force_wifi) {
	var commands = [];
	if (force_wifi != null) {
		commands.push("wifi " + force_wifi);
		setControlsEnabled(false, true, "Taking Wifi " + force_wifi);
	}
	commands.push("echo \"var weekly_time=\\\"`date \"+%w-%H-%M\"`\\\";\"");
	commands.push("echo \"var wifi_status = new Array();\"");
	commands.push("iwconfig 2>&1 | grep -v 'wireless' | sed '/^$/d' | awk -F'\\\n' '{print \"wifi_status.push(\\\"\"$0\"\\\");\" }'");
	
	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));		
	var stateChangeFunction = function(req) {
		if (req.readyState == 4) {
			var shell_output = req.responseText.replace(/Success/, "");
			eval(shell_output);
			ParseCurrentTime(weekly_time);
			SetWifiStatus(wifi_status);
			ToggleWifiButtons();
			if(force_wifi != null) {
				setControlsEnabled(true);
			}
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

