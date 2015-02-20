/*
 * UTF-8 (with BOM) English-EN text strings for OpenVPN elements
 */

ovpnS.OCfg="OpenVPN Configuration";
ovpnS.ODis="OpenVPN Disabled";
ovpnS.OClt="OpenVPN Client";
ovpnS.OSrv="OpenVPN Server";
ovpnS.OSts="OpenVPN Status";
ovpnS.OClrK="Clear All Existing OpenVPN Keys";
ovpnS.OClrC="This will permanently delete all keys in your previous configuration, are you sure you want to continue?"
ovpnS.OSCfg="OpenVPN Server: Configuration";
ovpnS.OInIP="OpenVPN Internal IP";
ovpnS.OIMsk="OpenVPN Internal Subnet Mask";
ovpnS.OPrt="OpenVPN Port";
ovpnS.OProto="OpenVPN Protocol";
ovpnS.OCiph="OpenVPN Cipher";
ovpnS.CCTr="Client-To-Client Traffic";
ovpnS.CtoC="Allow Clients To Communicate With Each Other";
ovpnS.CtoS="Clients Can Only Communicate With Server";
ovpnS.LSAc="LAN Subnet Access";
ovpnS.CtoH="Allow Clients To Access Hosts on LAN";
ovpnS.CnoL="Clients Can Not Access LAN";
ovpnS.CredR="Credential Re-Use";
ovpnS.CredSC="Credentials Are Specific to Each Client";
ovpnS.CredMC="Credentials Can Be Used By Multiple Clients";
ovpnS.CUse="Clients Use VPN For";
ovpnS.ATrff="All Client Traffic";
ovpnS.HTrff="Only Traffic Destined for Hosts Behind VPN";
ovpnS.OSAC="OpenVPN Server: Allowed Clients";
ovpnS.CClnt="Currently Configured Clients";
ovpnS.ZipCred="After generating client configuration, click download to obtain zip file containing necessary credentials, and place in your client&lsquo;s OpenVPN configuration folder";
ovpnS.CfgCred="Configure A New Client / Set of Credentials";
ovpnS.ClntN="Client Name";
ovpnS.ClntIP="Client Internal IP";
ovpnS.ClntConn="Client Connects To";
ovpnS.ClntSubN="Subnet Behind Client";
ovpnS.NoSub="No Subnet Defined";
ovpnS.RtSub="Route The Subnet Below";
ovpnS.SubIP="Subnet IP";
ovpnS.SubM="Subnet Mask";
ovpnS.UpCfgF="Upload Client Configuration File(s)";
ovpnS.CfgMan="Configure Client Manually";
ovpnS.UpFmt="Upload Format";
ovpnS.SZipF="Single Zip File";
ovpnS.CfgF="Individual Configuration Files";
ovpnS.ZipF="Zip File";
ovpnS.OCfgF="OpenVPN Config File";
ovpnS.CACF="CA Certificate File";
ovpnS.CCertF="Client Certificate File";
ovpnS.CKeyF="Client Key File";
ovpnS.TAKeyF="TLS-Auth Key File";
ovpnS.UseTAK="Use TLS-Auth Key";
ovpnS.OSrvAddr="OpenVPN Server Address";
ovpnS.OSrvPrt="OpenVPN Server Port";
ovpnS.Othr="Other";
ovpnS.Cphr="Cipher";
ovpnS.Keyopt="Key Size (optional)";
ovpnS.CfgUpd="Configuration below is updated automatically from parameters specified above";
ovpnS.CACert="CA Certificate";
ovpnS.CCert="Client Certificate";
ovpnS.CKey="Client Key";
ovpnS.TAKey="TLS-Auth Key";
ovpnS.TADir="TLS-Auth Direction";
ovpnS.Clnt="Client";
ovpnS.Symm="Omitted (Symmetric)";

//javascript
ovpnS.CryptoWaitMsg="This is the first time you have configured an OpenVPN Server.\n\nIt will take 5-10 minutes to generate the necessary cryptographic parameters.  This is a one-time wait -- updates after this one will be fast.\n\nProceed?";
ovpnS.SubMis="Client Subnet Mismatch";
ovpnS.ExpSubN="The OpenVPN expects your router to have a subnet of";
ovpnS.ActSubN="but your router is configured with a subnet of";
ovpnS.WantQ="Do you want to...";
ovpnS.Switch="Switch Router to expected subnet, with IP";
ovpnS.KeepC="Keep Current Subnet and Continue";
ovpnS.SrvPrtErr="OpenVPN server port conflicts with";
ovpnS.SrvAddErr="Server address is not defined";
ovpnS.OPrtErr="OpenVPN Port must be between 1-65535";
ovpnS.GTAPErr="Gargoyle does not support TAP OpenVPN configurations";
ovpnS.RunC="Running, Connected";
ovpnS.RunNC="Running, Not Connected";
ovpnS.RunNot="Not Running";
ovpnS.IntIP="Internal IP\n(Routed Subnet)";
ovpnS.CfgCredF="Credentials\n&amp; Config Files";
ovpnS.Dload="Download";
ovpnS.DDNS="Dynamic DNS";
ovpnS.WANIP="WAN IP";
ovpnS.OthIPD="Other IP or Domain (specified below)";
ovpnS.ClntIntIP="Specified Client Internal IP";
ovpnS.OSubErr="is not in OpenVPN Subnet";
ovpnS.AddCErr="Could not add client configuration.";
ovpnS.UpCErr="Could not update client configuration.";

//openvpn_allowed_client_edit.sh
ovpnS.EditOCS="Edit OpenVPN Client Settings";

//openvpn_upload_client.sh (handled by shell scripts)
ovpnS.uc_CA_f="Could not find CA file";
ovpnS.uc_crt_f="Could not find certificate file";
ovpnS.uc_key_f="Could not find key File";
ovpnS.uc_cfg_f="Could not find config file";
ovpnS.uc_TAP_Err="Gargoyle does not support TAP OpenVPN configurations";
ovpnS.uc_conn_Err="Parameters saved but OpenVPN failed to connect. Re-check your configuration.";

//openvpn_connections.sh
ovpnS.ConnOC="Connected OpenVPN Clients";

//openvpn_connections.js
ovpnS.ConnFr="Connected From";
ovpnS.ConnSc="Connected Since";
ovpnS.NoCConn="No Clients Connected";

ovpnS.NOVPNT="Non-OpenVPN Traffic";
ovpnS.AllowNOVPNT="Allow Non-OpenVPN Traffic";
ovpnS.BlockNOVPNT="Block All Non-OpenVPN Traffic";
openS.DescNOVPNT="If you want all traffic to pass through the VPN, it is best to block all non-OpenVPN traffic so that if OpenVPN fails traffic will not fallback to the default, unencrypted route. However if you are using OpenVPN only to access some remote clients you should allow Non-OpenVPN traffic.";
