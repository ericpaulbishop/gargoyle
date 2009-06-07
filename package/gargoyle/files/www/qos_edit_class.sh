#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	
	gargoyle_header_footer -m -c "internal.css" -j "qos.js"
?>
<fieldset id="edit_container">
	<legend class="sectionheader">Edit QoS Service Class</legend>

	<div>	
		<label class='leftcolumn' id="class_name_label" for='class_name' >Service Class Name:</label>
		<input class='rightcolumn' type='text' id='class_name' onkeyup="proofreadLengthRange(this,1,10)"  size='12' maxlength='10' />
	</div>
	
	<div>
		<label class='leftcolumn' id="percent_bandwidth_label" for='percent_bandwidth' >Percent Bandwidth At Capacity:</label>
		<span class='rightcolumn'>
			<input type='text' id='percent_bandwidth' onkeyup="proofreadNumericRange(this,1,100)" size='5' maxlength='3' /><em>%</em>
		</span>
	</div>
	
	
	<div>
		<label class= 'leftcolumn' id="minimize_delay_label" for='minimize_delay'>Minimize Delay</label>
		<select id='minimize_delay' class='rightcolumn'><option value="yes" >yes</option><option value="no" >no</option></select>
	</div>

	<div class='nocolumn'>Bandwidth Maximum:</div>
	<div class='indent'>
		<div class='leftcolumn'>
			<input type='radio' name="max_radio" id='max_radio1' onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
			<label for='max_radio1'>No Bandwidth Maximum</label>
		</div>	
		<div>
			<div class='leftcolumn'>
				<input type='radio' name="max_radio" id="max_radio2" onclick='enableAssociatedField(document.getElementById("max_radio2"),"max_bandwidth", "")' />
				<label id="max_bandwidth_label" for='max_radio2'>Bandwidth Maximum:</label>
			</div>
			<input class="rightcolumn" type='text' id='max_bandwidth' onkeyup="proofreadNumeric(this)"  size='10' maxlength='10' /> 
			<em>kbit/s</em>
		</div>	
	</div>



</fieldset>
<div id="bottom_button_container"></div>


<?
	echo '</body>'
	echo '</html>'
?>
