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
	iwinfo 2>&1 | sed '/^$/d;s/"//g' | awk -F'\n' '{print "wifi_status.push(\""$0"\");" }'

	if [ -e /tmp/strength.txt ]; then
		CSQ=$(awk -F[,\ ] '/^\+CSQ:/ {if ($2>31) {C=0} else {C=$2}} END {if (C==0) {printf "-"} else {printf "%d%% (%ddBm, CSQ: %d)\n", C*100/31, C*2-113, C}}' /tmp/strength.txt)
	else
		CSQ="-";
	fi
	echo "var csq='$CSQ';"

	tmodel=$(cat /tmp/sysinfo/model)
	case "$tmodel" in
	"Linksys WRT1900AC" | \
	"Linksys WRT1900ACv2")
		TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon2/temp1_input);
		TEMPMEM=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp1_input);
		TEMPWIFI=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp2_input);
		show_temp=1;;
	"Linksys WRT1200AC")
		TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp1_input);
		TEMPMEM=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp2_input);
		TEMPWIFI=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp1_input);
		show_temp=1;;
	*)
		TEMPCPU="-";
		TEMPMEM="-";
		TEMPWIFI="-";
		show_temp=0;;
	esac
	echo "var tempcpu='$TEMPCPU';"
	echo "var tempmem='$TEMPMEM';"
	echo "var tempwifi='$TEMPWIFI';"
	echo "var show_TEMP='$show_temp';"
%>
//-->
</script>

<fieldset>
	<legend class="sectionheader"><%~ overview.Sts %></legend>

	<div id="device_container">
		<div>
			<span><%~ DNam %>:</span><span id="device_name"></span>
		</div>
		<div>
			<span><%~ GVer %>:</span><span id="gargoyle_version"></span>
		</div>
		<div>
			<span><%~ Modl %>:</span><span id="device_model"></span>
		</div>
		<div>
			<span><%~ DevC %>:</span><span id="device_config"></span>
		</div>
		<div>
			<span><%~ MemU %>:</span><span id="memory"></span>
		</div>
		<div id="swap_container">
			<span><%~ SwMemU %>:</span><span id="swap"></span>
		</div>
		<div>
			<span><%~ Conns %>:</span><span id="connections"></span>
		</div>
		<div>
			<span><%~ CPUAvg %>:</span><span id="load_avg"></span><span>&nbsp;&nbsp;(1/5/15 <%~ minutes %>)</span>
		</div>
		<div id="temp_container">
		<div>
			<span><%~ TEMPcpu %>:</span><span id="temp_cpu"></span><span>&nbsp;&nbsp;&deg;C</span>
		</div>
		<div>
			<span><%~ TEMPmem %>:</span><span id="temp_mem"></span><span>&nbsp;&nbsp;&deg;C</span>
		</div>
		<div>
			<span><%~ TEMPwifi %>:</span><span id="temp_wifi"></span><span>&nbsp;&nbsp;&deg;C</span>
		</div>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="time_container">
		<div>
			<span><%~ Uptm %>:</span><span id="uptime"></span>
		</div>
		<div>
			<span><%~ CDaT %>:</span><span id="current_time"></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="bridge_container">
		<div>
			<span><%~ BrIPA %>:</span><span id="bridge_ip"></span>
		</div>
		<div>
			<span><%~ BrNMsk %>:</span><span id="bridge_mask"></span>
		</div>
		<div>
			<span><%~ BrMAdd %>:</span><span id="bridge_mac"></span>
		</div>
		<div>
			<span><%~ LGtwy %>:</span><span id="bridge_gateway"></span>
		</div>
		<div>
			<span><%~ Cvia %>:</span><span id="bridge_mode"></span>
		</div>
		<div>
			<span><%~ BrID %>:</span><span id="bridge_ssid"></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="wan_container">
		<div>
			<span><%~ WIPA %>:</span><span id="wan_ip"></span>
		</div>
		<div>
			<span><%~ WNmsk %>:</span><span id="wan_mask"></span>
		</div>
		<div>
			<span><%~ WMAdd %>:</span><span id="wan_mac"></span>
		</div>
		<div>
			<span><%~ WGtwy %>:</span><span id="wan_gateway"></span>
		</div>
		<div id="wan_dns_container">
			<span><%~ WDNS %>:</span><span id="wan_dns"></span>
		</div>
		<div id="wan_pppoe_container">
			<span><%~ WUptm %>:</span><span id="wan_pppoe_uptime"></span>
		</div>
		<div id="wan_3g_container">
			<span><%~ W3GSS %>:</span><span id="wan_3g"></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="lan_container">
		<div>
			<span><%~ LIPA %>:</span><span id="lan_ip"></span>
		</div>
		<div>
			<span><%~ LNmsk %>:</span><span id="lan_mask"></span>
		</div>
		<div>
			<span><%~ LMAdd %>:</span><span id="lan_mac"></span>
		</div>
		<div>
			<div id="ports_table_container" class="table-responsive"></div>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="wifi_container">
		<div>
			<span><%~ WlMod %>:</span><span id="wireless_mode"></span>
		</div>
		<div id="wireless_mac_div">
			<span><%~ WlMAdd %>:</span><span id="wireless_mac"></span>
		</div>
		<div id="wireless_apssid_div">
			<span id="wireless_apssid_label"><%~ APID %>:</span><span id="wireless_apssid"></span>
		</div>
		<div id="wireless_apssid_5ghz_div">
			<span id="wireless_apssid_5ghz_label"><%~ F5GID %>:</span><span id="wireless_apssid_5ghz"></span>
		</div>
		<div id="wireless_otherssid_div">
			<span id="wireless_otherssid_label"><%~ IDJoin %>:</span><span id="wireless_otherssid"></span>
		</div>
		<div class="internal_divider"></div>
	</div>

	<div id="services_container">
		<div>
			<span><%~ QUp %>:</span><span id="qos_upload"></span>
		</div>
		<div>
			<span><%~ QDwn %>:</span><span id="qos_download"></span>
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
