# Copyright Eric Bishop, 2008-2010
# This is free software licensed under the terms of the GNU GPL v2.0
#
. /lib/functions.sh
. /lib/functions/network.sh
include /lib/network

ra_mask="0x0080"
ra_mask_invert="0xFF7F"
ra_mark="$ra_mask"

death_mask=0x8000
death_mark="$death_mask"

quota_up_mask="0x7f"
quota_dn_mask="0x7f00"

wan_if=""

ip_family()
{
	ip="$1"
	ip4=$(echo "$ip" | grep -E "^\d+\.\d+\.\d+\.\d+$")
	[ -n "$ip4" ] && echo "ipv4"
	ip6=$(echo "$ip" | grep -E "^([0-9a-fA-F]{0,4}:){1,7}[0-9a-fA-F]{0,4}$")
	[ -n "$ip6" ] && echo "ipv6"
}

mask_to_cidr()
{
	mask="$1"
	bits=0;
	mask_parts=$(echo $mask | sed 's/\./ /g')
	for p in $mask_parts ; do
		case $p in
			255)
				bits=$(($bits + 8)) ;;
			254)
				bits=$(($bits + 7)) ;;
			252)
				bits=$(($bits + 6)) ;;
			248)
				bits=$(($bits + 5)) ;;
			240)
				bits=$(($bits + 4)) ;;
			224)
				bits=$(($bits + 3)) ;;
			192)
				bits=$(($bits + 2)) ;;
			128)
				bits=$(($bits + 1)) ;;
		esac
	done
	echo $bits
}

define_wan_if()
{
	no_wan="$(uci -q get network.wan)"
	if [ -z "$no_wan" ] ; then return ; fi
	if  [ -z "$wan_if" ] ;  then
		#Wait for up to 15 seconds for the wan interface to indicate it is up.
		wait_sec=15
		while ! network_is_up wan && [ "$wait_sec" -gt 0 ] ; do
			sleep 1
			wait_sec=$(($wait_sec - 1))
		done

		#The interface name will depend on if pppoe is used or not.  If pppoe is used then
		#the name we are looking for is in network.wan.l3_device.  If there is nothing there
		#use the device named by network.wan.device

		network_get_device wan_if wan
		if [ -z "$wan_if" ] ; then
			network_get_physdev wan_if wan
		fi
	fi
}

