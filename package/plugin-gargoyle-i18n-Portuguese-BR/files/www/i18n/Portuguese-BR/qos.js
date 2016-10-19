/*
 * UTF-8 (with BOM) Portuguese-BR text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.mQDl="QoS Download";
qosStr.mQUl="QoS Upload"; 
qosStr.URSection="Regras de Classificação QoS - Upload";
qosStr.DRSection="Regras de Classificação QoS - Download";
qosStr.UCSection="Classes de Serviço QoS - Upload";
qosStr.DCSection="Classes de Serviço QoS - Download";
qosStr.DACCSect="Controle Ativo de Congestionamento QoS - Download";
qosStr.UEnable="Habilitar Qualidade de Serviço (Upload)";
qosStr.DEnable="Habilitar Qualidade de Serviço (Download)";
qosStr.UTotBand="Velocidade Total de Upload";
qosStr.DTotBand="Velocidade Total de Download";
qosStr.USCAbout="Cada classe de upload é definida por três parâmetros: Alocação de Banda durante a saturação do link, Velocidade Mínima e Velocidade Máxima.";
qosStr.DSCAbout="Cada classe de download é definida por quatro parâmetros: Alocação de Banda durante a saturação do link, Velocidade Mínima, Velocidade Máxima e Otimização MinRTT (minimiza o tempo de resposta).";
qosStr.UMinBandAbout="<em><b>Velocidade Mínima:</b></em> define a velocidade mínima da banda que será alocada à classe durante a saturação do link. Para aplicações em tempo real, como ligações VoIP ou jogos online, é recomendado configurar uma classe com uma velocidade mínima definida (em kbps), ao invés de alocar uma porcentagem de banda à classe. O QoS irá satisfazer primeiro todas as classes com uma velocidade mínima definida, e somente depois irá alocar a banda disponível para o restante das classes.";
qosStr.DMinBandAbout="<em><b>Velocidade Mínima:</b></em> define a velocidade mínima da banda que será alocada à classe durante a saturação do link. Classes com uma velocidade mínima definida são classes em tempo real, e são gerenciadas pelo controlador de congestionamento. Jogos online, ligações VoIP e videoconferência, são todos exemplos de aplicações que necessitam que uma velocidade mínima seja definida para que funcionem corretamente. Para determinar a velocidade mínima de banda necessária (em kbps), utilize a aplicação desejada e observe em tempo real a velocidade da banda utilizada na rede. Insira um valor ligeiramente maior do que foi observado, por garantia. O QoS irá satisfazer primeiro todas as classes com uma velocidade mínima definida, e somente depois irá alocar a banda disponível para o restante das classes, portanto somente utilize esse parâmetro se realmente necessário.";
qosStr.UTotBandAbout="<em><b>Velocidade Total de Upload:</b></em> deve ser definida entre 95% a 98% da velocidade de upload disponível. Inserir um valor muito alto pode gerar problemas de alocação de banda disponível às classes, enquanto inserir um valor muito baixo penalizará sua velocidade de upload sem necessidade. Se estiver usando uma conexão PPPoE, insira o valor da velocidade de &quot;DSL Uplink&quot; (em kbps), que pode ser encontrado na página de configuração do modem/roteador. Outros tipos de conexão devem realizar testes de velocidade (e.g. speedtest.net) com o QoS desabilitado, para determinar a velocidade de upload real. A velocidade é definida em kbps (1kbyte = 8kbits).";
qosStr.DTotBandAbout="Definir a <em><b>Velocidade Total de Download</b></em> corretamente é crucial para que o QoS funcione corretamente. Se você habilitou o Controlador de Congestionamento, defina o valor (em kbps) com a velocidade máxima de download disponibilizada pela sua operadora. Se estiver usando uma conexão PPPoE, insira o valor da velocidade de &quot;DSL Downlink&quot; (em kbps), que pode ser encontrado na página de configuração do modem/roteador.";
qosStr.PerBandAboutU="<em><b>Alocação de Banda:</b></em> é a porcentagem da banda total disponível que será alocada à classe durante a saturação do link. Se o link não estiver totalmente saturado, a banda disponível restante será alocada à classe. As porcentagens das classes de serviço são configuradas proporcionalmente, de maneira que totalizem a soma dos 100% da banda disponível.";
qosStr.PerBandAboutD="<em><b>Alocação de Banda:</b></em> é a porcentagem da banda total disponível que será alocada à classe durante a saturação do link. Se o link não estiver totalmente saturado, a banda disponível restante será alocada à classe. As porcentagens das classes de serviço são configuradas proporcionalmente, de maneira que totalizem a soma dos 100% da banda disponível.";
qosStr.RTTAbout="<em><b>Otimização MinRTT:</b></em> informa ao controlador de congestionamento para minimizar o tempo de resposta quando a classe de serviço estiver ativa. Utilize essa opção para jogos online ou ligações VoIP, aplicações que necessitam de um tempo de resposta reduzido (ping baixo). A otimização MinRTT tem um custo: enquanto estiver ativa, a velocidade da banda sofrerá uma queda de aproximadamente 20%.";
qosStr.MinSpeedWarn="Se você não estiver usando o <em>Controle Ativo de Congestionamento</em>, então você deve estabelecer qual a velocidade mínima disponibilizada pela sua operadora e configurar o campo <em>Velocidade Total de Download</em> de acordo com esse valor. No geral, as operadoras não conseguem garantir uma velocidade mínima estável, portanto pode ser difícil definir um valor. Comece definindo o valor como metade da velocidade total esperada, e aumente aos poucos. Aumente o valor até notar problemas na operação do QoS. Isso ocorre quando a operadora fica congestionada com tantas demandas de clientes, não garantindo assim uma velocidade de conexão estável. Inserir um valor muito alto pode gerar problemas de alocação de banda disponível às classes, enquanto inserir um valor muito baixo penalizará sua velocidade de download sem necessidade. Por conta das complicações citadas acima, é recomendado que você habilite o <em>Controlador de Congestionamento</em>. A velocidade é definida em kbps (1kbyte = 8kbits).";
qosStr.QoSAbout="A Qualidade de Serviço (QoS) oferece uma maneira de gerenciar como a banda disponível será distribuída entre as aplicações. Os pacotes são classificados em diferentes &quot;classes de serviço&quot;, onde para cada classe é alocada uma parte da banda disponível. A QoS deve ser utilizada quando se deseja distribuir a banda disponível de maneira igualitária entre várias aplicações diferentes. Por exemplo, realizar uma ligação VoIP com qualidade enquanto faz downloads, ou reduzir a velocidade dos torrents enquanto navega na Internet.";
qosStr.MaxBandAbout="<em><b>Velocidade Máxima:</b></em> define a velocidade máxima da banda que será alocada à classe (em kbps). Mesmo se a banda disponível não estiver sendo utilizada, essa classe não poderá utilizar mais do que o valor definido.";
qosStr.PackAbout="Os pacotes são analisados conforme as regras de tráfego estabelecidas, em ordem decrescente de prioridade. Assim que o pacote corresponder à alguma regra de tráfego, ele é classificado e todas as regras restantes são ignoradas. A ordem de prioridade das regras de tráfego pode ser alterada clicando nas setas para cima/baixo.";
qosStr.DefServClassAbout="<em><b>Classe de Serviço Padrão:</b></em> é a classe padrão que será atribuída a todos os pacotes que não correspondem à nenhuma regra de classificação anteriormente especificada.";
qosStr.AbACC="<p>O <em><b>Controlador de Congestionamento</b></em> (CC) monitora a atividade no link de download e ajusta automaticamente o limite do link para manter a performance QoS adequada. O CC compensa automaticamente às variações de velocidade da banda disponibilizada pela operadora, e ajusta o link de download para a velocidade máxima estimada, conforme a necessidade da rede. O CC opera com eficiência entre 15% a 100% da <em>Velocidade Total de Download</em> definida acima.</p><p>Embora o controlador de congestionamento não ajuste a velocidade de upload, é necessário habilitar e configurar o <em>QoS Upload</em> para que o sistema funcione corretamente.</p><p><em><b>Destino de Monitoramento:</b></em> é o segmento de rede entre seu roteador e o destino de monitoramento onde o congestionamento da rede é detectado. Isso é feito através do monitoramento dos tempos de resposta até o destino de monitoramento. Por padrão, o destino de monitoramento é definido com endereço IP do Gateway WAN do roteador. Se desejar alterar o destino de monitoramento padrão, selecione a opção &quot;<em>Alterar destino de monitoramento</em>&quot;, e defina o novo endereço IP do destino.</p><p><em><b>Limite de Tempo de Resposta Manual:</b></em> Os tempos de resposta são comparados aos limites de tempo de resposta. O CC controla o limite de velocidade do link para manter o tempo de resposta abaixo do limite. Por padrão, o limite de tempo de resposta é calculado automaticamente em tempo real, baseado na velocidade e estabilidade do link. Não é possível alterar o <em>Limite de Tempo de Resposta</em> para o modo MINRTT, somente para o modo ACTIVE, o que altera o limite do tempo de resposta para o destino de monitoramento. O limite inserido é acrescentado ao tempo de resposta do destino de monitoramento entre os modos MINRTT e ACTIVE. O limite de tempo de resposta utilizado pelo CC pode ser observado entre os colchetes [].</p>"

qosStr.ServClass="Classe de Serviço Padrão";
qosStr.AddNewClassRule="Adicionar Nova Regra de Classificação";
qosStr.AddNewServiceRule="Adicionar Nova Classe de Serviço";
qosStr.SrcIP="Endereço IP de Origem";
qosStr.SrcPort="Porta(s) de Origem";
qosStr.DstIP="Endereço IP de Destino";
qosStr.DstPort="Porta(s) de Destino";
qosStr.MaxPktLen="Tamanho Máximo de Pacote";
qosStr.MinPktLen="Tamanho Mínimo de Pacote";
qosStr.TrProto="Protocolo";
qosStr.Conreach="Quando a transferência atingir (kbytes)";
qosStr.AppProto="Filtro L7 (<b>não recomendado</b>)";
qosStr.SetClass="Classe de Serviço";
qosStr.SrvClassName="Nome da Classe";
qosStr.PerBandCap="Alocação de Banda";
qosStr.BandMin="Velocidade Mínima";
qosStr.BandMinNo="Ilimitada";
qosStr.BandMax="Velocidade Máxima";
qosStr.BandMaxNo="Ilimitada";
qosStr.MinRTT="Tipo de Aplicação (Otimização MinRTT)";
qosStr.ActRTT="Aplic. em Tempo Real (jogos online, skype, voip, etc)";
qosStr.OptiWAN="Aplic. Multimídia (netflix, redes sociais, navegação, etc)";
qosStr.ACCOn="Habilitar Controlador de Congestionamento";
qosStr.ACC_Pt="Alterar destino de monitoramento";
qosStr.ACC_con="Ajustar manualmente tempo de resposta";
qosStr.ACC_Stat="Status do Controlador";
qosStr.ACC_L_Ck="Verifica se o destino de monitoramento está respondendo.";
qosStr.ACC_L_In="Estima um limite de tempo de resposta.";
qosStr.ACC_L_Act="Controlador ativo.";
qosStr.ACC_L_Min="Controlador ativo, otimização MinRTT ativa (minimiza o tempo de resposta).";
qosStr.ACC_L_Id="Sem congestionamento, controlador inativo.";
qosStr.ACC_L_Dis="Controlador desabilitado.";
qosStr.ACC_L_Lim="O limite atual do link de download.";
qosStr.ACC_L_Fr="O limite estimado do link de download.";
qosStr.ACC_L_Ld="A carga atual do link de download.";
qosStr.ACC_L_pg="O tempo de resposta do último ping.";
qosStr.ACC_L_Flt="O tempo de resposta filtrado.";
qosStr.ACC_L_plim="O ponto no qual o controlador irá atuar para manter a distribuição igualitária da banda disponível.";
qosStr.ACC_L_AC="Quantidade de classes de download com carga acima de 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Editar Classe de Serviço";
qosStr.QESrvName="Nome da Classe";

//qos_edit_rule.sh
qosStr.QERulClass="Editar Regra de Classificação";

//javascript
qosStr.MatchC="Regras de Tráfego (ordem decrescente)";
qosStr.Classn="Classe";
qosStr.Src="IP de Origem";
qosStr.SrcP="Porta de Origem";
qosStr.Dst="IP de Destino";
qosStr.DstP="Porta de Destino";
qosStr.Connb="Bytes de Conexão";
qosStr.APro="Filtro L7";
qosStr.pBdW="Alocação de Banda";
qosStr.mBdW="Veloc. Mín.";
qosStr.MBdW="Veloc. Máx.";
qosStr.qLd="Carga";
qosStr.CrErr="Nenhuma condição foi selecionada.";
qosStr.SvErr="Não foi possível adicionar classe de serviço.";
qosStr.SUErr="Não foi possível atualizar classe de serviço.";
qosStr.CsErr="Não foi possível adicionar regra de classificação.";
qosStr.CUErr="Não foi possível atualizar regra de classificação.";
qosStr.DCErr="O nome da classe de serviço já existe.";
qosStr.RemSCErr="Não é possível remover todas as classes de serviço.";
qosStr.TotErr="Erro no campo de Velocidade Total.\n\nNão foi possível atualizar QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="ilimitada";
qosStr.ZERO="zero";
qosStr.YES="Sim";

//qos_distribution.sh
qosStr.UBSect="Gráfico de Distribuição de Banda QoS - Upload";
qosStr.DBSect="Gráfico de Distribuição de Banda QoS - Download";
qosStr.uTFrm="Intervalo de Tempo";
qosStr.dTFrm="Intervalo de Tempo";
