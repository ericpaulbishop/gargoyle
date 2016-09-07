#!/bin/sh

#
# This program is copyright © 2015 dpint and is distributed under the terms of the GNU GPL
# version 2.0 with a special clarification/exception that permits adapting the program to
# configure proprietary "back end" software provided that all modifications to the web interface
# itself remain covered by the GPL.
# See http://gargoyle-router.com/faq.html#qfoss for more information
#

receiver=$(uci get email.@email[0].recipient)
data=$(uci get email.@email[0].data)
tlscert="/etc/ssl/certs/ca-certificates.crt"
tls=$(uci get email.@email[0].tls)
count=$(uci get email.@email[0].count)
thstyle="style='border: 1px solid #CCC; height: 30px;background:#F3F3F3;font-weight:700'"
tablestyle="style='color:#333;font-family:Helvetica,Arial,sans-serif;width:640px;border-collapse:collapse;border-spacing:0'"
tdstyle="style='border: 1px solid #CCC; height: 30px;background:#FAFAFA;text-align:center'"

if printf '%s' "$data" | egrep -q "0" || printf '%s' "$data" | egrep -q "1"
then
	sh /tmp/do_webmon_backup.sh
fi

#Email header
echo -e "Subject: Gargoyle Router report - $(date)\r\nFrom: $(cat /etc/msmtprc | grep from | sed "s/ from //")\r\nContent-Type: text/html; charset='UTF-8';\r\n<html><body>" > /tmp/email-log.txt;

if printf '%s' "$data" | egrep -q "0"
then
	#Recently visited sites, converting unix time
	echo -e "<h1>Recently visited sites:</h1><br><table $tablestyle>" >> /tmp/email-log.txt
	echo "<tr><th $thstyle>Time</th><th $thstyle>IP address</th><th $thstyle>Website</th></tr>" >> /tmp/email-log.txt;
	cat /usr/data/webmon_domains.txt > /tmp/work.tmp
	while read line
	do
	time=`echo $line | awk '{split($0,a," "); print a[1]}'`
	ip=`echo $line | awk '{split($0,a," "); print a[2]}'`
	domain=`echo $line | awk '{split($0,a," "); print a[3]}'`
	converttime=$(date -d @$time);
	echo "<tr><td $tdstyle>"$converttime"</td><td $tdstyle>"$ip"</td><td $tdstyle>"$domain"</td></tr>" >> /tmp/email-log.txt;
	done < /tmp/work.tmp
	rm /tmp/work.tmp
	echo -e "</table>" >> /tmp/email-log.txt
fi

if printf '%s' "$data" | egrep -q "1"
then
	#Recent web searches, converting unix time
	echo -e "<br><h1>Recent web searches:</h1><br><table $tablestyle>" >> /tmp/email-log.txt
	echo "<tr><th $thstyle>Time</th><th $thstyle>IP address</th><th $thstyle>Search text</th></tr>" >> /tmp/email-log.txt;
	cat /usr/data/webmon_searches.txt > /tmp/work.tmp
	while read line 
	do
	time=`echo $line | awk '{split($0,a," "); print a[1]}'`
	ip=`echo $line | awk '{split($0,a," "); print a[2]}'`
	domain=`echo $line | awk '{split($0,a," "); print a[3]}'`
	converttime=$(date -d @$time);
	echo "<tr><td $tdstyle>"$converttime"</td><td $tdstyle>"$ip"</td><td $tdstyle>"$domain"</td></tr>" >> /tmp/email-log.txt;
	done < /tmp/work.tmp
	rm /tmp/work.tmp
	echo -e "</table>" >> /tmp/email-log.txt	
fi

