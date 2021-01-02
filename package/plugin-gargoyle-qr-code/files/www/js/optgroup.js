/*
 * This program is copyright © 2020 Rouven Spreckels and is distributed under the terms of the GNU GPL
 * version 2.0 with a special clarification/exception that permits adapting the program to
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

// ECMAScript 6 (ES6) Strict Mode Code
"use strict";

// Add hidden option group. Adding an option to this group will unhide it.
function addOptGroup(select, label, hidden = true, disabled = false)
{
	let optgroup = document.createElement("optgroup");
	optgroup.label = label;
	optgroup.hidden = hidden;
	optgroup.disabled = disabled;
	select.appendChild(optgroup);
	return optgroup;
}

// Add option and unhide its group if present.
function addOption(selectOrOptgroup, text, value = "", selected = false, hidden = false, disabled = false)
{
	let option = document.createElement("option");
	option.text = text;
	option.value = value;
	option.selected = selected;
	option.hidden = hidden;
	option.disabled = disabled;
	selectOrOptgroup.appendChild(option);
	if(!hidden)
	{
		selectOrOptgroup.hidden = false;
	}
	return option;
}

// Add preselected hidden placeholder option.
function addPlaceholderOption(select, text)
{
	return addOption(select, text, "", true, true);
}

// Get selected option group.
function getOptGroup(select)
{
	let option = getOption(select);
	return option ? option.parentNode : null;
}

// Get selected option.
function getOption(select)
{
	let index = select.selectedIndex;
	return index == -1 ? null : select.options[index];
}

// Get first option by its text.
function getOptionByText(selectOrOptgroup, text)
{
	return Array.from(selectOrOptgroup.getElementsByTagName("option"))
		.find(option => option.text == text);
}

// Whether there are any selectable options.
function hasOptions(selectOrOptgroup)
{
	return Array.from(selectOrOptgroup.getElementsByTagName("option"))
		.some(option => !option.hidden && !option.disabled);
}
