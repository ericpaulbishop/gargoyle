#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "overview" -j "overview.js table.js" -z "overview.js" -i network wireless qos_gargoyle system gargoyle
%>

<script>
<!--
<%
	echo "var uptime=\"$(cat /proc/uptime)\";"

	if [ -h /etc/rc.d/S50qos_gargoyle ] ; then
		echo "var qosEnabled = true;"
	else
		echo "var qosEnabled = false;"
	fi

	gargoyle_version=$(cat data/gargoyle_version.txt)
	echo "var gargoyleVersion=\"$gargoyle_version\";"

	. /usr/lib/gargoyle/current_time.sh

	total_mem="$(sed -e '/^MemTotal: /!d; s#MemTotal: *##; s# kB##g' /proc/meminfo)"
	buffers_mem="$(sed -e '/^Buffers: /!d; s#Buffers: *##; s# kB##g' /proc/meminfo)"
	cached_mem="$(sed -e '/^Cached: /!d; s#Cached: *##; s# kB##g' /proc/meminfo)"
	free_mem="$(sed -e '/^MemFree: /!d; s#MemFree: *##; s# kB##g' /proc/meminfo)"
	free_mem="$(( ${free_mem} + ${buffers_mem} + ${cached_mem} ))"
	echo "var totalMemory=parseInt($total_mem);"
	echo "var freeMemory=parseInt($free_mem);"

	total_swap="$(sed -e '/^SwapTotal: /!d; s#SwapTotal: *##; s# kB##g' /proc/meminfo)"
	cached_swap="$(sed -e '/^SwapCached: /!d; s#SwapCached: *##; s# kB##g' /proc/meminfo)"
	free_swap="$(sed -e '/^SwapFree: /!d; s#SwapFree: *##; s# kB##g' /proc/meminfo)"
	free_swap="$(( ${free_swap} + ${cached_swap} ))"
	echo "var totalSwap=parseInt($total_swap);"
	echo "var freeSwap=parseInt($free_swap);"

	load_avg="$(awk '{print $1 " / " $2 " / " $3}' /proc/loadavg)"
	echo "var loadAvg=\"$load_avg\";"

	curconn="$(wc -l < /proc/net/nf_conntrack)"
	maxconn=$(cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null)
	if [ -z "$maxconn" ] ; then
		maxconn="4096"
	fi
	echo "var curConn=\"$curconn\";"
	echo "var maxConn=\"$maxconn\";"

	if [ -e /tmp/sysinfo/model ]; then
		echo "var model=\""$(cat /tmp/sysinfo/model)"\";"
	else
		echo "var model=\"Unknown\";"
	fi

	lan_ip=$(/sbin/uci get network.lan.ipaddr 2>/dev/null)
	if [ -n "$lan_ip" ] ; then
		echo "var wanDns=\""$(sed -e '/nameserver/!d; s#nameserver ##g' /tmp/resolv.conf.auto | sort | uniq | grep -v "$lan_ip" )"\";"
	else
		echo "var wanDns=\""$(sed -e '/nameserver/!d; s#nameserver ##g' /tmp/resolv.conf.auto | sort | uniq )"\";"
	fi

	echo "var ports = new Array();"
	/usr/lib/gargoyle/switchinfo.sh

	if [ -e /var/run/pppoe-wan.pid ]; then
		deltatime=$((`date -r /var/run/crond.pid +%s`-`date -r /var/run/pppoe-wan.pid +%s`))
		[ $deltatime -lt 60 ] && deltatime=0 || let deltatime-=30
		echo "var pppoeUptime=\""$((`date +%s`-`date -r /var/run/pppoe-wan.pid +%s`-$deltatime))"\";"
	fi

	echo "var wifi_status = new Array();"
	iwinfo 2>&1 | sed '/^$/d;s/"//g' | awk -F'\n' '{print "wifi_status.push(\""$0"\");" }'

	if [ -e /tmp/strength.txt ]; then
		CSQ=$(awk -F[,\ ] '/^\+CSQ:/ {if ($2>31) {C=0} else {C=$2}} END {if (C==0) {printf "-"} else {printf "%d%% (%ddBm, CSQ: %d)\n", C*100/31, C*2-113, C}}' /tmp/strength.txt)
	else
		CSQ="-";
	fi
	echo "var csq='$CSQ';"

	echo "var temps = new Array();"
	/usr/lib/gargoyle/tempinfo.sh
%>
//-->
</script>