# parse remote_accept sections in firewall config and add necessary rules
insert_remote_accept_rules()
{
	local config_name="firewall"
	local section_type="remote_accept"

	ssh_max_attempts=$(uci get dropbear.@dropbear[0].max_remote_attempts 2>/dev/null)
	ssh_port=$(uci get dropbear.@dropbear[0].Port)
	if [ -z "$ssh_max_attempts" ] || [ "$ssh_max_attempts" = "unlimited" ] ; then
		ssh_max_attempts=""
	else
		ssh_max_attempts=$(( $ssh_max_attempts + 1 ))
	fi

	#add rules for remote_accepts
	parse_remote_accept_config()
	{
		vars="local_port remote_port start_port end_port proto zone family"
		proto="tcp udp"
		zone="wan"
		family="ipv4"
		for var in $vars ; do
			config_get $var $1 $var
		done
		if [ "$proto" = "tcpudp" ] || [ -z "$proto" ] ; then
			proto="tcp udp"
		fi
		if [ "$family" = "any" ] ; then
			family="ipv4 ipv6"
		fi
		if [ -z "$family" ] ; then
			family="ipv4"
		fi

		for fam in $family ; do
			for prot in $proto ; do
				if [ -n "$local_port" ] ; then
					if [ -z "$remote_port"  ] ; then
						remote_port="$local_port"
					fi

					#Discourage brute force attacks on ssh from the WAN by limiting failed connection attempts.
					#Each attempt gets a maximum of 10 password tries by dropbear.
					if   [ -n "$ssh_max_attempts"  ] && [ "$local_port" = "$ssh_port" ] && [ "$prot" = "tcp" ] ; then
						# Check and delete the set
						setexists="$(nft -a list ruleset | grep "set ssh_check_${fam}")"
						[ -n "$setexists" ] && nft delete set inet fw4 "ssh_check_${fam}"
						ipstr="ip"
						[ "$fam" == "ipv6" ] && ipstr="ip6"
						nft add set inet fw4 "ssh_check_${fam}" \{ type "${fam}_addr"\; timeout 5m\; \}
						nft add rule inet fw4 "input_${zone}_rule" meta nfproto "$fam" "$prot" dport $ssh_port ct state new add "@ssh_check_${fam}" \{ "$ipstr" saddr \};
						nft add rule inet fw4 "input_${zone}_rule" "$ipstr" saddr "@ssh_check_${fam}" limit rate over $ssh_max_attempts/minute drop;
					fi

					if [ "$remote_port" != "$local_port" ] ; then
						if [ "$fam" = "ipv4" ] ; then
							#since we're inserting with -I, insert redirect rule first which will then be hit second, after setting connmark
							nft insert rule inet fw4 "dstnat_${zone}" meta nfproto "$fam" "$prot" dport $remote_port redirect to $local_port
							nft insert rule inet fw4 "dstnat_${zone}" meta nfproto "$fam" "$prot" dport $remote_port ct mark set ct mark \& "$ra_mask_invert" \| "$ra_mark"
							nft add rule inet fw4 "input_${zone}_rule" meta nfproto "$fam" "$prot" dport $local_port ct mark \& "$ra_mask" == "$ra_mark" accept
						else
							logger -t "gargoyle_firewall_util" "Port Redirect not supported for IPv6"
						fi
					else
						if [ "$fam" = "ipv4" ] ; then
							nft insert rule inet fw4 "dstnat_${zone}" meta nfproto "$fam" "$prot" dport $remote_port redirect to $local_port
						fi
						nft add rule inet fw4 "input_${zone}_rule" meta nfproto "$fam" "$prot" dport $local_port accept
					fi
				elif [ -n "$start_port" ] && [ -n "$end_port" ] ; then
					if [ "$fam" = "ipv4" ] ; then
						nft insert rule inet fw4 "dstnat_${zone}" meta nfproto "$fam" "$prot" dport "$start_port-$end_port" redirect
					fi
					nft add rule inet fw4 "input_${zone}_rule" meta nfproto "$fam" "$prot" dport "$start_port-$end_port" accept
				fi
			done
		done
	}
	config_load "$config_name"
	config_foreach parse_remote_accept_config "$section_type"
}

