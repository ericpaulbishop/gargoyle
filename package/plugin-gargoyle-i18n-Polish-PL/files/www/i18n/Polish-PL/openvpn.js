/*
 * UTF-8 (with BOM) Polish-PL text strings for OpenVPN elements
 */

ovpnS.OCfg="Konfiguracja OpenVPN";
ovpnS.ODis="Wyłączona";
ovpnS.OClt="Klient";
ovpnS.OSrv="Serwer";
ovpnS.OSts="Status OpenVPN";
ovpnS.OClrK="Wyczyść istniejące klucze OpenVPN";
ovpnS.OClrC="Usunie to na stałe wszystkie klucze wygenerowane dla poprzedniej konfiguracji. Kontynuować?";
ovpnS.OSCfg="Serwer OpenVPN: konfiguracja";
ovpnS.OInIP="Wewnętrzny adres IP serwera";
ovpnS.OIMsk="Wewnętrzna maska podsieci";
ovpnS.OPrt="Port serwera";
ovpnS.OProto="Protokół";
ovpnS.OCiph="Algorytm szyfrujący";
ovpnS.CCTr="Komunikacja pomiędzy klientami VPN";
ovpnS.CtoC="Dozwolona";
ovpnS.CtoS="Zabroniona";
ovpnS.LSAc="Dostęp do urządzeń w sieci LAN";
ovpnS.CtoH="Dopuszczony";
ovpnS.CnoL="Zabroniony, dostęp tylko do routera";
ovpnS.CredR="Poświadczenia";
ovpnS.CredSC="Niezależne dla każdego klienta";
ovpnS.CredMC="Identyczne dla wielu klientów";
ovpnS.CUse="Klienty używają VPN do";
ovpnS.ATrff="Całego ruchu internetowego";
ovpnS.HTrff="Dostępu tylko do zasobów lokalnych";
ovpnS.OSAC="Serwer OpenVPN: klienty";
ovpnS.CClnt="Obecna konfiguracja klientów";
ovpnS.ZipCred="Po wygenerowaniu konfiguracji pobierz archiwum zip zawierające wymagane poświadczenia, aby móc je następnie rozpakować u klienta w katalogu z konfiguracją OpenVPN";
ovpnS.CfgCred="Konfiguracja nowego klienta / ustawienia poświadczeń";
ovpnS.ClntN="Nazwa klienta";
ovpnS.ClntIP="Wewnętrzny adres IP klienta";
ovpnS.ClntConn="Klient łączy się do";
ovpnS.ClntSubN="Podsieć za klientem";
ovpnS.NoSub="Niezdefiniowana";
ovpnS.RtSub="Trasa zdefiniowana poniżej";
ovpnS.SubIP="Adres IP podsieci";
ovpnS.SubM="Maska podsieci";
ovpnS.UpCfgF="Załadowanie plików z konfiguracją klienta";
ovpnS.CfgMan="Ręczna konfiguracja klienta";
ovpnS.UpFmt="Format";
ovpnS.SZipF="Plik z archiwum konfiguracji";
ovpnS.CfgF="Pojedyncze pliki konfiguracyjne";
ovpnS.ZipF="Plik archiwum (zip, tgz)";
ovpnS.OCfgF="Plik konfiguracyjny OpenVPN";
ovpnS.CACF="Plik certyfikatu CA";
ovpnS.CCertF="Plik certyfikatu klienta";
ovpnS.CKeyF="Plik klucza klienta";
ovpnS.TAKeyF="Plik klucza TLS-Auth";
ovpnS.UseTAK="Użyj klucza TLS-Auth";
ovpnS.OSrvAddr="Adres serwera OpenVPN";
ovpnS.OSrvPrt="Port serwera OpenVPN";
ovpnS.Othr="Inny";
ovpnS.Cphr="Algorytm szyfrujący";
ovpnS.Keyopt="Wielkość klucza (opcjonalnie)";
ovpnS.CfgUpd="Konfiguracja jest automatycznie aktualizowana na podstawie wprowadzonych powyżej parametrów";
ovpnS.CACert="Certyfikat CA";
ovpnS.CCert="Certyfikat klienta";
ovpnS.CKey="Klucz klienta";
ovpnS.TAKey="Klucz TLS-Auth";
ovpnS.TADir="Użyj klucza TLS-Auth";
ovpnS.Clnt="Klient";
ovpnS.Symm="Pominięty (Symetryczny)";

