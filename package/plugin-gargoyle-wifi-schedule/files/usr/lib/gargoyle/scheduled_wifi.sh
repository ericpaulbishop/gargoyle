#!/bin/sh

case "$1" in
	down) logger -t wifi-schedule: crontab event to take wifi down
		  /sbin/wifi down
          ;;
	up)   logger -t wifi-schedule: crontab event to bring wifi up
		  /sbin/wifi		  
		  ;;
esac
