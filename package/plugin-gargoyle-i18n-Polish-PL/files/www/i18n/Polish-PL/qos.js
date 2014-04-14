/*
 * UTF-8 (with BOM) Polish-PL text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS (wysyłanie) - Reguły klasyfikacji";
qosStr.DRSection="QoS (pobieranie) - Reguły klasyfikacji";
qosStr.UCSection="QoS (wysyłanie) - Klasy usług";
qosStr.DCSection="QoS (pobieranie) - Klasy usług";
qosStr.DACCSect="QoS (pobieranie) - aktywna kontrola zatorów";
qosStr.UEnable="Włącz QoS (kierunek - wysyłanie)";
qosStr.DEnable="Włącz QoS (kierunek - pobieranie)";
qosStr.UTotBand="Całkowite pasmo wysyłana";
qosStr.DTotBand="Całkowite pasmo pobierania";

qosStr.USCAbout="Każda klasa usługi w przypadku wysyłania jest określona przez trzy parametry: procentową pojemność pasma, minimalne pasmo i maksymalne pasmo.";
qosStr.DSCAbout="Każda klasa usługi jest określona przez cztery parametry: procentową pojemność pasma, rzeczywiste pasmo, maksymalne pasmo i flagę minimalnego czasu podróży pakietów.";
qosStr.UMinBandAbout="<em>Minimalne pasmo</em> określna minimalne pasmo jakie zostanie przyznane tej klasie usługi. Dla niektórych aplikacji takich jak VoIP lub gier online wygodniej jest przyznać określone pasmo w bps niż procentowo określić jego pojemność. QoS będzie spełniać wymagania na minimalne pasmo wszystkich klas przed przyznaniem pasma dla innych grup.";
qosStr.DMinBandAbout="<em>Minimalne pasmo</em> określa minimalne pasmo jakie zostanie przyznane tej klasie usługi. Klasy które mają podane minimum są znane przez ACC jako klasy czasu rzeczywistego. Strumieniowanie wideo, VoIP i interaktywne gry wideo są przykładami aplikacji które muszą mieć zapewnione minimum pasma aby działać poprawnie. Aby sprawdzić ile pasma używa dana aplikacja należy użyć nieobciążonej sieci LAN i zaobserwować zużycie. Następnie należy wprowadzić liczbę tylko nieznacznie wyższą od zaobserwowanej wartości. QoS zajmuje najpierw pasmo dla wszystkich klas które mają określone minimum przed pozostałymi klasami, więc należy uważnie dobierać minimalne pasmo.";
qosStr.UTotBandAbout="<em>Całkowite pasmo wysyłania</em> powinno być ustawione na ok. 98% dostępnego pasma. Wprowadzenie za wysokiego może spowodować niepoprawne działania klas. Wprowadzenie za niskiego może ograniczyć pasmo wysyłania. Jeżeli używane jest podłączenie PPPoE należy sprawdzić stronę statusu modemu i ustawić pasmo wysyłania na podstawie szybkości linii. Przy innych typach połączeń należy użyć programu do sprawdzania szybkości wysyłania łącza (z wyłączonym QoS). Pasmo podane jest w kbps (kilobitach na sekundę). 8 kilobitów to jeden kilobajt.";
qosStr.DTotBandAbout="Podanie poprawnego <em>Całkowitego pasma pobierania</em> jest krytyczne do pracy QoS. Jeżeli używana jest ACC to należy ustawić tą wartość na maksymalne pasmo pobierania dostarczone przez ISP. Jeżeli używane jest połączenie PPPoE należy sprawdzić stronę konfiguracyjną modemu i wprowadzić wartość 'Downlink'.";
qosStr.PerBandAboutU="<em>Procent pojemności pasma</em> jest to procentowe określenie dostępnego pasma które powinno być przypisane do danej klasy połączena kiedy całe pasmo jest wykorzystane. Jeżeli dostępne jest nieużywane pasmo, więcej może być (i będzie) zarezerwowane. Procent może zostać określony jako więcej lub mniej niż 100, ale w momencie zapisu ustawień procenty zostaną skorygowane proporcjonalnie tak, żeby sumowały się do 100.";
qosStr.PerBandAboutD="<em>Procent pojemności pasma</em> jest to procentowe określenie dostępnego pasma które powinno być przypisane do danej klasy połączenia kiedy całe pasmo jest wykorzystane. Jeżeli dostępne jest nieużywane pasmo, więcej może być (i będzie) zarezerwowane. Procent może zostać określony jako więcej lub mniej niż 100, ale w momencie zapisu ustawień procenty zostaną skorygowane proporcjonalnie tak, żeby sumowały się do 100. Te ustawienia dotyczą sytuacji całkowitego wysycenia łącza WAN.";
qosStr.RTTAbout="<em>Minimalizuj RTT</em> jest wskazówką dla Aktywnej kontroli zatorów (ACC) że czas podróży pakietu (RTT) ma być jak najmniejszy kiedy ta klasa jest aktywna. Należy użyć tego ustawienia dla gier online lub aplikacji VoIP które wymagają minimalnego czasu ping. Minimalizacja RTT wykonywana jest kosztem efektywnej przepustowości WAN, więc kiedy ta klasa jest aktywna to przepustowość WAN spadnie (zwykle o około 20%).";
qosStr.MinSpeedWarn="Jeżeli nie jest używana ACC należy wprowadzić rzeczywistą wartość pobierania jaką można uzyskać na łączu. W ogólności ISP nie gwarantuje minimalnego pasma więc ustalenie tej wartości na drodze eksperymentów może być przyczyną frustracji. Jednym z rozwiązań jest ustawienie połowy wartości łącza które powinno być, obciążenie go na maksimum a następnie sprawdzenie czy wszystko działa poprawnie. Następnie zwiększyć liczbę aż do momentu kiedy QoS przestanie poprawnie działać. Można także zaobserwować przypadek kiedy QoS działa poprawnie przez chwilę a następnie już nie. Może to być spowodowanie przeciążeniem sieci dostawców usług internetowych podczas testów. Rozwiązanie - zmniejszyć limit. Wprowadzenie za wysokiej wartości może spowodować że QoS nie będzie spełniał wymagań klas. Wprowadzenie zbyt malej wartości może z kolei doprowadzić do obniżenia przepustowości łącza. Z powodu tych problemów zalecane jest używanie Aktywnej kontroli zatorów (ACC). Pasmo określa się w kilobitach/s. 8 kilobitów jeden kilobajt.";
qosStr.QoSAbout="QoS (ang. Quality of Service) to sposób kontrolowania pasma. Połączenia są klasyfikowane w różne 'klasy usług', z których każda ma przydzieloną część dostępnego pasma. QoS powinno się stosować w przypadkach kiedy wymagane jest sprawiedliwe podzielnie pasma. Dla przykładu - jeżeli chcemy aby telefon VoIP działał prawidłowo podczas pobierania plików multimedialnych lub w momencie przeglądania sieci działające połączenia bittorrenta zostały zmniejszone. Używanie QoS to przyjęcie kompromisów między maksymalną prędkością a podziałem pasma pomiędzy usługami.";
qosStr.MaxBandAbout="<em>Maksymalne pasmo</em> określa absolutne maksimum tej klasy przyznane w kbit/s. Nawet jeśli nieużywane pasmo jest dostępne, ta klasa usługi nigdy nie przekroczy nadanego limitu.";
qosStr.PackAbout="Pakiety są dopasowywane względem reguł w określonej kolejności - reguły na górze mają pierwszeństwo. Jeżeli pakiet pasuje do reguły to zostaje skasyfikowany i reszta reguł jest ignorowana. Kolejność reguł może być zmodyfikowana przy użyciu strzałek.";
qosStr.DefServClassAbout="<em>Domyślna klasa usług</em> określa jak pakiety powinny być klasyfikowane jeżeli nie spełniają żadnej reguły.";
qosStr.AbACC="<p>Aktywna kontrola zatorów (ang. Active Congestion Control - ACC) obserwuje stan pobierania i automatycznie dostosowuje maksymalne pasmo w celu zachowania poprawnej wydajności QoS. ACC automatycznie kompensuje zmiany prędkości pobierania przez dostawcę usług internetowych  i ustawia maksymalną możliwą prędkść pobierania danych dla której poprawnie działają funkcje QoS. Efektywny zakres kontroli wynosi od 15% do 100% wprowadzonego całkowitego pasma.</p><p>ACC nie ustawia automatycznie pasma wysyłania, więc należy poprawnie ustawić to pasmo w QoS aby całość działała sprawnie.</p><p><em>Cel ping</em> - segment sieci pomiędzy routerem a celem ping gdzie zatory są kontrolowane. Zatory są wykrywane przez sprawdzanie czasu odpowiedzi ping. Domyślnie ACC używa adresu bramy WAN jako celu. Jeżeli wiadomo że zatory mogą wystąpić w innym segmencie sieci, można podać niestandardowy cel ping.</p><p><em>Ręczny limit ping</em> - Czas odpowiedzi ping porównywany w celu wykrycia zatorów. ACC kontroluje połączenia pilnując, aby czas ping był utrzymany na odpowidenim poziomie. Domyślnie Gargoyle automatycznie wybiera odpowiednie wartości ping bazując na wprowdzonych wartościach w parametrach linku. Można sprawdzić inne limity wprowadzając ręcznie inną wartość czasu. Wprowadzenie dłuższego czasu doprowadzi do wyższych limitów ping, wprowadzenie krótszego - do mniejszego. Limity ACC można zobaczyć w nawiasach [] obok pola limitu czasu ping. </p>";
qosStr.ServClass="Domyślna klasa usług";

qosStr.AddNewClassRule="Nowa reguła klasyfikacji";
qosStr.AddNewServiceRule="Nowa klasa usługi";
qosStr.SrcIP="Adres IP źródłowy";
qosStr.SrcPort="Port źródłowy";
qosStr.DstIP="Adres IP docelowy";
qosStr.DstPort="Port docelowy";
qosStr.MaxPktLen="Maksymalna wielkość pakietu";
qosStr.MinPktLen="Minimalna wielkość pakietu";
qosStr.TrProto="Protokół transportu";
qosStr.Conreach="Połączenie przekracza";
qosStr.AppProto="Protokół aplikacji (Layer7)";
qosStr.SetClass="Ustaw klasę usług na";
qosStr.SrvClassName="Nazwa klasy usługi";
qosStr.PerBandCap="Procent pojemności pasma";
qosStr.BandMin="Minimalne pasmo";
qosStr.BandMinNo="Brak minimalnego pasma";
qosStr.BandMax="Maksymalne pasmo";
qosStr.BandMaxNo="Brak maksymalnego pasma";
qosStr.MinRTT="Minimalizacja czasu podróży pakietów (RTT)";
qosStr.ActRTT="Minimalizuj RTT (czas ping) kiedy klasa jest aktywna";
qosStr.OptiWAN="Optymalizuj wykorzystanie WAN";
qosStr.ACCOn="Włącz aktywną kontrolę zatorów (kierunek - pobieranie)";
qosStr.ACC_Pt="Użyj niestandardowego celu ping";
qosStr.ACC_con="Ręczna kontrola celu ping";
qosStr.ACC_Stat="Status kontroli zatorów";
qosStr.ACC_L_Ck="Sprawdzenie odpowiedzi celu pingu.";
qosStr.ACC_L_In="Oszacowanie limitu pingu.";
qosStr.ACC_L_Act="Kontrola jest aktywna.";
qosStr.ACC_L_Min="Kontrola jest aktywna, klasa minRTT jest aktywna.";
qosStr.ACC_L_Id="Brak kontroli, stan oczekiwania.";
qosStr.ACC_L_Dis="Kontrola nie jest włączona.";
qosStr.ACC_L_Lim="Pasmo pobierania jest aktualnie wymuszone.";
qosStr.ACC_L_Fr="Uczciwy podział pasma pobierania.";
qosStr.ACC_L_Ld="Bieżące obciążenie pobierania.";
qosStr.ACC_L_pg="Czas ostatniego pinga.";
qosStr.ACC_L_Flt="Czas pinga oznaczonego jako przefiltrowany.";
qosStr.ACC_L_plim="Punkt, w którym kontroler będzie działać na rzecz sprawiedliwego podziału.";
qosStr.ACC_L_AC="Liczba aktywnych klas pobierania z obciążeniem powyżej 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="QoS - Edycja klasy usługi";
qosStr.QESrvName="Nazwa klasy usługi";

//qos_edit_rule.sh
qosStr.QERulClass="QoS - Edycja reguły klasyfikacji";

//javascript
qosStr.MatchC="Reguła";
qosStr.Classn="Klasyfikacja";
qosStr.Src="Adres IP źródłowy";
qosStr.SrcP="Port źródłowy";
qosStr.Dst="Adres IP docelowy";
qosStr.DstP="Port docelowy";
qosStr.Connb="Połączenie przekracza";
qosStr.APro="Protokół aplikacji";
qosStr.pBdW="Procent pasma";
qosStr.mBdW="Min. pasmo";
qosStr.MBdW="Maks. pasmo";
qosStr.qLd="Obciążenie";
qosStr.CrErr="Nie wybrano elementu dopasowania.";
qosStr.SvErr="Nie można dodać nowej klasy usługi.";
qosStr.SUErr="Nie można zaktualizować klasy usługi.";
qosStr.CsErr="Nie można dodać reguły klasyfikacji.";
qosStr.CUErr="Nie można zaktualizować reguły klasyfikacji.";
qosStr.DCErr="Powielona nazwa klasy";
qosStr.RemSCErr="Wymagana jest co najmniej jedna klasa usługi.\nNie można usunąć usługi.";
qosStr.TotErr="Znaleziono błąd w polu Łączne pasmo.\n\nNie można zaktualizować QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="nolimit";
qosStr.ZERO="zero";
qosStr.YES="Yes";

//qos_distribution.sh
qosStr.UBSect="QoS - Podział pasma wysyłania";
qosStr.DBSect="QoS - Podział pasma pobierania";
qosStr.uTFrm="Przedział czasowy";
qosStr.dTFrm="Przedział czasowy";
