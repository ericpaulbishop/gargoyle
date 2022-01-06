#!/usr/bin/haserl --upload-limit=8192 --upload-dir=/tmp/
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "system" -p "update" -j "update.js" -z "update.js" network system
%>

<script>
<!--
<%
	awk -F= '/DISTRIB_TARGET/{printf "var distribTarget=%s;\n", $2}' /etc/openwrt_release
	gargoyle_version=$(cat data/gargoyle_version.txt)
	echo "var gargoyleVersion=\"$gargoyle_version\"";

	# Check if existing upgrade file exists and purge it if older than 1 hour
	upgrade_date=$(date -r /tmp/up/upgrade "+%s" 2>/dev/null)
	current_date=$(date "+%s")
	if [ $((current_date - upgrade_date)) -gt 3600 ] ; then
		rm -rf /tmp/up
	fi

	# Flag if upgrade file already exists
	[ -e "/tmp/up/upgrade" ] && echo "var upgradePresent=true;" || echo "var upgradePresent=false;"
	[ -e "/tmp/up/upgrade" ] && echo "var firmware_size=$(wc -c /tmp/up/upgrade | cut -f1 -d' ');" || echo "var firmware_size=0;"
	[ -e "/tmp/up/fwtool.json" ] && echo "var fwtoolStr='$(cat /tmp/up/fwtool.json)';" || echo "var fwtoolStr='{}';"
	[ -e "/tmp/up/validate_firmware_image.json" ] && echo "var validateFwStr='$(cat /tmp/up/validate_firmware_image.json)';" || echo "var validateFwStr='{}';"
	[ -e "/tmp/up/hash.md5" ] && echo "var md5hash='$(cat /tmp/up/hash.md5)';" || echo "var md5hash='';"
	[ -e "/tmp/up/hash.sha1" ] && echo "var sha1hash='$(cat /tmp/up/hash.sha1)';" || echo "var sha1hash='';"
	[ -e "/tmp/up/hash.sha256" ] && echo "var sha256hash='$(cat /tmp/up/hash.sha256)';" || echo "var sha256hash='';"

	echo "var procmtdLines = [];"
	echo "var procpartLines = [];"
	cat /proc/mtd | sed 's/"//g' | awk '{print "procmtdLines.push(\""$0"\");"}'
	cat /proc/partitions | sed 's/"//g' | awk '{print "procpartLines.push(\""$0"\");"}'
%>
//-->
</script>

<h1 class="page-header"><%~ update.UpFrm %></h1>
<div id="upgrade_section" class="row">
	<div class="col-lg-12">
		<div class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ UpFrm %></h3>
			</div>
			<div class="panel-body">
				<div class="row form-group">
					<div class="col-lg-12">
						<div class="alert alert-info" role="alert">
							<span><%~ CGV %>:</span>
							<span id="gargoyle_version"></span>
						</div>
					</div>

				</div>

				<div class="row form-group">
					<div class="col-lg-4">
						<form id="upgrade_form" enctype="multipart/form-data" method="post" action="utility/do_upgrade.sh" target="do_upgrade">
							<div id="upgrade_file1_container" class="row form-group">
								<label class="col-xs-5" id="upgrade_label" for="upgrade_file"><%~ SelF %>:</label>
								<span class="col-xs-7">
									<input type="file" id="upgrade_file" name="upgrade_file"/>
									<br/>
									<em><span id="upgrade_text"></span></em>
									<br/>
									<br/>
								</span>
							</div>


							<div class="row form-group">
								<label class="col-xs-5" id="firmware_hash_label" for="firmware_hash"><%~ EHash %> (MD5/SHA-1/SHA-256):</label>
								<span class="col-xs-7">
									<input class="form-control" type="text" id="firmware_hash" name="firmware_hash"/>
									<em>(<%~ optional %>)</em>
								</span>
							</div>
							<input id="upgrade_hash" name="hash" type="hidden" value="" />
							<span class="col-xs-12"><em><%~ ConfirmNext %></em></span>

							<div class="row form-group">
								<span class="col-xs-12"><button id="upgrade_button" class="btn btn-primary btn-lg" onclick="doUpload()"><%~ UploadFrm %></button></span>
							</div>
						</form>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<iframe id="do_upgrade" name="do_upgrade" src="#" style="display:none"></iframe>
<iframe id="reboot_test" onload="reloadPage()" style="display:none"></iframe>

<div class="modal fade" tabindex="-1" role="dialog" id="upgrade_confirm_modal" aria-hidden="true" aria-labelledby="upgrade_confirm_modal_title">
	<div class="modal-dialog modal-lg" role="document">
		<div class="modal-content">
			<div class="modal-header">
				<h3 id="upgrade_confirm_modal_title" class="panel-title"><%~ UpConfirm %></h3>
			</div>
			<div class="modal-body">
				<%in templates/upgrade_confirm_template %>
			</div>
			<div class="modal-footer" id="upgrade_confirm_modal_button_container">
			</div>
		</div>
	</div>
</div>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "system" -p "update"
%>
