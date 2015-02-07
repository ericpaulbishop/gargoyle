/*
 * UTF-8 (with BOM) English-EN text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS (Opplasting) -- Klassifisering Regler";
qosStr.DRSection="QoS (Nedlasting) -- Klassifisering Regler";
qosStr.UCSection="QoS (Opplasting) -- Tjeneste Klasser";
qosStr.DCSection="QoS (Nedlasting) -- Tjeneste Klasser";
qosStr.DACCSect="QoS (Nedlasting) -- Aktiv Kongestion Kontroll";
qosStr.UEnable="Aktiver Quality of Service (Opplasting Retning)";
qosStr.DEnable="Aktiver Quality of Service (Nedlasting Retning)";
qosStr.UTotBand="Total Opplastning Båndbredde";
qosStr.DTotBand="Total Nedlasting Båndbredde";

qosStr.USCAbout="Hver opplasting tjeneste klasse er spesifisert av tre parametre: prosent båndbredde av kapasitet, minimum båndbredde og maksimal båndbredde";
qosStr.DSCAbout="Hver tjeneste klasse er spesifisert av fire parametere: prosent båndbredde av kapasitet, sanntid båndbredde, maksimal båndbredde og minimere rundtur tid flagg";
qosStr.UMinBandAbout="<em>Minimum båndbredde</em> angir minimum tjeneste denne klassen vil bli tildelt når koblingen er på sin kapasitet. For enkelte applikasjoner som VoIP eller online spilling er det bedre å angi en minimum tjeneste i bps stedet for  en prosentandel. QoS vil tilfredsstille minimum tjeneste i alle klasser først før tildeling av gjenværende til andre ventende klasser.";
qosStr.DMinBandAbout="<em>Minimum båndbredde</em> angir minimum tjeneste denne klassen vil bli tildelt når koblingen er på sin kapasitet. Klasser som angir minimum tjeneste er kjent som sanntids klasser ved den aktive kongestion kontrolleren. Direkteavspilt video, VoIP og interaktiv online spilling er alle eksempler på applikasjoner som må ha en minimum båndbredde for å fungere. For å avgjøre hvilken verdi å sette inn, kan en bruke applikasjonene på et ubelastet LAN og observere hvor mye båndbredde de bruker. Deretter skriver du inn et tall bare litt høyere i dette feltet. QoS vil sikre minimum tjeneste i alle klasser før tildeling til andre ventende klasser, så det kan være lurt å være sparsom når en setter minimum båndbredder.";
qosStr.UTotBandAbout="<em>Total Opplasting Båndbredde</em> bør settes til rundt 98% av tilgjengelig opplasting båndbredde. Setter en inn et tall som er for høyt, vil det føre til QoS ikke oppfyller sine klassekrav. Å sette en et nummer som er for lavt, vil unødvendig gå ut over opplastingshastighet. Hvis du bruker en PPPoE tilkobling kan du sjekke modemets konfigurasjonside og bruke Uplink linjehastighet som opplasting båndbredde. Andre typer tilkoblinger bør bruke et hastighets testprogram (med QoS av) for å bestemme den tilgjengelige opplasting båndbredde. Merk at båndbredde angis i kbps, og at det er 8 kilobit per kilobyte.";
qosStr.DTotBandAbout="Å sette <em>Total Nedlasting Båndbredde</em> til rett verdi er avgjørende for QoS. Hvis du bruker Aktiv Kongestion Kontroll så sett det til den maksimale nedlastingshastigheten din ISP leverer. Hvis du bruker en PPPoE tilkobling kan du sjekke modemets konfigurasjonside og sette denne til DSL Downlink linjehastighet.";
qosStr.PerBandAboutU="<em>Prosent båndbredde av kapasitet</em> er prosentandelen av den totale tilgjengelige båndbredden som skal fordeles til denne klassen når all tilgjengelig båndbredde blir brukt. Hvis ubrukt båndbredde er tilgjengelig, kan mer (og vil) tildeles. Prosentene kan konfigureres til å tilsvare mer (eller mindre) enn 100, men når innstillingene er aktivert blir prosentene justert proporsjonalt slik at de blir lagt til 100.";
qosStr.PerBandAboutD="<em>Prosent båndbredde av kapasitet</em> er prosentandelen av den totale tilgjengelige båndbredden som skal fordeles til denne klassen når all tilgjengelig båndbredde blir brukt. Hvis ubrukt båndbredde er tilgjengelig, kan mer (og vil) tildeles. Prosentene kan konfigureres til å tilsvare mer (eller mindre) enn 100, men når innstillingene er aktivert blir prosentene justert proporsjonalt slik at de blir lagt til 100. Denne innstillingen trer først i kraft når kapasiteten på WAN kobling er brukt.";
qosStr.RTTAbout="<em>Minimer RTT</em> indikerer til den aktive overbelastning kontroller at du ønsker å minimere rundtur tiden (RTT) når denne klassen er aktiv. Bruk denne innstillingen for online spilling eller VoIP applikasjoner som trenger lav rund tur tider (ping tider). Redusering av RTT kommer på bekostning av effektiv WAN gjennomstrømming, så når denne klasse er aktiv vil WAN gjennomstrømningen avta (vanligvis rundt 20%).";
qosStr.MinSpeedWarn="Hvis du ikke bruker AKK så må du fastsette minimum hastighet din ISP leverer, og deretter setter tallet til det. Generelt oppgir ikke ISP'er en garantert minimum båndbredde, så det kan bli litt eksperimentering og frustrasjon for å komme fram til et tall. En god tilnærming er å starte med et tall som er halvparten av hva du tror det skal være, og deretter teste koblingen under full belastning og se at alt fungerer. Deretter å øke den steg vis til du ser QoS begynner å feile. En kan også se at QoS fungerer en stund for deretter slutter å virke. Dette er fordi din ISP blir overbelastet på grunn av krav fra andre kunder slik at de ikke lenger greier å levere deg den båndbredden de gjorde under testing din. En løsning er å sette dette tallet lavere. Å skrive inn et tall som er for høyt vil føre til at QoS ikke oppfyller sine klassekrav. Å skrive inn et tall som er for lavt, vil gå unødvendig utover nedlastingshastighet. På grunn av denne kompleksiteten kan det anbefales at du bruker AKK når det er mulig. Merk at båndbredden er spesifisert i kilobit/s og at det er 8000 bits per kilobyte.";
qosStr.QoSAbout="Quality of Service (QoS) er en måte å kontrollere hvordan den tilgjengelige båndbredde fordeles. Tilkoblinger er klassifisert i forskjellige "tjeneste klasser", som hver er tildelt en andel av den tilgjengelige båndbredde. QoS bør aktiveres i tilfeller der du ønsker å dele den tilgjengelige båndbredde mellom konkurrerende trafikk, for eksempel hvis du vil at VoIP telefon skal fungere riktig mens du laster ned videoer. Et annet tilfelle kan være hvis du vil at båndbredden til bittorrent'er strupes ned når du surfer på nettet.";
qosStr.MaxBandAbout="<em>Maksimal båndbredde</em> angir den absolutt maksimale mengde båndbredde denne klassen vil bli tildelt i kbit/s. Selv om ubrukt båndbredde er tilgjengelig, vil denne tjeneste klassen aldri få lov til å bruke mer enn denne båndbredde.";
qosStr.PackAbout="Pakker er testet mot reglene i den rekkefølgen som er angitt -- regler mot toppen har prioritet. Så snart en pakke passer til en regel som er klassifisert, vil resten av reglene blir ignorert. Rekkefølgen av reglene kan endres ved hjelp av piltastene.";
qosStr.DefServClassAbout="<em>Standard Tjenesten Klasse</em> angir hvordan pakker som ikke samsvarer med noen regel skal klassifiseres.";
qosStr.AbACC="<p>Aktiv kongestion kontroll (AKK) overvåker nedlasting aktiviteten og justerer automatisk nedlastings grensen for å opprettholde riktig QoS ytelse. AKK kompenserer automatisk for endringer i din ISP's nedlastingshastighet og justerer linjehastigheten ut fra etterspørselen fra nettverket til høyeste mulig hastighet som også opprettholder riktig QoS funksjon. Det effektive området av denne kontrollen er mellom 15% og 100% av den totale nedlasting båndbredde du har angitt ovenfor.</p><p>Da AKK ikke justerer opplasting linjehastighet må du aktivere og konfigurere din opplasting QoS for at det skal fungere ordentlig.</p><p><em>Ping Mål</em> Den delen av nettverket som er mellom ruteren og ping målet er der kongestion kontrolleres. Ved å overvåke rundtur ping tiden til målet blir kongestion oppdaget. Som standard bruker AKK WAN porten som ping målet. Hvis du vet at kongestion vil oppstå i et annet segment så kan du skrive inn et alternativt ping mål.</p><p><em>Manuell Ping Grense</em> Rundtur ping tider blir sammenlignet med ping grenser. AKK styrer koblingsgrensen for å opprettholde ping tider under passende grense. Som standard velger Gargoyle automatisk riktige ping grenser for deg, disse er basert på koblings hastigheter du har angitt og måling av linkens kapasitet under initialisering. Du kan ikke endre mål ping tiden for minRTT modus, men ved å legge inn en manuell tid kan du kontrollere mål ping tiden i aktiv modus. Den tiden du setter inn blir økningen i mål ping tiden mellom minRTT og aktiv modus. Du kan se grensene AKK bruker i [] klammene ved siden av ping grense feltet.</p>";
qosStr.ServClass="Standard Tjeneste Klasse";

qosStr.AddNewClassRule="Legg til ny Klassifisering Regel";
qosStr.AddNewServiceRule="Legg til ny Tjeneste Klasse";
qosStr.SrcIP="Kilde IP";
qosStr.SrcPort="Kilde Port(er)";
qosStr.DstIP="Destinasjon IP";
qosStr.DstPort="Destinasjon Port(er)";
qosStr.MaxPktLen="Maksimal Pakkelengde";
qosStr.MinPktLen="Minimum Pakkelengde";
qosStr.TrProto="Transport Protokoll";
qosStr.Conreach="Tilkobling bytes mål";
qosStr.AppProto="Applikasjons (Lag7) Protokoll";
qosStr.SetClass="Sett Tjeneste Klassen Til";
qosStr.SrvClassName="Tjeneste Klasse Navn";
qosStr.PerBandCap="Prosent Båndbredde av Kapasitet";
qosStr.BandMin="Minimum Båndbredde";
qosStr.BandMinNo="Ingen Minimum Båndbredde";
qosStr.BandMax="Maksimum Båndbredde";
qosStr.BandMaxNo="Ingen Maksimum Båndbredde";
qosStr.MinRTT="Minimer Rundtur Tider (RTT)";
qosStr.ActRTT="Minimer RTT (ping tid) når aktiv";
qosStr.OptiWAN="Optimaliser WAN Utnyttelse";
qosStr.ACCOn="Aktiver Aktiv Kongestion Kontroll (Nedlasting Retning)";
qosStr.ACC_Pt="Bruk ikke standard ping mål";
qosStr.ACC_con="Styr ping mål tiden manuelt";
qosStr.ACC_Stat="Kongestion Kontroll Status";
qosStr.ACC_L_Ck="Sjekk om ping målet svarer";
qosStr.ACC_L_In="Beregn ping grense";
qosStr.ACC_L_Act="Kongestion Kontroll Aktiv.";
qosStr.ACC_L_Min="Kongestion Kontroll Aktiv, minRTT klasse aktiv.";
qosStr.ACC_L_Id="Ingen Kongestion, kontroller inaktiv.";
qosStr.ACC_L_Dis="Kontroller er ikke aktivert";
qosStr.ACC_L_Lim="Nedlasting båndbredde grensen som for tiden håndheves.";
qosStr.ACC_L_Fr="Den tilsynelatende rettferdige nedlasting båndbreddegrense.";
qosStr.ACC_L_Ld="Den nåværende trafikk (Nedlasting).";
qosStr.ACC_L_pg="Rundtur tiden for siste ping.";
qosStr.ACC_L_Flt="Rundtur tiden filtrert.";
qosStr.ACC_L_plim="Punktet der kontrolleren vil handle for å opprettholde satte mål.";
qosStr.ACC_L_AC="Antall nedlasting klasser med last over 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Rediger QoS Tjenesten Klasse";
qosStr.QESrvName="Tjeneste Klasse Navn";

//qos_edit_rule.sh
qosStr.QERulClass="Rediger QoS Klassifisering Regel";

//javascript
qosStr.MatchC="Matche Kriteriene";
qosStr.Classn="Klassifisering";
qosStr.Src="Kilde";
qosStr.SrcP="Kilde Port";
qosStr.Dst="Destinasjon";
qosStr.DstP="Destinasjons Port";
qosStr.Connb="Tilkobling byte";
qosStr.APro="Applikasjon Protokoll";
qosStr.pBdW="Prosent BB";
qosStr.mBdW="Min BB";
qosStr.MBdW="Maks BB";
qosStr.qLd="Last";
qosStr.CrErr="Ingen matchende kriteriene er valgt.";
qosStr.SvErr="Kunne ikke legge til den nye tjenesteklasse.";
qosStr.SUErr="Kunne ikke oppdatere tjenesteklasse.";
qosStr.CsErr="Kunne ikke legge til klassifisering regel.";
qosStr.CUErr="Kunne ikke oppdatere klassifisering regel.";
qosStr.DCErr="Klasse navn finnes fra før.";
qosStr.RemSCErr="Minst én tjeneste klasse kreves.\nKan ikke fjerne tjeneste klasse.";
qosStr.TotErr="Det er en feil i Total Båndbredde feltet.\n\nKunne ikke oppdatere QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="ubegrenset";
qosStr.ZERO="null";
qosStr.YES="Ja";

//qos_distribution.sh
qosStr.UBSect="QoS Opplasting Båndbredde Fordeling";
qosStr.DBSect="QoS Nedlasting Båndbredde Fordeling";
qosStr.uTFrm="Opplasting Tidsramme";
qosStr.dTFrm="Nedlasting Tidsramme";
