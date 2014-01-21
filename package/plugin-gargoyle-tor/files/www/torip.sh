#!/usr/bin/haserl
<?
	echo "Content-Type: text/plain"
	echo ""

	tor_enabled=$(uci get tor.global.enabled 2>/dev/null)
	tor_client_mode=$(uci get tor.client.client_mode 2>/dev/null)
	tor_ip_file=$(uci get tor.client.enabled_ip_file 2>/dev/null)
	if [ "$tor_enabled" != "1" ] || [ "$tor_client_mode" != "2" ] || [ -z "$tor_ip_file" ] ; then
		echo "tor_per_ip_disabled"
		exit
	fi

	# Check that mac in arp entry for ip matches either a valid dhcp lease or a static IP definition
	# Otherwise someone could connect with a static IP and turn Tor off/on for other people, causing problems
	connect_ip="$REMOTE_ADDR"
	num_arp_entries=$(cat /proc/net/arp 2>/dev/null | grep "$connect_ip " | wc -l)
	if [ "$num_arp_entries" != "1" ] || [ -z "$connect_ip" ] ; then
		echo "bad_ip"
		exit
	fi

	connect_mac=$(cat /proc/net/arp 2>/dev/null | grep "$connect_ip " | awk '{ print $4 }')
	valid_dhcp=$(grep "$mac.*$connect_ip" /var/dhcp.leases 2>/dev/null)
	valid_static=$(grep "$mac.*$connect_ip" /etc/ethers 2>/dev/null)
	if [ -z "$valid_dhcp" ] && [ -z "$valid_static" ] ; then
		echo "bad_ip"
		exit
	fi

	tor_is_active=$(ipset --test tor_active_ips $connect_ip 2>&1 | grep -v NOT)
	result=""
	if [ -n "$tor_is_active" ] ; then
		#currently active, remove ip from set
		ipset --del tor_active_ips "$connect_ip"
		result="success_disabled"
	else
		#currently disabled, add ip to set
		ipset --add tor_active_ips "$connect_ip"
		result="success_enabled"
	fi

	ipset --list tor_active_ips 2>&1 | grep "\." > /tmp/tor.tmp.tmp
	mv /tmp/tor.tmp.tmp "$tor_ip_file"
	
	echo "$result"
?>
