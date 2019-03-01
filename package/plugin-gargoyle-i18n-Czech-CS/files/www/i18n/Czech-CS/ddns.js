/*
 * UTF-8 (with BOM) Czech-CS text strings for ddns.sh html elements
 */

DyDNS.mDDNS="Dynamická DNS";
DyDNS.DYSect="Služby dynamické DNS";
DyDNS.AddDy="Přidat službu dynamické DNS";
DyDNS.SvPro="Poskytovatel služby";
DyDNS.ChItv="Interval kontroly";
DyDNS.FUItv="Interval vynucené aktualizace";
DyDNS.AddDDNS="Přidat službu DDNS";
DyDNS.HelpCI="<em>Interval kontroly</em> určuje jak často router zkontroluje, zda Vaše aktuální IP se shoduje s IP aktuální provázanou s Vaším názvem domény. Tato kontrola se provádí bez připojení k Vašemu poskytovateli služby dynamického DNS, což znamená , že to nebude způsobovat problémy s poskytovateli kteří blokují uživatele, kteří se připojují příliš často (např. dyndns.com). Avšak z důvodu provedení kontroly se vytváří spojení, takže tato hodnota by neměla být příliš nízká. Interval kontroly v rozmendzí 10 až 20 minut je obvykle dostačující.";
DyDNS.HelpFI="<em>Interval vynucené aktualizace</em> určuje, jak často se router připojí k Vašemu poskytovateli služeb dynamického DNS a aktualizuje jeho záznamy, i když se Vaše IP nezměnila. Poskytovatelé služeb blokují uživatele, kteří aktualizují příliš často, ale mohou zrušit účty uživatelům, kteří neaktualizují déle než měsíc. Doporučuje se nastavit tento parametr na rozmezí 3 až 7 dní.";
DyDNS.UpErr1="Aktualizace nové konfigurace služby dynamické DNS selhala";
DyDNS.UpErr2="Služba(y) nemohla být správně aktualizována a byla proto odstraněna.";
DyDNS.cNams=["Doména", "Poslední aktualizace", "Povolené", "", ""];
DyDNS.InvErr="CHYBA: uvedený poskytovatel je neplatný";
DyDNS.DupErr="Uvedená duplicitní aktualizace.";
DyDNS.ForceU="Vynutit aktualizaci";
DyDNS.ModErr="Tato služba byla přidána/upravena a proto je nutné uložit Vaše změny před provedením aktualizace. Klikněte \""+UI.SaveChanges+"\" a zkuste znovu.";
DyDNS.UpFErr="Aktualizace selhala. Zkontrolujte, zda Vaše konfigurace je platná a zda jste připojeni k internetu.";
DyDNS.UpOK="Aktualizace byla úspěšná.";
DyDNS.UpSrvErr="Nepodařilo se aktualizovat třídu služby.";
DyDNS.NoScriptErr="Pro použití tohoto poskytovatele musíte nainstalovat další balíček!";

//ddns_edit.sh
DyDNS.EDSect="Úprava služby dynamické DNS";

// /etc/ddns_providers.conf
DyDNS.DoNm="Název domény";
DyDNS.UsrN="Název uživatele";
DyDNS.Pssw="Heslo";
DyDNS.Eml="E-mail";
DyDNS.Key="Klíč";
DyDNS.AKey="Klíč API";
DyDNS.Tokn="Token";
