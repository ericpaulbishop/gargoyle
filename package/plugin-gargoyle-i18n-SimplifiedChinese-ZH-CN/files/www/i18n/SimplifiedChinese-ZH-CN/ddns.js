/*
 * UTF-8 (with BOM) Simplified Chinese ZH-CN text strings for ddns.sh html elements
 */

DyDNS.DYSect="动态DNS服务";
DyDNS.AddDy="添加动态DNS服务";
DyDNS.SvPro="服务提供者";
DyDNS.ChItv="检查间隔";
DyDNS.FUItv="强制更新间隔";
DyDNS.AddDDNS="添加DDNS服务";
DyDNS.HelpCI="<em>Check Interval</em> specifies how often the router will check whether your current IP matches the one currently associated with your domain name. This check is performed without connecting to your dynamic DNS service provider, which means that this will not cause problems with providers that ban users who connect too frequently (e.g. dyndns.com). However, a network connection is established to perform this check, so this value should not be too low.  A check interval between 10 and 20 minutes is usually appropriate.";
DyDNS.HelpFI="<em>Force Update Interval</em> specifies how often the router will connect to your dynamic DNS service provider and update their records, even if your IP has not changed. Service providers will ban users who update too frequently, but may close accounts of users who do not update for over a month.  It is recommended that this parameter be set between 3 and 7 days.";
DyDNS.UpErr1="新的动态DNS服务更新失败";
DyDNS.UpErr2="服务不能正确更新，并因此被删除。";
DyDNS.cNams=["域名", "最后更新时间", "启用", "", "" ];
DyDNS.InvErr="错误：指定提供者无效";
DyDNS.DupErr="复制指定的更新。";
DyDNS.ForceU="强制更新";
DyDNS.ModErr="该服务已被添加或修改，进行更新前你需要保存之前的更改。点击\""+UI.SaveChanges+"\"并再试一次。";
DyDNS.UpFErr="更新失败。确保你的配置是有效的，并且你已连接到互联网。";
DyDNS.UpOK="更新成功。";
DyDNS.UpSrvErr="无法更新服务类。";

//ddns_edit.sh
DyDNS.EDSect="编辑动态DNS服务";

// /etc/ddns_providers.conf
DyDNS.DoNm="域名";
DyDNS.UsrN="用户名";
DyDNS.Pssw="密码";
DyDNS.Eml="E-mail";
DyDNS.Key="Key";
DyDNS.AKey="API Key";
DyDNS.Tokn="Token";