if printf '%s' "$data" | egrep -q "2"
then
	#Logs
	echo -e "<br><h1>System logs:</h1><br><table $tablestyle>" >> /tmp/email-log.txt
	echo "<tr><th $thstyle>Time</th><th $thstyle>Message</th></tr>" >> /tmp/email-log.txt;
	logread > /tmp/work.tmp
	while read line           
	do
	time=`echo $line | cut -f1-5 -d " "`
	log=`echo $line | cut -d " " -f6-`
	echo "<tr><td $tdstyle>$time</td><td $tdstyle>$log</td></tr>" >> /tmp/email-log.txt
	done < /tmp/work.tmp 
	rm /tmp/work.tmp
	echo -e "</table>" >> /tmp/email-log.txt	
fi

if printf '%s' "$data" | egrep -q "3"
then
	#DHCP Leases
	echo -e "<br><h1>DHCP leases:</h1><br><table $tablestyle>" >> /tmp/email-log.txt
	echo "<tr><th $thstyle>Time</th><th $thstyle>Mac address</th><th $thstyle>IP address</th><th $thstyle>Hostname</th></tr>" >> /tmp/email-log.txt;
	cat /tmp/dhcp.leases > /tmp/work.tmp
	while read line
	do
	time=`echo $line | awk '{split($0,a," "); print a[1]}'`
	mac=`echo $line | awk '{split($0,a," "); print a[2]}'`
	ip=`echo $line | awk '{split($0,a," "); print a[3]}'`
	domain=`echo $line | awk '{split($0,a," "); print a[4]}'`
	converttime=$(date -d @$time);
	echo "<tr><td $tdstyle>"$converttime"</td><td $tdstyle>"$mac"</td><td $tdstyle>"$ip"</td><td $tdstyle>"$domain"</td></tr>" >> /tmp/email-log.txt;
	done < /tmp/work.tmp
	rm /tmp/work.tmp
	echo -e "</table>" >> /tmp/email-log.txt
fi

if printf '%s' "$data" | egrep -q "4"
then
	#ARP Records
	echo -e "<br><h1>ARP records:</h1><br><table $tablestyle>" >> /tmp/email-log.txt
	echo "<tr><th $thstyle>IP address</th><th $thstyle>HW type</th><th $thstyle>Flags</th><th $thstyle>HW address</th><th $thstyle>Mask</th><th $thstyle>Device</th></tr>" >> /tmp/email-log.txt;
	cat /proc/net/arp | tail -n +2 > /tmp/work.tmp
	while read line
	do
	ip=`echo $line | awk '{split($0,a," "); print a[1]}'`
	hw=`echo $line | awk '{split($0,a," "); print a[2]}'`
	flags=`echo $line | awk '{split($0,a," "); print a[3]}'`
	hwaddr=`echo $line | awk '{split($0,a," "); print a[4]}'`
	mask=`echo "$line" | awk '{split($0,a," "); print a[5]}'`
	device=`echo "$line" | awk '{split($0,a," "); print a[6]}'`
	echo "<tr><td $tdstyle>"$ip"</td><td $tdstyle>"$hw"</td><td $tdstyle>"$flags"</td><td $tdstyle>"$hwaddr"</td><td $tdstyle>"$mask"</td><td $tdstyle>"$device"</td></tr>" >> /tmp/email-log.txt;
	done < /tmp/work.tmp
	rm /tmp/work.tmp
	echo -e "</table>" >> /tmp/email-log.txt
fi

