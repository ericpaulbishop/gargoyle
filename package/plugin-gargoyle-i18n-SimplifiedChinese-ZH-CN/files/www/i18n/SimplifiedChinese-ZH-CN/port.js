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
prtS.UPHelp="UPnP (Universal Plug and Play) and NAT-PMP (NAT Port Mapping Protocol) are both protocols which allows devices and applications on your LAN to automatically configure your router with the port forwards needed for proper operation. If a device supports either protocol it is not necessary to create manual port forward rules (see the top of this page) as they will automatically be created by the device.</p><p>When enabled Gargoyle shows a table of automatically created port forwards so you can see which devices have requested forwards and verify that this feature is working properly. This service may not work correctly in network configurations containing two or more routers (double NAT). If you see a single row with '***' it means there are no port forwards registered.</p><p>As part of the protocol the LAN device can request the speed of the WAN connection from the router. Two fields are provided to configure the response to such queries. Client applications can use this information to optimize their performance. But is important to note that the router does not do anything to limit speeds based on this data.  It is only reported to the requester. If zero is entered for either value the speed of the interface is reported, usually 100MB or 1GB depending on the router’s interface speed.</p> <p>There is some controversy about the security of this service and it does require additional RAM to run which may be important on memory constrained routers, so by default this feature is off.";

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
