#!/usr/bin/haserl
<? 
	# This program is copyright © 2008 Eric Bishop and is distributed under the terms of the GNU GPL 
	# version 2.0 with a special clarification/exception that permits adapting the program to 
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL. 
	# See http://gargoyle-router.com/faq.html#qfoss for more information


	echo "Content-type: text/plain"
	echo ""
	
	scan_atheros()
	{
		aths=$(iwconfig 2>/dev/null | grep -o "^ath..")
		scanif=""
		created=""
		for ath in $aths ; do
			if [ -z "$scanif" ] ; then 
				test=$(iwconfig $ath | grep "anaged")
				if [ -z "$test" ] ; then
					test=$(iwconfig $ath | grep "aster")
				fi
				if [ -n "$test" ] ; then
					scanif="$ath"
				fi
			fi
		done

		if [ -z "$scanif" ] ; then
			created="ath0"
			scanif=$created
			wlanconfig $created create wlandev wifi0 wlanmode sta >/dev/null 2>&1
		fi

		is_up=$(ifconfig 2>/dev/null | grep $scanif)
		if [ -z "$is_up" ] ; then
			ifconfig $scanif up
		fi
		sleep 4


		iwlist $scanif scanning  2>/dev/null

		if [ -z "$is_up" ] ; then
			ifconfig $scanif down 2>/dev/null
		fi
		if [ -n "$created" ] ; then
			wlanconfig $created destroy 2>/dev/null
		fi
	}	
	
	
	scan_brcm()
	{
		if_exists=$(ifconfig | grep wl0)
		is_disabled=$(uci get wireless.wl0.disabled)
		if [ -z "$if_exists" ] || [ "$is_disabled" = 1 ] ; then
			wl up
			ifconfig wl0 up
		fi
		sleep 4
	
		iwlist wl0 scanning
		if [ -z "$if_exists" ] ; then
			ifconfig wl0 down
		fi
	}

	if [ -e "/lib/wifi/broadcom.sh" ] ; then
		scan_brcm
	fi
	if [ -e "/lib/wifi/madwifi.sh" ] ; then
		scan_atheros
	fi

?>
