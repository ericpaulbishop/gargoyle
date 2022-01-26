#!/usr/bin/haserl
<?

eval $( gargoyle_session_validator -c "$COOKIE_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )	

expectedParentDir="/sys/fs/pstore"
requestedFile="$expectedParentDir/$GET_fileid"

if [ -e "$requestedFile" ] && [ $(dirname "$requestedFile") = "$expectedParentDir" ] ; then
	echo "Content-type: application/octet-stream"
	echo "Content-Disposition: attachment; filename=$GET_fileid.txt"
	echo ""
	cat /sys/fs/pstore/$GET_fileid
else
	echo "Content-type: text/plain"
	echo ""
	echo "ERROR: Requested ramoops file does not exist."
fi
?>
