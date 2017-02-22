/*
 * UTF-8 (with BOM) French-FR text strings for port_forwarding(single,multi).sh html elements
 */

prtS.mPFwding="Transfert de port";
prtS.PISect="Transfert de port individuel";
prtS.PRSect="Transfert d&apos;une plage de port";
prtS.ForIPort="Transfère des ports individuels du WAN vers le LAN";
prtS.ForRPort="Transfère des plages de port du WAN vers le LAN";
prtS.DMZ="DMZ";
prtS.UseDMZ="Utiliser DMZ (Zone démilitarisée)";
prtS.DMZIP="IP DMZ";
prtS.UP_NAT="UPnP/NAT-PMP";
prtS.UPNAT_En="Activer le service UPnP &amp; NAT-PMP";
prtS.APFor="Transferts de port actifs";
prtS.USpd="Upload speed to report";
prtS.DSpd="Download speed to report";
prtS.UPHelp="UPnP (Universal Plug and Play) et NAT-PMP (NAT Port Mapping Protocol) sont tous les deux des protocoles qui autorisent des applications et des matériels sur votre LAN à automatiquement configurer votre routeur avec les transferts de port nécessaires à leur fonctionnement. Si un matériel supporte l&apos;un de ces protocoles, il n&apos;est pas nécessaire de créer ces règles manuellement (voir le haut de cette page) car elle le seront automatiquement par le matériel.</p><p>Quand c&apos;est activé, Gargoyle montre la table des ports créés automatiquement pour que vous puissiez voir quels matériels ont créé des règles et vérifier que cette fonctionnalité fonctionne correctement. Ce service peut malfonctionner dans des configurations de réseau contenant plus de 2 routeurs (double NAT). Si vous voyez une unique colonne avec &apos;***&apos; cela veut dire qu&apos;il n&apos;y a pas de transfert de port enregistré.</p><p>Dans ce protocole le matériel sur le LAN peut demander au routeur la vitesse de la connexion Internet (WAN). Deux champs sont fournis pour configurer la réponse à de telles requêtes. Les applications clientes peuvent utiliser cette information pour optimiser leurs performances. Mais il est important de noter que le routeur ne fait rien pour limiter la vitesse basée sur ces données. C&apos;est seulement envoyé au demandeur. Si la valeur zéro est renseignée dans les champs, la vitesse de l&apos;interface physique est envoyée, généralement 100MO ou 1GO en fonction des caractéristiques de l&apos;interface réseau de votre routeur.</p> <p>Il y a controverse concernant la sécurité de ce service et il utilise de la mémoire RAM pour fonctionner ce qui peut être crucial sur des routeurs ayant peu de RAM, cette fonctionnalité est donc désactivée par défaut.";

//templates
prtS.Desc="Description";
prtS.optl="(optionnel)";
prtS.Proto="Protocole";
prtS.FPrt="Du port";
prtS.TIP="vers l&apos;IP";
prtS.TPrt="vers le port";
prtS.SPrt="Port de début";
prtS.EPrt="Port de fin";

//javascript
prtS.AFRErr="Ne peut pas ajouter la règle de transfert.";
prtS.GTErr="Port de début > port de fin";
prtS.DupErr="Certains ports de cette plage sont déjà transférés";
prtS.CopErr="Le port est déjà transféré";
prtS.UpErr="Ne peut pas mettre à jour la règle de transfert.";
prtS.Prot="Proto";
prtS.LHst="Hôte LAN";
prtS.Port="Port";

//edit.sh pages
prtS.PESect="Modifier le transfert de port";
