/*
 * UTF-8 (with BOM) Slovak-SK text strings for ddns.sh html elements
 */

DyDNS.mDDNS="Dynamická DNS";
DyDNS.DYSect="Služby dynamickej DNS";
DyDNS.AddDy="Pridať službu dynamickej DNS";
DyDNS.SvPro="Poskytovateľ služby";
DyDNS.ChItv="Interval kontroly";
DyDNS.FUItv="Interval vynútenia aktualizácie";
DyDNS.AddDDNS="Prodať službu DDNS";
DyDNS.HelpCI="<em>Interval kontroly</em> určuje ako často router skontroluje, či Vaša aktuálna IP sa zhoduje s IP aktuálne previazanou s Vašim názvom domény. Táto kontrola sa vykonáva bez pripojenia k Vašemu poskytovateľovi služby dynamického DNS, čo znamená, že to nebude spôsobovať problémy s poskytovateľmi ktorí blokujú užívateľov ktorí sa pripájajú príliš často (napr. dyndns.com). Avšak z dôvodu vykonania kontroly sa vytvára spojenie, takže táto hodnota by nemala byť príliš nízka.  Interval kontroly v rozmendzí 10 až 20 minút je zvyčajne dotačujúci.";
DyDNS.HelpFI="<em>Interval vynútenia aktualizácie</em> určuje, ako často sa router pripojí k Vášmu poskytovateľa služieb dynamického DNS a aktualizuje jeho záznamy, aj keď sa Vaša IP nezmenila. Poskytovatelia služieb blokujú užívateľov, ktorí aktualizujú príliš často, ale môžu zrušiť účty užívateľov, ktorí neaktualizujú dlhšie ako mesiac.  Odporúča sa nastaviť tento parameter na rozmedzí 3 až 7 dní.";
DyDNS.UpErr1="Aktualizácia novej konfigurácie služby dynamickej DNS zlyhala";
DyDNS.UpErr2="Služba(y) nemohla byť správne aktualizovaná a bola preto odstránená.";
DyDNS.cNams=["Doména", "Poskytovateľ služby", "Posledná aktualizácia", "Povolené", "", "" ];
DyDNS.InvErr="CHYBA: uvedený poskytovateľ je neplatný";
DyDNS.DupErr="Uvedená duplicitná aktualizácia.";
DyDNS.ForceU="Vynútiť aktualizáciu";
DyDNS.ModErr="Táto služba bola pridaná/upravená a preto je nutné uložiť Vaše zmeny pred vykonaním aktualizácie. Kliknite \""+UI.SaveChanges+"\" a skúste znova.";
DyDNS.UpFErr="Aktualizácia zlyhala. Skontrolujte, či Vaša konfigurácia je platná a či ste pripojení k internetu.";
DyDNS.UpOK="Aktualizácia bola úspešná.";
DyDNS.UpSrvErr="Nepodarilo sa aktualizovať triedu služby.";

//ddns_edit.sh
DyDNS.EDSect="Úprava služby dynamickej DNS";

// /etc/ddns_providers.conf
DyDNS.DoNm="Názov domény";
DyDNS.UsrN="Názov užívateľa";
DyDNS.Pssw="Heslo";
DyDNS.Eml="E-mail";
DyDNS.Key="Kľúč";
DyDNS.AKey="Kľúč API";
DyDNS.Tokn="Token";
