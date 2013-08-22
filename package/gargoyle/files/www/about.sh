#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "about" -c "internal.css" -j ""
%>

<fieldset>
	<legend class="sectionheader"><%~ about.CSect %></legend>

	<p><span class="contributer">Eric Bishop (<a href="http://gargoyle-router.com">gargoyle-router.com</a>):</span> <%~ EBishop %></p>

	<p><span class="contributer">Paul Bixel:</span> <%~ PBixel %></p>

	<p><span class="contributer">Artur Wronowski (<a href="http://openwrt.pl">openwrt.pl</a>):</span> <%~ AWronowski %></p>

	<p><span class="contributer">Cezary Jackiewicz (<a href="http://eko.one.pl">eko.one.pl</a>):</span> <%~ CJackiewicz %></p>

	<p><span class="contributer">Piotr Karbowski (<a href="https://github.com/slashbeast">github.com/slashbeast</a>):</span> <%~ PKarbowski %></p>

	<p><span class="contributer">Tony Butler:</span> <%~ TButler %></p>

	<p><span class="contributer">Igor Fedorenko:</span> <%~ IFedorenko %></p>

	<p><span class="contributer">Benjamin Coy (<a href="http://tenorposaune.net">tenorposaune.net</a>):</span> <%~ BCoy %></p>

	<p><span class="contributer">FRiC (<a href="http://ivoidwarranties.blogspot.com">ivoidwarranties.blogspot.com</a>):</span> <%~ FRiC %></p>

	<p><%~ logo %> (<a href="http://manfred-klein.ina-mar.com">manfred-klein.ina-mar.com</a>).</p>

	<p><%~ openwrt %> (<a href="http://openwrt.org">openwrt.org</a>).</p>

</fieldset>

<fieldset>
	<legend class="sectionheader"><%~ LSect %></legend>
	<p>Gargoyle is copyright &copy; 2008, 2009-2013 by Eric Bishop</p>

	<p>Gargoyle is free software; you can redistribute it and/or modify it under the terms of the 
	<a href="http://www.gnu.org/licenses/gpl-2.0.html">GNU General Public License version 2.0</a>
	as published by the Free Software Foundation, with the following clarification/exception that permits 
	adapting the program to configure proprietary &ldquo;back end&rdquo; software provided that all modifications to the 
	web interface portion remain covered by this license:
	</p>

	<div class="code">
		<p>The GNU General Public License (GPL) is vague as to what constitutes &ldquo;mere aggregation&rdquo; under
		section 2, and what contitutes a work &ldquo;based on the Program.&rdquo;  In the special case in which the Program
		is modified for the purpose of configuring other (potentially GPL-incompatible) software, the combination of the
		Program and this other software shall be considered "mere aggregation" if and only if the ONLY interaction
		between the Program and the other software being configured takes place via CGI (Common Gateway Interface) scripts 
		and/or programs.  However, these CGI scripts/programs as well  as any other additions and modifications necessary
		for the configuration of the other software shall be considered &ldquo;based on the Program&rdquo; for the purposes
		of this license.  Further, if any portion of the Program is used as part of an interface that can be rendered via a web browser, 
		all portions of that interface that can be rendered via a web browser (including, but not limited to, javascript, 
		svg/ecmascript, css, html, and shell/perl/php/other cgi scripts) shall be considered &ldquo;based on the Program.&rdquo;</p>

		<p>This clarification/exception shall apply to the license of all derived works, and must appear in all relevant
		documentation.  If you choose to release your modification to the Program under a later version of the GPL that 
		directly contridicts this clarification/exception, this clarification/exception shall supersede any contradictory 
		language in that version.</p>
	</div>

	<p>This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.</p>

</fieldset>

<%
	gargoyle_header_footer -f -s "system" -p "about"
%>
