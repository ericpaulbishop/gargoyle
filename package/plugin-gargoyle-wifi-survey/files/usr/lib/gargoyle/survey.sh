#!/bin/sh

newSurvey=/tmp/tmp_survey.txt
oldSurvey=/tmp/survey_data.txt
now=$(date "+%Y%m%d%H%M")
iwps=`ps | grep iwlist | grep -v grep`
memb=1

if [ ! -z "$iwps" ] || [ -e "$newSurvey" ] ; then
	exit
fi

echo "var sdata = new Array();" > "$newSurvey"

#searches, enrobes in quotes & prepares for javascript injection
iwlist scan 2>/dev/null | awk -v ts="$now" -F '[ :=]+' '/Address/{printf substr($0,30)"\",\""ts"\",\""}/Channel:/{printf $3"\",\""} /Freq/{printf $3"\",\""} /Quality/{printf $3"\",\""$6"\",\""} /Encr/{printf $4"\","} /ESS/{printf substr($0,27)",["} /Rates:/{printf $(NF-1)","} /Mode:/{printf "],\""$3"\"\n"} /IEEE 802.11i/ {split($4,a,"/"); printf ",[\""a[2]"\",\""} /WPA Version 1/ {printf ",[\"WPA1\",\""} /Group Cipher/{printf $4"\",\""} /Suites/{printf $5"\"]\n"}' | tr '\n' '\002' | awk '{gsub(/\002,\[/," ,["); printf"%s",$0}' | awk '{gsub(/\002/,"\n");printf"%s",$0}' | sed 's/^[^$]*$/sdata.push(\[\"&\]);/' | sed 's/,\],/\],/g' >> "$newSurvey"

iwps=`ps | grep iwlist | grep -v grep`
if [ ! -z "$iwps" ] ; then   #minimize impact of repeated webpage loadings
	exit
fi

while true; do
	if [ ! -e "$oldSurvey" ] ; then
		break #ah, our first time - its special
	fi
	aline=$(awk -v rec=$memb 'NR==rec {print $0}' "$oldSurvey")
	if [ -z "$aline" ] ; then
		break
	fi 
	
	amac=`echo "$aline" | awk -F '\"' '{print $2}'`
	ats=`echo "$aline" | awk -F '\"' '{print $4}'`
	if [ ! -z "$amac" ] ; then
		curr_mac=`grep -e "$amac" "$newSurvey"`
		
		if [ -z "$curr_mac" ] ; then
			if [ $(expr $now - $ats) -lt 450000 ] ; then
				echo $aline >> "$newSurvey"
			fi
		fi
	fi
	let memb++
done
iwps=`ps | grep iwlist | grep -v grep`
if [ ! -z "$iwps" ] ; then   #minimize impact of repeated webpage loadings
	exit
fi
mv -f "$newSurvey" "$oldSurvey"
cat "$oldSurvey"
