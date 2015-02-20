/*
 * UTF-8 (with BOM) English-EN text strings for ddns.sh html elements
 */

DyDNS.DYSect="Dynamic DNS Services";
DyDNS.AddDy="Add Dynamic DNS Service";
DyDNS.SvPro="Service Provider";
DyDNS.ChItv="Check Interval";
DyDNS.FUItv="Force Update Interval";
DyDNS.AddDDNS="Add DDNS Service";
DyDNS.HelpCI="<em>Check Interval</em> specifies how often the router will check whether your current IP matches the one currently associated with your domain name. This check is performed without connecting to your dynamic DNS service provider, which means that this will not cause problems with providers that ban users who connect too frequently (e.g. dyndns.com). However, a network connection is established to perform this check, so this value should not be too low.  A check interval between 10 and 20 minutes is usually appropriate.";
DyDNS.HelpFI="<em>Force Update Interval</em> specifies how often the router will connect to your dynamic DNS service provider and update their records, even if your IP has not changed. Service providers will ban users who update too frequently, but may close accounts of users who do not update for over a month.  It is recommended that this parameter be set between 3 and 7 days.";
DyDNS.UpErr1="Update of new dynamic DNS service configuration(s) failed";
DyDNS.UpErr2="Service(s) could not be updated properly and have therefore been removed.";
DyDNS.cNams=["Domain", "Last Update", "Enabled", "", "" ];
DyDNS.InvErr="ERROR: specified provider is invalid";
DyDNS.DupErr="Duplicate update specified.";
DyDNS.ForceU="Force Update";
DyDNS.ModErr="This service has been added/modified and therefore you must save your changes before an update can be performed. Click \""+UI.SaveChanges+"\" and try again.";
DyDNS.UpFErr="Update failed.  Ensure your configuration is valid and that you are connected to the internet.";
DyDNS.UpOK="Update successful.";
DyDNS.UpSrvErr="Could not update service class.";

//ddns_edit.sh
DyDNS.EDSect="Edit Dynamic DNS Service";

// /etc/ddns_providers.conf
DyDNS.DoNm="Domain Name";
DyDNS.UsrN="User Name";
DyDNS.Pssw="Password";
DyDNS.Eml="E-mail";
DyDNS.Key="Key";
DyDNS.AKey="API Key";
DyDNS.Tokn="Token";