# creates a chain that sets third byte of connmark to a value that denotes what l7 proto
# is associated with connection. This only sets the connmark, it does not save it to mark
create_l7marker_chain()
{
	# eliminate chain if it exists
	delete_chain_from_table "inet" "fw4" "mangle_l7marker"

	app_proto_num=1
	app_proto_shift=16
	app_proto_mask="0xFF0000"
	app_proto_mask_invert="0x00FFFF"

	all_prots=$(ls /etc/l7-protocols/* | sed 's/^.*\///' | sed 's/\.pat$//' )
	qos_active=$(ls /etc/rc.d/*qos_gargoyle* 2>/dev/null)
	if [ -n "$qos_active" ] ; then
		qos_l7=$(uci show qos_gargoyle | sed '/layer7=/!d; s/^.*=//g')
	fi
	fw_l7=$(uci show firewall | sed '/app_proto/!d; s/^.*=//g')
	all_used="$fw_l7 $qos_l7"

	if [ "$all_used" != " " ] ; then
		nft add chain inet fw4 mangle_l7marker
		nft insert rule inet fw4 mangle_prerouting ct packets <= 20 ct mark \& "$app_proto_mask" == 0x0 jump mangle_l7marker
		nft insert rule inet fw4 mangle_postrouting ct packets <= 20 ct mark \& "$app_proto_mask" == 0x0 jump mangle_l7marker

		for proto in $all_prots ; do
			proto_is_used=$(echo "$all_used" | grep "$proto")
			if [ -n "$proto_is_used" ] ; then
				app_proto_mark=$(printf "0x%X" $(($app_proto_num << $app_proto_shift)) )
				# There is no layer7 module for nftables currently. Below is a fictional rule that matches how the iptables version worked
				#nft add rule inet fw4 mangle_l7marker ct mark \& "$app_proto_mask" == 0x0 layer7 proto $proto ct mark set ct mark \& "$app_proto_mask_invert" \| "$app_proto_mark"
				echo "$proto	$app_proto_mark	$app_proto_mask" >> /tmp/l7marker.marks.tmp
				app_proto_num=$((app_proto_num + 1))
			fi
		done

		copy_file="y"
		if [ -e /etc/md5/layer7.md5 ] ; then
			old_md5=$(cat /etc/md5/layer7.md5)
			current_md5=$(md5sum /tmp/l7marker.marks.tmp | awk ' { print $1 ; } ' )
			if [ "$current_md5" = "$old_md5" ] ; then
				copy_file="n"
			fi
		fi

		if [ "$copy_file" = "y" ] ; then
			mv /tmp/l7marker.marks.tmp /etc/l7marker.marks
			mkdir -p /etc/md5
			md5sum /etc/l7marker.marks | awk ' { print $1 ; }' > /etc/md5/layer7.md5
		else
			rm /tmp/l7marker.marks.tmp
		fi
	fi
}

insert_pf_loopback_rules()
{
	config_name="firewall"
	section_type="redirect"

	#Need to always delete the old chains first.
	delete_chain_from_table "inet" "fw4" "nat_pf_loopback_A"
	delete_chain_from_table "inet" "fw4" "pf_loopback_B"
	delete_chain_from_table "inet" "fw4" "nat_pf_loopback_C"

	define_wan_if
	if [ -z "$wan_if" ]  ; then return ; fi
	network_get_ipaddr wan_ip wan
	network_get_subnet lan_mask lan

	if [ -n "$wan_ip" ] && [ -n "$lan_mask" ] ; then
		nft add chain inet fw4 nat_pf_loopback_A
		nft add chain inet fw4 pf_loopback_B
		nft add chain inet fw4 nat_pf_loopback_C

		nft insert rule inet fw4 dstnat_lan ip daddr $wan_ip jump nat_pf_loopback_A
		nft insert rule inet fw4 forward_lan jump pf_loopback_B
		nft insert rule inet fw4 srcnat_rule oifname br-lan jump nat_pf_loopback_C

		add_pf_loopback()
		{
			local vars="src dest proto src_dport dest_ip dest_port"
			local all_defined="1"
			for var in $vars ; do
				config_get $var $1 $var
				loaded=$(eval echo "\$$var")
				#echo $var =  $loaded
				if [ -z "$loaded" ] && [ ! "$var" = "$src_dport" ] ; then
					all_defined="0"
				fi
			done

			if [ -z "$src_dport" ] ; then
				src_dport=$dest_port
			fi

			sdp_dash=$src_dport
			sdp_colon=$(echo $sdp_dash | sed 's/\-/:/g')
			dp_dash=$dest_port
			dp_colon=$(echo $dp_dash | sed 's/\-/:/g')

			if [ "$all_defined" = "1" ] && [ "$src" = "wan" ] && [ "$dest" = "lan" ]  ; then
				nft add rule inet fw4 nat_pf_loopback_A $proto dport $sdp_dash dnat ip to $dest_ip:$dp_dash
				nft add rule inet fw4 pf_loopback_B $proto dport $dp_dash ip daddr $dest_ip accept
				nft add rule inet fw4 nat_pf_loopback_C $proto dport $dp_dash ip daddr $dest_ip ip saddr $lan_mask masquerade
			fi
		}

		config_load "$config_name"
		config_foreach add_pf_loopback "$section_type"
	fi
}

insert_dmz_rule()
{
	local config_name="firewall"
	local section_type="dmz"

	#add rules for remote_accepts
	parse_dmz_config()
	{
		vars="to_ip from"
		for var in $vars ; do
			config_get $var $1 $var
		done
		if [ -n "$from" ] ; then
			network_get_device from_if "$from" || \
				from_if=$(uci -q get network.$from.device)
		fi
		# echo "from_if = $from_if"
		if [ -n "$to_ip" ] && [ -n "$from"  ] && [ -n "$from_if" ] ; then
			nft add rule inet fw4 "dstnat_${from}" meta nfproto ipv4 iifname $from_if dnat ip to $to_ip
			nft insert rule inet fw4 "forward_${from}" ip daddr $to_ip accept
		fi
	}
	config_load "$config_name"
	config_foreach parse_dmz_config "$section_type"
}

insert_restriction_rules()
{
	define_wan_if
	if [ -z "$wan_if" ]  ; then return ; fi

	if [ -e /tmp/restriction_init.lock ] ; then return ; fi
	touch /tmp/restriction_init.lock

	egress_exists=$(nft list table inet fw4 | grep "chain egress_restrictions" 2>/dev/null)
	ingress_exists=$(nft list table inet fw4 | grep "chain ingress_restrictions" 2>/dev/null)

	if [ -n "$egress_exists" ] ; then
		delete_chain_from_table "inet" "fw4" "egress_whitelist"
		delete_chain_from_table "inet" "fw4" "egress_restrictions"
	fi
	if [ -n "$ingress_exists" ] ; then
		delete_chain_from_table "inet" "fw4" "ingress_whitelist"
		delete_chain_from_table "inet" "fw4" "ingress_restrictions"
	fi

	nft add chain inet fw4 egress_restrictions
	nft add chain inet fw4 ingress_restrictions
	nft add chain inet fw4 egress_whitelist
	nft add chain inet fw4 ingress_whitelist

	nft insert rule inet fw4 forward oifname $wan_if jump egress_restrictions
	nft insert rule inet fw4 forward iifname $wan_if jump ingress_restrictions

	nft insert rule inet fw4 egress_restrictions jump egress_whitelist
	nft insert rule inet fw4 ingress_restrictions jump ingress_whitelist

	package_name="firewall"
	parse_rule_config()
	{
		section=$1
		section_type=$(uci get "$package_name"."$section")

		config_get "enabled" "$section" "enabled"
		if [ -z "$enabled" ] ; then enabled="1" ; fi
		if [ "$enabled" = "1" ] && ( [ "$section_type"  = "restriction_rule" ] || [ "$section_type" = "whitelist_rule" ] ) ; then
			#convert app_proto && not_app_proto to connmark here
			config_get "app_proto" "$section" "app_proto"
			config_get "not_app_proto" "$section" "not_app_proto"

			if [ -n "$app_proto" ] ; then
				app_proto_connmark=$(cat /etc/l7marker.marks 2>/dev/null | grep $app_proto | awk '{ print $2 ; }' )
				app_proto_mask=$(cat /etc/l7marker.marks 2>/dev/null | grep $app_proto | awk '{ print $3 ;  }' )
				uci set "$package_name"."$section".connmark="$app_proto_connmark/$app_proto_mask"
			fi
			if [ -n "$not_app_proto" ] ; then
				not_app_proto_connmark=$(cat /etc/l7marker.marks 2>/dev/null | grep "$not_app_proto" | awk '{ print $2 }')
				not_app_proto_mask=$(cat /etc/l7marker.marks 2>/dev/null | grep "$not_app_proto" | awk '{ print $3 }')
				uci set "$package_name"."$section".not_connmark="$not_app_proto_connmark/$not_app_proto_mask"
			fi

			table="inet fw4"
			chain="egress_restrictions"
			ingress=""
			target="REJECT"

			config_get "is_ingress" "$section" "is_ingress"
			if [ "$is_ingress" = "1" ] ; then
				ingress=" -i "
				if [ "$section_type" = "restriction_rule"  ] ; then
					chain="ingress_restrictions"
				else
					chain="ingress_whitelist"
				fi
			else
				if [ "$section_type" = "restriction_rule"  ] ; then
					chain="egress_restrictions"
				else
					chain="egress_whitelist"
				fi
			fi

			if [ "$section_type" = "whitelist_rule" ] ; then
				target="ACCEPT"
			fi

			make_nftables_rules -p "$package_name" -s "$section" -t "$table" -c "$chain" -g "$target" $ingress
			make_nftables_rules -p "$package_name" -s "$section" -t "$table" -c "$chain" -g "$target" $ingress -r

			uci del "$package_name"."$section".connmark 2>/dev/null
			uci del "$package_name"."$section".not_connmark	 2>/dev/null
		fi
	}

	config_load "$package_name"
	config_foreach parse_rule_config "whitelist_rule"
	config_foreach parse_rule_config "restriction_rule"

	rm -rf /tmp/restriction_init.lock
}

initialize_quotas()
{
	define_wan_if
	if [ -z "$wan_if" ] ; then return ; fi

	if [  -e /tmp/quota_init.lock ] ; then return ; fi
	touch /tmp/quota_init.lock

	network_get_subnet lan_mask lan
	network_get_subnet6 lan_ipmask6 lan
	[ -z "$lan_ipmask6" ] && lan_ipmask6="2001:db8::/32"
	full_qos_enabled=$(ls /etc/rc.d/*qos_gargoyle 2>/dev/null)

	if [ -n "$full_qos_enabled" ] ; then
		full_up=$(uci get qos_gargoyle.upload.total_bandwidth 2>/dev/null)
		full_down=$(uci get qos_gargoyle.download.total_bandwidth 2>/dev/null)
		if [ -z "$full_up" ] && [ -z "$full_down" ] ; then
			full_qos_enabled=""
		fi
	fi


	# restore_quotas does the hard work of building quota chains & rebuilding crontab file to do backups
	#
	# this initializes qos functions ONLY if we have quotas that
	# have up and down speeds defined for when quota is exceeded
	# and full qos is not enabled
	if [ -z "$full_qos_enabled" ] ; then
		restore_quotas    -w $wan_if -d $death_mark -m $death_mask -s "$lan_mask" -t $lan_ipmask6 -c "0 0,4,8,12,16,20 * * * /usr/bin/backup_quotas >/dev/null 2>&1"
		initialize_quota_qos
	else
		restore_quotas -q -w $wan_if -d $death_mark -m $death_mask -s "$lan_mask" -t $lan_ipmask6 -c "0 0,4,8,12,16,20 * * * /usr/bin/backup_quotas >/dev/null 2>&1"
		cleanup_old_quota_qos
	fi

	#enable cron, but only restart cron if it is currently running
	#since we initialize this before cron, this will
	#make sure we don't start cron twice at boot
	/etc/init.d/cron enable
	cron_active=$(ps | grep "crond" | grep -v "grep" )
	if [ -n "$cron_active" ] ; then
		/etc/init.d/cron restart
	fi

	rm -rf /tmp/quota_init.lock
}

load_all_config_sections()
{
	local config_name="$1"
	local section_type="$2"

	all_config_sections=""
	section_order=""
	config_cb()
	{
		if [ -n "$2" ] || [ -n "$1" ] ; then
			if [ -n "$section_type" ] ; then
				if [ "$1" = "$section_type" ] ; then
					all_config_sections="$all_config_sections $2"
				fi
			else
				all_config_sections="$all_config_sections $2"
			fi
		fi
	}

	config_load "$config_name"
	echo "$all_config_sections"
}

cleanup_old_quota_qos()
{
	for iface in $(tc qdisc show | awk '{print $5}' | sort -u ); do
		tc qdisc del dev "$iface" root >/dev/null 2>&1
	done
}

initialize_quota_qos()
{
	cleanup_old_quota_qos

	#speeds should be in kbyte/sec, units should NOT be present in config file (unit processing should be done by front-end)
	quota_sections=$(load_all_config_sections "firewall" "quota")
	upload_speeds=""
	download_speeds=""
	config_load "firewall"
	for q in $quota_sections ; do
		config_get "exceeded_up_speed" $q "exceeded_up_speed"
		config_get "exceeded_down_speed" $q "exceeded_down_speed"
		if [ -n "$exceeded_up_speed" ] && [ -n "$exceeded_down_speed" ] ; then
			if [ $exceeded_up_speed -gt 0 ] && [ $exceeded_down_speed -gt 0 ] ; then
				upload_speeds="$exceeded_up_speed $upload_speeds"
				download_speeds="$exceeded_down_speed $download_speeds"
			fi
		fi
	done

	#echo "upload_speeds = $upload_speeds"

	unique_up=$( printf "%d\n" $upload_speeds 2>/dev/null | sort -u -n)
	unique_down=$( printf "%d\n" $download_speeds 2>/dev/null | sort -u -n)

	#echo "unique_up = $unique_up"

	num_up_bands=1
	num_down_bands=1
	if [ -n "$upload_speeds" ] ; then
		num_up_bands=$((1 + $(printf "%d\n" $upload_speeds 2>/dev/null | sort -u -n |  wc -l) ))
	fi
	if [ -n "$download_speeds" ] ; then
		num_down_bands=$((1 + $(printf "%d\n" $download_speeds 2>/dev/null | sort -u -n |  wc -l) ))
	fi

	#echo "num_up_bands=$num_up_bands"
	#echo "num_down_bands=$num_down_bands"

	if [ -n "$wan_if" ] && [ $num_up_bands -gt 1 ] && [ $num_down_bands -gt 1 ] ; then
		insmod sch_prio  >/dev/null 2>&1
		insmod sch_tbf   >/dev/null 2>&1
		insmod cls_fw    >/dev/null 2>&1
		insmod act_connmark >/dev/null 2>&1

		ifconfig ifb0 down 2>/dev/null
		rmmod ifb 2>/dev/null
		# Allow IFB to fail to load 3 times (15 seconds) before we bail out
		# No particularly graceful way to get out of this one. Quotas will be active but speed limits won't be enforced.
		insmod ifb >&- 2>&-
		ip link add ifb0 type ifb 2>/dev/null
		cnt=0
		while [ "$(ls -d /proc/sys/net/ipv4/conf/ifb* 2>&- | wc -l)" -eq "0" ]
			do
				logger -t "gargoyle_firewall_util" "insmod ifb failed. Waiting and trying again..."
				cnt=`expr $cnt + 1`
				if [ $cnt -ge 3 ] ; then
					logger -t "gargoyle_firewall_util" "Could not insmod ifb, too many retries. Stopping."
					cleanup_old_quota_qos
					return
				fi
				sleep 5
				insmod ifb >&- 2>&-
				ip link add ifb0 type ifb 2>/dev/null
			done
		ip link set ifb0 up

		#egress/upload
		tc qdisc del dev $wan_if root >/dev/null 2>&1
		tc qdisc add dev $wan_if handle 1:0 root prio bands $num_up_bands priomap 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		cur_band=2
		upload_shift=0
		for rate_kb in $unique_up ; do
			kbit=$(echo $((rate_kb*8))kbit)
			mark=$(printf "0x%x\n" $(($cur_band << $upload_shift)))
			tc filter add dev $wan_if parent 1:0 prio $cur_band protocol ip  handle $mark/$quota_up_mask fw flowid 1:$cur_band
			tc filter add dev $wan_if parent 1:0 prio $(($cur_band+1)) protocol ipv6 handle $mark/$quota_up_mask fw flowid 1:$cur_band
			tc qdisc  add dev $wan_if parent 1:$cur_band handle $cur_band: tbf rate $kbit burst $kbit limit $kbit
			cur_band=$(($cur_band+2))
		done

		#ingress/download
		tc qdisc del dev ifb0 root >/dev/null 2>&1
		tc qdisc add dev ifb0 handle 1:0 root prio bands $num_down_bands priomap 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		cur_band=2
		download_shift=8
		for rate_kb in $unique_down ; do
			kbit=$(echo $((rate_kb*8))kbit)
			mark=$(printf "0x%x\n" $(($cur_band << $download_shift)))
			tc filter add dev ifb0 parent 1:0 prio $cur_band protocol ip  handle $mark/$quota_dn_mask fw flowid 1:$cur_band
			tc filter add dev ifb0 parent 1:0 prio $(($cur_band+1)) protocol ipv6 handle $mark/$quota_dn_mask fw flowid 1:$cur_band
			tc qdisc  add dev ifb0 parent 1:$cur_band handle $cur_band: tbf rate $kbit burst $kbit limit $kbit
			cur_band=$(($cur_band+2))
		done

		tc qdisc add dev $wan_if handle ffff: ingress
		tc filter add dev $wan_if parent ffff: protocol ip u32 match u8 0 0 action connmark action mirred egress redirect dev ifb0 flowid ffff:1
		tc filter add dev $wan_if parent ffff: protocol ipv6 u32 match u8 0 0 action connmark action mirred egress redirect dev ifb0 flowid ffff:1

		#tc -s qdisc show dev $wan_if
		#tc -s qdisc show dev ifb0
	fi
}

enforce_dhcp_assignments()
{
	enforce_assignments=$(uci get firewall.@defaults[0].enforce_dhcp_assignments 2> /dev/null)
	delete_chain_from_table "inet" "fw4" "lease_mismatch_check"
	# Check and delete the sets
	setexists="$(nft -a list ruleset | grep "set lease_mismatch_ips")"
	[ -n "$setexists" ] && nft delete set inet fw4 lease_mismatch_ips
	setexists="$(nft -a list ruleset | grep "set lease_mismatch_macs")"
	[ -n "$setexists" ] && nft delete set inet fw4 lease_mismatch_macs
	setexists="$(nft -a list ruleset | grep "set lease_mismatch_pairs")"
	[ -n "$setexists" ] && nft delete set inet fw4 lease_mismatch_pairs

	local pairs1
	local pairs2
	local pairs
	pairs1=""
	pairs2=""
	if [ -e /tmp/dhcp.leases ] ; then
		pairs1=$(cat /tmp/dhcp.leases | sed '/^[ \t]*$/d' | awk ' { print tolower($2)"^"$3"\n" ; } ' )
	fi
	/usr/lib/gargoyle_firewall_util/cache_dhcpv4_leases.sh
	if [ -e /tmp/dhcpv4configleases.gargoyle ] ; then
		pairs2=$(cat /tmp/dhcpv4configleases.gargoyle | sed '/^[ \t]*$/d' | awk ' { print tolower($1)"^"$2"\n" ; } ' )
	fi
	pairs=$( printf "$pairs1\n$pairs2\n" | sort | uniq )

	if [ "$enforce_assignments" = "1" ] && [ -n "$pairs" ] ; then
		nft add chain inet fw4 lease_mismatch_check
		nft add set inet fw4 lease_mismatch_ips \{ type ipv4_addr\; \}
		nft add set inet fw4 lease_mismatch_macs \{ type ether_addr\; \}
		nft add map inet fw4 lease_mismatch_pairs \{ type ipv4_addr . ether_addr : verdict\; \}
		local p
		for p in $pairs ; do
			local mac
			local ip
			mac=$(echo $p | sed 's/\^.*$//g')
			ip=$(echo $p | sed 's/^.*\^//g')
			if [ -n "$ip" ] && [ -n "$mac" ] ; then
				nft add element inet fw4 lease_mismatch_ips \{ "$ip" \}
				nft add element inet fw4 lease_mismatch_macs \{ "$mac" \}
				nft add element inet fw4 lease_mismatch_pairs \{ "$ip" . "$mac" : return \}
			fi
		done
		nft insert rule inet fw4 forward_rule iifname br-lan meta nfproto ipv4 jump lease_mismatch_check
		nft add rule inet fw4 lease_mismatch_check ip saddr != @lease_mismatch_ips ether saddr != @lease_mismatch_macs return
		nft add rule inet fw4 lease_mismatch_check ip saddr . ether saddr vmap @lease_mismatch_pairs
		nft add rule inet fw4 lease_mismatch_check reject
	fi
}

force_router_dns()
{
	force_router_dns=$(uci get firewall.@defaults[0].force_router_dns 2> /dev/null)
	if [ "$force_router_dns" = "1" ] ; then
		nft insert rule inet fw4 dstnat_lan tcp dport 53 redirect
		nft insert rule inet fw4 dstnat_lan udp dport 53 redirect
	fi
}

add_adsl_modem_routes()
{
	wan_proto=$(uci -q get network.wan.proto)
	if [ "$wan_proto" = "pppoe" ] ; then
		wan_dev=$(uci -q get network.wan.device) #not really the interface, but the device
		nft add rule inet fw4 srcnat oifname $wan_dev masquerade
		nft add rule inet fw4 forward_rule oifname $wan_dev accept
		/etc/ppp/ip-up.d/modemaccess.sh firewall $wan_dev
	fi
}

initialize_firewall()
{
	nft insert rule inet fw4 forward_lan iifname br-lan oifname br-lan accept
	insert_remote_accept_rules
	insert_dmz_rule
	create_l7marker_chain
	enforce_dhcp_assignments
	force_router_dns
	add_adsl_modem_routes
	isolate_guest_networks
}

guest_mac_from_uci()
{
	local is_guest_network
	local macaddr
	config_get is_guest_network "$1" is_guest_network
	if [ "$is_guest_network" = "1" ] ; then
		config_get macaddr "$1" macaddr
		echo "$macaddr"
	fi
}
get_guest_macs()
{
	config_load "wireless"
	config_foreach guest_mac_from_uci "wifi-iface"
}
isolate_guest_networks()
{
	# Purge bridge table
	nft delete table bridge gfw 2>/dev/null
	#Establish bridge table
	nft add table bridge gfw
	nft add chain bridge gfw forward \{ type filter hook forward priority 0\; \}
	nft add chain bridge gfw input \{ type filter hook input priority 0\; \}
	local guest_macs=$( get_guest_macs )
	if [ -n "$guest_macs" ] ; then
		local lanifs=`brctl show br-lan 2>/dev/null | awk ' $NF !~ /interfaces/ { print $NF } '`
		local lif

		local lan_ip
		network_get_ipaddr lan_ip lan

		for lif in $lanifs ; do
			for gmac in $guest_macs ; do
				local is_guest=$(ifconfig "$lif"	2>/dev/null | grep -i "$gmac")
				if [ -n "$is_guest" ] ; then
					echo "$lif with mac $gmac is wireless guest"

					#Allow access to WAN and DHCP/DNS servers on LAN, but not other LAN hosts for anyone on guest network
					nft insert rule bridge gfw forward iifname "$lif" ip protocol udp udp dport 53 accept
					nft insert rule bridge gfw forward iifname "$lif" ip protocol udp udp dport 67 accept
					nft add rule bridge gfw forward iifname "$lif" oifname "br-lan" ip daddr 192.168.0.0/16 drop
					nft add rule bridge gfw forward iifname "$lif" oifname "br-lan" ip daddr 172.16.0.0/12 drop
					nft add rule bridge gfw forward iifname "$lif" oifname "br-lan" ip daddr 10.0.0.0/8 drop
					nft add rule bridge gfw forward iifname "$lif" meta protocol ip6 drop

					#Only allow DHCP/DNS access to router for anyone on guest network
					nft add rule bridge gfw input iifname "$lif" ether type arp accept
					nft add rule bridge gfw input iifname "$lif" ip protocol udp udp dport 53 accept
					nft add rule bridge gfw input iifname "$lif" ip protocol udp udp dport 67 accept
					nft add rule bridge gfw input iifname "$lif" ip daddr $lan_ip drop
					nft add rule bridge gfw input iifname "$lif" meta protocol ip6 drop
				fi
			done
		done
	fi
}


ifup_firewall()
{
	insert_restriction_rules
	initialize_quotas
	insert_pf_loopback_rules
}

