/*
 * UTF-8 (with BOM) Polish-PL text strings for port_forwarding(single,multi).sh html elements
 */

prtS.PISect="Przekierowanie portu";
prtS.PRSect="Przekierowanie zakresu portów";
prtS.ForIPort="Przekierowanie portu z WAN do LAN";
prtS.ForRPort="Przekierowanie zakresu portów z WAN do LAN";
prtS.DMZ="DMZ";
prtS.UseDMZ="Włącz DMZ (strefa zdemilitaryzowana)";
prtS.DMZIP="Adres IP DMZ";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="Włącz usługę UPnP &amp; NAT-PMP";
prtS.APFor="Aktywne przekierowania portów";
prtS.USpd="Raportowana prędkość wysyłania";
prtS.DSpd="Raportowana prędkość pobierania";
prtS.UPHelp="UPnP (ang. Univeral Plug and Play) i NAT-PMP (ang. NAT Port Mapping Protocol) są to dwa protokoły, które pozwalają urządzeniom i aplikacjom w sieci LAN na automatyczną konfigurację routera w zakresie przekierowania portów w celu poprawnego działania. Jeżeli urządzenie wspiera jeden z tych protokołów to nie ma potrzeby ręcznego tworzenia reguł przekierowania, ponieważ urządzenie może wykonać je samodzielnie.</p><p>Kiedy jest to włączone, Gargoyle pokazuje tabelę z automatycznie utworzonymi przekierowanami, więc można łatwo sprawdzić czy ta usługa działa poprawnie. Natomiast mogą być z nią problemy, jeżeli konfiguracja sieci zawiera dwa routery (podwójny NAT). Jeżeli tabela zawiera tylko jeden wiersz z '***', to oznacza że nie ma żadnego zarejestrowanego przekierowania.</p><p>Jako część protokołu, urządzenia w sieci LAN mogą żądać prędkości połączenia WAN routera. Dwa pola odpowiadają za konfigurację takich zapytań. Aplikacja kliencka może używać tych informacji do optymalizowania jej wydajności. Należy zauważyć że router nie robi żadnych operacji bazując na tych danych, są one tylko zwracane do urządzenia pytającego. Jeżeli zostało wprowadzone zero jako wartość, zwykle zwracane jest 100MB lub 1GB w zależności od szybkości interfejsu routera.</p><p>Istnieją pewne problemy z bezpieczeństwem tych usług oraz wymagają one dodatkowej pamięci RAM do działania, co może mieć znaczenie dla niektórych routerów. Domyślnie są więc wyłączone.";

//templates
prtS.Desc="Opis";
prtS.optl="(opcja)";
prtS.Proto="Protokół";
prtS.FPrt="Od portu";
prtS.TIP="Do IP";
prtS.TPrt="Do portu";
prtS.SPrt="Od portu";
prtS.EPrt="Do portu";

//javascript
prtS.AFRErr="Nie można dodać reguły przekierowania.";
prtS.GTErr="Port początkowy jest większy niż port końcowy";
prtS.DupErr="Port(y) w zakresie jest/są już przekierowane";
prtS.CopErr="Port jest już przekierowany";
prtS.UpErr="Nie można zaktualizować przekierowania portu.";
prtS.Prot="Protokół";
prtS.LHst="Urządzenie";
prtS.Port="Port";

//edit.sh pages
prtS.PESect="Edycja przekierowania portów";
