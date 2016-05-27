#!/usr/bin/haserl
<? 
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	echo "Content-type: text/plain"
	echo ""

	ls -1 /dev/ttyUSB* /dev/ttyACM* /dev/cdc-wdm* 2>/dev/null
?>
