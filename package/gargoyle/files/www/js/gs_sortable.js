// Copyright 2007 - 2010 Gennadiy Shvets
// The program is distributed under the terms of the GNU General
// Public License 3.0
//
// See http://www.allmyscripts.com/Table_Sort/index.html for usage details.

// Script version 1.8

var TSort_Store;
var TSort_All;

function TSort_StoreDef () {
	this.sorting = [];
	this.nodes = [];
	this.rows = [];
	this.row_clones = [];
	this.sort_state = [];
	this.initialized = 0;
//	this.last_sorted = -1;
	this.history = [];
	this.sort_keys = [];
	this.sort_colors = [ '#FF0000', '#800080', '#0000FF' ];
};

function tsInitOnload ()
{
	//	If TSort_All is not initialized - do it now (simulate old behavior)
	if	(TSort_All == null)
		tsRegister();

	for (var id in TSort_All)
	{
		tsSetTable (id);
		tsInit();
	}
	if	(window.onload_sort_table)
		window.onload_sort_table();
}

function tsInit()
{

	if	(TSort_Data.push == null)
		return;
	var table_id = TSort_Data[0];
	var table = document.getElementById(table_id);
	// Find thead
	var thead = table.getElementsByTagName('thead')[0];
	if	(thead == null)
	{
		alert ('Cannot find THEAD tag!');
		return;
	}
	var tr = thead.getElementsByTagName('tr');
	var cols, i, node, len;
	if	(tr.length > 1)
	{
		var	cols0 = tr[0].getElementsByTagName('th');
		if	(cols0.length == 0)
			cols0 = tr[0].getElementsByTagName('td');
		var cols1;
		var	cols1 = tr[1].getElementsByTagName('th');
		if	(cols1.length == 0)
			cols1 = tr[1].getElementsByTagName('td');
		cols = new Array ();
		var j0, j1, n;
		len = cols0.length;
		for (j0 = 0, j1 = 0; j0 < len; j0++)
		{
			node = cols0[j0];
			n = node.colSpan;
			if	(n > 1)
			{
				while (n > 0)
				{
					cols.push (cols1[j1++]);
					n--;
				}
			}
			else
			{
				if	(node.rowSpan == 1)
					j1++;
				cols.push (node);
			}
		}
	}
	else
	{
		cols = tr[0].getElementsByTagName('th');
		if	(cols.length == 0)
			cols = tr[0].getElementsByTagName('td');
	}
	len = cols.length;
	for (var i = 0; i < len; i++)
	{
		if	(i >= TSort_Data.length - 1)
			break;
		node = cols[i];
		var sorting = TSort_Data[i + 1].toLowerCase();
		if	(sorting == null)  sorting = '';
		TSort_Store.sorting.push(sorting);

		if	((sorting != null)&&(sorting != ''))
		{
//			node.tsort_col_id = i;
//			node.tsort_table_id = table_id;
//			node.onclick = tsDraw;
			node.innerHTML = "<a href='' onClick=\"tsDraw(" + i + ",'" +
				table_id + "'); return false\">" + node.innerHTML +
				'</a><b><span id="TS_' + i + '_' + table_id + '"></span></b>';
			node.style.cursor = "pointer";
		}
	}

	// Get body data
	var tbody = table.getElementsByTagName('tbody')[0];
	if	(tbody == null)	return;
	// Get TR rows
	var rows = tbody.getElementsByTagName('tr');
	var date = new Date ();
	var len, text, a;
	for (i = 0; i < rows.length; i++)
	{
		var row = rows[i];
		var cols = row.getElementsByTagName('td');
		var row_data = [];
		for (j = 0; j < cols.length; j++)
		{
			// Get cell text
			text = cols[j].innerHTML.replace(/^\s+/, '');
			text = text.replace(/\s+$/, '');
			var sorting = TSort_Store.sorting[j];
			if	(sorting == 'h')
			{
				text = text.replace(/<[^>]+>/g, '');
				text = text.toLowerCase();
			}
			else if	(sorting == 's')
				text = text.toLowerCase();
			else if (sorting == 'i')
			{
				text = parseInt(text);
				if	(isNaN(text))	text = 0;
			}
			else if (sorting == 'n')
			{
				text = text.replace(/(\d)\,(?=\d\d\d)/g, "$1");
				text = parseInt(text);
				if	(isNaN(text))	text = 0;
			}
			else if (sorting == 'c')
			{
				text = text.replace(/^\$/, '');
				text = text.replace(/(\d)\,(?=\d\d\d)/g, "$1");
				text = parseFloat(text);
				if	(isNaN(text))	text = 0;
			}
			else if (sorting == 'f')
			{
				text = parseFloat(text);
				if	(isNaN(text))	text = 0;
			}
			else if (sorting == 'g')
			{
				text = text.replace(/(\d)\,(?=\d\d\d)/g, "$1");
				text = parseFloat(text);
				if	(isNaN(text))	text = 0;
			}
			else if (sorting == 'd')
			{
				if	(text.match(/^\d\d\d\d\-\d\d?\-\d\d?(?: \d\d?:\d\d?:\d\d?)?$/))
				{
					a = text.split (/[\s\-:]/);
					text = (a[3] == null)?
						Date.UTC(a[0], a[1] - 1, a[2],    0,    0,    0, 0):
						Date.UTC(a[0], a[1] - 1, a[2], a[3], a[4], a[5], 0);
				}
				else
					text = Date.parse(text);
			}
			row_data.push(text);
		}
		TSort_Store.rows.push(row_data);
		// Save a reference to the TR element
		var new_row = row.cloneNode(true);
		new_row.tsort_row_id = i;
		TSort_Store.row_clones[i] = new_row;
	}
	TSort_Store.initialized = 1;

	if	(TSort_Store.cookie)
	{
		var allc = document.cookie;
		i = allc.indexOf (TSort_Store.cookie + '=');
		if	(i != -1)
		{
			i += TSort_Store.cookie.length + 1;
			len = allc.indexOf (";", i);
			text = decodeURIComponent (allc.substring (i, (len == -1)?
				allc.length: len));
			TSort_Store.initial = (text == '')? null: text.split(/\s*,\s*/);
		}
	}

	var	initial = TSort_Store.initial;
	if	(initial != null)
	{
		var itype = typeof initial;
		if	((itype == 'number')||(itype == 'string'))
			tsDraw(initial);
		else
		{
			for (i = initial.length - 1; i >= 0; i--)
				tsDraw(initial[i]);
		}
	}
}

