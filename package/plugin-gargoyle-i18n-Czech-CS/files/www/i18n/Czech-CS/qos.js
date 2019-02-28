/*
 * UTF-8 (with BOM) Czech-CS text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.mQDl="QoS (stahování)";
qosStr.mQUl="QoS (nahrávání)";
qosStr.URSection="QoS (nahrávání) - klasifikační pravidla";
qosStr.DRSection="QoS (stahování) - klasifikační pravidla";
qosStr.UCSection="QoS (nahrávání) - servisní třídy";
qosStr.DCSection="QoS (stahování) - servisní třídy";
qosStr.DACCSect="QoS (stahování) - Aktivní ovládání zahlcení";
qosStr.UEnable="Povolit kvalitu služby (směr nahrávání)";
qosStr.DEnable="Povolit kvalitu služby (směr stahování)";
qosStr.UTotBand="Celková šířka pásma pro nahrávání";
qosStr.DTotBand="Celková šířka pásma pro stahování";

qosStr.USCAbout="Každá servisní třída pro nahrávání je specifikována třemi parametry: procento kapacity šířky pásma, minimální šířka pásma a maximální šířka pásma.";
qosStr.DSCAbout="Každá servisní třída je určena čtyřmi parametry: procento kapacity šířky pásma, šířka pásma v reálném čase, maximální šířku pásma a minimalizovaný zpáteční časový příznak.";
qosStr.UMinBandAbout="<em>Minimální šířka pásma</em> stanovuje minimální službu která této třídě bude přidělena, když je linka využita na plný výkon. Pro určité aplikace, jako jsou VoIP nebo hraní online her je lepší stanovit minimální službu v bps , spíše než procento. QoS uspokojí nejprve minimální službu všech tříd, před přidělením zbývající služby ostatních čekajících tříd.";
qosStr.DMinBandAbout="<em>Minimální šířka pásma</em> stanovuje minimální službu která této třídě bude přidělena, když je linka využita na plný výkon. Třídy které stanovují minimální službu jsou známé jako třídy v reálném čase pro aktivní regulátor přetížení. Streamování videa, VoIP a interaktivní online hry jsou příklady aplikací, které musí mít pro své fungování minimální šířku pásma. pro stanovit co zadat, použijte aplikaci na nezatížené LAN a sledujte kolik šířky pásma používá. Poté zadejte do tohoto pole hodnotu jen o málo vyšší. QoS uspokojí nejprve minimální službu všech tříd, před přidělením zbývající služby ostatních čekajících tříd, takže buďte opatrní a použijte minimální šířky pásma s mírou.";
qosStr.UTotBandAbout="<em>Celková šířka pásma pro nahrávání</em> by měla být nastavena na přibližně 98% vaší dostupné šířky pásma pro nahrávání. Zadáním čísla které je příliš vysoké, bude mít za následek že QoS nedodrží své požadavky třídy. zadáním čísla, které je příliš nízké bude zbytečně penalizovat vaši rychlost pro nahrávání. Pokud používáte připojení PPPoE zkontrolujte webové stránky vašeho modemu a použijte rychlost nahrávání vaší linky pro hodnotu šířky pásma pro nahrávání. pro jiné typy připojení byste měli použít program pro testování rychlosti (s vypnutým QoS) pro stanovení dostupné šířky pásma pro nahrávání. Všimněte si, že šířka pásma je uvedena v kbps. Kilobyte obsahuje 8 kilobitů.";
qosStr.DTotBandAbout="Stanovení správné <em>celkové šířky pásma pro stahování</em> je velmi důležité k tomu, aby QoS fungovalo. Pokud používáte aktivní řízení přetížení pak jen toto nastavte na maximální rychlost stahování kterou Váš poskytovatel poskytne. Pokud používáte připojení PPPoE zkontrolujte webovou stránku Vašeho modemu a podle toho nastavte rychlost DSL downlink.";
qosStr.PerBandAboutU="<em>Procento kapacity šířky pásma</em> je procento celkové dostupné šířky pásma, které by mělo být přiděleno této třídě pokud se využívá celá dostupná šířka pásma. Pokud je k dispozici nevyužitá šířka pásma, více může (a bude) přiděleno. procentní hodnoty mohou být nakonfigurovány na více (nebo méně) než 100, ale když se použijí nastavení bude procentní hodnota se upraví proporcionálně tak, aby se přidalo do 100.";
qosStr.PerBandAboutD="<em>Procento kapacity šířky pásma</em> je procento celkové dostupné šířky pásma, které by mělo být přiděleno této třídě pokud se využívá celá dostupná šířka pásma. Pokud je k dispozici nevyužitá šířka pásma, více může (a bude) přiděleno. procentní hodnoty mohou být nakonfigurovány na více (nebo méně) než 100, ale když se použijí nastavení bude procentní hodnota se upraví proporcionálně tak, aby se přidalo do 100. Toto nastavení je aplikováno pouze když je nasycené spojení WAN.";
qosStr.RTTAbout="<em>Minimalizovat RTT</em> oznamuje aktivnímu regulátoru přetížení, že si přejete aby se pokud je aktivní tato třída minimalizoval zpáteční čas (RTT). Toto nastavení použijte pro on-line hraní her nebo VoIP aplikace, které vyžadují nízký čas RTT (ping odzvy). Minimalizace RTT je na úkor efektivní propustnosti WAN, takže když tyto třídy jsou aktivní propustnost WAN bude klesat (obvykle okolo 20%).";
qosStr.MinSpeedWarn="Pokud nevyužíváte ACC pak musíte stanovit, jaká minimální rychlost poskytovaná poskytovatelem, a následně použít toto číslo. Obecně ISP neposkytují garantovanou minimální šířku pásma, takže abyste získali tuto hodnotu je nutně trochu experimentovat. Jednou z možností je začít s číslem, které je polovina toho, co si myslíte, že by to mělo být a potom otestujte svou linku při plném zatížení a zkontrolujte, zda vše funguje. pak ji zvyšujte v krocích a testujte, dokud se QoS začne rozpadat. Můžete také pozorovat, že po vašem testování QoS funguje na chvíli a pak přestane fungovat. je to z důvodu, že váš ISP je stále přetížen kvůli požadavkům svých jiných zákazníků, takže už nedodává pro Vás šířku pásma jako během testování. Řešením je snížit toto číslo. Zadáním čísla, která je příliš vysoké bude mít za následek to, že QoS neplní požadavky své třídy . Zadáním čísla které je příliš nízké bude zbytečně penalizovat vaši rychlost stahování. Vzhledem ke všem těmto komplikacím doporučuji použít ACC, pokud je to možné. Upozorňujeme, že šířka pásma je uvedeno v kb / s. 8 kilobitů je jeden kilobyte.";
qosStr.QoSAbout="Kvalita služby (QoS) poskytuje způsob, jak je přidělována dostupná šířka pásma. Připojení jsou zařazeny do různých &ldquo; servisních tříd, &rdquo; přičemž každé je přidělena část dostupné šířky pásma. QoS by měly být použity v případech, kdy chcete rozdělit dostupnou šířku pásma mezi konkurenčními požadavky. Například pokud chcete, aby vaše VoIP telefon fungoval správně během stahování videí. Dalším případem by bylo, pokud chcete ztlumit Vaše bit torrenty, když surfujete po webu.";
qosStr.MaxBandAbout="<em>Maximální šířka pásma</em> určuje maximální absolutní množství šířky pásma této třídy v kbit/s které bude přiděleno. Dokonce i když je k dispozici nevyužitá šířka pásma, této servisní třídě nikdy nebude povoleno používat více než toto množství šířky pásma.";
qosStr.PackAbout="Pakety jsou testovány vůči pravidlům v pořadí určeném - pravidla směrem nahoru mají přednost. Jakmile paket odpovídá pravidlu je klasifikován, a zbytek pravidel se ignoruje. Pořadí pravidel lze měnit pomocí ovládacích prvků se šipkami.";
qosStr.DefServClassAbout="<em>Výchozí servisní třída</em> určuje, jak by měly být klasifikovány pakety, které neodpovídají žádnému pravidlu.";
qosStr.AbACC="<p>Aktivní regulátor přetížení (ACC) pozoruje Vaši aktivitu stahování a automaticky upravuje Váš limit stahování z důvodu zabezpečení plnění QoS. ACC automaticky kompenzuje změny ve vaší rychlosti stahování ISP a požadavky ze sítě nastavováním rychlosti připojení na nejvyšší možnou rychlost , což bude udržovat správnou funkci QoS. Efektivní rozsah této kontroly se pohybuje v rozmezí 15% až 100% z celkové šířky pásma pro stahování, které jste zadali výše.</p><p>Zatímco ACC neupraví Vaši rychlost stahování je nutné povolit a správně nakonfigurovat Vaši QoS nahrávání, aby to správně fungovalo.</p><p><em>Cíl pro ping -</em> Segment sítě mezi Vašim směrovačem a cílem pro ping kde je řízeno přetížení. přetížení je detekováno monitorováním zpátečního času ping na cíl. Ve výchozím nastavení ACC používá vaši WAN bránu jako cíl pro ping. Pokud víte, že dojde k zahlcení na vašem připojen jiném segmentu pak můžete zadat alternativní cíl pro ping.</p><p><em>Manuální limit pro ping -</em> Časy ping jsou porovnávány vůči limitem pro ping. ACC controls the link limit to maintain ping times under the appropriate limit. Ve výchozím nastavení Gargoyle se vám pokusí automaticky vybrat vhodný cílový limit pro ping na základě rychlosti připojení, které jste zadali a výkonu vašeho připojení změřeného při inicializaci. Nemůžete změnit cílový čas pro ping pro minRTT mód ale ručním zadáním času, můžete ovládat cílový čas pro ping aktivního režimu. Čas, který zadáte zvýší cílový čas pro ping mezi minRTT a aktivním režimem. Limity které ACC používá můžete vidět v [] závorkách vedle pole limitů času pro ping.</p>";
qosStr.ServClass="Výchozí servisní třída";

qosStr.AddNewClassRule="Přidat nové klasifikační pravidlo";
qosStr.AddNewServiceRule="Přidat novou servisní třídu";
qosStr.SrcIP="Zdrojová IP";
qosStr.SrcPort="Zdrojový Port(y)";
qosStr.DstIP="Cílová IP";
qosStr.DstPort="Cílový Port(y)";
qosStr.MaxPktLen="Maximální velikost packetů";
qosStr.MinPktLen="Minimální velikost packetů";
qosStr.TrProto="Transportní protokol";
qosStr.Conreach="Dosažené byty připojení";
qosStr.AppProto="Aplikační (Layer7) protokol";
qosStr.SetClass="Nastavit servisní třídu na";
qosStr.SrvClassName="Název servisní třídy";
qosStr.PerBandCap="Procento šířky pásna z celé kpacity";
qosStr.BandMin="Minimální šířka pásma";
qosStr.BandMinNo="žádaná minimální šířka pásma";
qosStr.BandMax="Maximální šířka pásma";
qosStr.BandMaxNo="žádaná maximální šířka pásma";
qosStr.MinRTT="Minimalizovat zpáteční čas (RTT)";
qosStr.ActRTT="Minimalizovat RTT (časy ping) během aktivity";
qosStr.OptiWAN="Optimalizovat využití WAN";
qosStr.ACCOn="Povolit aktivní regulátor přetížení (pro stahování)";
qosStr.ACC_Pt="Použít nestandardní ping cíl";
qosStr.ACC_con="Manuální ovládat cílový čas ping";
qosStr.resetFLL="Obnovit mezní limit spravedlivé vazby";
qosStr.ACC_Stat="Stav regulace přetížení";
qosStr.ACC_L_Ck="Zkontrolovat zda cíl pro ping bude reagovat";
qosStr.ACC_L_In="Odhadnout limit pro ping";
qosStr.ACC_L_Act="Regulace přetížení je aktivní.";
qosStr.ACC_L_Min="Regulace přetížení je aktivní, třída minRTT je aktivní.";
qosStr.ACC_L_Id="Bez přetížení, řízení nečinné.";
qosStr.ACC_L_Dis="Regulátor není povolen";
qosStr.ACC_L_Lim="Limit rychlosti stahování je aktuálně vynucený.";
qosStr.ACC_L_Fr="Očekávaný spravedlivý limit šířky pásma pro stahování.";
qosStr.ACC_L_Ld="Současný provoz na lince.";
qosStr.ACC_L_pg="Zpáteční čas pro poslední ping.";
qosStr.ACC_L_Flt="Zpáteční čas filtrován.";
qosStr.ACC_L_plim="Bod na kterém regulátor bude udržovat férovost.";
qosStr.ACC_L_AC="Počet tříd pro stahování se zatížením nad 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Upravit QoS servisní třídu";
qosStr.QESrvName="Název servisní třídy";

//qos_edit_rule.sh
qosStr.QERulClass="Upravit QoS klasifikační pravidlo";

//javascript
qosStr.MatchC="Srovnávací kritérium";
qosStr.Classn="Klasifikace";
qosStr.Comment="Poznámka";
qosStr.Src="Zdroj";
qosStr.SrcP="Zdrojový Port";
qosStr.Dst="Cíl";
qosStr.DstP="Cílový Port";
qosStr.Connb="Připojovací byty";
qosStr.APro="Aplikační protokol";
qosStr.pBdW="Procento BW";
qosStr.mBdW="Min BW";
qosStr.MBdW="Max BW";
qosStr.qLd="Zatížení";
qosStr.CrErr="Nebyly vybrány srovnávací kritéria.";
qosStr.SvErr="Nelze přidat novou servisní třídu.";
qosStr.SUErr="Nelze aktualizovat servisní třídu.";
qosStr.CsErr="Nelze přidat klasifikační pravidlo.";
qosStr.CUErr="Nelze aktualizovat klasifikační pravidlo.";
qosStr.DCErr="Duplicitní jméno třídy.";
qosStr.RemSCErr="Vyžaduje se alespoň jedna servisní třída. \nNemožno odstranit servisní třídu.";
qosStr.TotErr="Chyba v poli Celková šířka pásma. \n\nNemožno aktualizovat QoS.";

//one-word strings used in důvěrnosti
qosStr.NOLIMIT="bez_limitu";
qosStr.ZERO="nula";
qosStr.YES="Ano";

//qos_distribution.sh
qosStr.mQOS="Distribuce QoS";
qosStr.UBSect="QoS rozdělení šířky pásma pro nahrávání";
qosStr.DBSect="QoS rozdělení šířky pásma pro stahování";
qosStr.uTFrm="Časový rámec pro nahrávání";
qosStr.dTFrm="Časový rámec pro stahování";
