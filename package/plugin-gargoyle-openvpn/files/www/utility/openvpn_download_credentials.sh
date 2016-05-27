#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

if [ ! -d "/etc/openvpn/client_conf/$GET_id" ] ; then
	echo "Content-type: text/plain"
	echo ""
	echo "ERROR: Client ID does not exist."
else
	cd "/etc/openvpn/client_conf/$GET_id" >/dev/null 2>&1
	zip /tmp/vpn.ac.tmp.zip * >/dev/null 2>&1
	echo "Content-type: application/octet-stream"
	echo "Content-Disposition: attachment; filename=openvpn-credentials-$GET_id.zip"
	echo ""
	cat /tmp/vpn.ac.tmp.zip
	rm /tmp/vpn.ac.tmp.zip >/dev/null 2>&1
fi
?>