function tsDraw(p_id, p_table)
{
	if	(p_table != null)
		tsSetTable (p_table);

	if	((TSort_Store == null)||(TSort_Store.initialized == 0))
		return;

	var i = 0;
	var sort_keys = TSort_Store.sort_keys;
	var id;
	var new_order = '';
	if	(p_id != null)
	{
		if	(typeof p_id == 'number')
			id = p_id;
		else	if	((typeof p_id == 'string')&&(p_id.match(/^\d+[ADU]$/i)))
		{
			id = p_id.replace(/^(\d+)[ADU]$/i, "$1");
			new_order = p_id.replace(/^\d+([ADU])$/i, "$1").toUpperCase();
		}
	}
	if	(id == null)
	{
		id = this.tsort_col_id;
		if	((p_table == null)&&(this.tsort_table_id != null))
			tsSetTable (this.tsort_table_id);
	}
	var table_id = TSort_Data[0];

	var order = TSort_Store.sort_state[id];
	if	(new_order == 'U')
	{
		if	(order != null)
		{
			TSort_Store.sort_state[id] = null;
			obj = document.getElementById ('TS_' + id + '_' + table_id);
			if	(obj != null)	obj.innerHTML = '';
		}
	}
	else if	(new_order != '')
	{
		TSort_Store.sort_state[id] = (new_order == 'A')? true: false;
		//	Add column number to the sort keys array
		sort_keys.unshift(id);
		i = 1;
	}
	else
	{
		if	((order == null)||(order == true))
		{
			TSort_Store.sort_state[id] = (order == null)? true: false;
			//	Add column number to the sort keys array
			sort_keys.unshift(id);
			i = 1;
		}
		else
		{
			TSort_Store.sort_state[id] = null;
			obj = document.getElementById ('TS_' + id + '_' + table_id);
			if	(obj != null)	obj.innerHTML = '';
		}
	}

	var len = sort_keys.length;
	//	This will either remove the column completely from the sort_keys
	//	array (i = 0) or remove duplicate column number if present (i = 1).
	while (i < len)
	{
		if	(sort_keys[i] == id)
		{
			sort_keys.splice(i, 1);
			len--;
			break;
		}
		i++;
	}
	if	(len > 3)
	{
		i = sort_keys.pop();
		obj = document.getElementById ('TS_' + i + '_' + table_id);
		if	(obj != null)	obj.innerHTML = '';
		TSort_Store.sort_state[i] = null;
	}

	// Sort the rows
	TSort_Store.row_clones.sort(tsSort);

	// Save the currently selected order
	var new_tbody = document.createElement('tbody');
	var row_clones = TSort_Store.row_clones;
	len = row_clones.length;
	var classes = TSort_Store.classes;
	if	(classes == null)
	{
		for (i = 0; i < len; i++)
			new_tbody.appendChild (row_clones[i].cloneNode(true));
	}
	else
	{
		var clone;
		var j = 0;
		var cl_len = classes.length;
		for (i = 0; i < len; i++)
		{
			clone = row_clones[i].cloneNode(true);
			clone.className = classes[j++];
			if	(j >= cl_len)  j = 0;
			new_tbody.appendChild (clone);
		}
	}

	// Replace table body
	var table = document.getElementById(table_id);
	var tbody = table.getElementsByTagName('tbody')[0];
	table.removeChild(tbody);
	table.appendChild(new_tbody);

	var obj, color, icon, state;
	len = sort_keys.length;
	var sorting = new Array ();
	for (i = 0; i < len; i++)
	{
		id = sort_keys[i];
		obj = document.getElementById ('TS_' + id + '_' + table_id);
		if	(obj == null)  continue;
		state = (TSort_Store.sort_state[id])? 0: 1;
		icon = TSort_Store.icons[state];
		obj.innerHTML = (icon.match(/</))? icon:
			'<font color="' + TSort_Store.sort_colors[i] + '">' + icon + '</font>';
		sorting.push(id + ((state)? 'D': 'A'));
	}

	if	(TSort_Store.cookie)
	{
		//	Store the contents of "sorting" array into a cookie for 30 days
		var date = new Date();
		date.setTime (date.getTime () + 2592000);
		document.cookie = TSort_Store.cookie + "=" +
			encodeURIComponent (sorting.join(',')) + "; expires=" +
			date.toGMTString () + "; path=/";
	}
}

