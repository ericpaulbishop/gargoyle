#!/usr/bin/haserl
<%
	# This program is copyright © 2008-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	gargoyle_header_footer -h -s "status" -p "overview" -c "internal.css" -j "overview.js table.js" -z "overview.js" -i network wireless qos_gargoyle system gargoyle
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
	iwconfig 2>&1 | grep -v 'wireless' | sed '/^$/d;s/"//g' | awk -F'\n' '{print "wifi_status.push(\""$0"\");" }'

	if [ -e /tmp/strength.txt ]; then
		CSQ=$(awk -F[,\ ] '/^\+CSQ:/ {if ($2>31) {C=0} else {C=$2}} END {if (C==0) {printf "-"} else {printf "%d%% (%ddBm, CSQ: %d)\n", C*100/31, C*2-113, C}}' /tmp/strength.txt)
	else
		CSQ="-";
	fi
	echo "var csq='$CSQ';"
%>
//-->
</script>

<fieldset>
	<legend class="sectionheader"><%~ overview.Sts %></legend>

	<div id="device_container">
		<div>
			<span class='leftcolumn'><%~ DNam %>:</span><span id="device_name" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ GVer %>:</span><span id="gargoyle_version" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ Modl %>:</span><span id="device_model" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ DevC %>:</span><span id="device_config" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ MemU %>:</span><span id="memory" class='rightcolumn'></span>
		</div>
		<div id="swap_container">
			<span class='leftcolumn'><%~ SwMemU %>:</span><span id="swap" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ Conns %>:</span><span id="connections" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ CPUAvg %>:</span><span id="load_avg" class='rightcolumn'></span><span>&nbsp;&nbsp;(1/5/15 <%~ minutes %>)</span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="time_container">
		<div>
			<span class='leftcolumn'><%~ Uptm %>:</span><span id="uptime" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ CDaT %>:</span><span id="current_time" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="bridge_container">
		<div>
			<span class='leftcolumn'><%~ BrIPA %>:</span><span id="bridge_ip" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ BrNMsk %>:</span><span id="bridge_mask" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ BrMAdd %>:</span><span id="bridge_mac" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ LGtwy %>:</span><span id="bridge_gateway" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ Cvia %>:</span><span id="bridge_mode" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ BrID %>:</span><span id="bridge_ssid" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="wan_container">
		<div>
			<span class='leftcolumn'><%~ WIPA %>:</span><span id="wan_ip" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ WNmsk %>:</span><span id="wan_mask" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ WMAdd %>:</span><span id="wan_mac" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ WGtwy %>:</span><span id="wan_gateway" class='rightcolumn'></span>
		</div>
		<div id="wan_dns_container">
			<span class='leftcolumn'><%~ WDNS %>:</span><span id="wan_dns" class='rightcolumn'></span>
		</div>
		<div id="wan_pppoe_container">
			<span class='leftcolumn'><%~ WUptm %>:</span><span id="wan_pppoe_uptime" class='rightcolumn'></span>
		</div>
		<div id="wan_3g_container">
			<span class='leftcolumn'><%~ W3GSS %>:</span><span id="wan_3g" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="lan_container">
		<div>
			<span class='leftcolumn'><%~ LIPA %>:</span><span id="lan_ip" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ LNmsk %>:</span><span id="lan_mask" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ LMAdd %>:</span><span id="lan_mac" class='rightcolumn'></span>
		</div>
		<div>
			<span class="rightcolumnonly"><div id="ports_table_container"></div></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="wifi_container">
		<div>
			<span class='leftcolumn'><%~ WlMod %>:</span><span id="wireless_mode" class='rightcolumn'></span>
		</div>
		<div id="wireless_mac_div">
			<span class='leftcolumn'><%~ WlMAdd %>:</span><span id="wireless_mac" class='rightcolumn'></span>
		</div>
		<div id="wireless_apssid_div">
			<span class='leftcolumn' id="wireless_apssid_label"><%~ APID %>:</span><span id="wireless_apssid" class='rightcolumn'></span>
		</div>
		<div id="wireless_apssid_5ghz_div">
			<span class='leftcolumn' id="wireless_apssid_5ghz_label"><%~ F5GID %>:</span><span id="wireless_apssid_5ghz" class='rightcolumn'></span>
		</div>
		<div id="wireless_otherssid_div">
			<span class='leftcolumn' id="wireless_otherssid_label"><%~ IDJoin %>:</span><span id="wireless_otherssid" class='rightcolumn'></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="services_container">
		<div>
			<span class='leftcolumn'><%~ QUp %>:</span><span id="qos_upload" class='rightcolumn'></span>
		</div>
		<div>
			<span class='leftcolumn'><%~ QDwn %>:</span><span id="qos_download" class='rightcolumn'></span>
		</div>
	</div>

</fieldset>

<script>
<!--
	resetData();
//-->
</script>

<%
	gargoyle_header_footer -f -s "status" -p "overview"
%>
