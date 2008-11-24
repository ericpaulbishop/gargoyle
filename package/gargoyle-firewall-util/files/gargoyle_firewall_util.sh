# Copyright Eric Bishop, 2008
# This is free software licensed under the terms of the GNU GPL v2.0
#
source /etc/functions.sh
include /lib/network

delete_chain_from_table()
{
	table=$1
	target=$2


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
		
		if [ $chain = $target ] ; then
			iptables -t $table -F $target
			iptables -t $table -X $target
		fi
	done
	
}


create_l7marker_chain()
{
	# eliminate chain if it exists
	delete_chain_from_table "mangle" "l7marker"

	iptables -t mangle -N l7marker
	iptables -t mangle -I INPUT -j l7marker
	iptables -t mangle -I FORWARD -j l7marker
	iptables -t mangle -I POSTROUTING -j l7marker
	

	app_proto_num=1
	app_proto_shift=16
	app_proto_mask="0xFF0000"
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

