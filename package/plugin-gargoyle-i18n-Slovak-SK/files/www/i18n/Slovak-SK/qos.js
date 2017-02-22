/*
 * UTF-8 (with BOM) Slovak-SK text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.mQDl="QoS (sťahovanie)";
qosStr.mQUl="QoS (nahrávanie)";
qosStr.URSection="QoS (nahrávanie) -- klasifikačné pravidlá";
qosStr.DRSection="QoS (sťahovanie) -- klasifikačné pravidlá";
qosStr.UCSection="QoS (nahrávanie) -- servisné triedy";
qosStr.DCSection="QoS (sťahovanie) -- servisné triedy";
qosStr.DACCSect="QoS (sťahovanie) -- Aktívne ovládanie zahltenia";
qosStr.UEnable="Povoliť kvalitu služby (smer nahrávanie)";
qosStr.DEnable="Povoliť kvalitu služby (smer sťahovanie)";
qosStr.UTotBand="Celková šírka pásma pre nahrávanie";
qosStr.DTotBand="Celková šírka pásma pre sťahovanie";

qosStr.USCAbout="Každá servisná trieda pre nahrávanie je špecifikovaná tromi parametrami: percento kapacity šírky pásma, minimálna šírka pásma a maximálna šírka pásma.";
qosStr.DSCAbout="Každá servisná trieda je určená štyrmi parametrami: percento kapacity šírky pásma, šírka pásma v reálnom čase, maximálna šírku pásma a minimalizovaný spiatočný časový príznak.";
qosStr.UMinBandAbout="<em>Minimálna šírka pásma</em> stanovuje minimálnu službu ktorá tejto triede bude pridelená, keď je linka využitá na plný výkon. Pre určité aplikácie, ako sú VoIP alebo hranie online hier je lepšie stanoviť minimálnu službu v bps, skôr než percento. QoS uspokojí najprv minimálnu službu všetkých tried, pred pridelením zostávajúcej služby ostatných čakajúcich tried.";
qosStr.DMinBandAbout="<em>Minimálna šírka pásma</em> stanovuje minimálnu službu ktorá tejto triede bude pridelená, keď je linka využitá na plný výkon. Triedy ktoré stanovujú minimálnu službu sú známe ako triedy v reálnom čase pre aktívny regulátor preťaženia. Streamovanie videa, VoIP a interaktívne online hry sú príklady aplikácií, ktoré musia mať pre svoje fungovanie minimálnu šírku pásma. Ak chcete stanoviť čo zadať, použite aplikáciu na nezaťaženej LAN a sledujte koľko šírky pásma používa. Potom zadajte do tohoto poľa hodnotu len o málo vyššiu. QoS uspokojí najprv minimálnu službu všetkých tried, pred pridelením zostávajúcej služby ostatných čakajúcich tried, takže buďte opatrní a použite minimálnu šírky pásma s mierou.";
qosStr.UTotBandAbout="<em>Celková šírka pásma pre nahrávanie</em> by mala byť nastavená na približne 98% vašej dostupnej šírky pásma pre nahrávanie. Zadaním čísla ktoré je príliš vysoké, bude mať za následok že QoS nedodrží svoje požiadavky triedy. Zadaním čísla, ktoré je príliš nízke bude zbytočne penalizovať vašu rýchlosť pre nahrávanie. Ak používate pripojenie PPPoE skontrolujte webové stránky vašeho modemu a použite rýchlosť nahrávania vašej linky pre hodnotu šírky pásma pre nahrávanie. Pre iné typy pripojenia by ste mali použiť program na testovanie rýchlosti (s vypnutým QoS) na stanovenie dostupné šírky pásma pre nahrávanie . Všimnite si, že šírka pásma je uvedená v kbps. Kilobyte obsahuje 8 kilobitov.";
qosStr.DTotBandAbout="Stanovenie správnej <em>celkovej šírky pásma pre sťahovanie</em> je veľmi dôležité na to, aby QoS fungovalo. Ak používate aktívne riadenie preťaženia potom len toto nastavte na maximálnu rýchlosť sťahovania ktorú Váš poskytovateľ poskytne. Ak používate pripojenie PPPoE skontrolujte webovú stránku Vašeho modemu a podľa toho nastavte rýchlosť DSL downlink.";
qosStr.PerBandAboutU="<em>Percento kapacity šírky pásma</em> je percento celkovej dostupnej šírky pásma, ktoré by malo byť pridelené tejto triede ak sa využíva celá dostupná šírka pásma. Ak je k dispozícii nevyužitá šírka pásma, viacej môže (a bude) pridelené. Percentuálne hodnoty môžu byť nakonfigurované na viac (alebo menej) ako 100, ale keď sa použijú nastavenia bude percentuálna hodnota sa upraví proporcionálne tak, aby sa pridalo do 100.";
qosStr.PerBandAboutD="<em><em>Percento kapacity šírky pásma</em> je percento celkovej dostupnej šírky pásma, ktoré by malo byť pridelené tejto triede ak sa využíva celá dostupná šírka pásma. Ak je k dispozícii nevyužitá šírka pásma, viacej môže (a bude) pridelené. Percentuálne hodnoty môžu byť nakonfigurované na viac (alebo menej) ako 100, ale keď sa použijú nastavenia bude percentuálna hodnota sa upraví proporcionálne tak, aby sa pridalo do 100. Toto nastavenie je aplikované iba keď je nasýtené spojenie WAN.";
qosStr.RTTAbout="<em>Minimalizovať RTT</em> oznamuje aktívnemu regulátoru preťaženia, že si prajete aby sa ak je aktívna táto trieda minimalizoval spiatočný čas (RTT). Toto nastavenie použite pre on-line hranie hier alebo VoIP aplikácie, ktoré vyžadujú nízky čas RTT (ping odzvy). Minimalizácia RTT je na úkor efektívnej priepustnosti WAN, takže keď tieto triedy sú aktívne  priepustnosť  WAN bude klesať (zvyčajne okolo 20%).";
qosStr.MinSpeedWarn="Ak nevyužívate ACC potom musíte stanoviť, aká minimálna rýchlosť poskytovaná poskytovateľom, a následne použiť toto číslo. Vo všeobecnosti ISP neposkytujú garantovanú minimálnu šírku pásma, takže aby ste získali túto hodnotu je nutne trochu experimentovať. Jednou z možností je začať s číslom, ktoré je polovica toho, čo si myslíte, že by to malo byť a potom otestujte svoju linku pri plnom zaťažení a skontrolujte, či všetko funguje. Potom ju zvýšujte v krokoch a testujte, kým sa QoS začne rozpadať. Môžete taktiež pozorovať, že po vašom testovaní QoS funguje na chvíľu a potom prestane fungovať. Je to z dôvodu, že váš ISP je stále preťažený kvôli požiadavkám svojich iných zákazníkov, takže už nedodáva pre Vás šírku pásma ako počas testovania. Riešením je znížiť toto číslo. Zadaním čísla, ktoré je príliš vysoké bude mať za následok to, že QoS neplní požiadavky svojej triedy. Zadaním čísla ktoré je príliš nízke bude zbytočne penalizovať vašu rýchlosť sťahovania. Vzhľadom k všetkým týmto komplikáciám odporúčam použiť ACC, pokiaľ je to možné. Upozorňujeme, že šírka pásma je uvedené v kilobit/s. 8 kilobitov je jeden kilobyte.";
qosStr.QoSAbout="Kvalita služby (QoS) poskytuje spôsob, ako je pridelovaná dostupná šírka pásma. Pripojenia sú zaradené do rôznych &ldquo;servisných tried,&rdquo; pričom každej je pridelená časť dostupnej šírky pásma. QoS by mali byť použité v prípadoch, keď chcete rozdeliť dostupnú šírku pásma medzi konkurenčnými požiadavkami. Napríklad ak chcete, aby vaše VoIP telefón fungoval správne počas sťahovania videí. Ďalším prípadom by bolo, ak chcete stlmiť Vaše bit torrenty, keď surfujete po webe.";
qosStr.MaxBandAbout="<em>Maximálna šírka pásma</em> určuje maximálne absolútne množstvo šírky pásma tejto triedy v kbit/s ktoré bude pridelené. Dokonca aj keď je k dispozícii nevyužitá šírka pásma, tejto servisnej triede nikdy nebude povolené používať viac ako toto množstvo šírky pásma.";
qosStr.PackAbout="Pakety sú testované voči pravidlám v poradí určenom -- pravidlá smerom hore majú prednosť. Akonáhle paket zodpovedá pravidlu je klasifikovaný, a zvyšok pravidiel sa ignoruje. Poradie pravidiel možno meniť pomocou ovládacích prvkov so šípkami.";
qosStr.DefServClassAbout="<em>Predvolená servisná trieda</em> určuje, ako by maly byť klasifikované pakety , ktoré nezodpovedajú žiadnemu pravidlu.";
qosStr.AbACC="<p>Aktívny regulátor preťaženia (ACC) pozoruje Vašu aktivitu sťahovania a automaticky upravuje Váš limit sťahovania z dovôdu zabezpečenia plnenia QoS. ACC automaticky kompenzuje zmeny vo vašej rýchlosti sťahovania ISP a požiadavky zo siete nastavovaním rýchlosti pripojenia na najvyššiu možnú rýchlosť, čo bude udržiavať správnu funkciu QoS. Efektívny rozsah tejto kontroly sa pohybuje v rozmedzí 15% až 100% z celkovej šírky pásma pre sťahovanie, ktoré ste zadali vyššie.</p><p>Kým ACC neupraví Vašu rýchlosť sťahovania je nutné povoliť a správne nakonfigurovať Vašu QoS nahrávania, aby to správne fungovalo.</p><p><em>Cieľ pre Ping -</em> Segment siete medzi Vašim smerovačom a cieľom pre ping kde je riadené preťaženie. Preťaženie je detekované monitorovaním spiatočného času ping na cieľ. V predvolenom nastavení ACC používa vašu WAN bránu ako cieľ pre ping. Ak viete, že dôjde k zahlteniu na vašom pripojení inom segmente potom môžete zadať alternatívny cieľ pre ping.</p><p><em>Manuálny limit pre ping-</em> Časy ping sú porovnávané voči limitom pre ping. ACC controls the link limit to maintain ping times under the appropriate limit. V predvolenom nastavení Gargoyle sa vám pokúsi automaticky vybrať vhodný cielový limit pre ping na základe rýchlosti pripojenia, ktoré ste zadali a výkonu vášho pripojenia zmeraného pri inicializácii.  Nemôžete zmeniť cieľový čas pre ping pre minRTT mód ale ručným zadaním času, môžete ovládať cieľový čas pre ping aktívneho režimu.  Čas, ktorý zadáte zvýši cieľový čas pre ping medzi minRTT a aktívnym režimom. Limity ktoré ACC používa môžete vidieť v [] zátvorkách vedľa poľa limitov času pre ping. </p>";
qosStr.ServClass="Predvolená servisná trieda";

qosStr.AddNewClassRule="Pridať nové klasifikačné pravidlo";
qosStr.AddNewServiceRule="Pridať novú servisnú triedu";
qosStr.SrcIP="Zdrojová IP";
qosStr.SrcPort="Zdrojový Port(y)";
qosStr.DstIP="Cieľová IP";
qosStr.DstPort="Cieľový Port(y)";
qosStr.MaxPktLen="Maximálna veľkosť packetu";
qosStr.MinPktLen="Minimálna veľkosť packetu";
qosStr.TrProto="Transportný protokol";
qosStr.Conreach="Dosiahnuté byty pripojenia";
qosStr.AppProto="Aplikačný (Layer7) protokol";
qosStr.SetClass="Nastaviť servisnú triedu na";
qosStr.SrvClassName="Názov servisnej triedy";
qosStr.PerBandCap="Percento šírky pásna z celej kpacity";
qosStr.BandMin="Minimálna šírka pásma";
qosStr.BandMinNo="Žiadana minimálna šírka pásma";
qosStr.BandMax="Maximálna šírka pásma";
qosStr.BandMaxNo="Žiadana maximálna šírka pásma";
qosStr.MinRTT="Minimalizovať spiatočný čas (RTT)";
qosStr.ActRTT="Minimalizovať RTT (časy ping) počas aktivity";
qosStr.OptiWAN="Optimalizovať využitie WAN ";
qosStr.ACCOn="Povoliť aktívny regulátor preťaženia (pre sťahovanie)";
qosStr.ACC_Pt="Použiť neštandardný ping cieľ";
qosStr.ACC_con="Manuálne ovládať cieľový čas ping";
qosStr.ACC_Stat="Stav regulácie preťaženia";
qosStr.ACC_L_Ck="Skontrolovať či cieľ pre ping bude reagovať";
qosStr.ACC_L_In="Odhadnúť limit pre ping";
qosStr.ACC_L_Act="Regulácia preťaženia je aktívna.";
qosStr.ACC_L_Min="Regulácia preťaženia je aktívna, trieda minRTT je aktívna.";
qosStr.ACC_L_Id="Bez preťaženia, riadenie nečinné.";
qosStr.ACC_L_Dis="Regulátor nie je povolený";
qosStr.ACC_L_Lim="Limit rýchlosti sťahovania je aktuálne vynútený.";
qosStr.ACC_L_Fr="Očakávaný spravodlivý limit šírky pásma pre sťahovanie.";
qosStr.ACC_L_Ld="Súčasná prevádzka na linke.";
qosStr.ACC_L_pg="Spiatočný čas pre posledný ping.";
qosStr.ACC_L_Flt="Spiatočný čas filtrovaný.";
qosStr.ACC_L_plim="Bod na ktorom regulátor bude udržiavať férovosť.";
qosStr.ACC_L_AC="Počet tried pre sťahovanie so zaťažením nad 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Upraviť QoS servisnú triedu";
qosStr.QESrvName="Názov servisnej triedy";

//qos_edit_rule.sh
qosStr.QERulClass="Upraviť QoS klasifikačné pravidlo";

//javascript
qosStr.MatchC="Porovnávacie kritérium";
qosStr.Classn="Klasifikácia";
qosStr.Src="Zdroj";
qosStr.SrcP="Zdrojový Port";
qosStr.Dst="Cieľ";
qosStr.DstP="Cieľový Port";
qosStr.Connb="Pripojovacie byty";
qosStr.APro="Aplikačný protokol";
qosStr.pBdW="Percento BW";
qosStr.mBdW="Min BW";
qosStr.MBdW="Max BW";
qosStr.qLd="Zaťaženie";
qosStr.CrErr="Neboli vybrané porovnávacie kritériá.";
qosStr.SvErr="Nemožno pridať novú servisnú triedu.";
qosStr.SUErr="Nemožno aktualizovať servisnú triedu.";
qosStr.CsErr="Nemožno pridať klasifikačné pravidlo.";
qosStr.CUErr="Nemožno aktualizovať klasifikačné pravidlo.";
qosStr.DCErr="Duplikované meno triedy.";
qosStr.RemSCErr="Vyžaduje sa aspoň jedna servisná trieda.\nNemožno odstrániť servisnú triedu.";
qosStr.TotErr="Chyba v poli Celková šírka pásma.\n\nNemožno aktualizovať QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="bez_limitu";
qosStr.ZERO="nula";
qosStr.YES="Áno";

//qos_distribution.sh
qosStr.mQOS="Distribúcia QoS";
qosStr.UBSect="QoS rozdelenie šírky pásma pre nahrávanie";
qosStr.DBSect="QoS rozdelenie šírky pásma pre sťahovanie";
qosStr.uTFrm="Časový rámec pre nahrávanie";
qosStr.dTFrm="Časový rámec pre sťahovanie";
