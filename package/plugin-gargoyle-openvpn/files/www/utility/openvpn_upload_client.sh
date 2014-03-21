#!/usr/bin/haserl --upload-limit=1048576 --upload-dir=/tmp/
<%
	# This program is copyright © 2012-2013 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	



echo "Content-Type: text/html; charset=utf-8"
echo ""

echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
echo '<html xmlns="http://www.w3.org/1999/xhtml">'
echo '<body>'


ip_to_int()
{
	ip=$1
	ip_parts=$(echo $ip | sed 's/\./ /g')
	mult=256*256*256;
	count=0
	for p in $ip_parts ; do
		count=$(( $count + ($mult*p) ))
		mult=$(( $mult/256 ))
	done
	echo $count
}
int_to_ip()
{
	int=$1
	ip=""
	for m in $((256*256*256)) $((256*256)) 256 1 ; do
		next=$(( $int/$m ))
		int=$(( $int - ($next*$m) ))
		if [ -z "$ip" ] ; then
			ip="$next"
		else
			ip="$ip.$next"
		fi
	done
	echo $ip
}

restart_network=0
old_replace_ip=""
new_replace_ip=""

dir_rand=$(</dev/urandom tr -dc a-z | head -c 12)
tmp_dir="/tmp/vpn_client_upload_$dir_rand"
mkdir -p "$tmp_dir"
cd "$tmp_dir"

client_name=$(uci get openvpn_gargoyle.client.id 2>/dev/null)
if [ -z "$client_name" ] ; then
	client_name_rand=$(</dev/urandom tr -dc a-z | head -c 12)
	client_name="grouter_client_$client_name_rand"
fi

error=""
tab=$(printf "\t")

ta_direction=""
block_non_openvpn=0
if [ -s "$FORM_openvpn_client_zip_file" ] ; then

	is_targz=$(dd if="$FORM_openvpn_client_zip_file" bs=1 count=2 2>/dev/null | hexdump -v -e '1/1 "%02x"')
	if [ "x$is_targz" = "x1f8b" ] ; then
		tar xzf "$FORM_openvpn_client_zip_file" >/dev/null 2>&1
	else
		unzip   "$FORM_openvpn_client_zip_file" >/dev/null 2>&1
	fi

	OLD_IFS="$IFS"
	IFS=$(printf "\n\r")
	files=$(find .)
	for f in $files ; do
		if [ ! -d "$f" ] ; then mv "$f" . ; fi
	done
	for f in $files ; do
		if [ -d "$f" ] && [ "$f" != "." ] ; then rm -rf "$f" ; fi
	done
	IFS="$OLD_IFS"


	conf_file=$(grep -l "^[$tab ]*ca\|^[$tab ]*cert" * 2>/dev/null | head -n 1)
	ca_file=$(  egrep "^[$tab ]*ca[$tab ]+"             "$conf_file" | sed 's/^.*\///g' | sed 's/[\t ]*$//g' | sed 's/^.*[\t ]//g' )
	cert_file=$(egrep "^[$tab ]*cert[$tab ]+"           "$conf_file" | sed 's/^.*\///g' | sed 's/[\t ]*$//g' | sed 's/^.*[\t ]//g' )
	key_file=$( egrep "^[$tab ]*key[$tab ]+"            "$conf_file" | sed 's/^.*\///g' | sed 's/[\t ]*$//g' | sed 's/^.*[\t ]//g' )
	ta_file=$(  egrep "^[$tab ]*tls\-auth[$tab ]+"      "$conf_file" | egrep "^[$tab ]*tls\-auth[$tab ]+" | awk ' { print $2 } ' | sed 's/^.*\///g' )
	ta_direction=$(  egrep "^[$tab ]*tls\-auth[$tab ]+" "$conf_file" | egrep "^[$tab ]*tls\-auth[$tab ]+" | awk ' { print $3 } ' )
	if [ -e ./block_non_openvpn ] ; then
		block_non_openvpn=1
	fi

	if [ -s network ] ; then
		expected_ip=$(awk ' $0 ~ /ipaddr/ { print $NF }' network)
		expected_mask=$(awk ' $0 ~ /netmask/ { print $NF }' network)
		
		if [ -n "$expected_ip" ] && [ -n "$expected_mask" ] ; then
			cur_ip=$(uci get network.lan.ipaddr)
			cur_mask=$(uci get network.lan.netmask)
			cur_ip_int=$(ip_to_int $cur_ip)
			cur_mask_int=$(ip_to_int $cur_mask)
			cur_sub_ip_int=$(($cur_ip_int & $cur_mask_int))
			cur_sub_ip=$(int_to_ip $cur_sub_ip_int)
			
			

			exp_ip_int=$(ip_to_int $expected_ip)
			exp_mask_int=$(ip_to_int $expected_mask)
			cur_test=$(( $cur_mask_int & $cur_ip_int ))
			exp_test=$(( $exp_mask_int & $exp_ip_int ))
			if [ "$cur_test" != "$exp_test" ] ; then
				
				new_ip_int=$(($exp_ip_int+1))
				new_ip=$(int_to_ip $new_ip_int)
				if [ "$FORM_net_mismatch_action" = "query" ] ; then
					echo "<script type=\"text/javascript\">top.clientNetMismatchQuery(\"$expected_ip/$expected_mask\",\"$cur_sub_ip/$cur_mask\", \"$new_ip\" );</script>"
					echo "</body></html>"
					cd /tmp
					rm -rf "$tmp_dir"
					exit
				elif [ "$FORM_net_mismatch_action" = "change" ] ; then
					old_dns=$(uci get network.lan.dns)
					if [ "$old_dns" = "$cur_ip" ] ; then
						uci set network.lan.dns="$new_ip"
					fi
					uci set network.lan.ipaddr="$new_ip"
					uci set network.lan.netmask="$expected_mask"
					uci commit
					restart_network=1
					old_replace_ip="$cur_ip"
					new_replace_ip="$new_ip"
				fi
				#do nothing if net_mismatch_action is "keep"
			fi
		fi
	fi

	if   [ ! -f "$ca_file" ] ; then
		error=$(i18n openvpn.uc_CA_f)
	elif [ ! -f "$cert_file" ] ; then
		error=$(i18n openvpn.uc_crt_f)
	elif [ ! -f "$key_file" ] ; then
		error=$(i18n openvpn.uc_key_f)
	elif [ ! -f "$conf_file" ] ; then
		error=$(i18n openvpn.uc_cfg_f)
	else
		cat "$conf_file" | tr -d "\r" > "${client_name}.conf"
		cat "$ca_file"   | tr -d "\r" > "${client_name}_ca.crt"
		cat "$cert_file" | tr -d "\r" > "${client_name}.crt"
		cat "$key_file"  | tr -d "\r" > "${client_name}.key"
		rm  "$conf_file" "$ca_file" "$cert_file" "$key_file"
		if [ -f "$ta_file" ] ; then
			cat "$ta_file" | tr -d "\r" > "${client_name}_ta.key"
			rm "$ta_file"
		fi	

	fi

	rm "$FORM_openvpn_client_zip_file" 

