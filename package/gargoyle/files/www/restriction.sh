#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	
	gargoyle_header_footer -h -s "firewall" -p "restriction" -c "internal.css" -j "table.js restrictions.js" gargoyle restricter_gargoyle

?>

<style>
	.narrowleftcolumn
	{
		display:block;
		float:left;
		width:125px;
		margin-left:5px;
		margin-right:5px;
	}
	.widerightcolumnonly
	{
		display:block;
		margin-left:135px;
	}
	.middlecolumn
	{
		display:block;
		float:left;
		width:110px;
		margin-right:10px;
		margin-bottom:5px;
	}
</style>

<form>
	<fieldset>
		<legend class="sectionheader">Access Restrictions</legend>
	
		<span id="add_restriction_label"><p>Add New Restriction Rule:</p></span>	
		
		<div class='indent'>
			<div>
				<label class='narrowleftcolumn' for='restriction_name' id='restriction_name_label'>Rule Description:</label>
				<input type='text' class='rightcolumn' id='restriction_name'  size='17' maxlength='17'  />
			</div>
			<div>
				<label class='narrowleftcolumn' for='restriction_name' id='restriction_name_label'>Rule Applies To:</label>
				<select class='rightcolumn' id='rule_applies_to' onchange='setVisibility()'>
					<option value='all'>All Hosts</option>
					<option value='except'>All Hosts Except . . .</option>
					<option value='only'>Only The Following Hosts. . .</option>
				</select>
			</div>
			<div id="rule_applies_to_container" >
				<div class="widerightcolumnonly" id="applies_to_table_container"></div>
				<div class="widerightcolumnonly">
					<input type='text' id='applies_to_ip'  size='17' maxlength='17'  />
					<input type="button" class="default_button" id="add_applies_to_ip" value="Add" onclick="addAppliesToIp()" />
				</div>
			</div>
			<div>
				<label class='narrowleftcolumn' for='restriction_name' id='restriction_name_label'>Schedule:</label>
				<input type='checkbox' id='all_day' /><label for="all_day">All Day</label>
				<input type='checkbox' id='every_day' /><label for="every_day">Every Day</label>&nbsp;&nbsp;
				<select id='schedule_repeats' onchange='setVisibility()'>
					<option value='daily'>Schedule Repeats Daily</option>
					<option value='weekly'>Schedule Repeats Weekly</option>
				</select>&nbsp;&nbsp;
			</div>
			<div id="days_active" class="indent">
				<label class="narrowleftcolumn">Days Active:</label>
				<input type='checkbox' id='sun' /><label for="sun">Sun</label>
				<input type='checkbox' id='mon' /><label for="mon">Mon</label>
				<input type='checkbox' id='tue' /><label for="tue">Tue</label>
				<input type='checkbox' id='wed' /><label for="wed">Wed</label>
				<input type='checkbox' id='thu' /><label for="thu">Thu</label>
				<input type='checkbox' id='fri' /><label for="fri">Fri</label>
				<input type='checkbox' id='sat' /><label for="sat">Sat</label>
			</div>	
			<div id="hours_active" class="indent">
				<label class='narrowleftcolumn' for='hours_active' id='hours_active_label'>Hours Active:</label>
				<input type='text' id='hours_active' size='30'  />
				<br/>
				<div class="widerightcolumnonly">
					<em>e.g. 00:30-13:15, 14:00-15:00</em>
				</div>
			</div>


			<div id="days_and_hours_active" class="indent">
				<label class='narrowleftcolumn' for='days_and_hours_active' id='days_and_hours_active_label'>Days And Hours Active:</label>
				<input type='text' id='days_and_hours_active'  size='30'  />
				<br/>
				<div class="widerightcolumnonly">
					<em>e.g. Mon 00:30 - Thu 13:15, Fri 14:00 - Fri 15:00</em>
				</div>
			</div>

			<div>
				<label class='narrowleftcolumn' for='all_access' id='all_access_label'>Restricted Resources:</label>
				<input type='checkbox' id='all_access' /><label for="all_access">All Network Access</label>
			</div>
			
			<div class="indent" id="restricted_resources">
				<div>
					<span class='narrowleftcolumn'>
						<label id="remote_ip_label" for='remote_ip'>Remote IP(s):</label>
					</span>
					<select class='middlecolumn' id="remote_ip_active">
						<option value="all">Block All</option>
						<option value="only">Block Only</option>
						<option value="except">Block All Except</option>
					</select>
					<span class='rightcolumn'>	
						
						<input type='text' id='remote_ip' onkeyup='proofreadIpRange(this)' size='17' maxlength='31' />	
					</span>
				</div>
				<div>
					<span class='narrowleftcolumn'>
						<label id="remote_port_label" for='remote_port'>Remote Port(s):</label>
					</span>
					<select class='middlecolumn' id="remote_port_active">
						<option value="all">Block All</option>
						<option value="only">Block Only</option>
						<option value="except">Block All Except</option>
					</select>
					<span class='rightcolumn' >
						<input  type='text' id='remote_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />	
					</span>
				</div>
				<div>
					<span class='narrowleftcolumn'>
						<label id="local_port_label" for='local_port'>Local Port(s):</label>
					</span>
					<select class='middlecolumn' id="local_port_active">
						<option value="all">Block All</option>
						<option value="only">Block Only</option>
						<option value="except">Block All Except</option>
					</select>
					<span>	
						<input class='rightcolumn'  type='text' id='local_port' onkeyup='proofreadPortOrPortRange(this)' size='17' maxlength='11' />	
					</span>
				</div>

				

				<div>
					<span class='narrowleftcolumn'>
						<label class='rightcolumn'  id="transport_protocol_label" for='transport_protocol'>Transport Protocol:</label>
					</span>	
					<select class="middlecolumn" id="transport_protocol"/>
						<option value="both">Block All</option>
						<option value="TCP">Block TCP</option>
						<option value="UDP">Block UDP</option>
					</select>
				</div>
				<div>
					<span class='narrowleftcolumn'>
						<label id="app_protocol_label" for='app_protocol'>Application Protocol:</label>
					</span>
					<select id="app_protocol_active" class='middlecolumn'>
						<option value="all">Block All</option>
						<option value="only">Block Only</option>
						<option value="except">Block All Except</option>
					</select>
					
					<select class='rightcolumn' id="app_protocol">
						<option>HTTP</option>
						<option>FTP</option>
						<option>SSL</option>
						<option>POP3</option>
						<option>SMTP</option>
						<option>Ident</option>
						<option>NTP</option>
						<option>VNC</option>
						<option>IRC</option>
						<option>Jabber</option>
						<option>MSN Messenger</option>
						<option>AIM</option>
						<option>FastTrack</option>
						<option>BitTorrent</option>
						<option>Gnutella</option>
						<option>eDonkey</option>
						<option>Any P2P</option>
					</select>
				</div>
				<div>
					<span class='narrowleftcolumn'>
						<label id="weburl_label" for='weburl'>Website URL(s):</label>
					</span>
					<select id="weburl_active" class='rightcolumn'>
						<option value="all">Block All</option>
						<option value="only">Block Only (Blacklist)</option>
						<option value="except">Block All Except (Whitelist)</option>
					</select>	
				</div>
				<div id="url_match_list" class="indent">
					<div id="url_match_table_container"></div>
					<div>
						<select>
							<option value="exact">URL matches exactly:</option>
							<option value="contains">URL contains:</option>
							<option value="regex">URL matches Regex:</option>
						</select>
						<input type='text' id='url_match'  size='30'/>
						<input type="button" class="default_button" id="add_url_match" value="Add" onclick="addUrlMatch()" />
					</div>
				</div>
			</div>

			<div>
				<input type="button" id="add_restriction_button" class="default_button" value="Add New Rule" onclick="addNewRestriction()" />
			</div>	
		</div>
	
	
		<div id='internal_divider1' class='internal_divider'></div>
	


		<span id="current_restrictions_label"><p>Current Restrictions:</p></span>	

		<div id="restrictions_table_container"></div>
		
	</fieldset>
	<div id="bottom_button_container">
		<input type='button' value='Save Changes' id="save_button" class="bottom_button" onclick='saveChanges()' />
		<input type='button' value='Reset' id="reset_button" class="bottom_button" onclick='resetData()'/>
	</div>


	<span id="update_container" >Please wait while new settings are applied. . .</span>
</form>

<!-- <br /><textarea style="margin-left:20px;" rows=30 cols=60 id='output'></textarea> -->

<script>
<!--
	resetData();
//-->
</script>


<?
	gargoyle_header_footer -f -s "firewall" -p "restriction"
?>