<h1 class="page-header"><%~ overview.Sts %></h1>
<div class="row">
	<div class="col-lg-6">
		<div id="device_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mDevice %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item"><span class="list-group-item-title"><%~ DNam %>:</span><span id="device_name"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ GVer %>:</span><span id="gargoyle_version"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ Modl %>:</span><span id="device_model"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ DevC %>:</span><span id="device_config"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ MemU %>:</span><span id="memory"></span></li>
					<li id="swap_container" class="list-group-item"><span class="list-group-item-title"><%~ SwMemU %>:</span><span id="swap"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ Conns %>:</span><span id="connections"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ CPUAvg %>:</span><span><span id="load_avg"></span><span>&nbsp;(1/5/15 <%~ minutes %>)</span></span></li>
					<li id="temp_container" class="list-group-item">
						<div><%~ TEMPcpu %>:<span><span id="temp_cpu"></span><span>&nbsp;&deg;C</span></span></div>
						<div><%~ TEMPmem %>:<span><span id="temp_mem"></span><span>&nbsp;&deg;C</span></span></div>
						<div><%~ TEMPwifi %>:<span><span id="temp_wifi"></span><span>&nbsp;&deg;C</span></span></div>
					</li>
				</ul>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div id="wan_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mWAN %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item"><span class="list-group-item-title"><%~ WIPA %>:</span><span id="wan_ip"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ WNmsk %>:</span><span id="wan_mask"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ WMAdd %>:</span><span id="wan_mac"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ WGtwy %>:</span><span id="wan_gateway"></span></li>
					<li id="wan_dns_container" class="list-group-item"><span class="list-group-item-title"><%~ WDNS %>:</span><span id="wan_dns"></span></li>
					<li id="wan_pppoe_container" class="list-group-item"><span class="list-group-item-title"><%~ WUptm %>:</span><span id="wan_pppoe_uptime"></span></li>
					<li id="wan_3g_container" class="list-group-item"><span class="list-group-item-title"><%~ W3GSS %>:</span><span id="wan_3g"></span></li>
				</ul>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div id="lan_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mLAN %></h3>
			</div>

			<div class="panel-body">
				<ul id="lan_list_group" class="list-group">
					<li class="list-group-item"><span class="list-group-item-title"><%~ LIPA %>:</span><span id="lan_ip"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ LNmsk %>:</span><span id="lan_mask"></span></li>
					<li class="list-group-item"><span class="list-group-item-title"><%~ LMAdd %>:</span><span id="lan_mac"></span></li>
				</ul>
				<div id="ports_table_container" class="table-responsive"></div>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div id="wifi_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mWireless %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ WlMod %>:</span>
						<span id="wireless_mode"></span>
					</li>

					<li id="wireless_mac_div" class="list-group-item">
						<span class="list-group-item-title"><%~ WlMAdd %>:</span>
						<span id="wireless_mac"></span>
					</li>

					<li id="wireless_apssid_div" class="list-group-item">
						<span id="wireless_apssid_label" class="list-group-item-title"><%~ APID %>:</span>
						<span id="wireless_apssid"></span>
					</li>

					<li id="wireless_apssid_5ghz_div" class="list-group-item">
						<span id="wireless_apssid_5ghz_label" class="list-group-item-title"><%~ F5GID %>:</span>
						<span id="wireless_apssid_5ghz"></span>
					</li>

					<li id="wireless_otherssid_div" class="list-group-item">
						<span class="list-group-item-title"><%~ IDJoin %>:</span>
						<span id="wireless_otherssid"></span>
					</li>
				</ul>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div id="bridge_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mBridge %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ BrRIP %>:</span>
						<span id="bridge_relay_ip"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ BrIPA %>:</span>
						<span id="bridge_ip"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ LGtwy %>:</span>
						<span id="bridge_gateway"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ LNmsk %>:</span>
						<span id="bridge_mask"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ BrMAdd %></span>
						<span id="bridge_mac"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ Cvia %>:</span>
						<span id="bridge_mode"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ BrID %>:</span>
						<span id="bridge_ssid"></span>
					</li>
				</ul>
			</div>
		</div>
	</div>
</div>

<div class="row">
	<div class="col-lg-6">
		<div id="time_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mTime %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ Uptm %>:</span>
						<span id="uptime"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ CDaT %>:</span>
						<span id="current_time"></span>
					</li>
				</ul>
			</div>
		</div>
	</div>

	<div class="col-lg-6">
		<div id="services_container" class="panel panel-default">
			<div class="panel-heading">
				<h3 class="panel-title"><%~ overview.mServices %></h3>
			</div>

			<div class="panel-body">
				<ul class="list-group">
					<li class="list-group-item">
						<span class="list-group-item-title"><%~ QUp %>:</span>
						<span id="qos_upload"></span>
					</li>

					<li class="list-group-item">
						<span class="list-group-item-title"><%~ QDwn %>:</span>
						<span id="qos_download"></span>
					</li>
				</ul>
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
	gargoyle_header_footer -f -s "status" -p "overview"
%>
