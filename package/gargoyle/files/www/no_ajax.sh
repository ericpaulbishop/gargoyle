#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	gargoyle_header_footer -h -s "" -p "" -c "internal.css" -j ""
%>


<fieldset>
	<legend class="sectionheader"><%~ AJAX %></legend>

	<p><%~ AJAXUpg %></p>

</fieldset>
<%
	gargoyle_header_footer -f -s "" -p ""
%>
