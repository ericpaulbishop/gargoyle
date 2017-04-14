/*
 * UTF-8 (with BOM) French-FR text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.mQDl="QoS Download";
qosStr.mQUl="QoS Upload";
qosStr.URSection="QoS (Upload) -- Règles de classement";
qosStr.DRSection="QoS (Download) -- Règles de classement";
qosStr.UCSection="QoS (Upload) -- Classes de service";
qosStr.DCSection="QoS (Download) -- Classes de service";
qosStr.DACCSect="QoS (Download) -- Active Congestion Control";
qosStr.UEnable="Activer QoS (Upload Direction)";
qosStr.DEnable="Activer QoS (Download Direction)";
qosStr.UTotBand="Bande passante totale (Upload)";
qosStr.DTotBand="Bande passante totale (Download)";

qosStr.USCAbout="Chaque classe de service en upload est définie par 3 paramètres: % de bande passante à capacité, bande passante minimum et maximum.";
qosStr.DSCAbout="Chaque classe de service est définie par 4 paramètres: % de bande passante à capacité, bande passante temps réel, bande passante maximum et le drapeau d&apos;optimisation du temps de parcours (RTT).";
qosStr.UMinBandAbout="<em>Bande passante minimum</em> spécifie le service minimum alloué à cette classe quand ce lien est à capacité. Pour certaines applications comme la VoIP ou le jeu en ligne, il est préférable de spécifier un service minimum en bps plutôt qu&apos;en pourcentage. QoS satisfera d&apos;abord le service minimum de toutes les classes avant d&apos;allouer le service restant à d&apos;autres classes en attente.";
qosStr.DMinBandAbout="<em>Bande passante minimum</em> spécifie le service minimum alloué à cette classe quand ce lien est à capacité. Les classes qui spécifient un service minimum sont appelées classes temps réel par le contrôle de congestion (ACC). Le streaming vidéo, la VoIP et le jeu en ligne sont autant d'exemples d'application qui doivent avoir une bande passante minimum pour fonctionner. Pour déterminer la valeur à entrer, utiliser l'application sur un réseau non utilisé par autre chose et observer quelle quantité de bande passante elle utilise. Utiliser une valeur légèrement supérieure au résultat dans ce champ. QoS va satisfaire le service minimum de toutes les classes avant d'allouer des ressources au reste des classes, utilisez ce paramètre avec parcimonie.";
qosStr.UTotBandAbout="<em>Bande passante totale en upload</em> devrait être d&apos;environ 98% de la bande passante upload disponible. Entrer une valeur trop haute fera que QoS ne pourra satisfaire ses critères de classe. Entrer une valeur trop basse pénalisera inutilement votre vitesse d&apos;upload. Si vous utilisez une connexion PPPoE vérifiez les débits de votre modem et utilisez le débit d&apos;uplink de votre ligne comme valeur. Pour les autres types de connexions, utilisez un programme de test de vitesse (avec QoS désactivé) pour déterminer les bandes passantes disponibles. Notez que la bande passante est souvent spécifiée en kilobits par seconde (kbps). Il y a 8 kilobits par kiloOctet";
qosStr.DTotBandAbout="Spécifier la <em>bande passante totale en download</em> correctement est crucial pour que QoS fonctionne. Si vous utilisez ACC alors réglez cette valeur au maximum de ce que votre fournisseur Internet vous délivre en réel en download. Si vous utilisez une connexion PPPoE, vérifiez les débits de votre modem et utilisez le débit downlink de votre ligne comme valeur.";
qosStr.PerBandAboutU="<em>Pourcentage de bande passante à capacité</em> est le pourcentage de la totalité de la bande passante disponible qui devrait être alloué à cette classe quand toute la bande passante disponible est utilisée. Si de la bande passante est libre, un supplément peut (et sera) alloué. Les pourcentages peuvent être réglé de façon à totaliser environ 100, mais lorsque les réglages seront appliqués, ils seront corrigés pour faire exactement 100.";
qosStr.PerBandAboutD="<em>Pourcentage de bande passante à capacité</em> est le pourcentage de la totalité de la bande passante disponible qui devrait être alloué à cette classe quand toute la bande passante disponible est utilisée. Si de la bande passante est libre, un supplément peut (et sera) alloué. Les pourcentages peuvent être réglé de façon à totaliser environ 100, mais lorsque les réglages seront appliqués, ils seront corrigés pour faire exactement 100. Ce réglage ne sera activé que si le lien WAN est saturé.";
qosStr.RTTAbout="<em>Minimiser RTT</em> indique au contrôleur de congestion que vous voulez minimiser le temps de parcours (RTT) quand cette classe est active. Utilisez ce réglage pour le jeu en ligne ou la VoIP qui demandent de faibles RTT (temps de ping). Minimiser ce paramètre entraîne une baisse d&apos;efficacité au niveau de la sortie WAN. Lorsque cette classe est active, le débit WAN va chuter (généralement d&apos;environ 20%).";
qosStr.MinSpeedWarn="Si vous n'utilisez pas ACC, vous devez établir quelle est la vitesse minimum que votre fournisseur vous accorde et régler cette valeur avec. En général les ISPs ne garantissent pas de bande passante minimum. Vous devrez expérimenter un peu avant d'arriver à un résultat. Une approche est de commencer avec une valeur qui est la moitié de ce que ça devrait être et tester votre lien à pleine charge en vérifiant que tout va bien. Vous pouvez alors tester en augmentant par petit incréments jusqu'à ce que QoS ne fonctionne plus correctement. Il se peut qu&apos;après vos tests QoS marche un temps et puis ne marche plus. C'est parce que votre ISP est surchargé par la demande d&apos;autres clients et il ne vous délivre plus la bande passante que vous aviez pendant vos tests. La solution, diminuer votre valeur. Entrez une valeur trop élevée fera que QoS ne pourra satisfaire ses critères de classe. Entrer une valeur trop basse pénalisera inutilement votre vitesse de download. A cause de toutes ces complications, je recommande l&apos;utilisation du contrôleur de congestion (ACC) quand c'est possible. Notez que la bande passante est souvent spécifiée en kilobits par seconde (kbps). Il y a 8 kilobits par kiloOctet";
qosStr.QoSAbout="La Qualité de Service (QoS) fournit un moyen de contrôler comment la bande passante disponible est allouée. Les connexions sont classées en différentes &ldquo;classes de service&rdquo; auxquelles est allouée une part de la bande passante disponible. QoS devrait être utilisé dans les cas où vous devez partager les ressources entre des besoins concurrents. Par exemple si vous voulez que votre téléphone VoIP fonctionne correctement pendant le chargement de vidéos. Un autre cas pourrait être par exemple de réduire le débit du service torrent pendant que vous surfez.";
qosStr.MaxBandAbout="<em>Bande passante maximum</em> spécifie le montant maximum de bande passante qui pourra être alloué à cette classe en kilobit/s. Même si il reste de la bande passante inutilisée, cette classe de service ne pourra utiliser plus que cette limite.";
qosStr.PackAbout="Les paquets sont testés par rapport aux règles dans l&apos;ordre spécifié -- les règles qui sont en haut de la pile ont une priorité élevée. Dés qu'un paquet vérifie une règle, il est classé et les autres règles sont ignorées. L&apos;ordre des règles peut être modifié en utilisant les flèches.";
qosStr.DefServClassAbout="La <em>classe de service par défaut</em> spécifie comment seront traités les paquets qui ne vérifient aucune des autres règles.";
qosStr.AbACC="<p>Le contrôle de congestion (ACC) observe l&apos;évolution de votre download et ajuste automatiquement votre limite de download pour maintenir une performance QoS acceptable. ACC compense automatiquement les variations de la vitesse de download provenant de votre ISP et celle de la demande de votre réseau. Il ajuste la vitesse du lien à la plus élevée possible compatible avec un fonctionnement correct de QoS. La plage effective de ce contrôle se situe entre 15% et 100% de la bande passante totale que vous avez renseignée au dessus.</p><p>Comme ACC ne gère pas la vitesse du lien en upload, vous devez activer et configurer correctement votre QoS en upload pour qu'il fonctionne.</p><p><em>Destination de ping-</em> La portion de réseau entre votre routeur et la destination du ping est la base de référence pour le contrôle de congestion. En contrôlant le temps de parcours (temps ping) jusqu'à la destination, la congestion est détectée. Par défaut, ACC utilise votre passerelle WAN comme destination. Si vous savez que la congestion arrivera sur une portion de réseau différente, vous pouvez pouvez renseigner une destination de ping alternative.</p><p><em>Limite de ping manuelle-</em> Les temps RTT sont comparés aux limites de ping. ACC contrôle la limite du lien pour maintenir les temps de ping en dessous de la limite appropriée. Par défaut Gargoyle essaye de déterminer automatiquement la limite de ping de destination appropriée pour vous. Elle est basée sur la vitesse du lien que vous avez renseignée et la performance de votre lien mesurée pendant l'initialisation. Vous ne pouvez pas changer le temps de ping destination pour le mode minRTT mais en renseignant une valeur manuellement vous pouvez choisir le temps de ping destination du mode actif. Le temps que vous entrez s&apos;ajoute au temps de ping destination du mode minRTT. Vous pouvez voir les limites que ACC utilise dans les [] crochets à côté du champ limite de temps ping. </p>";
qosStr.ServClass="Classe de service par défaut";

qosStr.AddNewClassRule="Ajouter nouvelle règle de classement";
qosStr.AddNewServiceRule="Ajouter nouvelle classe de service";
qosStr.SrcIP="IP source";
qosStr.SrcPort="Port(s) source";
qosStr.DstIP="IP de destination";
qosStr.DstPort="Port(s) de destination";
qosStr.MaxPktLen="longueur de paquet maximum";
qosStr.MinPktLen="longueur de paquet minimum";
qosStr.TrProto="Protocole de transport";
qosStr.Conreach="Connection bytes reach";
qosStr.AppProto="Protocole applicatif (Layer7)";
qosStr.SetClass="Régler la classe de service à";
qosStr.SrvClassName="Nom de la classe de service";
qosStr.PerBandCap="Percent Bandwidth At Capacity";
qosStr.BandMin="Minimum de bande passante";
qosStr.BandMinNo="Pas de minimum de bande passante";
qosStr.BandMax="Maximum de bande passante";
qosStr.BandMaxNo="Pas de maximum de bande passante";
qosStr.MinRTT="Minimiser temps de parcours (RTT)";
qosStr.ActRTT="Minimiser RTT (temps de ping) quand actif";
qosStr.OptiWAN="Optimiser l'utilisation du WAN";
qosStr.ACCOn="Activer contrôle de congestion (ACC)(Download Direction)";
qosStr.ACC_Pt="Utiliser destination de ping non standard";
qosStr.ACC_con="Contrôler manuellement temps de ping cible";
qosStr.ACC_Stat="Statut du contrôle de congestion";
qosStr.ACC_L_Ck="Vérifier pour voir si la destination du ping va répondre";
qosStr.ACC_L_In="Estimer une limite de ping";
qosStr.ACC_L_Act="Contrôle de congestion actif.";
qosStr.ACC_L_Min="Contrôle de congestion actif, classe minRTT active.";
qosStr.ACC_L_Id="Pas de congestion, contrôle en veille.";
qosStr.ACC_L_Dis="Le contrôleur n&apos;est pas activé";
qosStr.ACC_L_Lim="La limite de bande passante de téléchargement en cours.";
qosStr.ACC_L_Fr="La limite de bande passante de téléchargement a priori équitable.";
qosStr.ACC_L_Ld="Trafic actuel en downlink.";
qosStr.ACC_L_pg="Le temps de parcours (RTT) du dernier ping.";
qosStr.ACC_L_Flt="Le temps de parcours (RTT) filtré.";
qosStr.ACC_L_plim="Seuil à partir duquel le contrôleur va agir pour maintenir l&apos;équité.";
qosStr.ACC_L_AC="Nombre de classes de téléchargement dont la charge est supérieure à 4 kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Modifier classe de service QoS";
qosStr.QESrvName="Nom de la classe de service";

//qos_edit_rule.sh
qosStr.QERulClass="Modifier règle de classement QoS";

//javascript
qosStr.MatchC="Vérifie critère";
qosStr.Classn="Classement";
qosStr.Comment="Commentaire";
qosStr.Src="Source";
qosStr.SrcP="Port source";
qosStr.Dst="Destination";
qosStr.DstP="Port destination";
qosStr.Connb="Octets de connexion";
qosStr.APro="Protocole applicatif";
qosStr.pBdW="Pourcentage BP";
qosStr.mBdW="Min BP";
qosStr.MBdW="Max BP";
qosStr.qLd="Charge";
qosStr.CrErr="Aucun critère de correspondance sélectionné.";
qosStr.SvErr="Ne peut pas ajouter nouvelle classe de service.";
qosStr.SUErr="Ne peut pas modifier classe de service.";
qosStr.CsErr="Ne peut pas ajouter règle de classement.";
qosStr.CUErr="Ne peut pas modifier règle de classement.";
qosStr.DCErr="Doublon de nom de classe.";
qosStr.RemSCErr="Au moins une classe de service est requise.\nNe peut enlever classe de service.";
qosStr.TotErr="Il y a une erreur dans le champ Bande passante totale.\n\nNe peut modifier QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="sans limite";
qosStr.ZERO="zéro";
qosStr.YES="Oui";

//qos_distribution.sh
qosStr.mQOS="Distribution QoS";
qosStr.UBSect="Distribution QoS de la BP upload";
qosStr.DBSect="Distribution QoS de la BP download";
qosStr.uTFrm="Fenêtre temps upload";
qosStr.dTFrm="Fenêtre temps download";
