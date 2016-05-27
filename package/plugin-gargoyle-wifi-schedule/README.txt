/*
TODO:
• handle (not currently used, but some user might manually edit) crontabs with   * / 2   1,2,3,4,5   1-4,5 hours or days
• bring wifi up/down should timeout so the windoid doesn't spin endlessly

ENHANCEMENTS:
A disclosure triangle to show actual crontabs - I don't think is needed.
  It would just confuse new users when the summary presents a natural language representation of the crontabs.
*/

/* in this table:
	+minutes represent an hour where wifi is up UNTIL that minute, where it goes down for the remainder of the hour
	-minutes represent an hour where wifi is down UNTIL that minute, where it goes up for the remainder of the hour
	 (- for when the hour starts wifi-off and some minutes later turns on)
	 (+ for when the hour starts wifi-on and some minutes later turns on)
	
	all tables *display* positive minutes, but their .value may be positive or negative or 0 (fully off for the hour) or 60 (fully on)
	an hour can only have negative minutes coming after an hour that was off (meaning this hour will go up) OR
	 or after an hour that has positive minutes (@12:+05 goes down, up @ 13:-45)
	
	an hour can only have positive minutes coming after an hour that was on (meaning this hour will go down at some minutes) OR
	 or after an hour that has positive minutes (was up at 23:+05, down @ 00:-55)
*/

/* version history
v1.0 	initial release
v1.1 	transition to storing positive/negative minutes in each table cell
		display wifi status based on iwconfig & evaluate current time to find the wifi status @ current time
		added diagonal gradients for cells with 1-59 minutes (Safari/Chrome/Firefox)
		fix cycling issues by forcing/keeping specific events
		check wifi every 5 seconds to update status against a stripped down cloned array
		added warnings & manual wifi buttons
		added even day crontab to delete /tmp/cron-(datestamp).backup files older than 7 days
		...and revert back to not saving full backup crontabs & deleting every 7 days; instead just 1 backup of system (non-wifi) crontabs
v1.1.1	write temp crontabs to emptied file
		opkg removal removes scheduled_wifi.sh crontabs
		uci inject webpage after USB plugins
		fixed issue of displaying wifi status during an hour where wifi goes up/down on XX minutes
v1.1.2	fix correlating current time to schedule when displaying wifi status (off by 1 day)
		minor beautification + move legend to top of fieldset
		
	r2	bugfix displaying wifi radio status when scheduled to go down during the current hour
*/
