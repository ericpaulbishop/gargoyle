/*
 * UTF-8 (with BOM) English-EN text strings for port_forwarding(single,multi).sh html elements
 */

portStr.PISect="Individual Port Forwarding";
portStr.PRSect="Port Range Forwarding";
portStr.ForIPort="Forward Individual Ports From WAN to LAN";
portStr.ForRPort="Forward Port Range From WAN to LAN";
portStr.DMZ="DMZ";
portStr.UseDMZ="Use DMZ (De-Militarized Zone)";
portStr.DMZIP="DMZ IP";
portStr.UP_NAT="UPnP / NAT-PMP";
portStr.UPNAT_En="Enable UPnP &amp; NAT-PMP service";
portStr.APFor="Active port forwards";
portStr.USpd="Upload speed to report";
portStr.DSpd="Download speed to report";
portStr.scnd="second";
portStr.UPHelp="UPnP (Universal Plug and Play) and NAT-PMP (NAT Port Mapping Protocol) are both protocols which allows devices and applications on your LAN to automatically configure your router with the port forwards needed for proper operation. If a device supports either protocol it is not necessary to create manual port forward rules (see the top of this page) as they will automatically be created by the device.</p><p>When enabled Gargoyle shows a table of automatically created port forwards so you can see which devices have requested forwards and verify that this feature is working properly. This service may not work correctly in network configurations containing two or more routers (double NAT). If you see a single row with '***' it means there are no port forwards registered.</p><p>As part of the protocol the LAN device can request the speed of the WAN connection from the router. Two fields are provided to configure the response to such queries. Client applications can use this information to optimize their performance. But is important to note that the router does not do anything to limit speeds based on this data.  It is only reported to the requester. If zero is entered for either value the speed of the interface is reported, usually 100MB or 1GB depending on the router’s interface speed.</p> <p>There is some controversy about the security of this service and it does require additional RAM to run which may be important on memory constrained routers, so by default this feature is off.";

//templates
portStr.Desc="Description";
portStr.optl="(optional)";
portStr.Proto="Protocol";
portStr.FPrt="From Port";
portStr.TIP="To IP";
portStr.TPrt="To Port";
portStr.SPrt="Start Port";
portStr.EPrt="End Port";

//javascript
portStr.AFRErr="Could not add forwarding rule.";
portStr.GTErr="Start Port > End Port";
portStr.DupErr="Port(s) Within Range Is/Are Already Being Forwarded";
portStr.CopErr="Port Is Already Being Forwarded";
portStr.UpErr="Could not update port forward.";
portStr.Prot="Proto";
portStr.LHst="LAN Host";
portStr.Port="Port";

//edit.sh pages
portStr.PESect="Edit Port Forward";
