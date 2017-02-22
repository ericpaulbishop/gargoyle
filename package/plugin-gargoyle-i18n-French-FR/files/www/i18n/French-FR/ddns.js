/*
 * UTF-8 (with BOM) French-FR text strings for ddns.sh html elements
 */

DyDNS.mDDNS="DNS Dynamique";
DyDNS.DYSect="DNS Dynamique";
DyDNS.AddDy="Ajouter un service DDNS";
DyDNS.SvPro="Fournisseur du service";
DyDNS.ChItv="Intervalle de vérification";
DyDNS.FUItv="Intervalle de mise à jour forcée";
DyDNS.AddDDNS="Ajouter un service DDNS";
DyDNS.HelpCI="<em>Intervalle de vérification</em> définit à quelle fréquence le routeur vérifie que votre IP est bien celle associée avec votre nom de domaine. Cette vérification est faite sans se connecter à votre service DDNS, cela ne causera pas de problème avec les fournisseurs DDNS qui blacklistent les utilisateurs qui se connectent trop souvent (e.g. dyndns.com). Cependant, une connection est établie pour la vérification, cette valeur ne doit donc pas être trop basse. Un intervalle entre 10 et 20 minutes est généralement approprié.";
DyDNS.HelpFI="<em>Intervalle de maj forcée</em> définit à quelle fréquence le routeur se connecte à votre fournisseur DDNS et met à jour leur table, même si votre IP n&apos;a pas changé. Les fournisseurs de DDNS blacklistent les utilisateurs qui le font trop fréquemment, mais risquent aussi de clôturer le compte si aucune mise a jour n&apos;est faite dans le mois. Il est recommandé de régler ce paramètre entre 3 et 7 jours.";
DyDNS.UpErr1="La mise à jour de la nouvelle configuration DDNS a échoué";
DyDNS.UpErr2="Le service n&apos;a pu être mis à jour correctement et a donc été supprimé,";
DyDNS.cNams=["Domaine", "Dernière mise à jour", "Activé", "", ""];
DyDNS.InvErr="ERREUR: le fournisseur indiqué est invalide";
DyDNS.DupErr="Mise à jour spécifiée en doublon.";
DyDNS.ForceU="Forcer la mise à jour";
DyDNS.ModErr="Ce service à été ajouté/modifié et vous devez sauver vos changements avant qu&apos;une mise à jour puisse être effectuée. Cliquez \""+UI.SaveChanges+"\" et réessayez.";
DyDNS.UpFErr="Mise à jour échouée. Vérifiez que votre configuration est valide et que vous êtes connecté à l&apos;Internet,";
DyDNS.UpOK="Mise à jour réussie.";
DyDNS.UpSrvErr="Impossible de mettre à jour la classe de service.";

//ddns_edit.sh
DyDNS.EDSect="Modifier DNS Dynamique";

// /etc/ddns_providers.conf
DyDNS.DoNm="Nom de domaine";
DyDNS.UsrN="Nom utilisateur";
DyDNS.Pssw="Mot de passe";
DyDNS.Eml="E-mail";
DyDNS.Key="Clé";
DyDNS.AKey="Clé API";
DyDNS.Tokn="Token";
