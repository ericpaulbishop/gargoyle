/*
 * This program is copyright © 2013 BashfulBladder and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL. 
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

//
//  GenLangContainer initiates generating the language container with 2 flanking dividing lines
//
function GenLangContainer() {
	function genDivider() { var a_div=document.createElement('div'); a_div.className="internal_divider"; return a_div }
	var lc=document.getElementById("lang_container");
	lc.appendChild(genDivider());
    lc.appendChild(GenLangDiv(1));
    lc.appendChild(genDivider());
}

//
//  GenLangMenu a highly simplistic language drop down menu with 3 preset languages
//
function GenLangMenu() {
	var opts = [{lang:"English", iso:"EN"}, {lang:"Spanish", iso:"ES"}, {lang:"German", iso:"DE"}, {lang:"Portugues BR", iso:"BR"}];
	var elem = document.createElement("select");
	elem.className="select";

	for (var i = 0; i < opts.length; i++) {
    	var mop = document.createElement("option");
    	mop.value = opts[i].lang;
    	mop.appendChild(document.createTextNode(opts[i].lang+" ("+opts[i].iso+")"))
    	elem.appendChild(mop);
	}
	return elem
}

//
//  GenLangForm generates the form+file elements for firstboot
//    input fields of 'hash' & 'fname' are created that get passed in the POST section of the form
//    the 'hash' seems to be required under Gargoyle for the upload-to-router to occur
//    the fname is used to pass the original filename (which gets purposefully lost)
//    'Save' is clicked, the form has its .submit function called which... ? targets the Iframe to download the file in POST?
//       that part is opaque - the html elements of the upgrade.sh page were followed & javascript-ized
//
function GenLangForm() {
	var a_div=document.createElement('div');
	var a_form=document.createElement("form");
	var a_iframe=document.createElement('iframe');
	var a_input=document.createElement('input');
	var f_input=document.createElement('input');
	var h_input=document.createElement('input');

	a_form.action="utility/do_fb_lang.sh";
	a_form.method="post";
	a_form.enctype="multipart/form-data";
	a_form.target="get_lfile";
	a_form.id="lfile_form";
	
	a_iframe.style.display="none";
	a_iframe.src="#";
	a_iframe.name="get_lfile";
    a_iframe.id="get_lfile";
    
	a_input.type='file';
	a_input.name='lfile';
	a_input.id='lfile';
	
	f_input.type='hidden';
	f_input.name='fname';
	f_input.id='lfile_fname';
	f_input.value='';
	
	h_input.type='hidden';
	h_input.name='hash';
	h_input.id='lfile_hash';
	h_input.value='';

	a_form.appendChild(a_input);
	a_form.appendChild(f_input);
	a_form.appendChild(h_input);
	a_div.appendChild(a_form);
	a_div.appendChild(a_iframe);

	return a_div
}

function GenLangDiv(field) {
	var lang_back = ["語","언어","ภาษา","भाषा","لغة","שפה","زبان","язык","γλώσσα",
					"dil","Sprache","kieli","język","langue","lingua","lengua","language"];

	var a_div=document.createElement('div');
	var b_div=document.createElement('div');
	a_div.style.textAlign="center";
	b_div.style.width='auto';
	b_div.style.height='100px';
	b_div.style.fontSize="30px";
	b_div.style.lineHeight="27px";
	for (var i=0; i < lang_back.length; i++) {
		b_div.innerHTML+=lang_back[i]+ " ";
	}
	a_div.appendChild(b_div);
	if (field == 1) {
		a_div.appendChild(GenLangForm());
	} /* else {
		a_div.appendChild(GenLangMenu());
	} */
	return a_div;
}

function get_lfile() {
	var fname=document.getElementById('lfile').value;
	
	if (fname.length > 0 ) {
		if (fname.match(/plugin-gargoyle-i18n/) && fname.substring(fname.lastIndexOf('.') + 1) == 'ipk') {
			document.getElementById('lfile_fname').value = document.getElementById('lfile').files[0].fileName;
			document.getElementById('lfile_hash').value = document.cookie.replace(/^.*hash=/,"").replace(/[\t ;]+.*$/, "");
			document.getElementById('lfile_form').submit();
			setControlsEnabled(false, true, "Uploading Language File");
			//nothing further needs to be done-firstboot.js will load overview.sh
		}
	}
}


function createUseButton(type)
{
	var useButton = createInput("button");
	if (type == 1) {
		useButton.value = "Activate";
		useButton.className="default_button";
		useButton.onclick = activateLang;
	} else {
		useButton.value = "Active";
		useButton.className="default_button_disabled";
		useButton.onclick = ""
	}
	return useButton;
}

function resetData()
{
	var tableContainer = document.getElementById('lang_table_container');
	tableContainer.appendChild(GenLangDiv(0));
	
	var columnNames = ['', '', ''];
	var TableData = new Array();
	var active_lang = uciOriginal.get("gargoyle", "global", "language");

	for (idx=0; idx < langs.length; idx++)
	{
		if (langs[idx] == "universal") { continue; }
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
	cmd.push("sleep 5");

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
