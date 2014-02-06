#!/usr/bin/haserl
<?
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information

	echo "HTTP/1.1 301 Moved Permanently" 
	echo "Set-Cookie:hash=loggedout;"
	echo "Location: login.sh?logout=1"
	exit
?>
