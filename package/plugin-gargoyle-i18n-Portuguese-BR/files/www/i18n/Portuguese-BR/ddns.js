/*
 * UTF-8 (with BOM) Portuguese-BR text strings for ddns.sh html elements
 */

DyDNS.mDDNS="DNS Dinâmico";
DyDNS.DYSect="Configuração DDNS";
DyDNS.AddDy="Adicionar Serviço DDNS";
DyDNS.SvPro="Provedor de Serviço";
DyDNS.ChItv="Intervalo de Verificação";
DyDNS.FUItv="Forçar Intervalo de Verificação";
DyDNS.AddDDNS="Adicionar Serviço DDNS";
DyDNS.HelpCI="<em><b>Intervalo de Verificação:</b></em> especifica com que frequência o roteador irá verificar se o seu endereço IP atual corresponde ao endereço IP associado ao seu domínio. Essa verificação é feita sem conectar ao servidor DDNS, então não há problemas em utilizar essa opção com servidores que costumam banir usuários que se conectam com muita frequência (e.g. dyndns.com). O intervalo de verificação recomendado é de 10 a 20 minutos.";
DyDNS.HelpFI="<em><b>Forçar Intervalo de Verificação:</b></em> especifica com que frequência o roteador irá conectar ao servidor DDNS e atualizar os registros, mesmo se o seu endereço IP não for alterado. Servidores DDNS costumam banir usuários que se conectam com muita frequência, e podem desabilitar contas de usuários inativos por mais de um mês. O intervalo de verificação (forçado) recomendado é de 3 a 7 dias.";
DyDNS.UpErr1="Problema ao atualizar configurações do novo serviço DDNS.";
DyDNS.UpErr2="Problema ao atualizar serviço, serviço removido.";
DyDNS.cNams=["Domínio", "Provedor de Serviço", "Última Atualização", "Habilitado", "", ""];
DyDNS.InvErr="ERRO: Servidor inválido.";
DyDNS.DupErr="Serviço DDNS já existente.";
DyDNS.ForceU="Forçar Atualização";
DyDNS.ModErr="Você deve salvar as alterações antes de atualizar o serviço. Clique em \""+UI.SaveChanges+"\" e tente novamente.";
DyDNS.UpFErr="Problema ao atualizar. Verifique se as configurações estão corretas e se você está conectado à Internet.";
DyDNS.UpOK="Atualização bem-sucedida.";
DyDNS.UpSrvErr="Não foi possível atualizar classe de serviço.";

//ddns_edit.sh
DyDNS.EDSect="Editar Serviço DDNS";

// /etc/ddns_providers.conf
DyDNS.DoNm="Nome do Domínio";
DyDNS.UsrN="Nome de Usuário";
DyDNS.Pssw="Senha";
DyDNS.Eml="E-mail";
DyDNS.Key="Chave";
DyDNS.AKey="Chave API";
DyDNS.Tokn="Token";
