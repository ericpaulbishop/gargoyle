#!/usr/bin/haserl
<?
	# This program is copyright © 2008-2011 Eric Bishop and is distributed under the terms of the GNU GPL
	# version 2.0 with a special clarification/exception that permits adapting the program to
	# configure proprietary "back end" software provided that all modifications to the web interface
	# itself remain covered by the GPL.
	# See http://gargoyle-router.com/faq.html#qfoss for more information
	eval $( gargoyle_session_validator -c "$POST_hash" -e "$COOKIE_exp" -a "$HTTP_USER_AGENT" -i "$REMOTE_ADDR" -r "login.sh" -t $(uci get gargoyle.global.session_timeout) -b "$COOKIE_browser_time"  )
	echo "Content-type: text/plain"
	echo ""

	scan_madwifi()
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

	scan_mac80211()
	{
		radio_disabled1=$(uci get wireless.@wifi-device[0].disabled 2>/dev/null)
		radio_disabled2=$(uci get wireless.@wifi-device[1].disabled 2>/dev/null)
		g_sta=$(iwconfig 2>/dev/null | egrep "802.11((b)|(bg)|(gb)|(g)|(gn)|(bgn))" | grep -v "Master" | grep -v "Monitor" | awk '{ print $1 ; }' )
		test_ifs="$g_sta"
		if [ -z "$g_sta" ] || [ "$radio_disabled1" = "1" ] || [ "$radio_disabled2" = "1" ]  ; then
			g_sta=""
			test_ifs="phy0"
		fi

		if [ `uci show wireless | grep wifi-device | wc -l`"" = "2" ] && [ -e "/sys/class/ieee80211/phy1" ] && [ ! `uci get wireless.@wifi-device[0].hwmode`"" = `uci get wireless.@wifi-device[1].hwmode`""  ] ; then
			a_sta=$(iwconfig 2>/dev/null | egrep "802.11an" | grep -v "Master" | grep -v "Monitor" | awk '{ print $1 ; }' )
			phy0_is_g=$(iw phy0 info | grep " 2.*MHz")
			g_phy="phy0"
			a_phy="phy1"
			if [ -z "$phy0_is_g" ] ; then
				g_phy="phy1"
				a_phy="phy0"
			fi
			if [ -z "$g_sta" ] ; then
				test_ifs="$g_phy"
			fi
			if [ -z "$a_sta" ] || [ "$radio_disabled1" = "1" ] || [ "$radio_disabled2" = "1" ] ; then
				test_ifs="$test_ifs $a_phy"
			else
				test_ifs="$test_ifs $a_sta"
			fi
		fi

		for if in $test_ifs ; do
			if [ "$if" = "phy0" ] || [ "$if" = "phy1" ] ; then
				iw phy $if interface add tmpsta type managed
				ifconfig tmpsta hw ether 00:11:22:33:55:77
				ifconfig tmpsta up
				iwlist tmpsta scanning
				ifconfig tmpsta down
				iw dev tmpsta del
			else
				iwlist $if scanning
			fi
		done

	}

	if [ -e "/lib/wifi/broadcom.sh" ] ; then
		scan_brcm
	elif [ -e "/lib/wifi/mac80211.sh" ] ; then
		scan_mac80211
	elif [ -e "/lib/wifi/madwifi.sh" ] ; then
		scan_madwifi
	fi

?>