function tsSort(a, b)
{
	var data_a = TSort_Store.rows[a.tsort_row_id];
	var data_b = TSort_Store.rows[b.tsort_row_id];
	var sort_keys = TSort_Store.sort_keys;
	var len = sort_keys.length;
	var id;
	var type;
	var order;
	var result;
	for (var i = 0; i < len; i++)
	{
		id = sort_keys[i];
		type = TSort_Store.sorting[id];

		var v_a = data_a[id];
		var v_b = data_b[id];
		if	(v_a == v_b)  continue;
		if	((type == 'i')||(type == 'f')||(type == 'd'))
			result = v_a - v_b;
		else
			result = (v_a < v_b)? -1: 1;
		order = TSort_Store.sort_state[id];
		return (order)? result: 0 - result;
	}

	return (a.tsort_row_id < b.tsort_row_id)? -1: 1;
}

function tsRegister()
{
	if	(TSort_All == null)
		TSort_All = new Object();

	var ts_obj = new TSort_StoreDef();
	ts_obj.sort_data = TSort_Data;
	TSort_Data = null;
	if	(typeof TSort_Classes != 'undefined')
	{
		ts_obj.classes = TSort_Classes;
		TSort_Classes = null;
	}
	if	(typeof TSort_Initial != 'undefined')
	{
		ts_obj.initial = TSort_Initial;
		TSort_Initial = null;
	}
	if	(typeof TSort_Cookie != 'undefined')
	{
		ts_obj.cookie = TSort_Cookie;
		TSort_Cookie = null;
	}
	if	(typeof TSort_Icons != 'undefined')
	{
		ts_obj.icons = TSort_Icons;
		TSort_Icons = null;
	}
	if	(ts_obj.icons == null)
		ts_obj.icons = new Array ("\u2193", "\u2191");

	if	(ts_obj.sort_data != null)
		TSort_All[ts_obj.sort_data[0]] = ts_obj;
}

function	tsSetTable (p_id)
{
	TSort_Store = TSort_All[p_id];
	if	(TSort_Store == null)
	{
		alert ("Cannot set table '" + p_id + "' - table is not registered");
		return;
	}
	TSort_Data = TSort_Store.sort_data;
}

if	(window.addEventListener)
	window.addEventListener("load", tsInitOnload, false);
else if (window.attachEvent)
	window.attachEvent ("onload", tsInitOnload);
else
{
	if  ((window.onload_sort_table == null)&&(window.onload != null))
		window.onload_sort_table = window.onload;
	// Assign new onload function
	window.onload = tsInitOnload;
}
