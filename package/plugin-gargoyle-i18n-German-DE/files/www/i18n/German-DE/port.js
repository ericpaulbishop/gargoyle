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
prtS.UPHelp="UPnP (Universal Plug and Play) und NAT-PMP (NAT Port Mapping Protocol) sind beides Protokolle die es Geräten und Programmen in Ihrem Netzwerk erlauben Ihren Router mit den für ihren ordnungsgemäßen Betrieb benötigten Port-Weiterleitungen automatisch zu konfigurieren. Wenn ein Gerät dieses Protokoll unterstützt ist es nicht nötig die Port-Weiterleitungsregeln von Hand zu erstellen (siehe oben auf dieser Seite), da sie automatisch von dem Gerät erstellt werden.</p><p>Wenn aktiviert, zeigt Gargoyle eine Tabelle der automatisch erstellten Port-Weiterleitungen an, so dass Sie sehen können welche Geräte Weiterleitungen angefordert haben und sicherstellen können das diese Funktion ordnungsgemäß funktioniert. Dieser Dienst wird möglicherweise in einer Netzwerkkonfiguration mit mehreren Routern nicht ordnungsgemäß funktionieren (mehrfaches NAT). Wenn Sie eine einzelne Zeile mit '***' sehen bedeutet dies, dass keine Port-Weiterleitungen eingetragen sind.</p><p>Als Teil des Protokolls kann das Netzwerkgerät die Geschwindigkeit der WAN-Verbindung vom Router erfragen. Zwei Felder sind dazu vorgesehen die Reaktion auf solche Anfragen einzusstellen. Die Computer-Anwendungsprogramme können diese Information nutzen um ihre Geschwindigkeit zu optimieren. Aber es ist wichtig zu beachten, dass der Router aufgrund dieser Daten nicht alles macht um die Geschwindigkeit zu begrenzen.  Es wird nur dem Anfragenden mitgeteilt. Wenn nichts eingetragen ist, wird die Geschwindigkeit der Netzwerkschnittstelle zurückgemeldet, abhängig von der Geschwindigkeit des Netzwerkschnittstelle des Router ist dies normalerweise 100MB oder 1GB.</p> <p>Es gibt hinsichtlich der Sicherheit des Dienstes unterschiedliche Ansichten und es belegt zusätzlichen Platz im Arbeitsspeicher, was bei Routern mit wenig RAM wichtig sein kann. Deshalb ist diese Funktion in der Voreinstellung abgeschaltet.";

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
