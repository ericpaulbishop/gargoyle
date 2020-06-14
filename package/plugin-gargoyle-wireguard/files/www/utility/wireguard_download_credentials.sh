#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

if [ "$(uci get wireguard_gargoyle.$GET_id)" != "allowed_client" ] ; then
	echo "Content-type: text/plain"
	echo ""
	echo "ERROR: Client ID does not exist."
else
	echo "Content-type: application/octet-stream"
	echo "Content-Disposition: attachment; filename=wg-$GET_id.conf"
	echo ""
	cat /tmp/wg.ac.tmp.conf
	rm /tmp/wg.ac.tmp.conf >/dev/null 2>&1
fi
?>


