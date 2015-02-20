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

qosStr.USCAbout="Jede Upload-Dienst-Klasse wird durch drei Parameter festgelegt: prozentuale Bandbreite, Mindestbandbreite und maximale Bandbreite.";
qosStr.DSCAbout="Jede Dienst-Klasse wird durch vier Parameter festgelegt:  prozentuale Bandbreite, Mindestbandbreite, maximale Bandbreite und das minRTT-Flag.";
qosStr.UMinBandAbout="<em>Mindestbandbreite</em> gibt das Minimum an, die der Dienst dieser Klasse beansprucht wenn die Verbindung an der Kapazitätsgrenze arbeitet. Für einige Anwendungen wie VoIP oder Onlinespiele ist es besser eine absolute, minimale Bandbreite in bps statt eines prozentuellen Wertes anzugeben. QoS sorgt dafür, das die minimale Bandbreite in bps zuerst erfüllt wird, bevor die restliche Bandbreite aufgeteilt wird.";
qosStr.DMinBandAbout="<em>Mindestbandbreite</em> gibt das Minimum an, die der Dienst dieser Klasse beansprucht wenn die Verbindung an der Kapazitätsgrenze arbeitet. Klassen die eine Mindestbandbreite spezifizieren werden durch ACC als Echtzeitklassen behandelt. Videostreaming, VoIP und interaktive Onlinespiele sind Beispiele für Anwendungen die eine Mindestbandbreite für eine ordnungsgemäße Funktion benötigen. Zur Ermittlung der Mindestbandbreite einer Anwendung kann man diese im unbelasteten LAN nutzen und die genutzte Bandbreite ermitteln. Eingetragen werden sollte ein Wert der etwas höher als der so ermittelte Wert ist. QoS wird die Mindestbandbreite aller Klasse zuerst erfüllen, bevor andere Klassen bedient werden. Es sollte daher sparsam eingesetzt werden.";
qosStr.UTotBandAbout="<em>Gesamte Upload Bandbreite</em> sollte auf etwa 98% der verfügbaren Upload Bandbreite eingestellt werden. Eine zu große Zahl hat zur Folge dass QoS nicht die Anforderungen der Klassen sicherstellen kann. Eine zu kleine Zahl wird die Uploadgeschwindigkeit unnötig einschränken. Wenn Sie eine PPPoE Verbindung nutzen, rufen Sie die Webseite des Modems auf und verwenden Ihre Uplink-Verbindungsgeschwindigkeit für Ihre Upload-Bandbreite. Bei anderen Verbindungsarten kann ein Speed Test Programm (mit ausgeschaltetem QoS) helfen, die verfügbare Bandbreite zu ermitteln. Beachten Sie dass die Bandbreite in kbps angegeben wird. Dies sind 8 Kilobits für ein Kilobyte.";
qosStr.DTotBandAbout="Die korrekte Angabe der <em>Gesammt Download Bandbreite</em>  ist entscheidend für QoS. Wenn Sie Active Congestion Control verwenden, setzen Sie diesen Wert auf die maximale Download-Geschwindigkeit Ihres ISP. Bei PPPoE finden Sie den Wert u.U. auf der Webseite des Modems.";
qosStr.PerBandAboutU="<em>Prozentuale Bandbreite</em> gibt den Anteil an der total verfügbaren Bandbreite bei voller Auslastung, welche dieser Klasse zugewiesen werden soll. Wenn ungenutzte Bandbreite vorhanden ist, kann (und wird) diese zusätzlich zugewiesen. Die Summe der Prozente darf mehr (oder weniger) als 100 betragen. In diesem Fall wird der Wert proportional angepasst.";
qosStr.PerBandAboutD="<em>Prozentuale Bandbreite</em> gibt den Anteil an der total verfügbaren Bandbreite bei voller Auslastung, welche dieser Klasse zugewiesen werden soll. Wenn ungenutzte Bandbreite vorhanden ist, kann (und wird) diese zusätzlich zugewiesen. Die Summe der Prozente darf mehr (oder weniger) als 100 betragen. In diesem Fall wird der Wert proportional angepasst. Diese Einstellung hat nur einen Effekt, wenn der WAN-Link ausgelastet ist";
qosStr.RTTAbout="<em>Minimiere RTT</em> gibt an, das ACC versuchen soll die Rundtripzeit für diese Klasse zu minimieren. Diese Einstellung ist für Onlinespiele und VoIP, welche möglichst kleine Rundtripzeiten (Ping-Zeit) benötigen. Dies geht auf Kosten der nutzbaren WAN-Bandbreite. Während diese Klassen aktiv sind, wird der WAN-Durchsatz reduziert (um etwa 20%).";
qosStr.MinSpeedWarn="Wenn Sie ACC nicht nutzen, müssen Sie die minimale Geschwindigkeit die Ihnen ihr ISP liefert ermitteln und eintragen. Normalerweise garantiert ein ISP keine minimale Bandbreite, so das Sie den Wert experimentell ermitteln müssen. Ein Ansatz dazu ist mit etwa der Hälfte der gedachten Geschwindigkeit zu starten und dann den Link unter voller Last zu testen. Anschliessend wird die Geschwindigkeit in kleinen Schritten erhöht, bis es nicht mehr funktioniert. Es kann vorkommen, das QoS nach einer Weile (oder auch sporadisch) aufhört zu funktionieren. Dies passiert, wenn die minimale Geschwindigkeit z.B. aufgrund von Überlastung des ISP oder durch Störungen auf der DSL-Leitung kleiner als während der Tests wird. Zur Behebung muss die eingetragen minimale Geschwindigkeit reduziert werden. Wird ein zu hoher Wert eingetragen funktioniert QoS nicht. Ein zu kleiner Wert reduziert grundlos die erreichbare Downloadgeschwindigkeit. Daher ist es empfohlen ACC zu nutzen. Hinweis: Die Bandbreite wird in Kilobit/s angegeben. Das sind 8 Kilobits je Kilobyte.";
qosStr.QoSAbout="Quality of Service (QoS) erlaubt die Kontrolle, wie die zur Verfügung stehende Bandbreite genutzt wird. Verbindungen werden in verschiedene &ldquo;Serviceklassen&rdquo; eingeordnet. Jeder einzelnen Serviceklasse wird ein entsprechender Anteil der Bandbreite zugewiesen. QoS sollte verwendet, wenn Sie die Bandbreite zwischen sich widersprechenden Anforderungen aufteilen wollen. Zum Beispiel können Sie sicherstellen, das Ihr VoIP-Telefon auch bei großen Downloads weiterhin korrekt funktioniert. Ein anderes Beispiel ist die Drosselung von Bit-Torrents beim surfen.";
qosStr.MaxBandAbout="<em>Maximale Bandbreite</em> gibt den absoluten maximalen Wert der Bandbreite in kbit/s an die diese Klasse nutzen kann. Auch wenn noch ungenutze Bandbreite verfügbar ist, wird dieser Serviceklasse nicht gestattet mehr als diese Bandbreite zu nutzen.";
qosStr.PackAbout="Pakete werden in der Reihenfolge der Regeln klassifiziert -- Regeln am Anfang haben dabei Priorität. Sowie ein Paket klassifiziert werden konnten, werden die restlichen Regeln ignoriert. Die Reihenfolge der Regeln kann mit den Pfeilen abgeändert werden.";
qosStr.DefServClassAbout="Die <em>Standard Serviceklasse</em> gibt an wie Packete auf die keine Regel zutrifft eingestuft werden sollen.";
qosStr.AbACC="<p>Die Active Congestion Control (ACC) beobachtet die Download-Aktivität und passt die maximal verfügbare Bandbreite automatisch an. ACC kompensiert Änderung der Downloadgeschwindigkeit durch den ISP mit dem Ziel eine maximale Downloadgeschwindigkeit bei funktionierendem QoS zu gewährleisten. Der Regelbereich liegt im Bereich zwischen 15% und 100% der oben eingetragenen totalen Download Bandbreite.</p><p>ACC passt die Upload-Geschwindigkeit nicht an und muss manuell entsprechend konfiguriert werden.</p><p><em>Ping Ziel-</em>Der Bereich zwischen Ihrem Router und dem Ping-Ziel ist der Bereich in dem die Auslastung kontrolliert wird, indem die Rundtripzeit zum Ping-Ziel gemessen wird. Normalerweise nutzt ACC das WAN-Gateway des Providers.</p><p><em>Manuelles Ping-Limit:</em> Rundtripzeiten werden mit dem angegebenen Limit verglichen. ACC versucht die Rundtripzeit unterhalb dieses Limits zu halten. Standardmäßig versucht Gargoyle dieses Limit automatisch anhand der eingetragenen Geschwindigkeit und einer Performancemessung während der Initialisierung  zu ermitteln. Sie können den Wert im minRTT-Modus nicht ändern. Durch eintragen einer manuellen Zeit wird das Limit im aktiven Modus kontrolliert. Die eingetragene Zeit erhäöht das Limit zwischen minRTT- und aktiven Modus. Die aktuellen Limits sind in den []-Klammern neben dem Ping-Limit-Feld.</p>";
qosStr.ServClass="Standard Serviceklasse";

