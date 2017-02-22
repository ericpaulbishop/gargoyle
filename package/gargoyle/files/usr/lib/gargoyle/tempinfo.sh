#!/bin/sh

[ -e /tmp/sysinfo/model ] || exit 0

tmodel=$(cat /tmp/sysinfo/model)
show_temp=1

# javascript temps array -> "show_temps CPU RAM WIFI"

case "$tmodel" in
"Linksys WRT1900AC")
	TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon2/temp1_input);
	TEMPMEM=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp1_input);
	TEMPWIFI=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp2_input);;
"Linksys WRT1900ACv2" | \
"Linksys WRT1900ACS" | \
"Linksys WRT1200AC" | \
"Linksys WRT3200ACM")
	TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp1_input);
	TEMPMEM=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp2_input);
	TEMPWIFI=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp1_input);;
*)
	TEMPCPU="-";
	TEMPMEM="-";
	TEMPWIFI="-";
	show_temp=0;;
esac

echo "temps.push(\"$show_temp\",\"$TEMPCPU\",\"$TEMPMEM\",\"$TEMPWIFI\");"

exit 0
