/*
 * UTF-8 (with BOM) Portuguese-BR text strings for usb_storage.sh html elements
 */

usbSStr.mUSB="Armazenamento USB";
usbSStr.SDisk="Discos Compartilhados";
usbSStr.Nomdsk="Nenhum disco USB detectado.";
usbSStr.WFTP="Permitir Acesso Externo (WAN) ao FTP";
usbSStr.WpFTP="Permitir Acesso Externo nas Portas";
usbSStr.CFWkg="Grupo CIFS";
usbSStr.CFUsr="Usuários CIFS / FTP";
usbSStr.NewU="Novo Usuário";
usbSStr.AddU="Adicionar Usuário";
usbSStr.Pasw="Senha";
usbSStr.CfPass="Confirmar Senha";
usbSStr.ADir="Adicionar Disco Compartilhado / Diretório";
usbSStr.ADsk="Adicionar Disco Compartilhado";
usbSStr.CShare="Discos Compartilhados Cadastrados";
usbSStr.Umnt="Desmontar Disco USB";
usbSStr.UmntB="Desmontar Todos os Discos USB";
usbSStr.UmntWarn="O disco USB deve ser desmontado antes de ser removido do roteador. O disco USB será montado automaticamente na próxima inicialização.";
usbSStr.FDisk="Formatação de Disco";
usbSStr.NoUmntDev="<p>Nenhum disco desmontado detectado.</p><p>É necessário desmontar o disco antes de formatar.</p>";
usbSStr.FmtWarn="AVISO: A formatação de disco apagará permanentemente todos os dados do disco.<p>O disco será formatado e será criada uma partição com o sistema de arquivos EXT4.<br/>A partição EXT4 pode não ser lida em sistemas Windows/Mac</p>.";
usbSStr.DskForm="Disco para Formatar";
usbSStr.PSwap="Porcentagem Swap";
usbSStr.PStor="Porcentagem de Armazenamento";
usbSStr.MExtr="Utilizar Disco como Raiz Externa (Extroot)";
usbSStr.FmtNow="Formatar";
usbSStr.ExtrS="Extroot";
usbSStr.ExtrOff="Desabilitar Raiz Externa";
usbSStr.ExtDt="Raiz Externa detectada em";
usbSStr.ExtrWarn="Ao utilizar a raiz de disco externo ou <strong>extroot</strong>, o disco USB conectado torna-se o disco raiz do roteador. Isso permite expandir a memória da raiz de disco, mas se removido, todas as configurações (definidas após ativação do extroot) serão perdidas.";

//template
usbSStr.Disk="Disco";
usbSStr.SDir="Subdiretório";
usbSStr.SNam="Nome de Compartilhamento";
usbSStr.SAppl="Aplica-se a";
usbSStr.SPart="Mesma Partição Para Qualquer Disco USB";
usbSStr.SDriv="Apenas o Disco";
usbSStr.STyp="Tipo(s) de Compartilhamento";
usbSStr.FAAcc="Acesso Anônimo FTP/CIFS";
usbSStr.ANon="Nenhum";
usbSStr.AROn="Somente Leitura";
usbSStr.ARWr="Leitura/Escrita";
usbSStr.FAUsr="Usuários FTP/CIFS com Acesso";
usbSStr.NAcc="Acesso NFS";
usbSStr.NAccPo="Política de Acesso NFS";
usbSStr.AnonAcc="Permitir Acesso Anônimo";
usbSStr.OnlyIPs="Permitir Apenas o(s) Endereço(s) IP";
usbSStr.IPSub="Especificar um Endereço IP ou Sub-rede.";
usbSStr.FPath="Caminho FTP";
usbSStr.NPath="Caminho NFS";

//edit files
usbSStr.EshDS="Editar Disco Compartilhado";
usbSStr.ChUPass="Alterar Senha de Usuário";
usbSStr.User="Usuário";
usbSStr.NPass="Nova Senha";

//javascript
usbSStr.ICWErr="Grupo CIFS inválido.";
usbSStr.FWPErr="Não é possível habilitar acesso externo (WAN) ao FTP porque a porque conflita com";
usbSStr.FprErr="Intervalo de portas para FTP passivo inválido.";
usbSStr.FprcErr="Intervalo de portas para FTP passivo conflita com";
usbSStr.SSetErr="Não foi possível salvar configuração.";
usbSStr.UsEmErr="Digite o nome de usuário.";
usbSStr.UsChErr="O nome de usuário só pode conter letras e números.";
usbSStr.PwEmErr="Digite a senha.";
usbSStr.PwEqErr="As senhas inseridas são diferentes.";
usbSStr.PwUErr="Senha inalterada.";
usbSStr.UsDErr="Nome de usuário já existente.";
usbSStr.Usnm="Nome de Usuário";
usbSStr.RsvErr="é reservado e não permitido";
usbSStr.AUsErr="Não foi possível adicionar usuário.";
usbSStr.AShErr="Não foi possível adicionar compartilhamento.";
usbSStr.NUsrErr="Nenhum usuário de compartilhamento cadastrado.";
usbSStr.NShUsrErr="Nenhum usuário configurado.";
usbSStr.NoShTypErr="Selecione pelo menos um tipo de compartilhamento (FTP, CIFS, NFS)";
usbSStr.NoAAUsrErr="Tipo de compartilhamento FTP e/ou CIFS selecionado, mas nenhum tipo de acesso ou usuário foi configurado.";
usbSStr.DupDirErr="Diretório de compartilhamento já configurado.";
usbSStr.DupShrErr="Nome de compartilhamento já existente.";
usbSStr.UpShrErr="Não foi possível atualizar compartilhamento.";
usbSStr.SwpPErr="ERRO: Porcentagem de alocação inválida.";
usbSStr.InvPErr="ERRO: Senha inválida.";
usbSStr.ChPass="Alterar Senha";
usbSStr.Name="Nome";
usbSStr.UDisk="Desmontando Discos";
usbSStr.FDmsg1="Você tem certeza que deseja formatar o disco ?";
usbSStr.FDmsg2="Todos os dados do disco serão apagados.";
usbSStr.FDmsg3="Digite a senha de Administrador";
usbSStr.CfmPass1="Usuário Autenticado.";
usbSStr.CfmPass2="Deseja formatar o disco ?";
usbSStr.FmtMsg="Formatando disco, aguarde…";
usbSStr.FmtCplt="Formatação concluída.";
usbSStr.ExtOffWarn="Ao desabilitar extroot, todos os plugins instalados e configurações podem ser perdidas. Após desabilitar extroot, é necessário reiniciar o roteador. Deseja continuar ?";
usbSStr.ExtOffMsg="Desabilitando extroot…";
usbSStr.SRboot="Reiniciando o roteador, aguarde…";
