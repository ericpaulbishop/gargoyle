#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

if [ ! -d "/etc/openvpn/client_conf/$GET_id" ] ; then
	echo "Content-type: text/plain"
	echo ""
	echo "ERROR: Client ID does not exist."
elif [ $GET_configtype == "multiple-files" ] ; then
	cd "/etc/openvpn/client_conf/$GET_id" >/dev/null 2>&1
	zip /tmp/vpn.ac.tmp.zip * >/dev/null 2>&1
	echo "Content-type: application/octet-stream"
	echo "Content-Disposition: attachment; filename=openvpn-credentials-$GET_id.zip"
	echo ""
	cat /tmp/vpn.ac.tmp.zip
	rm /tmp/vpn.ac.tmp.zip >/dev/null 2>&1
elif [ $GET_configtype == "single-ovpn" ] ; then
	cat << EOF > /tmp/vpn.ac.tmp.ovpn
$(cat /etc/openvpn/client_conf/$GET_id/$GET_id.conf)
$(echo -e '<ca>')
$(cat /etc/openvpn/client_conf/$GET_id/ca.crt)
$(echo -e '</ca>\n<cert>')
$(cat /etc/openvpn/client_conf/$GET_id/$GET_id.crt)
$(echo -e '</cert>\n<key>')
$(cat /etc/openvpn/client_conf/$GET_id/$GET_id.key)
$(echo -e '</key>\n<tls-auth>')
$(cat /etc/openvpn/client_conf/$GET_id/ta.key)
$(echo -e '</tls-auth>')
EOF
	echo "Content-type: application/octet-stream"
	echo "Content-Disposition: attachment; filename=openvpn-credentials-$GET_id.ovpn"
	echo ""
	cat /tmp/vpn.ac.tmp.ovpn
	rm /tmp/vpn.ac.tmp.ovpn >/dev/null 2>&1
fi
?>

