﻿/*
 * UTF-8 (with BOM) Simplified Chinese ZH-CN text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS（上传）－分类规则";
qosStr.DRSection="QoS（下载）－分类规则";
qosStr.UCSection="QoS（上传）－服务类型";
qosStr.DCSection="QoS（下载）－服务类型";
qosStr.DACCSect="QoS（下载）－主动拥塞控制";
qosStr.UEnable="启用Qos服务（上传）";
qosStr.DEnable="启用Qos服务（下载）";
qosStr.UTotBand="上传总带宽";
qosStr.DTotBand="下载总带宽";

qosStr.USCAbout="每个上传服务类型由三个参数指定：带宽占用百分比、最小带宽和最大带宽";
qosStr.DSCAbout="每个下载服务类型由四个参数指定：带宽占用百分比、实时带宽、最大带宽和最小往返延时标志。";
qosStr.UMinBandAbout="<em>最小带宽：</em>指定将被分配用于该服务类型的最低链路带宽容量。对于某些应用如VoIP或在线游戏最好使用一个固定值而不是一个百分比来设置带宽。在分配剩余带宽到其它服务类型前，QoS将优先满足所有服务类型的最小带宽要求。";
qosStr.DMinBandAbout="<em>最小带宽：</em>指定将被分配用于该服务类型的最低链路带宽容量。指定了最小带宽的服务类型会被主动拥塞控制器看作实时类。视频流、VoIP和在线互动游戏全都是必须设置最小带宽的例子。要确定需要多少最小带宽，可以在一个没有负载的局域网中使用应用程序并观察它使用了多少带宽。然后输入一个略高于这个值的数字到字段中。在分配剩余带宽到其它服务类型前，QoS将优先满足所有服务类型的最小带宽要求，所以要谨慎地使用最小带宽。";
qosStr.UTotBandAbout="<em>上传总带宽：</em>应被设置为你可用上传带宽的98%左右。输入数值太高将导致QoS不能匹配服务类型的要求。输入数值太低将造成不必要的上传速度限制。如果你使用PPPoE连接则要检查你Modem的管理页面并使用你的上行速度作为你的上传带宽。其他连接类型应使用测速程序（QoS需关闭）以确定可用的上传带宽。要注意的是带宽以kbps（千比特率）为单位，8Kbps等于1Kbyte（千字节）。";
qosStr.DTotBandAbout="<em>下载总带宽：</em>设置正确对于QoS的工作至关重要。如果你启用主动拥塞，那只需将它设置为ISP提供给你的最大下载速度即可。如果你使用PPPoE连接则要检查你Modem的管理页面并把它设置为你DSL的下行速度。";
qosStr.PerBandAboutU="<em>带宽占用百分比：</em>是指当所有可用带宽被占满后该服务类型占据总带宽的百分比。如果带宽未用完，该服务类型会被分配更多带宽。该百分比值可被设置为等于、大于或小于100，但当设置被应用时，百分比值将会被按比例调整以便使它们加起来等于100。";
qosStr.PerBandAboutD="<em>带宽占用百分比：</em>是指当所有可用带宽被占满后该服务类型占据总带宽的百分比。如果带宽未用完，该服务类型会被分配更多带宽。该百分比值可被设置为等于、大于或小于100，但当设置被应用时，百分比值将会被按比例调整以便使它们加起来等于100。该设置只在WAN端链路饱和时才生效。<em>（PS：WAN端链路饱和即出口带宽被占用完）</em>";
qosStr.RTTAbout="<em>最小往返延时：</em>告诉主动拥塞控制器你希望该服务类型启用时尽量减少往返延时（RTT）。该设置一般用在VoIP或在线游戏这类需要低延时（ping times）的应用上。减小往返延时（RTT）会带来WAN有效吞吐量的额外花销，所以当这些服务类型启用时你的WAN吞吐量将下降（通常在20%左右）。";
qosStr.MinSpeedWarn="如果你不使用主动拥塞控制（ACC）那么你必须确定ISP提供的最低带宽是多少并设置该值。一般情况下ISP不提供最低保证带宽所以你需要自己做些实验以得出这个数值。其中一种方法是设置一个你认为的数值的一半，然后在你的链路满载的情况下测试并确保一切正常。然后一步步增加、测试直到QoS开始控速。你也可能会看到在你测试后QoS工作一段时间然后停止工作。这是因为你的ISP由于其他客户的需求而超过负载，所以在你测试期间他们不再提供给你带宽。解决方案，降低该数值。输入数值太高将导致Qos不符合等级要求。输入数值太低将造成不必要的下载速度限制。基于所有这些情况，我建议你在可能的情况下使用主动拥塞控制（ACC）。要注意的是带宽以Kbps（千比特率）为单位，8Kbps等于1Kbyte（千字节）。";
qosStr.QoSAbout="服务质量（QoS）提供了一种方法来控制分配可用带宽。连接被归类为不同的“服务类型”，各个类型被分配一个份额的可用带宽。QoS适用于需要在相互竞争的情况下分割可用带宽。例如你希望在下载视频的同时你的VoIP电话也能正常的工作。另一种情况是你希望你在上网冲浪的时候限速你的BT下载。";
qosStr.MaxBandAbout="<em>最大带宽：</em>指定一个以kbit/s为单位的该类型可被分配的带宽最大值。即使存在未使用带宽，该服务类型也将永远不被允许使用超过此量的带宽。";
qosStr.PackAbout="数据包将按规则中指定的顺序进行匹配－－靠上的规则优先匹配。一旦数据包匹配一条规则那它将被归类，并且其余的规则将被忽略。使用上下箭头可调整规则的顺序。";
qosStr.DefServClassAbout="<em>默认服务类型：</em>指定当数据包不匹配任何规则时将被如何归类。";
qosStr.AbACC="<p>主动拥塞控制系统（ACC）观察你的下载活动并自动调整你的下载链接限制以保持适当的QoS性能。ACC自动调整QoS功能以补偿来自你ISP的下载速度变化及来自你网络链接速度的调整需求，使速度最大化。这个控制的有效范围在你上面输入的下载总带宽的15%至100%之间。</p><p>虽然ACC不调整你的上传链路速度，但你必须启用并正确配置你的上传QoS以使该功能正常。</p><p><em>Ping目标：</em>在路由器和Ping目标之间的网络部分是拥塞控制的地方。拥塞通过监视和目标间的Ping延时来检测。默认情况下ACC使用你的WAN网关作为Ping的目标。假如你知道拥塞会在你链路的不同段发生，你可用输入一个备用的Ping目标。</p><p><em>手动Ping限制：</em>Ping延时会与Ping限制进行比较。ACC控制链路限制以保持Ping延时在适当范围。默认情况下，石像鬼（Gargoyle）会自动根据你输入的链接适当为你选择适当的Ping限制。如果你想尝试不同的Ping限制，你可以在这里输入一个时间值。输入高的时间值将导致更高的Ping限制，低的时间值会有更低的限制。你可以在Ping时间限制字段旁边的括号[]中看见ACC正在使用的限制值。</p>";
qosStr.ServClass="默认服务类型";

qosStr.AddNewClassRule="添加新的分类规则";
qosStr.AddNewServiceRule="添加新的服务类型";
qosStr.SrcIP="来源IP";
qosStr.SrcPort="来源端口（或范围）";
qosStr.DstIP="目标IP";
qosStr.DstPort="目标端口（或范围）";
qosStr.MaxPktLen="最大包长";
qosStr.MinPktLen="最小包长";
qosStr.TrProto="传输协议";
qosStr.Conreach="连接流量达到";
qosStr.AppProto="应用程序协议（L7）";
qosStr.SetClass="服务类型设置为";
qosStr.SrvClassName="服务类型名称";
qosStr.PerBandCap="带宽占用百分比";
qosStr.BandMin="最小带宽";
qosStr.BandMinNo="不限制最小带宽";
qosStr.BandMax="最大带宽";
qosStr.BandMaxNo="不限制最大带宽";
qosStr.MinRTT="最小往返延时(MinRTT)";
qosStr.ActRTT="生效时降低延时（Ping时间）";
qosStr.OptiWAN="优化WAN利用率";
qosStr.ACCOn="启用主动拥塞控制（下载方向）";
qosStr.ACC_Pt="使用自定义Ping目标";
qosStr.ACC_con="使用自定义ping延时";
qosStr.ACC_Stat="拥塞控制状态";
qosStr.ACC_L_Ck="检查Ping的目标是否有回应";
qosStr.ACC_L_In="评估Ping限制";
qosStr.ACC_L_Act="拥塞控制生效";
qosStr.ACC_L_Min="拥塞控制生效，minRTT类生效";
qosStr.ACC_L_Id="无拥塞，控制器空闲";
qosStr.ACC_L_Dis="控制器未启用";
qosStr.ACC_L_Lim="下载带宽限制当前正在执行";
qosStr.ACC_L_Fr="平均下载带宽限制";
qosStr.ACC_L_Ld="下行链路中有通信";
qosStr.ACC_L_pg="最后一次Ping的往返时间";
qosStr.ACC_L_Flt="被过滤的往返延时";
qosStr.ACC_L_plim="控制器将采取行动以维持平衡点";
qosStr.ACC_L_AC="下载类中负载超过4Kbps的数量";

//qos_edit_class.sh
qosStr.QESrvClass="编辑QoS服务类型";
qosStr.QESrvName="服务类型名称";

//qos_edit_rule.sh
qosStr.QERulClass="编辑QoS分类规则";

//javascript
qosStr.MatchC="匹配标准";
qosStr.Classn="分类";
qosStr.Src="来源";
qosStr.SrcP="来源端口";
qosStr.Dst="目标";
qosStr.DstP="目标端口";
qosStr.Connb="连接流量达到";
qosStr.APro="应用程序协议";
qosStr.pBdW="带宽占用百分比";
qosStr.mBdW="最小带宽";
qosStr.MBdW="最大带宽";
qosStr.qLd="负载";
qosStr.CrErr="未选中匹配标准。";
qosStr.SvErr="不能添加新的服务类型。";
qosStr.SUErr="不能更新服务类型。";
qosStr.CsErr="不能添加新的分类规则。";
qosStr.CUErr="不能更新分类规则。";
qosStr.DCErr="类型名称重复。";
qosStr.RemSCErr="必须至少有一个服务类型。\n不能删除服务类型。";
qosStr.TotErr="总带宽字段有误。\n不能更新QoS。";

//one-word strings used in rules
qosStr.NOLIMIT="无限";
qosStr.ZERO="零";
qosStr.YES="是";

//qos_distribution.sh
qosStr.UBSect="QoS上传流量分布";
qosStr.DBSect="QoS下载流量分布";
qosStr.uTFrm="上传流量分布周期";
qosStr.dTFrm="下载流量分布周期";
