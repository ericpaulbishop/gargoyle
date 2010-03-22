Compiling FonFlash for OSX / Linux should be easy, just run make.  Make sure you 1) have libpcap installed and 2) have an internet connection so wxWindows can be downloaded and configured for static linkage (so you only need it if you're doing the building).  

If you're trying to compile FonFlash for Windows... I pity you.  You need to install both wxWidgets and WinPcap.  Because I'm a lazy bum, the vcproj file has the locations of these on MY system, but probably not on yours.  You'll have to adjust them for the locations you installed wxWidgets/WpdPack.  For reference on my system wxWidgets is in c:\wxWidgets-2.8.9 and WpdPack is in C:\WpdPack.  Have fun and good luck compiling.  You'll need it.

Running fon flash, on the other hand, is easy everywhere.  You do need to have pcap installed (WinPcap for windows) and an ethernet cord plugged into your ethernet port and connected to the Fon.  That, along with the firmware files should be all you need.  The GUI should be pretty self-explanatory.
 
