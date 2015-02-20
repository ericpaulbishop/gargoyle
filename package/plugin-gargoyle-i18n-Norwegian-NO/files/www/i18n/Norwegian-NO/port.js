/*
 * UTF-8 (with BOM) English-EN text strings for port_forwarding(single,multi).sh html elements
 */

prtS.PISect="Individuell Port Videresending";
prtS.PRSect="Port Område Videresending";
prtS.ForIPort="Videresende Individuelle Porter Fra WAN til LAN";
prtS.ForRPort="Videresende Port Område Fra WAN til LAN";
prtS.DMZ="DMZ";
prtS.UseDMZ="Bruk DMZ (De-Militarized Zone)";
prtS.DMZIP="DMZ IP";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="Aktiver UPnP &amp; NAT-PMP tjeneste";
prtS.APFor="Aktiver Port videresending";
prtS.USpd="Opplasting hastighet å rapportere";
prtS.DSpd="Nedlasting hastighet å rapportere";
prtS.UPHelp="UPnP (Universal Plug and Play) og NAT-PMP (NAT Port Mapping Protocol) er begge protokoller som tillater at enheter eller tjenester i ditt LAN automatisk kan konfigurere ruterens port videresending til det som er nødvendig for dem. Om en enhet støtter en av protokollene er det ikke nødvendig å sette opp port videresending manuelt(se på toppen av siden) da det automatisk vil bli opprettet av enheten.</p><p>Når aktivert vil Gargoyle automatisk vise en tabell med opprettede port videresendinger, du kan derfor se hvilken enhet som har spurt om videresending og verifisere at det fungerer korrekt. Det er ikke sikkert denne tjenesten virker som den skal om du har to eller flere rutere i nettverket (dobbel NAT). Om du ser en enkel rad med '***' betyr det at det ikke er registrert noen port videresendinger.</p><p>Som en del av protokollen kan LAN enheten spørre ruteren om hastigheten på WAN oppkoblingen. To felt er satt for å konfigurere svaret på slike spørringer. Klient programmer kan bruke denne informasjonen til å optimere ytelsen. Men det er viktig å vite at ruteren ikke gjør noe for å begrense hastigheten basert på disse data. Da de blir kun rapporter tilbake til enheten som spør. Om null er satt som en av verdiene, vil hastigheten bli rapportert, vanligvis 100MB eller 1GB (avhengig av ruterens hastighet).</p> <p>Det er noe kontrovers om denne tjenesten og det kreves ekstra RAM å kjøre den, noe som kan være mangelvare på mange rutere. Derfor er tjenesten slått av som standard.";

//templates
prtS.Desc="Beskrivelse";
prtS.optl="(valgfritt)";
prtS.Proto="Protokoll";
prtS.FPrt="Fra Port";
prtS.TIP="Til IP";
prtS.TPrt="Til Port";
prtS.SPrt="Start Port";
prtS.EPrt="Slutt Port";

//javascript
prtS.AFRErr="Kunne ikke legge til videresendings regel.";
prtS.GTErr="Start Port > Slutt Port";
prtS.DupErr="Port(er) Innenfor Området er Allerede Videresendte";
prtS.CopErr="Port er Allerede Videresendt";
prtS.UpErr="Kunne ikke oppdater port videresending.";
prtS.Prot="Proto";
prtS.LHst="LAN Klient";
prtS.Port="Port";

//edit.sh pages
prtS.PESect="Endre Port Videresending";