elif [ -s "$FORM_openvpn_client_conf_file" ] && [ -s "$FORM_openvpn_client_ca_file" ] && [ -s "$FORM_openvpn_client_cert_file" ] && [ -s "$FORM_openvpn_client_key_file" ] ; then 
	
	cat "$FORM_openvpn_client_conf_file" | tr -d "\r" > "${client_name}.conf"
	cat "$FORM_openvpn_client_ca_file"   | tr -d "\r" > "${client_name}_ca.crt"
	cat "$FORM_openvpn_client_cert_file" | tr -d "\r" > "${client_name}.crt"
	cat "$FORM_openvpn_client_key_file"  | tr -d "\r" > "${client_name}.key"
	rm  "$FORM_openvpn_client_conf_file" "$FORM_openvpn_client_ca_file" "$FORM_openvpn_client_cert_file" "$FORM_openvpn_client_key_file"
	if [ -s "$FORM_openvpn_client_ta_key_file" ] ; then
		ta_direction=$(  egrep "^[$tab ]*tls\-auth[$tab ]+" "${client_name}.conf" | egrep "^[$tab ]*tls\-auth[$tab ]+" | awk ' { print $3 } ' )
		cat "$FORM_openvpn_client_ta_key_file"  | tr -d "\r" > "${client_name}_ta.key"
		rm  "$FORM_openvpn_client_ta_key_file"
	fi


elif [ -n "$FORM_openvpn_client_conf_text" ] && [ -n "$FORM_openvpn_client_ca_text" ] && [ -n "$FORM_openvpn_client_cert_text" ] && [ -n "$FORM_openvpn_client_key_text" ] ; then

	printf "$FORM_openvpn_client_conf_text" | tr -d "\r" > "${client_name}.conf"
	printf "$FORM_openvpn_client_ca_text"   | tr -d "\r" > "${client_name}_ca.crt"
	printf "$FORM_openvpn_client_cert_text" | tr -d "\r" > "${client_name}.crt"
	printf "$FORM_openvpn_client_key_text"  | tr -d "\r" > "${client_name}.key"
	if [ -n "$FORM_openvpn_client_ta_key_text" ] ; then
		ta_direction=$(  egrep "^[$tab ]*tls\-auth[$tab ]+" "${client_name}.conf" | egrep "^[$tab ]*tls\-auth[$tab ]+" | awk ' { print $3 } ' )
		printf "$FORM_openvpn_client_ta_key_text"  | tr -d "\r" > "${client_name}_ta.key"
	fi

