# Copyright Eric Bishop, 2008
# This is free software licensed under the terms of the GNU GPL v2.0
#
. /etc/functions.sh
include /lib/network

ra_mask="0x0080"
ra_mark="0x0080/$ra_mask"


delete_chain_from_table()
{
	table=$1
	target=$2


	found_chain="0"

	chains=$(iptables -t $table -L | awk ' {if($0 ~ /^Chain/){ print $2; };} '  )
	for chain in $chains ; do
		rule_nums=$(iptables -t $table -L $chain --line-numbers | awk " {if(\$1~/^[0-9]+$/ && \$2 ~/^$target/){ printf(\"%s\n\", \$1);};}")
		
		#delete higher number rule nums first so rule numbers remain valid
		sorted_rules=$(echo -e "$rule_nums" | sort -n -r )
		if [ -n "$sorted_rules" ] ; then
			for rule_num in $sorted_rules ; do
				iptables -t $table -D $chain $rule_num
			done
		fi
		
		if [ "$chain" = "$target" ] ; then
			found_chain="1"
		fi
	done
	if [ "$found_chain" = "1" ] ; then
		iptables -t $table -F $target
		iptables -t $table -X $target
	fi
	
}


# echo "1" if death_mark chain exists, otherwise echo "0"
death_mark_exists()
{
	echo 0
}



# parse remote_accept sections in firewall config and add necessary rules
insert_remote_accept_rules()
{
	local config_name="firewall"
	local section_type="remote_accept"

	#add rules for remote_accepts
	parse_remote_accept_config()
	{
		vars="local_port remote_port proto zone"
		proto="tcp"
		zone="wan"
		for var in $vars ; do
			config_get $var $1 $var
		done
		if [ -n "$local_port" ] ; then
			if [ -z "$remote_port"  ] ; then
				remote_port="$local_port"
			fi
			if [ "$remote_port" != "$local_port" ] ; then
				#since we're inserting with -I, insert redirect rule first which will then be hit second, after setting connmark
				iptables -t nat -I "zone_"$zone"_prerouting" -p "$proto" --dport "$remote_port" -j REDIRECT --to-ports "$local_port"
				iptables -t nat -I "zone_"$zone"_prerouting" -p "$proto" --dport "$remote_port" -j CONNMARK --set-mark "$ra_mark"
				iptables -t filter -A "input_$zone" -p $proto --dport "$local_port" -m connmark --mark "$ra_mark" -j ACCEPT
			else
				iptables -t nat -I "zone_"$zone"_prerouting" -p "$proto" --dport "$remote_port" -j REDIRECT --to-ports "$local_port"
				iptables -t filter -A "input_$zone" -p $proto --dport "$local_port" -j ACCEPT
			fi
			echo iptables -t filter -A "input_$zone" -p $proto --dport "$local_port" -m connmark --mark "$ra_mark" -j ACCEPT
		fi
	}
	config_load "$config_name"
	config_foreach parse_remote_accept_config "$section_type"

}