qosStr.AddNewClassRule="Neue Klassifizierungsregel einfügen";
qosStr.AddNewServiceRule="Neue Serviceklasse einfügen";
qosStr.SrcIP="Quell-IP";
qosStr.SrcPort="Quellport(s)";
qosStr.DstIP="Ziel-IP";
qosStr.DstPort="Zielport(s)";
qosStr.MaxPktLen="Maximale Paketlänge";
qosStr.MinPktLen="Minimale Paketlänge";
qosStr.TrProto="Transport Protokoll";
qosStr.Conreach="Transferierte Datenmenge erreicht";
qosStr.AppProto="Anwendungsprotokoll (Layer7)";
qosStr.SetClass="Setze Serviceklasse auf";
qosStr.SrvClassName="Name der Serviceklasse";
qosStr.PerBandCap="Prozentuale Bandbreite";
qosStr.BandMin="Minimale Bandbreite";
qosStr.BandMinNo="Keine minimale Bandbreite";
qosStr.BandMax="Maximale Bandbreite";
qosStr.BandMaxNo="Keine maximale Bandbreite";
qosStr.MinRTT="Minimiere Rundtripzeiten (RTT)";
qosStr.ActRTT="Minimiere RTT (Ping-Zeiten) wenn aktiv";
qosStr.OptiWAN="Optimiere WAN Auslastung";
qosStr.ACCOn="Aktiviere Active Congestions Control (Download)";
qosStr.ACC_Pt="Definiere Ping-Ziel manuell";
qosStr.ACC_con="Manually control target ping time";
qosStr.ACC_Stat="Congestion Control Status";
qosStr.ACC_L_Ck="Prüfe, ob das Ping-Ziel antwortet";
qosStr.ACC_L_In="Ermittle ein Ping-Limit";
qosStr.ACC_L_Act="Congestion control aktiv.";
qosStr.ACC_L_Min="Congestion control aktiv, minRTT class aktiv.";
qosStr.ACC_L_Id="Keine Überlastung, Control inaktiv.";
qosStr.ACC_L_Dis="Controller ist nicht aktiviert";
qosStr.ACC_L_Lim="The download bandwidth limit currently enforce.";
qosStr.ACC_L_Fr="The apparent fair download bandwidth limit.";
qosStr.ACC_L_Ld="Aktuelle Downloadgeschwindigkeit.";
qosStr.ACC_L_pg="Rundtripzeit des letzen Ping.";
qosStr.ACC_L_Flt="Gemittelte Rundtripzeit.";
qosStr.ACC_L_plim="The point at which the controller will act to maintain fairness.";
qosStr.ACC_L_AC="Anzahl der Downloadklassen mit mehr als 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Editiere QoS Serviceklasse";
qosStr.QESrvName="Name der Serviceklasse";

