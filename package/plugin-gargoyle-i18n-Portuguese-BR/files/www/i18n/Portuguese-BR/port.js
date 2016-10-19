/*
 * UTF-8 (with BOM) Portuguese-BR text strings for port_forwarding(single,multi).sh html elements
 */

prtS.mPFwding="Redirecionamento de Portas";
prtS.PISect="Redirecionamento de Portas";
prtS.PRSect="Redirecionamento de Intervalo de Portas";
prtS.ForIPort="Redirecionar Porta WAN para LAN";
prtS.ForRPort="Redirecionar Intervalo de Portas WAN para LAN";
prtS.DMZ="DMZ (Zona Desmilitarizada)";
prtS.UseDMZ="Habilitar DMZ";
prtS.DMZIP="Endereço IP DMZ";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="Habilitar UPnP &amp; Serviço NAT-PMP";
prtS.APFor="Portas Redirecionadas";
prtS.USpd="Velocidade de Upload (Referência)";
prtS.DSpd="Velocidade de Download (Referência)";
prtS.UPHelp="<em><b>UPnP (Universal Plug and Play) e NAT-PMP (NAT Port Mapping Protocol)</b></em> são ambos protocolos que permitem que dispositivos e aplicações configurem automaticamente o redirecionamento de portas na sua rede LAN. Se o dispositivo suportar algum dos protocolos, não será necessária a configuração manual do redirecionamento de portas (veja topo da página), pois as portas necessárias serão configuradas automaticamente pelo dispositivo.</p><p>Quando ativado, o Gargoyle mostra uma tabela com os redirecionamentos de porta gerados automaticamente, para que veja quais dispositivos solicitaram redirecionamento de porta e verifique se o serviço está operando corretamente. O serviço pode não operar corretamente em configurações de rede com dois ou mais roteadores (double NAT). Se a tabela apresentar somente uma linha com '***', significa que não há nenhum redirecionamento de porta configurado.</p><p>Como parte do protocolo, o dispositivo LAN pode requisitar a velocidade da conexão WAN do roteador. Existem dois campos para configurar os valores de download e upload. Aplicações podem usar esses valores como referência para otimizar operações. É importante ressaltar que a configuração dos valores serve somente como referência para requisições de aplicações, e não para limitar as velocidades de conexão. Se for inserido &quot;0&quot; como valor, a velocidade máxima da interface WAN é atribuída (100MB ou 1GB, dependendo do roteador).</p> <p>Essa funcionalidade é desabilitada por padrão, pois oferece riscos de segurança e requer utilização adicional de memória RAM para operar corretamente, o que pode ser um problema para roteadores com pouca memória RAM disponível.";

//templates
prtS.Desc="Descrição";
prtS.optl="(opcional)";
prtS.Proto="Protocolo";
prtS.FPrt="Porta de Origem";
prtS.TIP="IP de Destino";
prtS.TPrt="Porta de Destino";
prtS.SPrt="Porta Inicial";
prtS.EPrt="Porta Final";

//javascript
prtS.AFRErr="Não foi possível adicionar redirecionamento de porta.";
prtS.GTErr="A Porta Inicial é maior que a Porta Final.";
prtS.DupErr="Essas portas já foram redirecionadas.";
prtS.CopErr="Essa porta já foi redirecionada.";
prtS.UpErr="Não foi possível atualizar redirecionamento de porta.";
prtS.Prot="Protocolo";
prtS.LHst="Host LAN";
prtS.Port="Porta";

//edit.sh pages
prtS.PESect="Editar Redirecionamento de Porta";
