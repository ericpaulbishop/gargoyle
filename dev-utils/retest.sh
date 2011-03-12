#!/bin/bash

firmware_file="openwrt-ar71xx-tl-wr1043nd-v1-squashfs-sysupgrade.bin"
firmware=bin/ar71xx/$firmware_file
router_ip="192.168.1.1"
pass="password"
user="root"

cd build*/linux*/iptable*
find . -name "*.*o*" | xargs rm -rf
cd ../../..
cd build*/linux*/linux-2.6*/net/ipv4/netfilter
find . -name "*.*o*" | xargs rm -rf
cd ../../../../../..

rm -rf bin
make V=99

if [ ! -d bin ] ; then
	exit;
fi

my_in=/tmp/my_in
my_out=/tmp/my_out
rm -rf $my_in $my_out


empty -f -i $my_in  -o $my_out /usr/bin/scp -o StrictHostKeyChecking=no  "$firmware" "$user@$router_ip:/tmp"
empty -w -i $my_out -o $my_in "word: " "$pass\n"
sleep 3
empty -k >/dev/null 2>&1
killall empty >/dev/null 2>&1


empty -f -i $my_in  -o $my_out /usr/bin/ssh -o StrictHostKeyChecking=no  $user@$router_ip
empty -w -i $my_out -o $my_in "word: " "$pass\n"


empty -s -o $my_in "sysupgrade -n /tmp/$firmware_file\n"
empty -w -i $my_out -o $my_in "boot " "\n" >/dev/null 2>&1 
empty -k >/dev/null 2>&1
killall empty >/dev/null 2>&1

rm -rf $my_in $my_out