//javascript
ovpnS.CryptoWaitMsg="To jest pierwsza konfiguracja serwera OpenVPN.\n\nBędzie ona trwała około 5-10 minut w celu wygenerowania niezbędnych parametrów kryptograficznych. Jest to jednorazowa operacja - późniejsze zmiany będą już szybsze.\n\nKontynuować?";
ovpnS.SubMis="Różnica podsieci klienta";
ovpnS.ExpSubN="OpenVPN oczekuje, że router będzie miał podsieć";
ovpnS.ActSubN=", ale został on skonfigurowany na podsieć";
ovpnS.WantQ=". Czy chcesz...";
ovpnS.Switch="Przełączyć router na spodziewaną podsieć, z adresem IP";
ovpnS.KeepC="Zachować aktualną podsieć i kontynuować";
ovpnS.SrvPrtErr="Port serwera OpenVPN koliduje z wartością wpisaną w polu";
ovpnS.SrvAddErr="Adres serwera nie jest określony";
ovpnS.OPrtErr="Port serwera musi być z zakresu 1-65535";
ovpnS.GTAPErr="Gargoyle nie wspiera konfiguracji typu TAP OpenVPN";
ovpnS.RunC="Uruchomiony, połączony";
ovpnS.RunNC="Uruchomiony, niepołączony";
ovpnS.RunNot="Nie uruchomiony";
ovpnS.IntIP="Wewnętrzne IP\n(obsługiwana podsieć)";
ovpnS.CfgCredF="Poświadczenia\ni konfiguracja";
ovpnS.Dload="Pobierz";
ovpnS.DDNS="Dynamiczny DNS";
ovpnS.WANIP="WAN IP";
ovpnS.OthIPD="Inny adres lub domena (podane poniżej)";
ovpnS.ClntIntIP="Podany adres IP klienta";
ovpnS.OSubErr="nie jest z zakresu podsieci OpenVPN";
ovpnS.AddCErr="Nie można dodać konfiguracji klienta.";
ovpnS.UpCErr="Nie można zaktualizować konfiguracji klienta.";

//openvpn_allowed_client_edit.sh
ovpnS.EditOCS="Edycja ustawień klienta OpenVPN";

//openvpn_upload_client.sh (handled by shell scripts)
ovpnS.uc_CA_f="Nie można znaleźć pliku CA";
ovpnS.uc_crt_f="Nie można znaleźć pliku certyfikatu";
ovpnS.uc_key_f="Nie można znaleźć pliku klucza";
ovpnS.uc_cfg_f="Nie można znaleźć pliku konfiguracji";
ovpnS.uc_TAP_Err="Gargoyle nie wspiera konfiguracji typu TAP OpenVPN";
ovpnS.uc_conn_Err="Parametry zostały zapisane, ale nie zostało nawiązane połączenie z serwerem OpenVPN. Należy sprawdzić ponownie konfigurację.";

//openvpn_connections.sh
ovpnS.ConnOC="Połączone klienty OpenVPN";

//openvpn_connections.js
ovpnS.ConnFr="Połączony z";
ovpnS.ConnSc="Połączony od";
ovpnS.NoCConn="Brak połączonych klientów";

ovpnS.NOVPNT="Ruch poza VPN";
ovpnS.AllowNOVPNT="Akceptuj";
ovpnS.BlockNOVPNT="Blokuj";
openS.DescNOVPNT="Jeżeli cały ruch ma przechodzić przez VPN, najlepiej jest blokować inny ruch, dzięki czemu jeżeli nie będzie działał tunel OpenVPN to nie będzie ruchu niezaszyfrowanego. Jeżeli OpenVPN używany jest tylko do dostępu do zdalnych zasobów, inny ruch może być akceptowalny.";
