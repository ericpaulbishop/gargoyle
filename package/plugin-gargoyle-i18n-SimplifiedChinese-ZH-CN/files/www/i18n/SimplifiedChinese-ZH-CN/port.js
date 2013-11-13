/*
 * UTF-8 (with BOM) Simplified Chinese ZH-CN text strings for port_forwarding(single,multi).sh html elements
 */

prtS.PISect="单个端口转发";
prtS.PRSect="端口范围转发";
prtS.ForIPort="从WAN到LAN转发单个端口";
prtS.ForRPort="从WAN到LAN转发端口范围";
prtS.DMZ="DMZ";
prtS.UseDMZ="使用DMZ（隔离区）";
prtS.DMZIP="DMZ IP";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="启用UPnP和NAT-PMP服务";
prtS.APFor="活动的端口转发";
prtS.USpd="上传速度报告";
prtS.DSpd="下载速度报告";
prtS.UPHelp="UPnP（通用即插即用）和NAT-PMP（NAT端口映射协议）两种协议允许局域网内的设备和应用程序为自身的正确运行而自动配置路由器的端口转发。如果路由器支持这两者协议则无需手工创建端口转发规则（见本页顶部），而是由路由器自动创建。</p><p>一但启用石像鬼（Gargoyle）将用一张表格显示自动创建的端口转发，你可以看到哪些设备申请了端口转发并检验该功能是否工作正常。当网络上含有两个或以上配置该服务的路由器时（双NAT），该服务可能无法正常工作。</p><p>作为协议的一部分，局域网内的设备可以向路由器请求广域网连接的速度。本页提供了两个字段用于配置这些请求的响应值。客户端程序可以根据这些信息优化他们的性能。但值得注意的是路由器不会基于这些数据做任何速度的限制。它仅仅是报告给请求者。如果在任一接口速度字段输入0，路由器会根据接口速度自动报告，通常是100MB或1GB。</p><p>对于这项服务的安全性存在一些争议且该项服务需要额外的内存来运行，这可能是路由器的一个重要约束，所以该服务默认情况下是关闭的。";

//templates
prtS.Desc="描述";
prtS.optl="（可选）";
prtS.Proto="协议";
prtS.FPrt="从端口";
prtS.TIP="到IP";
prtS.TPrt="到端口";
prtS.SPrt="起始端口";
prtS.EPrt="结束端口";

//javascript
prtS.AFRErr="无法添加转发规则。";
prtS.GTErr="起始端口大于结束端口";
prtS.DupErr="范围内的端口已在转发列表中";
prtS.CopErr="端口已被转发";
prtS.UpErr="无法更新端口转发。";
prtS.Prot="协议";
prtS.LHst="LAN主机";
prtS.Port="端口";

//edit.sh pages
prtS.PESect="编辑端口转发";
