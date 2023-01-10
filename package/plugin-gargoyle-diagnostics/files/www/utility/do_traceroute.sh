#!/usr/bin/haserl
<? 
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )

	echo "Content-type: text/plain"
	echo ""

	if [ -n "$FORM_tracerttarget" ] && [ -n "$FORM_tracertfamily" ] ; then
		traceroute -"$FORM_tracertfamily" -w 1 "$FORM_tracerttarget" 2>&1
	fi
	echo "Success"
?>
