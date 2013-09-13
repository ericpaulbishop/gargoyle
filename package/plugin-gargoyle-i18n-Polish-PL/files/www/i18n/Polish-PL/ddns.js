/*
 * UTF-8 (with BOM) Polish-PL text strings for ddns.sh html elements
 */

DyDNS.DYSect="Usługi Dynamiczny DNS";
DyDNS.AddDy="Usługa Dynamiczny DNS";
DyDNS.SvPro="Operator usługi";
DyDNS.ChItv="Okres sprawdzenia";
DyDNS.FUItv="Okres wymuszonej aktualizacji";
DyDNS.AddDDNS="Dodaj usługę DDNS";
DyDNS.HelpCI="<em>Okres sprawdzenia</em> określa jak często router będzie sprawdzał, czy aktualny adres IP pasuje do tego skojarzonego z Twoją domeną. Sprawdzenie jest wykonywane bez połączenia z operatorem usługi Dynamiczny DNS, co oznacza że nie będzie to powodowało problemów z operatorami nakładającymi blokady na użytkowników którzy łączą się zbyt często (np. dyndns.com). Niezbędne jest nawiązanie połączenia siecowego do wykonania tego sprawdzenia, więc ta wartość nie powinna być zbyt niska. Okres sprawdzenia ustawiony pomiędzy 10 a 20 minut jest zwykle odpowiedni.";
DyDNS.HelpFI="<em>Okres wymuszonej aktualizacji</em> określa jak często router będzie łączył się z operatorem usługi Dynamiczny DNS i aktualizował wpisy, nawet jeżeli adres IP nie zmienił się. Operatorzy usług mogą blokować konta użytkowników którzy aktualizują dane zbyt często, ale także zamykać konta które nie się aktualizowane ponad miesiąc. Polecane jest ustawienie tego parametru od 3 do 7 dni.";
DyDNS.UpErr1="Błąd aktualizacji nowej usługi Dynamiczny DNS";
DyDNS.UpErr2="Serwis nie może być zaktualizowany poprawnie, zostanie więc usunięty.";
DyDNS.cNams=["Domena", "Ostatnia aktualizacja", "Włączone", "", "" ];
DyDNS.InvErr="BŁĄD: podany operator jest błędny";
DyDNS.DupErr="Podano powtórzoną domenę.";
DyDNS.ForceU="Aktualizuj";
DyDNS.ModErr="Serwis został dodany lub zmodyfikowany. Zmiany muszą zostać zapisane przed wykonaniem aktualizacji. Kliknij \""+UI.SaveChanges+"\" i sprawdź ponownie.";
DyDNS.UpFErr="Błąd aktualizacji. Upewnij się, że Twoja konfiguracja jest poprawna i jest połączenie do internetu.";
DyDNS.UpOK="Aktualizacja zakończona powodzeniem.";
DyDNS.UpSrvErr="Nie można zaktualizować klasy serwisu.";

//ddns_edit.sh
DyDNS.EDSect="Edycja usługi Dynamiczny DNS";

// /etc/ddns_providers.conf
DyDNS.DoNm="Nazwa domeny";
DyDNS.UsrN="Nazwa użytkownka";
DyDNS.Pssw="Hasło";
DyDNS.Eml="E-mail";
DyDNS.Key="Klucz";
DyDNS.AKey="Klucz API";
DyDNS.Tokn="Token";
