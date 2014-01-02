/*
 * UTF-8 (with BOM) English-EN text strings for ddns.sh html elements
 */

DyDNS.DYSect="Dynamiske DNS Tjenester";
DyDNS.AddDy="Legg til Dynamisk DNS Tjeneste";
DyDNS.SvPro="Tjenesteleverandør";
DyDNS.ChItv="Sjekk Intervall";
DyDNS.FUItv="Tvunget Oppdateringsintervall";
DyDNS.AddDDNS="Legg til DDNS Tjeneste";
DyDNS.HelpCI="<em>Sjekk Intervall</em> angir hvor ofte ruteren vil sjekke om din nåværende IP matcher den som i øyeblikket er knyttet til ditt domenenavn. Denne kontrollen utføres uten å koble til din dynamiske DNS leverandør, noe som betyr at dette ikke vil føre til problemer med leverandører som forbyr brukere som kobler til for ofte (f.eks dyndns.com). Men en nettverkstilkobling blir etablert for å utføre denne kontrollen, så denne verdien ikke bør være for lav heller. Et sjekk intervall på mellom 10 og 20 minutter er vanligvis passende.";
DyDNS.HelpFI="<em>Tvunget Oppdateringsintervall</em> angir hvor ofte ruteren vil koble til den dynamiske DNS leverandøren og oppdatere oppføringene der, dette selv om IP ikke har endret seg. Tjenesteleverandører vil forby brukere som oppdaterer for ofte, men kan også stenge kontoene til brukere som ikke oppdaterer før etter en måned. Det anbefales at denne parameteren settes til mellom 3 og 7 dager.";.
DyDNS.UpErr1="Oppdatering av ny dynamisk DNS tjeneste konfigurasjon(er) mislyktes";
DyDNS.UpErr2="Tjeneste(ne) kunne ikke oppdateres på riktig måte, og har derfor blitt fjernet.";
DyDNS.cNams=["Domene", "Siste Oppdatering", "Aktivert", "", "" ];
DyDNS.InvErr="FEIL: spesifisert leverandøren er ugyldig";
DyDNS.DupErr="Duplikat oppdatering angitt.";
DyDNS.ForceU="Tving Oppdatering";
DyDNS.ModErr="Denne tjenesten har blitt tillagt/endret og derfor må du lagre endringene før en oppdatering kan utføres. Klikk \""+UI.SaveChanges+"\" og prøv igjen.";
DyDNS.UpFErr="Oppdatering mislyktes. Kontroller at konfigurasjonen er gyldig og at du er koblet til Internett.";
DyDNS.UpOK="Oppdatering vellykket.";
DyDNS.UpSrvErr="Kunne ikke oppdatere tjeneste klasse.";

//ddns_edit.sh
DyDNS.EDSect="Rediger Dynamisk DNS Tjeneste";

// /etc/ddns_providers.conf
DyDNS.DoNm="Domene Navn";
DyDNS.UsrN="Brukernavn";
DyDNS.Pssw="Passord";
DyDNS.Eml="E-post";
DyDNS.Key="Nøkkel";
DyDNS.AKey="API Nøkkel";
DyDNS.Tokn="Token";
