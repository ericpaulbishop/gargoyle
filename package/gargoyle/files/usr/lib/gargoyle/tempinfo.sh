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
"Linksys WRT32X" | \
"Linksys WRT3200ACM")
	TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon1/temp1_input);
	TEMPMEM=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp2_input);
	TEMPWIFI=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp1_input);;
"Netgear Nighthawk X4S R7800")
	TEMPCPU=$(cut -c1-2 /sys/class/thermal/thermal_zone1/temp);
	TEMPMEM=$(cut -c1-2 /sys/class/thermal/thermal_zone2/temp);
	TEMPWIFI=$(cat /sys/class/hwmon/hwmon0/temp1_input 2>/dev/null || cat /sys/class/hwmon/hwmon1/temp1_input 2>/dev/null || echo "-");
	TEMPWIFI=$(echo $TEMPWIFI | cut -c1-2);;
*)
	TEMPCPU="-";
	TEMPMEM="-";
	TEMPWIFI="-";
	show_temp=0;
	# Try to load whatever info we can find from hwmon or thermal_zone
	[ -e /sys/class/thermal/thermal_zone0/temp ] && { TEMPCPU=$(cut -c1-2 /sys/class/thermal/thermal_zone0/temp); [ -n "$TEMPCPU" ] && show_temp=1; };
	[ -e /sys/class/hwmon/hwmon0/temp1_input ] && { TEMPCPU=$(cut -c1-2 /sys/class/hwmon/hwmon0/temp1_input); [ -n "$TEMPCPU" ] && show_temp=1; };;
esac

echo "temps.push(\"$show_temp\",\"$TEMPCPU\",\"$TEMPMEM\",\"$TEMPWIFI\");"

exit 0
