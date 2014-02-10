/*
 * UTF-8 (with BOM) German-DE text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS (Upload) -- Klassifizierungsregeln";
qosStr.DRSection="QoS (Download) -- Klassifizierungsregeln";
qosStr.UCSection="QoS (Upload) -- Serviceklasse";
qosStr.DCSection="QoS (Download) -- Serviceklasse";
qosStr.DACCSect="QoS (Download) -- Active Congestion Control";
qosStr.UEnable="Dienstequalität aktivieren (Upload)";
qosStr.DEnable="Dienstequalität aktivieren (Download)";
qosStr.UTotBand="Totale (Upload) Bandbreite";
qosStr.DTotBand="Totale Download Bandbreite";

qosStr.USCAbout="Jede Upload-Dienst-Klasse wird durch drei Parameter festgelegt: prozentuale Auslastung der Bandbreite, Mindestbandbreite und maximale Bandbreite.";
qosStr.DSCAbout="Jede Dienst-Klasse wird durch vier Parameter festgelegt: percent bandwidth at capacity, realtime bandwidth and maximum bandwidth and the minimimze round trip time flag.";
qosStr.UMinBandAbout="<em>Mindestbandbreite</em> gibt das Minimum an, die der Dienst dieser Klasse beansprucht wenn die Verbindung an der Kapazitätsgrenze arbeitet. For certain applications like VoIP or online gaming it is better to specify a minimum service in bps rather than a percentage. QoS will satisfiy the minimum service of all classes first before allocating the remaining service to other waiting classes.";
qosStr.DMinBandAbout="<em>Mindestbandbreite</em> gibt das Minimum an, die der Dienst dieser Klasse beansprucht wenn die Verbindung an der Kapazitätsgrenze arbeitet. Classes which specify minimum service are known as realtime classes by the active congestion controller. Streaming video, VoIP and interactive online gaming are all examples of applications that must have a minimum bandwith to function. To determine what to enter use the application on an unloaded LAN and observe how much bandwidth it uses. Then enter a number only slightly higher than this into this field. QoS will satisfiy the minimum service of all classes first before allocating to other waiting classes so be careful to use minimum bandwidths sparingly.";
qosStr.UTotBandAbout="<em>Gesammt Upload Bandbreite</em> sollte auf etwa 98% der verfügbaren Upload Bandbreite eingestellt werden. Eine zu große Zahl hat zur folge dass QoS nicht die Anforderungen der Klassen sicherstellen kann. Eine zu kleine Zahl wird die Uploadgeschwindigkeit unnötig einschränken. Wenn Sie eine PPPoE Verbindung nutzen, rufen Sie die Webseite des modems auf und verwenden Ihre uplink-Verbindungsgeschwindigkeit für Ihre Upload-Bandbreite. Bei anderen Verbindungsarten kann ein Speed Test Programm (mit ausgeschaltetem QoS) helfen, die verfügbare Bandbreite zu ermitteln. Beachten Sie dass die Bandbreite in kbps angegeben wird. Dies sind 8 Kilobits für ein kilobyte.";
qosStr.DTotBandAbout="Die korrekte Angabe der <em>Gesammt Download Bandbreite</em>  ist entscheident für QoS. If you are using the active congestion then just set this to the maximum download speed your ISP will deliver. If you are using a PPPoE connection check your modem's webpage and set this to your DSL downlink speed.";
qosStr.PerBandAboutU="<em>Percent bandwidth at capacity</em> is the percentage of the total available bandwidth that should be allocated to this class when all available bandwidth is being used. If unused bandwidth is available, more can (and will) be allocated. The percentages can be configured to equal more (or less) than 100, but when the settings are applied the percentages will be adjusted proportionally so that they add to 100.";
qosStr.PerBandAboutD="<em>Percent bandwidth at capacity</em> is the percentage of the total available bandwidth that should be allocated to this class when all available bandwidth is being used. If unused bandwidth is available, more can (and will) be allocated. The percentages can be configured to equal more (or less) than 100, but when the settings are applied the percentages will be adjusted proportionally so that they add to 100. This setting only comes into effect when the WAN link is saturated.";
qosStr.RTTAbout="<em>Minimize RTT</em> indicates to the active congestion controller that you wish to minimize round trip times (RTT) when this class is active. Use this setting for online gaming or VoIP applications that need low round trip times (ping times). Minimizing RTT comes at the expense of efficient WAN throughput so while these class are active your WAN throughput will decline (usually around 20%).";
qosStr.MinSpeedWarn="If you are not using the ACC then you must establish what the minimum speed your ISP will deliver is and then set this number to that. In general ISPs do not provide a guaranteed minimum bandwith so it will take some experimentation and frustration on your part to arrive at a number. One approach is to start with a number which is half of what you think it should be and then test your link under full load and make sure everything works. Then increase it in steps, testing as you go until QoS starts to break down. You also may see that after your testing QoS works for a while and then stops working. This is because your ISP is getting overloaded due to demands from their other customers so they are no longer delivering to you the bandwidth they did during your testing. he solution, lower this number. Entering a number which is too high will result in QoS not meeting its class requirements. Entering a number which is too low will needlessly penalize your download speed. Because of all these complication I recommned you use the ACC when possible. Note that bandwidth is specified in kilobit/s. There are 8000 bits per kilobyte.";
qosStr.QoSAbout="Quality of Service (QoS) provides a way to control how available bandwidth is allocated. Connections are classified into different &ldquo;service classes,&rdquo; each of which is allocated a share of the available bandwidth. QoS should be applied in cases where you want to divide available bandwidth between competing requirements. For example if you want your VoIP phone to work correctly while downloading videos. Another case would be if you want your bit torrents throttled back when you are web surfing.";
qosStr.MaxBandAbout="<em>Maximale Bandbreite</em> gibt den absoluten maximalen Wert der Bandbreite in kbit/s an die diese Klasse nutzen kann. Auch wenn noch ungenutze Bandbreite verfügbar ist, wird dieser Serviceklasse nicht gestattet mehr als diese Bandbreite zu nutzen.";
qosStr.PackAbout="Packets are tested against the rules in the order specified -- rules toward the top have priority. As soon as a packet matches a rule it is classified, and the rest of the rules are ignored. The order of the rules can be altered using the arrow controls.";
qosStr.DefServClassAbout="Die <em>Standard Serviceklasse</em> gibt an wie Packete auf die keine Regel zutrifft eingestuft werden sollen.";
qosStr.AbACC="<p>The active congestion control (ACC) observes your download activity and automatically adjusts your download link limit to maintain proper QoS performance. ACC automatically compensates for changes in your ISP's download speed and the demand from your network adjusting the link speed to the highest speed possible which will maintain proper QoS function. The effective range of this control is between 15% and 100% of the total download bandwidth you entered above.</p><p>While ACC does not adjust your upload link speed you must enable and properly configure your upload QoS for it to function properly.</p><p><em>Ping Target-</em> The segment of network between your router and the ping target is where congestion is controlled. By monitoring the round trip ping times to the target congestion is detected. By default ACC uses your WAN gateway as the ping target. If you know that congestion on your link will occur in a different segment then you can enter an alternate ping target.</p><p><em>Manual Ping Limit-</em> Round trip ping times are compared against the ping limits. ACC controls the link limit to maintain ping times under the appropriate limit. By default Gargoyle attempts to automatically select appropriate target ping limits for you based on the link speeds you entered and the performance of your link it measures during initialization.  You cannot change the target ping time for the minRTT mode but by entering a manual time you can control the target ping time of the active mode.  The time you enter becomes the increase in the target ping time between minRTT and active mode. You can see the limits ACC is using in the [] brackets next to ping time limits field. </p>";
qosStr.ServClass="Standard Serviceklasse";

qosStr.AddNewClassRule="Neue Classification Regel einfügen";
qosStr.AddNewServiceRule="Neue Serviceklasse einfügen";
qosStr.SrcIP="Quell-IP";
qosStr.SrcPort="Quellport(s)";
qosStr.DstIP="Ziel-IP";
qosStr.DstPort="Zielport(s)";
qosStr.MaxPktLen="Maximale Paketlänge";
qosStr.MinPktLen="Minimale Paketlänge";
qosStr.TrProto="Transport Protokoll";
qosStr.Conreach="Connection bytes reach";
qosStr.AppProto="Anwendungsprotokoll (Layer7)";
qosStr.SetClass="Set Service Class To";
qosStr.SrvClassName="Name der Serviceklasse";
qosStr.PerBandCap="Percent Bandwidth At Capacity";
qosStr.BandMin="Minimale Bandbreite";
qosStr.BandMinNo="Keine minimale Bandbreite";
qosStr.BandMax="Maximale Bandbreite";
qosStr.BandMaxNo="Keine maximale Bandbreite";
qosStr.MinRTT="Minimize Round Trip Times (RTT)";
qosStr.ActRTT="Minimize RTT (ping times) when active";
qosStr.OptiWAN="Optimize WAN utilization";
qosStr.ACCOn="Enable active congestions control (Download Direction)";
qosStr.ACC_Pt="Use non-standard ping target";
qosStr.ACC_con="Manually control target ping time";
qosStr.ACC_Stat="Congestion Control Status";
qosStr.ACC_L_Ck="Check to see if the ping target will respond";
qosStr.ACC_L_In="Estimate a ping limit";
qosStr.ACC_L_Act="Congestion control active.";
qosStr.ACC_L_Min="Congestion control active, minRTT class active.";
qosStr.ACC_L_Id="No Congestion, control idle.";
qosStr.ACC_L_Dis="Controller is not enabled";
qosStr.ACC_L_Lim="The download bandwidth limit currently enforce.";
qosStr.ACC_L_Fr="The apparent fair download bandwidth limit.";
qosStr.ACC_L_Ld="The current traffic in the downlink.";
qosStr.ACC_L_pg="The round trip time of the last ping.";
qosStr.ACC_L_Flt="The round trip time filtered.";
qosStr.ACC_L_plim="The point at which the controller will act to maintain fairness.";
qosStr.ACC_L_AC="Number of download classes with load over 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Editiere QoS Serviceklasse";
qosStr.QESrvName="Name der Serviceklasse";

//qos_edit_rule.sh
qosStr.QERulClass="Editiere QoS Klassifizierungsregeln";

//javascript
qosStr.MatchC="Match Criteria";
qosStr.Classn="Klassifizierung";
qosStr.Src="Quelle";
qosStr.SrcP="Quellport";
qosStr.Dst="Ziel";
qosStr.DstP="Zielport";
qosStr.Connb="Connection bytes";
qosStr.APro="Anwendungsprotokoll";
qosStr.pBdW="Percent BW";
qosStr.mBdW="Min BW";
qosStr.MBdW="Max BW";
qosStr.qLd="Load";
qosStr.CrErr="No match criteria have been selected.";
qosStr.SvErr="Could not add new service class.";
qosStr.SUErr="Could not update service class.";
qosStr.CsErr="Could not add classification rule.";
qosStr.CUErr="Could not update classification rule.";
qosStr.DCErr="Duplicate class name.";
qosStr.RemSCErr="At least one service class is required.\nCannot remove service class.";
qosStr.TotErr="There is an error in Total Bandwidth field.\n\nCould not update QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="nolimit";
qosStr.ZERO="zero";
qosStr.YES="Yes";

//qos_distribution.sh
qosStr.UBSect="QoS Upload Bandwidth Distribution";
qosStr.DBSect="QoS Download Bandwidth Distribution";
qosStr.uTFrm="Upload Time Frame";
qosStr.dTFrm="Download Time Frame";
