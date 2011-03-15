#/bin/sh

tz="$1"

timezone_lines=$(wc -l /www/data/timezones.txt | awk ' { print $1 ; } ' )
tz_line_num=$(grep -n "\"$tz\"" /www/data/timezones.txt | sed 's/\:.*$//g')
opp_tz_line_num=$(( 1+ (($tz_line_num+($timezone_lines/2)) % $timezone_lines) ))
opp_tz=$( head -n $opp_tz_line_num /www/data/timezones.txt | tail -n 1 | sed 's/^[^\t]*\t*\"//g' | sed 's/\".*$//g' )


wan_gateway=$(uci -P /var/state get network.wan.gateway)
echo "$opp_tz" >/etc/TZ
set_kernel_timezone
ping -c 2 $wan_gateway >/dev/null 2>/dev/null 
bw_get -h -i bdist1-upload-minute-15 >/dev/null 2>/dev/null

uci set system.@system[0].timezone="$tz"
uci commit
echo "$tz" >/etc/TZ
set_kernel_timezone
ping -c 2 $wan_gateway >/dev/null 2>/dev/null
bw_get -h -i bdist1-upload-minute-15 >/dev/null 2>/dev/null