# creates a chain that sets third byte of connmark to a value that denotes what l7 proto 
# is associated with connection. This only sets the connmark, it does not save it to mark
create_l7marker_chain()
{
	# eliminate chain if it exists
	delete_chain_from_table "mangle" "l7marker"

	app_proto_num=1
	app_proto_shift=16
	app_proto_mask="0xFF0000"

	iptables -t mangle -N l7marker
	iptables -t mangle -I PREROUTING  -m connbytes --connbytes 0:75 --connbytes-dir both --connbytes-mode packets -m connmark --mark 0x0/$app_proto_mask -j l7marker
	iptables -t mangle -I POSTROUTING -m connbytes --connbytes 0:75 --connbytes-dir both --connbytes-mode packets -m connmark --mark 0x0/$app_proto_mask -j l7marker


	prots=$(ls /etc/l7-protocols/* | sed 's/^.*\///' | sed 's/\.pat$//' )
	for proto in $prots ; do
		app_proto_mark=$(printf "0x%X" $(($app_proto_num << $app_proto_shift)) )
		iptables -t mangle -A l7marker -m connmark --mark 0x0/$app_proto_mask -m layer7 --l7proto $proto -j CONNMARK --set-mark $app_proto_mark/$app_proto_mask
		echo "$proto	$app_proto_mark	$app_proto_mask" >> /tmp/l7marker.marks.tmp
		app_proto_num=$((app_proto_num + 1))
	done

	ipp2p_mark=$(printf "0x%X" $(($app_proto_num << $app_proto_shift)) )
	iptables -t mangle -A l7marker -m connmark --mark 0x0/$app_proto_mask -m ipp2p --ipp2p -j CONNMARK --set-mark $ipp2p_mark/$app_proto_mask
	echo "ipp2p	$ipp2p_mark	$app_proto_mask" >> /tmp/l7marker.marks.tmp
	
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

	
}

insert_pf_loopback_rules()
{
	config_name="firewall"
	section_type="redirect"

	wan_ip=$(uci -p /tmp/state get network.wan.ipaddr)
	lan_mask=$(uci -p /tmp/state get network.lan.netmask)

	if [ -n "$wan_ip" ] && [ -n "$lan_mask" ] ; then
		delete_chain_from_table "nat"    "pf_loopback_A"
		delete_chain_from_table "filter" "pf_loopback_B"
		delete_chain_from_table "nat"    "pf_loopback_C"


		iptables -t nat    -N "pf_loopback_A"
		iptables -t filter -N "pf_loopback_B"
		iptables -t nat    -N "pf_loopback_C"

		iptables -t nat    -I zone_lan_prerouting -d $wan_ip -j pf_loopback_A
		iptables -t filter -I zone_lan_forward               -j pf_loopback_B
		iptables -t nat    -I postrouting_rule -o br-lan     -j pf_loopback_C


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
				iptables -t nat    -A pf_loopback_A -p $proto --dport $sdp_colon -j DNAT --to-destination $dest_ip:$dp_dash
				iptables -t filter -A pf_loopback_B -p $proto --dport $dp_colon -d $dest_ip -j ACCEPT
				iptables -t nat    -A pf_loopback_C -p $proto --dport $dp_colon -d $dest_ip -s $dest_ip/$lan_mask -j MASQUERADE
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
			from_if=$(uci -p /tmp/state get network.$from.ifname)
		fi
		echo "from_if = $from_if"
		if [ -n "$to_ip" ] && [ -n "$from"  ] && [ -n "$from_if" ] ; then
			iptables -t nat -A PREROUTING -i $from_if -j DNAT --to-destination $to_ip
			echo "iptables -t nat -A PREROUTING -i $from_if -j DNAT --to-destination $to_ip"
			iptables -t filter -I "zone_"$from"_forward" -d $to_ip -j ACCEPT
		fi
	}
	config_load "$config_name"
	config_foreach parse_dmz_config "$section_type"
}

insert_restriction_rules()
{
	wan_if=$(uci -P "/var/state" get network.wan.ifname)                                                   
	if [ -z "$wan_if" ]  ; then return ; fi                                                                       
	
	egress_exits=$(iptables -t filter -L egress_restrictions 2>/dev/null)
	ingress_exits=$(iptables -t filter -L ingress_restrictions 2>/dev/null)
	if [ -n "$egress_exists" ] ; then
		delete_chain_from_table filter egress_restrictions
	fi
	if [ -n "$ingress_exists" ] ; then
		delete_chain_from_table filter ingress_restrictions
	fi
	
	iptables -t filter -N egress_restrictions	
	iptables -t filter -N ingress_restrictions	

	iptables -t filter -I FORWARD -o $wan_if -j egress_restrictions	
	iptables -t filter -I OUTPUT -o $wan_if -j egress_restrictions	
	iptables -t filter -I FORWARD -i $wan_if -j ingress_restrictions	
	iptables -t filter -I INPUT -i $wan_if -j ingress_restrictions	
	
	
	package_name="firewall"	
	parse_rule_config()
	{
		section=$1
		section_type=$(uci get "$package_name"."$section")
		
		config_get "enabled" "$section" "enabled"
		if [ -z "$enabled" ] ; then enabled="1" ; fi
		if [ "$enabled" = "1" ] ; then
			#convert app_proto && not_app_proto to connmark here
			config_get "app_proto" "$section" "app_proto"
			config_get "not_app_proto" "$section" "not_app_proto"
			if [ -n "$app_proto" ] ; then
				app_proto_connmark=$(cat /etc/l7marker.marks 2>/dev/null | grep "$app_proto" | awk '{ print $2 }')
				app_proto_mask=$(cat /etc/l7marker.marks 2>/dev/null | grep "$app_proto" | awk '{ print $3 }')
				uci set "$package_name"."$section".connmark="$app_proto_connmark/$app_proto_mask"
			fi	
			if [ -n "$not_app_proto" ] ; then
				not_app_proto_connmark=$(cat /etc/l7marker.marks 2>/dev/null | grep "$not_app_proto" | awk '{ print $2 }')
				not_app_proto_mask=$(cat /etc/l7marker.marks 2>/dev/null | grep "$not_app_proto" | awk '{ print $3 }')
				uci set "$package_name"."$section".not_connmark="$not_app_proto_connmark/$not_app_proto_mask"
			fi
			
			table="filter"
			chain="egress_restrictions"
			ingress=""
			target="REJECT"
			
			config_get "is_ingress" "$section" "is_ingress"
			if [ "$is_ingress" = "1" ] ; then
				ingress=" -i "
				chain="ingress_restrictions"
			fi
		
			if [ "$section_type" = "restriction_whitelist" ] ; then
				target="ACCEPT"
			fi
			
			make_iptables_rules -p "$package_name" -s "$section" -t "$table" -c "$chain" -g "$target" $ingress
			make_iptables_rules -p "$package_name" -s "$section" -t "$table" -c "$chain" -g "$target" $ingress -r
				
			uci del "$package_name"."$section".connmark 2>/dev/null	
			uci del "$package_name"."$section".not_connmark	 2>/dev/null
		fi
	}

	config_load "$package_name"
	config_foreach parse_rule_config "whitelist_rule"
	config_foreach parse_rule_config "restriction_rule"
}

