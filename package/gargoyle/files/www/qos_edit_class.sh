#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -m -c "internal.css" -j "qos.js" -z "qos.js"
%>
<fieldset id="edit_container">
	<legend class="sectionheader"><%~ qos.QESrvClass %></legend>

	<div>
		<label class='leftcolumn' id="class_name_label" for='class_name' ><%~ QESrvName %>:</label>
		<input class='rightcolumn' type='text' id='class_name' onkeyup="proofreadLengthRange(this,1,10)"  size='12' maxlength='10' />
	</div>

	<div>
		<label class='leftcolumn' id="percent_bandwidth_label" for='percent_bandwidth' ><%~ PerBandCap %>:</label>
		<span class='rightcolumn'>
			<input type='text' id='percent_bandwidth' onkeyup="proofreadNumericRange(this,1,100)" size='5' maxlength='3' /><em>%</em>
		</span>
	</div>

	<div class='nocolumn'><%~ BandMin %>:</div>
	<div class='indent'>
	<div class='nocolumn'>
			<input type='radio' name="min_radio" id='min_radio1' onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
			<label for='min_radio1'><%~ BandMinNo %></label>
		</div>
		<div>
			<span class='leftcolumn'>
				<input type='radio' name="min_radio" id="min_radio2" onclick='enableAssociatedField(document.getElementById("min_radio2"),"min_bandwidth", "")' />
				<label id="min_bandwidth_label" for='min_radio2'><%~ BandMin %>:</label>
			</span>
			<span class='rightcolumn'>
				<input type='text' class="rightcolumn" id='min_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
				<em><%~ Kbs %></em>
			</span>
		</div>
	</div>

	<br>
	<div class='nocolumn'><%~ BandMax %>:</div>
	<div class='indent'>
		<div class='nocolumn'>
			<input type='radio' name="max_radio" id='max_radio1' onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
			<label for='max_radio1'><%~ BandMaxNo %></label>
		</div>
		<div>
			<span class='leftcolumn'>
				<input type='radio' name="max_radio" id="max_radio2" onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
				<label id="max_bandwidth_label" for='max_radio2'><%~ BandMax %>:</label>
			</span>
			<span class='rightcolumn'>
				<input type='text' class="rightcolumn" id='max_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' />
				<em><%~ Kbs %></em>
			</span>
		</div>
	</div>

	<div id='rttdiv'>
	<br>
	<div class='nocolumn'><%~ MinRTT %>:</div>
	<div class='indent'>
		<div class='nocolumn'>
			<input type='radio' name="rtt_radio" id='rtt_radio1'/>
			<label for='rtt_radio1'><%~ ActRTT %></label>
		</div>
		<div>
			<span class='leftcolumn'>
				<input type='radio' name="rtt_radio" id="rtt_radio2" />
				<label for='rtt_radio2'><%~ OptiWAN %></label>
			</span>
		</div>
	</div></div>

</fieldset>
<div id="bottom_button_container"></div>
<%
	echo '</body>'
	echo '</html>'
%>
