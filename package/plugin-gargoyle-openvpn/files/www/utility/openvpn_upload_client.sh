#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

echo "Content-type: text/html"
echo ""

mkdir -p /tmp/vpn_client_upload_tmp
cd /tmp/vpn_client_upload_tmp

if [ -e "$FORM_openvpn_client_zip_file" ] ; then
	unzip "$FORM_openvpn_client_zip_file" 
	conf_file=$(grep -l "^[\t ]*ca\|^[\t ]*cert" * 2>/dev/null | head -n 1)
	mv "$conf_file" grouter_client.conf
	sed 's/ca.*\//ca    \/etc\/openvpn\//g'   grouter_client.conf
	sed 's/cert.*\//cert  \/etc\/openvpn\//g' grouter_client.conf
	sed 's/key.*\//key   \/etc\/openvpn\//g'  grouter_client.conf

elif [ -e "$FORM_openvpn_client_conf_file" ] && [ -e "$FORM_openvpn_client_ca_file" ] && [ -e "$FORM_openvpn_client_cert_file" ] && [ -e "$FORM_openvpn_client_key_file" ] ; then 
	
	mv "$FORM_openvpn_client_conf_file" grouter_client.conf
	mv "$FORM_openvpn_client_ca_file"   ca.crt
	mv "$FORM_openvpn_client_cert_file" grouter_client.crt
	mv "$FORM_openvpn_client_key_file"  grouter_client.key

	sed 's/ca.*$/ca    \/etc\/openvpn\/ca.crt/g'               grouter_client.conf
	sed 's/cert.*$/cert  \/etc\/openvpn\/grouter_client.crt/g' grouter_client.conf
	sed 's/key.*$/key   \/etc\/openvpn\/grouter_client.key/g'  grouter_client.conf
		
	
elif [ -n "$FORM_openvpn_client_conf_text" ] && [ -n "$FORM_openvpn_client_ca_text" ] && [ -n "$FORM_openvpn_client_cert_text" ] && [ -n "$FORM_openvpn_client_key_text" ] ; then
	printf "$FORM_openvpn_client_conf_text" > grouter_client.conf
	printf "$FORM_openvpn_client_ca_text"   > ca.crt
	printf "$FORM_openvpn_client_cert_text" > grouter_client.crt
	printf "$FORM_openvpn_client_key_text"  > grouter_client.key

	sed 's/ca.*$/ca    \/etc\/openvpn\/ca.crt/g'               grouter_client.conf
	sed 's/cert.*$/cert  \/etc\/openvpn\/grouter_client.crt/g' grouter_client.conf
	sed 's/key.*$/key   \/etc\/openvpn\/grouter_client.key/g'  grouter_client.conf
	
fi

if [ -e grouter_client.conf ] ; then
	mv * /etc/openvpn/
else
	#ERROR
fi

cd /tmp
rm -rf /tmp/vpn_client_upload_tmp




?>
