/*
TODO:
¥ tell user how to get missing OUIs.js
¥ figure out how to package OUIs.js for install on usb first/ ram if ram > 32? or so?

ENHANCEMENTS:
Long text fields could use some auto horizontal scrolling
Display wifi status when wifi is down (to telling when it could go back up). Things not changing for 1/2 hour may cause concern.
*/

/* in this table:

  Data is scraped from stock 'iwlist scan' output & harvested to a javascript array using /usr/lib/gargoyle/survey.sh
  survey.sh regurgitates data at the end of the run when the webpage tells it to run.
  iwlist takes some time, so there is a delay from table drawing to updated data. Sorry, asynchronous javascript to blame.
  Quality is represented as the bar on right, color coded for <.333, .333 - .666, >.666

  Vendor lookup starts with a 2.8MB file from here: http://standards.ieee.org/develop/regauth/oui/oui.txt
  Data is scraped with this command:
  
	echo "var vdr=new Array();" > ~/Desktop/OUIs.js && grep -e "(base 16)" ~/Desktop/oui.txt | sed 's/\"/\\\"/g' | sed 's/\//\\\//g' | awk '{printf "vdr.push([\""$1"\",\""} {for(i=4;i<=NF;++i) printf("%s",  $i) } {printf "\"]);\n"}' >> ~/Desktop/OUIs.js
	
  OUIs.js is now 775kb & chock full o' vendors (17,500+ lines) in a javascript array; no 775kb file should go onto the router flash chip, but... RAM is a different story. Take that OUIs.js and copy it to /tmp and the wifi_survey.sh webpage will pull it from the /tmp directory & present it to the browser. Or on an attached USB drive.

*/

/* version history
v1.0 	initial release
*/