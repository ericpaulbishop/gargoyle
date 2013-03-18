/*
 * This program is copyright © 2013 BashfulBladder and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

var shellvarsupdater = null;

var vdr=new Array();

function MatchOUI(mac) {
	var devOUI = mac.substr(0,2) + mac.substr(3,2) + mac.substr(6,2);
	//document.getElementById("note_txt").innerHTML+=("mac" + mac + " Oui: " + devOUI + "-" + vdr[2]+ "<br/>\n");
	for (var i=0; i < vdr.length; i++) { 
		if (devOUI.match(vdr[i][0])) { return vdr[i][1]; }
		//document.getElementById("note_txt").innerHTML+=("mac" + mac + " Oui: " + devOUI + "vdr.test: " + vdr[i][0] + "<br/>\n");
	}
	return "unknown"
}

function CleanTable(table) {
	if (table == null) { return; }
	for(var i = table.rows.length; i > 0; i--) {
		table.deleteRow(i-1);
	}
}

function NewTextDiv(strArray, col, width) {
	var a_div=document.createElement('div')
	a_div.style.width=width + "px";
	a_div.id="col" + col;
	a_div.style.textAlign="center"
	for (var i=0; i < strArray.length; i++) {
		a_div.innerHTML+=strArray[i]+ "<br/>\n";
	}
	return a_div;
}

function SignalDiv(qual, strength, width, row) {
	var a_tag = document.createElement('a');
	if (row%2 == 1) {
		a_tag.className = "backer";
	} else {
		a_tag.className = "dbacker";
	}
	a_tag.title="Quality of signal: " + qual;
	
	var a_span = document.createElement('span');
	var fillage=eval(qual);
	a_span.style.width=90 * fillage + "px";
	if (fillage < 0.333) {
		a_span.className = "rfiller";
	} else if (fillage < 0.666) {
		a_span.className = "yfiller";
	} else {
		a_span.className = "gfiller";
	}	
	setSingleChild(a_tag, a_span);
	
	var a_div=document.createElement('div');
	a_div.id="col4";
	a_div.style.width=width + "px";
	a_div.style.textAlign="center";
	a_div.appendChild(a_tag);
	a_div.title="Signal level, in dBm";
	a_div.innerHTML+=strength+ "<br/>\n";
	return a_div;
}

function strtotime(ats) {
	var adate = new Date();
	adate.setFullYear(ats.substr(0, 4));
	adate.setMonth(ats.substr(4, 2));
	adate.setDate(ats.substr(6, 2));
	adate.setHours(ats.substr(8, 2));
	adate.setMinutes(ats.substr(10, 2));
	return adate;
}

function milliToDHM(msec) {
	var mdays = 86400000; //24*60*60*1000
	var mhrs = 3600000; //60*60*1000
	var d = Math.floor(msec / 86400000);
	msec-= d * 86400000;
	var h = Math.floor(msec / 3600000);
	msec-= h * 3600000;
	var m = Math.round(msec / 60000);
	return (d > 0 ? d + "d " : "") + (h > 0 ? h + "h " : "") + (m > 0 ? m + "m " : "") + "ago";
}

function LastSeen(time_now, atimestamp) {
	var diff = Math.abs( strtotime(time_now) - strtotime(atimestamp) );
	return ( diff < 60000 ? "now" : milliToDHM(diff) );
}

function Speed(sparr) {
	var speed = 0;
	for (var i = 0; i < sparr.length; i++) {
		if (eval(sparr[i] > speed)) { speed = eval(sparr[i]); }
	}
	if (speed == 0) { return "unknown"; }
	if (speed <= 11) { return "802.11b"; }
	if (speed <= 54) { return "802.11g"; }
	if (speed <= 150) { return "802.11n"; }
	return ("802.11n" + " N" + speed);
}

function Crypt(pass, karr) {
	if (pass.match("off")) { return "none"; }
	if (pass.match("on")) {
		if (karr.length > 0) {
			return ( karr[0][0] + " (" + karr[0][1] + "/" + karr[0][2] + ")" );
		}
		return "WEP";
	}
	return "unknown";
}

function FillTable(new_shell_vars, now_time) {
	var nTime=(now_time == null ? curr_time : now_time);
	var stations = (new_shell_vars == null ? station_data : new_shell_vars);
	var tableData = new Array();
	
	if (stations.length == 0) {
		document.getElementById("note_txt").innerHTML="No stations were found <br/>\n";
	} else {
		document.getElementById("note_txt").innerHTML="";
	}
	
	CleanTable(document.getElementById("station_table"));
	
	for (var i=0; i < stations.length; i++) {
		var crypos = new Array();
		for (var j=0; j < stations[i].length-10; j++) {
			crypos.push(stations[i][10+j]);
		}
		var col1div=NewTextDiv([stations[i][7], stations[i][0], MatchOUI(stations[i][0])], 1, 155);
		var col2div=NewTextDiv(["Ch " + stations[i][2] + " - " + stations[i][3] + "GHz", Speed(stations[i][8]), Crypt(stations[i][6], crypos)], 2, 120);
		var col3div=NewTextDiv([stations[i][9], LastSeen(nTime, stations[i][1]) ], 3, 80);
		var col4div=SignalDiv(stations[i][4], stations[i][5], 100, i);
		tableData.push([col1div, col2div, col3div, col4div]);

	}
	
	var sTable = createTable([""], tableData, "station_table", false, false);
	var stableC = document.getElementById('station_table_container');
	setSingleChild(stableC, sTable);
	
	if (new_shell_vars == null || new_shell_vars.length == 0) {
		document.getElementById("note_txt").innerHTML+="<br/>\nUpdating... <br/>\n";
		UpdateSurvey();
	};
}

function InitSurvey() {
	shellvarsupdater = setInterval("UpdateSurvey(null)", 120000);
	
	//document.getElementById("note_txt").innerHTML+="finished loading:" + vdr.length + "<br/>\n";

	FillTable(null);
}

function UpdateSurvey() {
	var commands = [];
	setControlsEnabled(true, false, "Updating station data");
	commands.push("echo \"var curr_time=\\\"`date \"+%Y%m%d%H%M\"`\\\";\"");
	commands.push("if [ ! -e \"/tmp/tmp_survey.txt\" ] ; then exec /usr/lib/gargoyle/survey.sh ; fi ;");
	
	var param = getParameterDefinition("commands", commands.join("\n")) + "&" + getParameterDefinition("hash", document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, ""));
	
	var stateChangeFunction = function(req) {
		//document.getElementById("note_txt").innerHTML+= stateChangeFunction + "<br/>\n";
		if (req.readyState == 4) {
			//document.getElementById("note_txt").innerHTML+=req.responseText + "<br/>\n";
			var shell_output = req.responseText.replace(/Success/, "");
			//document.getElementById("note_txt").innerHTML+=shell_output + "<br/>\n";
			eval(shell_output);
			FillTable(sdata, curr_time);
			setControlsEnabled(true);
		}
	}
	runAjax("POST", "utility/run_commands.sh", param, stateChangeFunction);
}

