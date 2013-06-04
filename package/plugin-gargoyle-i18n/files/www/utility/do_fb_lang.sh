#!/usr/bin/haserl --upload-limit=8192 --upload-dir=/tmp/
<?
	# This program is copyright © 2013 BashfulBladder and is distributed under the GNU GPLv2.0
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )

	logger -s $FORM_lfile
	logger -s $POST_fname
	cp $FORM_lfile /tmp/$POST_fname
	#sh /usr/lib/gargoyle/add_fb_lang.sh	/tmp/$POST_fname
	. /usr/lib/gargoyle/i18nServices.sh
	install_lang_pack /tmp/$POST_fname
	change_menu_language /tmp/$POST_fname
	
	rm $FORM_lfile
	
	echo "Content-Type: text/html;charset=utf-8"
	echo ""

	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'
	echo '<html xmlns="http://www.w3.org/1999/xhtml">'
	echo "<body></body></html>"
?>
