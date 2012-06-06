#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

echo "Content-type: text/html"
echo ""

if [ -e "$FORM_openvpn_client_zip_file" ] ; then
	mkdir -p /tmp/vpn_client_upload_tmp
	cd /tmp/vpn_client_upload_tmp
	unzip "$FORM_openvpn_client_zip_file" 
	conf_file=$(grep -l "^[\t ]*ca\|^[\t ]*cert" * 2>/dev/null | head -n 1)
	mv "$conf_file" client.conf
	
	
	
elif [ -e "$FORM_openvpn_client_conf_file" ] && [ -e "$FORM_openvpn_client_ca_file" ] && [ -e "$FORM_openvpn_client_cert_file" ] && [ -e "$FORM_openvpn_client_key_file" ] ; then 
	
elif [ -n "$FORM_openvpn_client_conf_text" ] && [ -n "$FORM_openvpn_client_ca_text" ] && [ -n "$FORM_openvpn_client_cert_text" ] && [ -n "$FORM_openvpn_client_key_text" ] ; then
	
else
	# ERROR!!!
	exit
fi




?>
