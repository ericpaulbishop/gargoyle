#!/usr/bin/haserl
<%
	# This program is copyright © 2008,2009,2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m -c "internal.css" -j gargoyle_header_footer -m -c "internal.css"
%>
	<strong><span id="plot_title" style="margin-left:10px;"></span></strong>
	<embed id="bandwidth_plot" style="margin-left:10px; margin-right:10px; width:800px; height:600px;" src="bandwidth.svg"  type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed>

</body>
</html>
