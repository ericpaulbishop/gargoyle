#!/usr/bin/haserl
<? 
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time" )

	echo "Content-type: text/plain"
	echo ""

	if [ -n "$FORM_pingtarget" ] && [ -n "$FORM_pingfamily" ] ; then
		ping -"$FORM_pingfamily" -c 5 "$FORM_pingtarget" 2>&1
	fi
	echo "Success"
?>
