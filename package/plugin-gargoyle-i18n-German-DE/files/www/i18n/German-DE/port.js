/*
 * UTF-8 (with BOM) German-DE text strings for port_forwarding(single,multi).sh html elements
 */

prtS.PISect="Individuelle Portweiterleitungen";
prtS.PRSect="Weiterleitung eines Portbereichs";
prtS.ForIPort="Einzelnen Port vom WAN ins LAN weiterleiten";
prtS.ForRPort="Portbereich vom WAN ins LAN weiterleiten";
prtS.DMZ="DMZ";
prtS.UseDMZ="DMZ (De-Militarisierte Zone) nutzen";
prtS.DMZIP="DMZ IP";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="UPnP &amp; NAT-PMP Dienst aktivieren";
prtS.APFor="Aktive Portweiterleitungen";
prtS.USpd="Upload-Geschwindigkeit";
prtS.DSpd="Download-Geschwindigkeit";
prtS.UPHelp="UPnP (Universal Plug and Play) and NAT-PMP (NAT Port Mapping Protocol) are both protocols which allows devices and applications on your LAN to automatically configure your router with the port forwards needed for proper operation. If a device supports either protocol it is not necessary to create manual port forward rules (see the top of this page) as they will automatically be created by the device.</p><p>When enabled Gargoyle shows a table of automatically created port forwards so you can see which devices have requested forwards and verify that this feature is working properly. This service may not work correctly in network configurations containing two or more routers (double NAT). If you see a single row with '***' it means there are no port forwards registered.</p><p>As part of the protocol the LAN device can request the speed of the WAN connection from the router. Two fields are provided to configure the response to such queries. Client applications can use this information to optimize their performance. But is important to note that the router does not do anything to limit speeds based on this data.  It is only reported to the requester. If zero is entered for either value the speed of the interface is reported, usually 100MB or 1GB depending on the router’s interface speed.</p> <p>There is some controversy about the security of this service and it does require additional RAM to run which may be important on memory constrained routers, so by default this feature is off.";

//templates
prtS.Desc="Beschreibung";
prtS.optl="(optional)";
prtS.Proto="Protokoll";
prtS.FPrt="Quellport";
prtS.TIP="Ziel-IP";
prtS.TPrt="Zielport";
prtS.SPrt="Startport";
prtS.EPrt="Endport";

//javascript
prtS.AFRErr="Weiterleitung konnte nicht eingerichtet werden.";
prtS.GTErr="Startport > Endport";
prtS.DupErr="Ein(ige) Port(s) im Bereich werden schon weitergeleitet";
prtS.CopErr="Port wurde schon weitergeleitet";
prtS.UpErr="Portweiterleitung konnte nicht aktualisiert werden.";
prtS.Prot="Proto";
prtS.LHst="LAN Rechner";
prtS.Port="Port";

//edit.sh pages
prtS.PESect="Portweiterleitung editieren";