fi


#For client config, ta_direction can be 1 (client) or omitted, but never 0 (server) or anything else
if [ "$ta_direction" != "1" ] ; then
	ta_direction=""
else
	ta_direction=" 1"
fi

if [ ! -f "${client_name}.conf" ] ; then
	error=$(i18n openvpn.uc_cfg_f)
fi

if [ -z "$error" ] ; then
	
	sed -i 's/^[\t ]*ca[\t ].*$/ca    \/etc\/openvpn\/'"${client_name}_ca.crt"'/g'    "${client_name}.conf"
	sed -i 's/^[\t ]*cert[\t ].*$/cert  \/etc\/openvpn\/'"${client_name}.crt"'/g'     "${client_name}.conf"
	sed -i 's/^[\t ]*key[\t ].*$/key   \/etc\/openvpn\/'"${client_name}.key"'/g'      "${client_name}.conf"
	sed -i 's/^[\t ]*status[\t ].*$/status  \/var\/openvpn\/current_status/g'         "${client_name}.conf"
	if [ -f "${client_name}_ta.key" ]  ; then
		sed -i 's/^[\t ]*tls\-auth[\t ].*$/tls-auth    \/etc\/openvpn\/'"${client_name}_ta.key${ta_direction}"'/g'    "${client_name}.conf"
	fi

	#proofreading
	use_tap=$(egrep  "^[$tab ]*dev[$tab ]+tap" "${client_name}.conf")
	if [ -n "$use_tap" ] ; then
		error=$(i18n openvpn.uc_TAP_Err)
	fi

	if [ -z "$error" ] ; then
		mv "${client_name}.conf" "${client_name}_ca.crt" "${client_name}.crt" "${client_name}.key"  /etc/openvpn/
		if [ -e "${client_name}_ta.key" ]  ; then
			mv "${client_name}_ta.key"  /etc/openvpn/
		fi

		#run constant uci commands
		uci set openvpn_gargoyle.server.enabled="false"                        >/dev/null 2>&1
		uci set openvpn_gargoyle.client.enabled="true"                         >/dev/null 2>&1
		uci set openvpn_gargoyle.client.id="$client_name"                      >/dev/null 2>&1
		uci set openvpn.custom_config.config="/etc/openvpn/$client_name.conf"  >/dev/null 2>&1
		uci set openvpn.custom_config.enable="1"                               >/dev/null 2>&1

		#block non-openvpn traffic to prevent leak if openvpn quits unexpectedly?
		if [ "$block_non_openvpn" = "1" ] ; then
			uci set openvpn_gargoyle.@client[0].block_non_openvpn="true"
		fi
		uci commit
		
		#run other commands passed to script (includes firewall config and openvpn restart)
		if [ -n "$FORM_commands" ] ; then	
			tmp_file="$tmp_dir/tmp.sh"
			printf "%s" "$FORM_commands" | tr -d "\r" > "$tmp_file"
			sh "$tmp_file"
			if [ "$restart_network" = "1" ]  && [ -n "$old_replace_ip" ] && [ -n "$new_replace_ip" ] ; then
				sh /usr/lib/gargoyle/update_router_ip.sh "$old_replace_ip" "$new_replace_ip"
				sh /usr/lib/gargoyle/restart_network.sh
			fi
		fi

		wait_secs=25
		have_tun_if=$(ifconfig 2>/dev/null | grep "^tun")
		while [ -z "$have_tune_if" ] && [ "$wait_secs" -gt 0 ] ; do
			sleep 1
			have_tun_if=$(ifconfig 2>/dev/null | grep "^tun")
			wait_secs=$(( $wait_secs - 1 ))
		done
		
		if [ -z "$have_tun_if" ] ; then
			error=$(i18n openvpn.uc_conn_Err)
		fi
	fi
fi


result="$error"
if [ -z "$error" ] ; then
	result="Success"
fi

echo "<script type=\"text/javascript\">top.clientSaved(\"$result\");</script>"
echo "</body></html>"

cd /tmp
rm -rf "$tmp_dir"



%>
