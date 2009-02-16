# Copyright Eric Bishop, 2008
# This is free software licensed under the terms of the GNU GPL v2.0
#
. /etc/functions.sh
include /lib/network

death_mask="0xFF000000"
death_mark="0xDD000000/$death_mask"


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


# creates a chain in the filter table that always rejects
# if the last byte of the connmark is DD.  This byte is
# set in dnat table if we are redirecting port, but don't
# want to allow connections on original port.  This same
# byte is used by restricter, but at beginning of filter
#table, this hasn't been hit yet
create_death_mark_chain()
{
	delete_chain_from_table "filter" "death_mark"
	iptables -t filter -N death_mark
	iptables -t filter -I INPUT 1 -j death_mark
	iptables -t filter -I FORWARD 1 -j death_mark
	iptables -t filter -I death_mark 1 -m connmark --mark "$death_mark" -m state --state NEW -j REJECT
}

# echo "1" if death_mark chain exists, otherwise echo "0"
death_mark_exists()
{
	local inp=$(iptables -t filter -L INPUT | grep "death_mark" 2>/dev/null)
	local fow=$(iptables -t filter -L FORWARD | grep "death_mark" 2>/dev/null)
	local exists=0
	if [ -n "$inp" ] && [ -n "$fow" ] ; then exists=1; fi
	echo $exists
}

# parse remote_accept sections in firewall config and add necessary rules
insert_remote_accept_rules()
{
	local config_name="firewall"
	local section_type="remote_accept"

	#add rules for remote_accepts
	death_mark_ports=""
	parse_remote_accept_config()
	{
		vars="local_port remote_port proto zone"
		proto="tcp"
		zone="wan"
		for var in $vars ; do
			config_get $var $1 $var
		done
		if [ -n "$local_port" ] ; then
			echo "LOCAL_PORT = $local_port"
			if [ -z "$remote_port"  ] ; then
				remote_port="$local_port"
			fi
			if [ "$remote_port" != "$local_port" ] ; then
				echo iptables -t nat -A "zone_"$zone"_prerouting" -p "$proto" --dport "$remote_port" -j REDIRECT --to-ports "$local_port"
				iptables -t nat -A "zone_"$zone"_prerouting" -p "$proto" --dport "$remote_port" -j REDIRECT --to-ports "$local_port"
				death_mark_ports="$local_port $death_mark_ports"
			fi
			echo iptables -t filter -A "input_$zone" -p $proto --dport "$local_port" -j ACCEPT
			iptables -t filter -A "input_$zone" -p $proto --dport "$local_port" -j ACCEPT
		fi
	}
	config_load "$config_name"
	config_foreach parse_remote_accept_config "$section_type"

	#remove any marks on relevant byte of connmark
	if [ -n "$death_mark_ports" ] ; then
		iptables -t nat -A zone_"$zone"_prerouting  -j CONNMARK --set-mark "0x0/$death_mask"
	fi

	#add death marks
	for dmp in $death_mark_ports ; do
		echo iptables -t nat -A zone_"$zone"_prerouting -p tcp --dport "$dmp" -j CONNMARK --set-mark "$death_mark"
		echo iptables -t nat -A zone_"$zone"_prerouting -p udp --dport "$dmp" -j CONNMARK --set-mark "$death_mark"
		iptables -t nat -A zone_"$zone"_prerouting -p tcp --dport "$dmp" -j CONNMARK --set-mark "$death_mark"
		iptables -t nat -A zone_"$zone"_prerouting -p udp --dport "$dmp" -j CONNMARK --set-mark "$death_mark"
	done

	if [ -n "$death_mark_ports" ] ; then
		create_death_mark_chain
	fi
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
	iptables -t mangle -I PREROUTING  -m connmark --mark 0x0/$app_proto_mask -j l7marker
	iptables -t mangle -I POSTROUTING -m connmark --mark 0x0/$app_proto_mask -j l7marker


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
				if [ -z "$loaded" ] ; then
					all_defined="0"
				fi
			done	
				
			if [ "$all_defined" = "1" ] && [ "$src" = "wan" ] && [ "$dest" = "lan" ] && [ ! "$src_dport" = "$dest_port" ] ; then
				#echo "here!"
				#echo iptables -t nat    -A pf_loopback_A -p $proto --dport $src_dport -j DNAT --to-destination $dest_ip:$dest_port
				#echo iptables -t filter -A pf_loopback_B -p $proto --dport $dest_port -d $dest_ip -j ACCEPT
				#echo iptables -t nat    -A pf_loopback_C -p $proto --dport $dest_port -d $dest_ip -s $dest_ip/$lan_mask -j MASQUERADE
				iptables -t nat    -A pf_loopback_A -p $proto --dport $src_dport -j DNAT --to-destination $dest_ip:$dest_port
				iptables -t filter -A pf_loopback_B -p $proto --dport $dest_port -d $dest_ip -j ACCEPT
				iptables -t nat    -A pf_loopback_C -p $proto --dport $dest_port -d $dest_ip -s $dest_ip/$lan_mask -j MASQUERADE
			fi	
		}

		config_load "$config_name"
		config_foreach add_pf_loopback "$section_type"
	fi
}

