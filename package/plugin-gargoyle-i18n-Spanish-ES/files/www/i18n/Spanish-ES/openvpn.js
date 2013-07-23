/*
 * UTF-8 (with BOM) Spanish-ES text strings for OpenVPN elements
 */

ovpnS.OCfg="Configuración de OpenVPN";
ovpnS.ODis="OpenVPN desactivado";
ovpnS.OClt="Cliente OpenVPN";
ovpnS.OSrv="Servidor OpenVPN";
ovpnS.OSts="Estado de OpenVPN";
ovpnS.OSCfg="Servidor OpenVPN: Configuración";
ovpnS.OInIP="IP interna de OpenVPN";
ovpnS.OIMsk="Máscara de subred interna de OpenVPN";
ovpnS.OPrt="Puerto de OpenVPN";
ovpnS.OProto="Protocolo de OpenVPN";
ovpnS.OCiph="Cifrado de OpenVPN";
ovpnS.CCTr="Tráfico cliente-a-cliente";
ovpnS.CtoC="Permitir a los clientes comunicarse entre sí";
ovpnS.CtoS="Los clientes pueden comunicarse solamente con el servidor";
ovpnS.LSAc="Acceso a subred LAN";
ovpnS.CtoH="Permitir a los clientes acceder a equipos de la LAN";
ovpnS.CnoL="Denegar a los clientes acceder a equipos de la LAN";
ovpnS.CredR="Reutilización de credencial";
ovpnS.CredSC="Las credenciales son específicas para cada cliente";
ovpnS.CredMC="Las credenciales pueden ser utilizadas por varios clientes";
ovpnS.CUse="Los clientes utilizan VPN para";
ovpnS.ATrff="Todo el tráfico del cliente";
ovpnS.HTrff="Sólo el tráfico destinado a equipos detrás de VPN";
ovpnS.OSAC="Servidor OpenVPN: Clientes permitidos";
ovpnS.CClnt="Clientes configurados actualmente";
ovpnS.ZipCred="Después de generar la configuración del cliente, haga clic en Descargar para obtener el archivo zip que contiene las credenciales necesarias, y ubíquelo en la carpeta de configuración de su cliente OpenVPN";
ovpnS.CfgCred="Configurar un nuevo cliente / conjunto de credenciales";
ovpnS.ClntN="Nombre del cliente";
ovpnS.ClntIP="IP interna del cliente";
ovpnS.ClntConn="El cliente se conecta a";
ovpnS.ClntSubN="Subred detrás del cliente";
ovpnS.NoSub="No se ha definido una subred";
ovpnS.RtSub="Enrutar la subred debajo";
ovpnS.SubIP="IP de la subnet";
ovpnS.SubM="Máscara de subred";
ovpnS.UpCfgF="Cargar archivo(s) de configuración de cliente";
ovpnS.CfgMan="Configurar manualmente cliente";
ovpnS.UpFmt="Formato a cargar";
ovpnS.SZipF="Un único archivo zip";
ovpnS.CfgF="Archivos de configuración individuales";
ovpnS.ZipF="Archivo zip";
ovpnS.OCfgF="Archivo de configuración de OpenVPN";
ovpnS.CACF="Archivo de certificado CA";
ovpnS.CCertF="Archivo de certificado de cliente";
ovpnS.CKeyF="Archivo de clave de cliente";
ovpnS.TAKeyF="Archivo de clave de TLS-Auth";
ovpnS.UseTAK="Utilizar clave TLS-Auth";
ovpnS.OSrvAddr="Dirección del servidor OpenVPN";
ovpnS.OSrvPrt="Puerto del servidor OpenVPN";
ovpnS.Othr="Otro";
ovpnS.Cphr="Cifrado";
ovpnS.Keyopt="Tamaño de clave (opcional)";
ovpnS.CfgUpd="La configuración que se encuentra a continuación se actualiza automáticamente a partir de los parámetros especificados anteriormente";
ovpnS.CACert="Certificado de CA";
ovpnS.CCert="Certificado de cliente";
ovpnS.CKey="Clave de cliente";
ovpnS.TAKey="Clave de TLS-Auth";
ovpnS.TADir="Dirección de TLS-Auth";
ovpnS.Clnt="Cliente";
ovpnS.Symm="Omitido (simétrico)";

//javascript
ovpnS.CryptoWaitMsg="Esta es la primera vez que se configura un servidor OpenVPN.\n\nTomará entre 5-10 minutos para generar los parámetros criptográficos necesarios. Este es un tiempo de espera de una sola vez -- las actualizaciones posteriores a ésta serán más rápidas.\n\n¿Desea continuar?";
ovpnS.SubMis="La subred del cliente no coincide";
ovpnS.ExpSubN="OpenVPN requiere que el enrutador posea una subred de";
ovpnS.ActSubN="pero el enrutador está configurado con una subred de";
ovpnS.WantQ="¿Quieres...";
ovpnS.Switch="Cambiar enrutador a subred esperada, con IP";
ovpnS.KeepC="Mantener subred actual y continuar";
ovpnS.SrvPrtErr="El puerto del servidor OpenVPN se encuentra en conflicto con";
ovpnS.SrvAddErr="La dirección del servidor no ha sido definida";
ovpnS.OPrtErr="El puerto de OpenVPN debe estar entre 1-65535";
ovpnS.GTAPErr="Gargoyle no admite configuraciones de OpenVPN TAP";
ovpnS.RunC="Ejecutando, Conectado";
ovpnS.RunNC="Ejecutando, No conectado";
ovpnS.RunNot="No se está ejecutando";
ovpnS.IntIP="IP interna\n(Subred enrutada)";
ovpnS.CfgCredF="Credenciales\ny Archivos de configuración";
ovpnS.Dload="Descargar";
ovpnS.DDNS="DNS Dinámico";
ovpnS.WANIP="WAN IP";
ovpnS.OthIPD="Otra IP o dominio (especificado abajo)";
ovpnS.ClntIntIP="IP interna del cliente especificado";
ovpnS.OSubErr="no se encuentra en la subred de OpenVPN";
ovpnS.AddCErr="No se pudo añadir la configuración del cliente.";
ovpnS.UpCErr="No se pudo actualizar la configuración del cliente.";

//openvpn_allowed_client_edit.sh
ovpnS.EditOCS="Editar configuración del cliente OpenVPN";

//openvpn_upload_client.sh (handled by shell scripts)
ovpnS.uc_CA_f="No se pudo encontrar el archivo de CA";
ovpnS.uc_crt_f="No se pudo encontrar el archivo de certificado";
ovpnS.uc_key_f="No se pudo encontrar el archivo de claves";
ovpnS.uc_cfg_f="No se pudo encontrar el archivo de configuración";
ovpnS.uc_TAP_Err="Gargoyle no admite configuraciones de OpenVPN TAP";
ovpnS.uc_conn_Err="Los parámetros se han guardado pero OpenVPN no se pudo conectar. Vuelva a comprobar la configuración.";

//openvpn_connections.sh
ovpnS.ConnOC="Clientes OpenVPN conectados";

//openvpn_connections.js
ovpnS.ConnFr="Conectado de";
ovpnS.ConnSc="Conectado desde";
ovpnS.NoCConn="No hay clientes conectados";