if printf '%s' "$data" | egrep -q "5"
then
	#Bandwidth usage
	echo -e "<br><h1>Bandwidth usage:</h1><br>" >> /tmp/email-log.txt
	config=$(uci get email.@email[0].bandwidthInterval)
	cat /tmp/bw_backup/do_bw_backup.sh | grep bw_get | sed 's/.*bw_get/bw_get/' | sed 's/\-f .*/-t/g' | grep $config | grep "bdist" > /tmp/tmp.bw.sh
	sh  /tmp/tmp.bw.sh | sed 's/^[^\-]*\-//g' |  sed 's/\-/,/g' | sed '/^\s*$/d' > /tmp/work.tmp
	rm /tmp/tmp.bw.sh
	while read line           
	do           
		type=$(echo $line | cut -f4 -d,)
		if [ "$type" == "COMBINED" ]; then
			end_time=$(echo $line | cut -f6 -d,)
			if [ "$end_time" != "0" ]; then
				direction=$(echo $line | cut -f1 -d,)
				data=$(echo $line | cut -f7 -d,)
				if [ "$direction" == "download" ]; then
					converttime=$(date -d @$(echo $line | cut -f5 -d,));
					if [ "$config" == "day" ]; then
						echo "$end_time,$(echo $converttime | cut -f2,3 -d " ")" >> /tmp/emailtime.tmp
					else
						echo "$end_time,$(echo $converttime | cut -f4 -d " " | cut -f1-2 -d:)" >> /tmp/emailtime.tmp
					fi
					echo "$end_time,$data" >> /tmp/emaildownload.tmp
				else
					echo "$end_time,$data" >> /tmp/emailupload.tmp
				fi
			fi
		fi
	done < /tmp/work.tmp
	rm /tmp/work.tmp
	echo "<table $tablestyle><tr><th $thstyle>Time</th><th $thstyle>Download</th><th $thstyle>Upload</th></tr>" >> /tmp/email-log.txt
	while read line           
	do
		epochtime=$(echo $line | cut -f1 -d,);
		time=$(echo $line | cut -f2 -d,);
		upload=$(cat /tmp/emailupload.tmp | grep $epochtime | cut -f2 -d,)
		if [[ $upload -gt $((1024*1024*1024*1024)) ]]; then
			upload=$(awk "BEGIN {printf \"%.2f\",$upload/1024/1024/1024/1024}" && printf " TB")
		elif [[ $upload -gt $((1024*1024*1024)) ]]; then
			upload=$(awk "BEGIN {printf \"%.2f\",$upload/1024/1024/1024}" && printf " GB")
		elif [[ $upload -gt $((1024*1024)) ]]; then
			upload=$(awk "BEGIN {printf \"%.2f\",$upload/1024/1024}" && printf " MB")
		elif [[ $upload -gt $((1024)) ]]; then
			upload=$(awk "BEGIN {printf \"%.2f\",$upload/1024}" && printf " KB")
		else
			upload=$(printf "$upload B")
		fi
		download=$(cat /tmp/emaildownload.tmp | grep $epochtime | cut -f2 -d,)
		if [[ $download -gt $((1024*1024*1024*1024)) ]]; then
			download=$(awk "BEGIN {printf \"%.2f\",$download/1024/1024/1024/1024}" && printf " TB")
		elif [[ $download -gt $((1024*1024*1024)) ]]; then
			download=$(awk "BEGIN {printf \"%.2f\",$download/1024/1024/1024}" && printf " GB")
		elif [[ $download -gt $((1024*1024)) ]]; then
			download=$(awk "BEGIN {printf \"%.2f\",$download/1024/1024}" && printf " MB")
		elif [[ $download -gt $((1024)) ]]; then
			download=$(awk "BEGIN {printf \"%.2f\",$download/1024}" && printf " KB")
		else
			download=$(printf "$download B")
		fi
		echo "<tr><td $tdstyle>$time</td><td $tdstyle>$download</td><td $tdstyle>$upload</td></tr>" >> /tmp/bandwidth.tmp
	done < /tmp/emailtime.tmp 
	cat /tmp/bandwidth.tmp | tail -n $count >> /tmp/email-log.txt
	rm /tmp/emaildownload.tmp
	rm /tmp/emailupload.tmp
	rm /tmp/emailtime.tmp
	rm /tmp/bandwidth.tmp
	echo "</table>" >> /tmp/email-log.txt
fi

echo "</body></html>" >> /tmp/email-log.txt

if printf '%s' "$tls" | egrep -q "0"
then
	cat /tmp/email-log.txt | sendmail $receiver 
else
	cat /tmp/email-log.txt | sendmail --tls-trust-file $tlscert $receiver 
fi

rm /tmp/email-log.txt
