/*
 * UTF-8 (with BOM) Simplified Chinese ZH-CN text strings for ddns.sh html elements
 */

DyDNS.DYSect="动态DNS服务";
DyDNS.AddDy="添加动态DNS服务";
DyDNS.SvPro="服务提供商";
DyDNS.ChItv="检查时间间隔";
DyDNS.FUItv="强制更新间隔";
DyDNS.AddDDNS="添加DDNS服务";
DyDNS.HelpCI="<em>检查时间间隔：</em>指的是路由器多长时间检查一次当前IP与你的域名是否匹配。该检查不会连接到你的动态域名提供商，因此不会引起由于用户连接过于频繁而被服务提供商禁止的问题（如：dyndns.com）。然而，要执行检查需要建立一个外网连接，所有这个值不应该设得太低。通常检查时间间隔设置在10至20分钟比较合适。";
DyDNS.HelpFI="<em>强制更新间隔：</em>指定路由器将在多长时间连接到你的动态域名提供商并更新纪录，即使你的IP没有变化。服务提供商禁止用户过于频繁的更新，但可能会因为用户超过一个月没更新而关闭其账号。建议该参数设置在3至7天。";
DyDNS.UpErr1="动态DNS服务更新失败";
DyDNS.UpErr2="服务不能正确更新，并因此被删除。";
DyDNS.cNams=["域名", "最后更新时间", "启用", "", ""];
DyDNS.InvErr="错误：指定动态DNS提供商无效";
DyDNS.DupErr="指定的域名已存在。";
DyDNS.ForceU="强制更新";
DyDNS.ModErr="该动态域名已被修改，在进行更新前你需要保存之前的更改。先点击\+UI.SaveChanges+\"然后再试一次。"";
DyDNS.UpFErr="更新失败。确保你的配置是有效的，并且你已连接到互联网。";
DyDNS.UpOK="更新成功。";
DyDNS.UpSrvErr="无法更新该类服务。";

//ddns_edit.sh
DyDNS.EDSect="编辑动态DNS服务";

// /etc/ddns_providers.conf
DyDNS.DoNm="域名";
DyDNS.UsrN="用户名";
DyDNS.Pssw="密码";
DyDNS.Eml="E-mail";
DyDNS.Key="Key";
DyDNS.AKey="API Key";
DyDNS.Tokn="令牌";