//qos_edit_rule.sh
qosStr.QERulClass="Editiere QoS Klassifizierungsregeln";

//javascript
qosStr.MatchC="Vergleichskriterium";
qosStr.Classn="Klassifizierung";
qosStr.Src="Quelle";
qosStr.SrcP="Quellport";
qosStr.Dst="Ziel";
qosStr.DstP="Zielport";
qosStr.Connb="Transferierte Datenmenge";
qosStr.APro="Anwendungsprotokoll";
qosStr.pBdW="Prozentuale Bandbreite";
qosStr.mBdW="Min Bandbreite";
qosStr.MBdW="Max Bandbreite";
qosStr.qLd="Auslastung";
qosStr.CrErr="Es wurden keine Vergleichskriterien ausgewählt:";
qosStr.SvErr="Serviceklasse konnte nicht hinzugefügt werden.";
qosStr.SUErr="Serviceklasse konnte nicht aktualisiert werden.";
qosStr.CsErr="Klassifizierungsregel konnte nicht hinzugefügt werden.";
qosStr.CUErr="Klassifizierungsregel konnte nicht aktualisiert werden.";
qosStr.DCErr="Name der Serviceklasse existiert schon.";
qosStr.RemSCErr="Mindestens ein Serviceklasse wird benötigt.\nServiceklasse kann nicht entfernt werden.";
qosStr.TotErr="Fehler im Feld Totale Bandbreite.\n\nKonnte QoS nicht aktualisieren.";

//one-word strings used in rules
qosStr.NOLIMIT="unbegrenzt";
qosStr.ZERO="null";
qosStr.YES="Ja";

//qos_distribution.sh
qosStr.UBSect="QoS Upload Bandbreitenverteilung";
qosStr.DBSect="QoS Download Bandbreitenverteilung";
qosStr.uTFrm="Upload Zeitraum";
qosStr.dTFrm="Download Zeitraum";
